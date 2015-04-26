#include "minisphere.h"
#include "api.h"
#include "vector.h"

#include "galileo.h"

static duk_ret_t js_GetDefaultShaderProgram (duk_context* ctx);
static duk_ret_t js_new_Group               (duk_context* ctx);
static duk_ret_t js_Group_finalize          (duk_context* ctx);
static duk_ret_t js_Group_get_angle         (duk_context* ctx);
static duk_ret_t js_Group_get_rotX          (duk_context* ctx);
static duk_ret_t js_Group_get_rotY          (duk_context* ctx);
static duk_ret_t js_Group_get_shader        (duk_context* ctx);
static duk_ret_t js_Group_get_x             (duk_context* ctx);
static duk_ret_t js_Group_get_y             (duk_context* ctx);
static duk_ret_t js_Group_set_angle         (duk_context* ctx);
static duk_ret_t js_Group_set_rotX          (duk_context* ctx);
static duk_ret_t js_Group_set_rotY          (duk_context* ctx);
static duk_ret_t js_Group_set_shader        (duk_context* ctx);
static duk_ret_t js_Group_set_x             (duk_context* ctx);
static duk_ret_t js_Group_set_y             (duk_context* ctx);
static duk_ret_t js_Group_draw              (duk_context* ctx);
static duk_ret_t js_new_ShaderProgram       (duk_context* ctx);
static duk_ret_t js_new_Shape               (duk_context* ctx);
static duk_ret_t js_Shape_finalize          (duk_context* ctx);
static duk_ret_t js_Shape_get_image         (duk_context* ctx);
static duk_ret_t js_Shape_set_image         (duk_context* ctx);

struct vertex
{
	float   x, y;
	float   u, v;
	color_t color;
};

struct shape
{
	unsigned int refcount;
	image_t*     texture;
	int          max_vertices;
	int          num_vertices;
	vertex_t     *vertices;
};

struct group
{
	unsigned int refcount;
	float        x, y, rot_x, rot_y;
	double       theta;
	vector_t*    shapes;
};

group_t*
new_group(void)
{
	group_t* group;

	if (!(group = calloc(1, sizeof(group_t))))
		goto on_error;
	group->shapes = new_vector(sizeof(shape_t*));
	return ref_group(group);

on_error:
	free(group);
	return NULL;
}

group_t*
ref_group(group_t* group)
{
	if (group != NULL)
		++group->refcount;
	return group;
}

void
free_group(group_t* group)
{
	iter_t*  iter;
	shape_t* i_shape;

	if (group == NULL || --group->refcount > 0)
		return;

	iter = iterate_vector(group->shapes);
	while (next_vector_item(group->shapes, &iter, &i_shape))
		free_shape(i_shape);
	free_vector(group->shapes);
	free(group);
}

shape_t*
get_group_shape(group_t* group, int index)
{
	shape_t* shape;

	get_vector_item(group->shapes, index, &shape);
	return shape;
}

void
set_group_shape(group_t* group, int index, shape_t* shape)
{
	shape_t* old_shape;

	get_vector_item(group->shapes, index, &old_shape);
	shape = ref_shape(shape);
	set_vector_item(group->shapes, index, &shape);
	free_shape(old_shape);
}

bool
add_shape(group_t* group, shape_t* shape)
{
	shape = ref_shape(shape);
	push_back_vector(group->shapes, &shape);
	return true;
}

void
remove_shape(group_t* group, int index)
{
	remove_vector_item(group->shapes, index);
}

void
clear_group(group_t* group)
{
	shape_t* shape;

	iter_t* iter;

	iter = iterate_vector(group->shapes);
	while (next_vector_item(group->shapes, &iter, &shape))
		free_shape(shape);
	clear_vector(group->shapes);
}

void
draw_group(group_t* group)
{
	shape_t* shape;

	iter_t* iter;

	iter = iterate_vector(group->shapes);
	while (next_vector_item(group->shapes, &iter, &shape))
		draw_shape(shape, group->x, group->y);
}

shape_t*
new_shape(image_t* texture)
{
	shape_t* shape;
	
	if (!(shape = calloc(1, sizeof(shape_t))))
		goto on_error;
	shape->texture = ref_image(texture);
	return ref_shape(shape);

on_error:
	free(shape);
	return NULL;
}

shape_t*
ref_shape(shape_t* shape)
{
	if (shape != NULL)
		++shape->refcount;
	return shape;
}

void
free_shape(shape_t* shape)
{
	if (shape == NULL || --shape->refcount > 0)
		return;
	free_image(shape->texture);
	free(shape);
}

image_t*
get_shape_texture(shape_t* shape)
{
	return shape->texture;
}

vertex_t
get_shape_vertex(shape_t* shape, int index)
{
	return shape->vertices[index];
}

void
set_shape_vertex(shape_t* shape, int index, vertex_t vertex)
{
	shape->vertices[index] = vertex;
}

void
set_shape_texture(shape_t* shape, image_t* texture)
{
	image_t* old_texture;
	
	old_texture = shape->texture;
	shape->texture = ref_image(texture);
	free_image(old_texture);
}

bool
add_vertex(shape_t* shape, vertex_t vertex)
{
	int       new_max;
	vertex_t  *new_buffer;

	if (shape->num_vertices + 1 > shape->max_vertices) {
		new_max = (shape->num_vertices + 1) * 2;
		if (!(new_buffer = realloc(shape->vertices, new_max * sizeof(vertex_t))))
			return false;
		shape->vertices = new_buffer;
		shape->max_vertices = new_max;
	}
	++shape->num_vertices;
	shape->vertices[shape->num_vertices - 1] = vertex;
	return true;
}

void
remove_vertex(shape_t* shape, int index)
{
	int i;

	--shape->num_vertices;
	for (i = index; i < shape->num_vertices; ++i)
		shape->vertices[i] = shape->vertices[i + 1];
}

void
draw_shape(shape_t* shape, float x, float y)
{
	int             draw_mode;
	ALLEGRO_VERTEX* vbuf;
	int             w_texture;
	int             h_texture;

	int i;

	draw_mode = shape->num_vertices == 1 ? ALLEGRO_PRIM_POINT_LIST
		: shape->num_vertices == 2 ? ALLEGRO_PRIM_LINE_LIST
		: shape->num_vertices == 4 ? ALLEGRO_PRIM_TRIANGLE_FAN
		: ALLEGRO_PRIM_TRIANGLE_STRIP;
	w_texture = get_image_width(shape->texture);
	h_texture = get_image_height(shape->texture);
	if (!(vbuf = calloc(shape->num_vertices, sizeof(ALLEGRO_VERTEX))))
		return;
	for (i = 0; i < shape->num_vertices; ++i) {
		vbuf[i].x = shape->vertices[i].x + x;
		vbuf[i].y = shape->vertices[i].y + y;
		vbuf[i].u = shape->vertices[i].u * w_texture;
		vbuf[i].v = shape->vertices[i].v * h_texture;
		vbuf[i].color = nativecolor(shape->vertices[i].color);
	}
	al_draw_prim(vbuf, NULL, get_image_bitmap(shape->texture),
		0, shape->num_vertices, draw_mode);
	free(vbuf);
}

void
init_galileo_api(void)
{
	// Shape object
	register_api_ctor(g_duk, "Shape", js_new_Shape, js_Shape_finalize);
	register_api_prop(g_duk, "Shape", "image", js_Shape_get_image, js_Shape_set_image);

	// ShaderProgram object
	register_api_function(g_duk, NULL, "GetDefaultShaderProgram", js_GetDefaultShaderProgram);
	register_api_ctor(g_duk, "ShaderProgram", js_new_ShaderProgram, NULL);
	
	// Group object
	register_api_ctor(g_duk, "Group", js_new_Group, js_Group_finalize);
	register_api_prop(g_duk, "Group", "angle", js_Group_get_angle, js_Group_set_angle);
	register_api_prop(g_duk, "Group", "rotX", js_Group_get_rotX, js_Group_set_rotX);
	register_api_prop(g_duk, "Group", "rotY", js_Group_get_rotY, js_Group_set_rotY);
	register_api_prop(g_duk, "Group", "shader", js_Group_get_shader, js_Group_set_shader);
	register_api_prop(g_duk, "Group", "x", js_Group_get_x, js_Group_set_x);
	register_api_prop(g_duk, "Group", "y", js_Group_get_y, js_Group_set_y);
	register_api_function(g_duk, "Group", "draw", js_Group_draw);
}

static duk_ret_t
js_new_Group(duk_context* ctx)
{
	duk_require_object_coercible(ctx, 0);
	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Shape(): First argument must be an array");
	duk_require_sphere_obj(ctx, 1, "ShaderProgram");

	size_t    num_shapes;
	group_t*  group;
	shape_t*  shape;

	duk_uarridx_t i;

	if (!(group = new_group()))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Group(): Failed to create group object");
	num_shapes = duk_get_length(ctx, 0);
	for (i = 0; i < num_shapes; ++i) {
		duk_get_prop_index(ctx, 0, i);
		shape = duk_require_sphere_obj(ctx, -1, "Shape");
		if (!add_shape(group, shape))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Group(): Shape list allocation failure");
	}
	duk_push_sphere_obj(ctx, "Group", group);
	return 1;
}

static duk_ret_t
js_Group_finalize(duk_context* ctx)
{
	group_t* group;

	group = duk_require_sphere_obj(ctx, 0, "Group");
	free_group(group);
	return 0;
}

static
js_Group_get_angle(duk_context* ctx)
{
	group_t* group;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	duk_push_number(ctx, group->theta);
	return 1;
}

static
js_Group_set_angle(duk_context* ctx)
{
	group_t* group;
	double theta = duk_require_number(ctx, 0);

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	group->theta = theta;
	return 0;
}

static duk_ret_t
js_Group_get_shader(duk_context* ctx)
{
	duk_push_sphere_obj(ctx, "ShaderProgram", NULL);
	return 1;
}

static duk_ret_t
js_Group_set_shader(duk_context* ctx)
{
	return 0;
}

static
js_Group_get_rotX(duk_context* ctx)
{
	group_t* group;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	duk_push_number(ctx, group->rot_x);
	return 1;
}

static
js_Group_set_rotX(duk_context* ctx)
{
	group_t* group;
	double value = duk_require_number(ctx, 0);

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	group->rot_x = value;
	return 0;
}

static
js_Group_get_rotY(duk_context* ctx)
{
	group_t* group;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	duk_push_number(ctx, group->rot_y);
	return 1;
}

static
js_Group_set_rotY(duk_context* ctx)
{
	group_t* group;
	double value = duk_require_number(ctx, 0);

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	group->rot_y = value;
	return 0;
}

static
js_Group_get_x(duk_context* ctx)
{
	group_t* group;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	duk_push_number(ctx, group->x);
	return 1;
}

static
js_Group_set_x(duk_context* ctx)
{
	group_t* group;
	double value = duk_require_number(ctx, 0);

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	group->x = value;
	return 0;
}

static
js_Group_get_y(duk_context* ctx)
{
	group_t* group;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	duk_push_number(ctx, group->y);
	return 1;
}

static
js_Group_set_y(duk_context* ctx)
{
	group_t* group;
	double value = duk_require_number(ctx, 0);

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	group->y = value;
	return 0;
}

static duk_ret_t
js_Group_draw(duk_context* ctx)
{
	group_t* group;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	duk_pop(ctx);
	draw_group(group);
	return 0;
}

static duk_ret_t
js_GetDefaultShaderProgram(duk_context* ctx)
{
	duk_push_sphere_obj(ctx, "ShaderProgram", NULL);
	return 1;
}

static duk_ret_t
js_new_ShaderProgram(duk_context* ctx)
{
	return 0;
}

static duk_ret_t
js_new_Shape(duk_context* ctx)
{
	duk_require_object_coercible(ctx, 0);
	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Shape(): First argument must be an array");
	image_t* texture = duk_require_sphere_obj(ctx, 1, "Image");

	duk_idx_t stack_idx;
	size_t    num_vertices;
	shape_t*  shape;
	vertex_t  vertex;

	duk_uarridx_t i;

	if (!(shape = new_shape(texture)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Shape(): Failed to create shape object");
	num_vertices = duk_get_length(ctx, 0);
	for (i = 0; i < num_vertices; ++i) {
		duk_get_prop_index(ctx, 0, i); stack_idx = duk_normalize_index(ctx, -1);
		vertex.x = duk_get_prop_string(ctx, stack_idx, "x") ? duk_require_number(ctx, -1) : 0.0f;
		vertex.y = duk_get_prop_string(ctx, stack_idx, "y") ? duk_require_number(ctx, -1) : 0.0f;
		vertex.u = duk_get_prop_string(ctx, stack_idx, "u") ? duk_require_number(ctx, -1) : 0.0f;
		vertex.v = duk_get_prop_string(ctx, stack_idx, "v") ? duk_require_number(ctx, -1) : 0.0f;
		vertex.color = duk_get_prop_string(ctx, stack_idx, "color")
			? duk_require_sphere_color(ctx, -1)
			: rgba(255, 255, 255, 255);
		duk_pop_n(ctx, 6);
		if (!add_vertex(shape, vertex))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Shape(): Vertex buffer allocation failure");
	}
	duk_push_sphere_obj(ctx, "Shape", shape);
	return 1;
}

static duk_ret_t
js_Shape_finalize(duk_context* ctx)
{
	shape_t* shape;

	shape = duk_require_sphere_obj(ctx, 0, "Shape");
	free_shape(shape);
	return 0;
}

static duk_ret_t
js_Shape_get_image(duk_context* ctx)
{
	shape_t* shape;

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	duk_pop(ctx);
	duk_push_sphere_obj(ctx, "Shape", ref_image(get_shape_texture(shape)));
	return 1;
}

static duk_ret_t
js_Shape_set_image(duk_context* ctx)
{
	shape_t* shape;
	image_t* texture = duk_require_sphere_obj(ctx, 0, "Image");

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	duk_pop(ctx);
	set_shape_texture(shape, texture);
	return 0;
}
