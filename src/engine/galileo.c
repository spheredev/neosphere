#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "shader.h"
#include "vector.h"

#include "galileo.h"

static duk_ret_t js_GetDefaultShaderProgram (duk_context* ctx);
static duk_ret_t js_new_Group               (duk_context* ctx);
static duk_ret_t js_Group_finalize          (duk_context* ctx);
static duk_ret_t js_Group_get_shader        (duk_context* ctx);
static duk_ret_t js_Group_get_transform     (duk_context* ctx);
static duk_ret_t js_Group_set_shader        (duk_context* ctx);
static duk_ret_t js_Group_set_transform     (duk_context* ctx);
static duk_ret_t js_Group_draw              (duk_context* ctx);
static duk_ret_t js_Group_setFloat          (duk_context* ctx);
static duk_ret_t js_Group_setInt            (duk_context* ctx);
static duk_ret_t js_Group_setMatrix         (duk_context* ctx);
static duk_ret_t js_new_Shape               (duk_context* ctx);
static duk_ret_t js_Shape_finalize          (duk_context* ctx);
static duk_ret_t js_Shape_get_texture       (duk_context* ctx);
static duk_ret_t js_Shape_set_texture       (duk_context* ctx);
static duk_ret_t js_Shape_draw              (duk_context* ctx);
static duk_ret_t js_new_Transform           (duk_context* ctx);
static duk_ret_t js_Transform_finalize      (duk_context* ctx);
static duk_ret_t js_Transform_compose       (duk_context* ctx);
static duk_ret_t js_Transform_identity      (duk_context* ctx);
static duk_ret_t js_Transform_rotate        (duk_context* ctx);
static duk_ret_t js_Transform_scale         (duk_context* ctx);
static duk_ret_t js_Transform_translate     (duk_context* ctx);

static void assign_default_uv   (shape_t* shape);
static void free_cached_uniform (group_t* group, const char* name);
static bool have_vertex_buffer  (const shape_t* shape);
static void render_shape        (shape_t* shape);

enum uniform_type
{
	UNIFORM_INT,
	UNIFORM_INT_VEC,
	UNIFORM_FLOAT,
	UNIFORM_FLOAT_VEC,
	UNIFORM_MATRIX,
};
struct uniform
{
	char              name[256];
	enum uniform_type type;
	union {
		ALLEGRO_TRANSFORM mat_value;
		int               int_value;
		float             float_value;
	};
};

struct shape
{
	unsigned int           refcount;
	unsigned int           id;
	image_t*               texture;
	shape_type_t           type;
	ALLEGRO_VERTEX*        sw_vbuf;
	int                    max_vertices;
	int                    num_vertices;
	vertex_t*              vertices;
#ifdef MINISPHERE_USE_VERTEX_BUF
	ALLEGRO_VERTEX_BUFFER* vbuf;
#endif
};

struct group
{
	unsigned int refcount;
	unsigned int id;
	shader_t*    shader;
	vector_t*    shapes;
	matrix_t*    transform;
	vector_t*    uniforms;
};

static shader_t*    s_def_shader = NULL;
static unsigned int s_next_group_id = 0;
static unsigned int s_next_shape_id = 0;

void
initialize_galileo(void)
{
	console_log(1, "initializing Galileo");
}

void
shutdown_galileo(void)
{
	console_log(1, "shutting down Galileo");
	shader_free(s_def_shader);
}

shader_t*
get_default_shader(void)
{
	const char* fs_filename;
	char*       fs_pathname;
	const char* vs_filename;
	char*       vs_pathname;

	if (s_def_shader == NULL) {
		console_log(3, "compiling Galileo default shaders");
		vs_filename = kev_read_string(g_sys_conf, "GalileoVertShader", "shaders/galileo.vs.glsl");
		fs_filename = kev_read_string(g_sys_conf, "GalileoFragShader", "shaders/galileo.fs.glsl");
		vs_pathname = strdup(systempath(vs_filename));
		fs_pathname = strdup(systempath(fs_filename));
		s_def_shader = shader_new(vs_pathname, fs_pathname);
		free(vs_pathname);
		free(fs_pathname);
	}
	return s_def_shader;
}

vertex_t
vertex(float x, float y, float z, float u, float v, color_t color)
{
	vertex_t vertex;

	vertex.x = x;
	vertex.y = y;
	vertex.z = z;
	vertex.u = u;
	vertex.v = v;
	vertex.color = color;
	return vertex;
}

group_t*
group_new(shader_t* shader)
{
	group_t* group;

	console_log(4, "creating new group #%u", s_next_group_id);
	
	group = calloc(1, sizeof(group_t));
	group->shapes = vector_new(sizeof(shape_t*));
	group->transform = matrix_new();
	group->shader = shader_ref(shader);
	group->uniforms = vector_new(sizeof(struct uniform));
	
	group->id = s_next_group_id++;
	return group_ref(group);
}

group_t*
group_ref(group_t* group)
{
	if (group != NULL)
		++group->refcount;
	return group;
}

void
group_free(group_t* group)
{
	shape_t** i_shape;
	
	iter_t iter;

	if (group == NULL || --group->refcount > 0)
		return;

	console_log(4, "disposing group #%u no longer in use", group->id);

	iter = vector_enum(group->shapes);
	while (i_shape = vector_next(&iter))
		shape_free(*i_shape);
	vector_free(group->shapes);
	shader_free(group->shader);
	matrix_free(group->transform);
	vector_free(group->uniforms);
	free(group);
}

shader_t*
group_shader(const group_t* group)
{
	return group->shader;
}

matrix_t*
group_transform(const group_t* group)
{
	return group->transform;
}

bool
group_add_shape(group_t* group, shape_t* shape)
{
	console_log(4, "adding shape #%u to group #%u", shape->id, group->id);

	shape = shape_ref(shape);
	vector_push(group->shapes, &shape);
	return true;
}

void
group_draw(const group_t* group, image_t* surface)
{
	iter_t iter;
	struct uniform* p;

	if (surface != NULL)
		al_set_target_bitmap(get_image_bitmap(surface));
	
	shader_use(group->shader != NULL ? group->shader : get_default_shader());
	if (are_shaders_active()) {
		iter = vector_enum(group->uniforms);
		while (p = vector_next(&iter)) {
			switch (p->type) {
			case UNIFORM_FLOAT:
				al_set_shader_float(p->name, p->float_value);
				break;
			case UNIFORM_INT:
				al_set_shader_int(p->name, p->int_value);
				break;
			case UNIFORM_MATRIX:
				al_set_shader_matrix(p->name, &p->mat_value);
				break;
			}
		}
	}

	screen_transform(g_screen, group->transform);
	iter = vector_enum(group->shapes);
	while (vector_next(&iter))
		render_shape(*(shape_t**)iter.ptr);
	screen_transform(g_screen, NULL);
	shader_use(NULL);

	if (surface != NULL)
		al_set_target_backbuffer(screen_display(g_screen));
}

void
group_put_float(group_t* group, const char* name, float value)
{
	struct uniform unif;

	free_cached_uniform(group, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_FLOAT;
	unif.float_value = value;
	vector_push(group->uniforms, &unif);
}

void
group_put_int(group_t* group, const char* name, int value)
{
	struct uniform unif;

	free_cached_uniform(group, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_INT;
	unif.int_value = value;
	vector_push(group->uniforms, &unif);
}

void
group_put_matrix(group_t* group, const char* name, const matrix_t* matrix)
{
	struct uniform unif;

	free_cached_uniform(group, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_MATRIX;
	al_copy_transform(&unif.mat_value, matrix_transform(matrix));
	vector_push(group->uniforms, &unif);
}

shape_t*
shape_new(shape_type_t type, image_t* texture)
{
	shape_t*    shape;
	const char* type_name;

	type_name = type == SHAPE_POINT_LIST ? "point list"
		: type == SHAPE_LINE_LIST ? "line list"
		: type == SHAPE_TRIANGLE_LIST ? "triangle list"
		: type == SHAPE_TRIANGLE_FAN ? "triangle fan"
		: type == SHAPE_TRIANGLE_STRIP ? "triangle strip"
		: "automatic";
	console_log(4, "creating shape #%u as %s", s_next_shape_id, type_name);
	
	shape = calloc(1, sizeof(shape_t));
	shape->texture = ref_image(texture);
	shape->type = type;
	
	shape->id = s_next_shape_id++;
	return shape_ref(shape);
}

shape_t*
shape_ref(shape_t* shape)
{
	if (shape != NULL)
		++shape->refcount;
	return shape;
}

void
shape_free(shape_t* shape)
{
	if (shape == NULL || --shape->refcount > 0)
		return;
	console_log(4, "disposing shape #%u no longer in use", shape->id);
	free_image(shape->texture);
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (shape->vbuf != NULL)
		al_destroy_vertex_buffer(shape->vbuf);
#endif
	free(shape->sw_vbuf);
	free(shape->vertices);
	free(shape);
}

float_rect_t
shape_bounds(const shape_t* shape)
{
	float_rect_t bounds;

	int i;

	if (shape->num_vertices < 1)
		return new_float_rect(0.0, 0.0, 0.0, 0.0);
	bounds = new_float_rect(
		shape->vertices[0].x, shape->vertices[0].y,
		shape->vertices[0].x, shape->vertices[0].y);
	for (i = 1; i < shape->num_vertices; ++i) {
		bounds.x1 = fmin(shape->vertices[i].x, bounds.x1);
		bounds.y1 = fmin(shape->vertices[i].y, bounds.y1);
		bounds.x2 = fmax(shape->vertices[i].x, bounds.x2);
		bounds.y2 = fmax(shape->vertices[i].y, bounds.y2);
	}
	return bounds;
}

image_t*
shape_texture(const shape_t* shape)
{
	return shape->texture;
}

void
shape_set_texture(shape_t* shape, image_t* texture)
{
	image_t* old_texture;
	
	old_texture = shape->texture;
	shape->texture = ref_image(texture);
	free_image(old_texture);
	shape_upload(shape);
}

bool
shape_add_vertex(shape_t* shape, vertex_t vertex)
{
	int      new_max;
	vertex_t *new_buffer;

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
shape_draw(shape_t* shape, matrix_t* matrix, image_t* surface)
{
	if (surface != NULL)
		al_set_target_bitmap(get_image_bitmap(surface));
	screen_transform(g_screen, matrix);
	render_shape(shape);
	screen_transform(g_screen, NULL);
	if (surface != NULL)
		al_set_target_backbuffer(screen_display(g_screen));
}

void
shape_upload(shape_t* shape)
{
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_VERTEX* vertices = NULL;

	int i;

	console_log(3, "uploading shape #%u vertices to GPU", shape->id);
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (shape->vbuf != NULL)
		al_destroy_vertex_buffer(shape->vbuf);
#endif
	free(shape->sw_vbuf); shape->sw_vbuf = NULL;
	bitmap = shape->texture != NULL ? get_image_bitmap(shape->texture) : NULL;

	// create a vertex buffer
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (shape->vbuf = al_create_vertex_buffer(NULL, NULL, shape->num_vertices, ALLEGRO_PRIM_BUFFER_STATIC))
		vertices = al_lock_vertex_buffer(shape->vbuf, 0, shape->num_vertices, ALLEGRO_LOCK_WRITEONLY);
#endif
	if (vertices == NULL) {
		// hardware buffer couldn't be created, fall back to software
		console_log(3, "unable to create a VBO for shape #%u", shape->id);
		if (!(shape->sw_vbuf = malloc(shape->num_vertices * sizeof(ALLEGRO_VERTEX))))
			return;
		vertices = shape->sw_vbuf;
	}

	// upload vertices
	for (i = 0; i < shape->num_vertices; ++i) {
		vertices[i].x = shape->vertices[i].x;
		vertices[i].y = shape->vertices[i].y;
		vertices[i].z = 0;
		vertices[i].color = nativecolor(shape->vertices[i].color);
		vertices[i].u = shape->vertices[i].u;
		vertices[i].v = shape->vertices[i].v;
	}

	// unlock hardware buffer, if applicable
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (vertices != shape->sw_vbuf)
		al_unlock_vertex_buffer(shape->vbuf);
	else if (shape->vbuf != NULL) {
		al_destroy_vertex_buffer(shape->vbuf);
		shape->vbuf = NULL;
	}
#endif
}

static void
assign_default_uv(shape_t* shape)
{
	// this assigns default UV coordinates to a shape's vertices. note that clockwise
	// winding from top left is assumed; if the shape is wound any other way, the
	// texture will be rotated accordingly. if this is not what you want, explicit U/V
	// coordinates should be supplied.
	
	double phi;

	int i;

	console_log(4, "auto-calculating U/V for shape #%u", shape->id);

	for (i = 0; i < shape->num_vertices; ++i) {
		// circumscribe the UV coordinate space.
		// the circumcircle is rotated 135 degrees counterclockwise, which ensures
		// that the top-left corner of a clockwise quad is mapped to (0,0).
		phi = 2 * M_PI * i / shape->num_vertices - M_PI_4 * 3;
		shape->vertices[i].u = cos(phi) * M_SQRT1_2 + 0.5;
		shape->vertices[i].v = sin(phi) * -M_SQRT1_2 + 0.5;
	}
}

static bool
have_vertex_buffer(const shape_t* shape)
{
#ifdef MINISPHERE_USE_VERTEX_BUF
	return shape->vbuf != NULL || shape->sw_vbuf != NULL;
#else
	return shape->sw_vbuf != NULL;
#endif
}

static void
free_cached_uniform(group_t* group, const char* name)
{
	iter_t iter;
	struct uniform* p;

	iter = vector_enum(group->uniforms);
	while (p = vector_next(&iter)) {
		if (strcmp(p->name, name) == 0)
			iter_remove(&iter);
	}
}

static void
render_shape(shape_t* shape)
{
	ALLEGRO_BITMAP* bitmap;
	int             draw_mode;

	if (shape->num_vertices == 0)
		return;

	if (!have_vertex_buffer(shape))
		shape_upload(shape);
	if (shape->type == SHAPE_AUTO)
		draw_mode = shape->num_vertices == 1 ? ALLEGRO_PRIM_POINT_LIST
			: shape->num_vertices == 2 ? ALLEGRO_PRIM_LINE_LIST
			: ALLEGRO_PRIM_TRIANGLE_STRIP;
	else
		draw_mode = shape->type == SHAPE_LINE_LIST ? ALLEGRO_PRIM_LINE_LIST
			: shape->type == SHAPE_TRIANGLE_LIST ? ALLEGRO_PRIM_TRIANGLE_LIST
			: shape->type == SHAPE_TRIANGLE_STRIP ? ALLEGRO_PRIM_TRIANGLE_STRIP
			: shape->type == SHAPE_TRIANGLE_FAN ? ALLEGRO_PRIM_TRIANGLE_FAN
			: ALLEGRO_PRIM_POINT_LIST;
	
	bitmap = shape->texture != NULL ? get_image_bitmap(shape->texture) : NULL;
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (shape->vbuf != NULL)
		al_draw_vertex_buffer(shape->vbuf, bitmap, 0, shape->num_vertices, draw_mode);
	else
		al_draw_prim(shape->sw_vbuf, NULL, bitmap, 0, shape->num_vertices, draw_mode);
#else
	al_draw_prim(shape->sw_vbuf, NULL, bitmap, 0, shape->num_vertices, draw_mode);
#endif
}

void
init_galileo_api(void)
{
	register_api_const(g_duk, "SHAPE_AUTO", SHAPE_AUTO);
	register_api_const(g_duk, "SHAPE_POINTS", SHAPE_POINT_LIST);
	register_api_const(g_duk, "SHAPE_LINES", SHAPE_LINE_LIST);
	register_api_const(g_duk, "SHAPE_TRIANGLES", SHAPE_TRIANGLE_LIST);
	register_api_const(g_duk, "SHAPE_TRI_STRIP", SHAPE_TRIANGLE_STRIP);
	register_api_const(g_duk, "SHAPE_TRI_FAN", SHAPE_TRIANGLE_FAN);

	register_api_method(g_duk, NULL, "GetDefaultShaderProgram", js_GetDefaultShaderProgram);

	register_api_ctor(g_duk, "Group", js_new_Group, js_Group_finalize);
	register_api_prop(g_duk, "Group", "shader", js_Group_get_shader, js_Group_set_shader);
	register_api_prop(g_duk, "Group", "transform", js_Group_get_transform, js_Group_set_transform);
	register_api_method(g_duk, "Group", "draw", js_Group_draw);
	register_api_method(g_duk, "Group", "setFloat", js_Group_setFloat);
	register_api_method(g_duk, "Group", "setInt", js_Group_setInt);
	register_api_method(g_duk, "Group", "setMatrix", js_Group_setMatrix);

	register_api_ctor(g_duk, "Shape", js_new_Shape, js_Shape_finalize);
	register_api_prop(g_duk, "Shape", "texture", js_Shape_get_texture, js_Shape_set_texture);
	register_api_method(g_duk, "Shape", "draw", js_Shape_draw);

	register_api_ctor(g_duk, "Transform", js_new_Transform, js_Transform_finalize);
	register_api_method(g_duk, "Transform", "compose", js_Transform_compose);
	register_api_method(g_duk, "Transform", "identity", js_Transform_identity);
	register_api_method(g_duk, "Transform", "rotate", js_Transform_rotate);
	register_api_method(g_duk, "Transform", "scale", js_Transform_scale);
	register_api_method(g_duk, "Transform", "translate", js_Transform_translate);
}

static duk_ret_t
js_new_Group(duk_context* ctx)
{
	group_t*  group;
	int       num_args;
	size_t    num_shapes;
	shader_t* shader;
	shape_t*  shape;

	duk_uarridx_t i;

	num_args = duk_get_top(ctx);
	duk_require_object_coercible(ctx, 0);
	shader = num_args >= 2
		? duk_require_sphere_obj(ctx, 1, "ShaderProgram")
		: get_default_shader();

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "argument 1 to Group() must be an array");
	if (!(group = group_new(shader)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to create Galileo group");
	num_shapes = duk_get_length(ctx, 0);
	for (i = 0; i < num_shapes; ++i) {
		duk_get_prop_index(ctx, 0, i);
		shape = duk_require_sphere_obj(ctx, -1, "Shape");
		group_add_shape(group, shape);
	}
	duk_push_sphere_obj(ctx, "Group", group);
	return 1;
}

static duk_ret_t
js_Group_finalize(duk_context* ctx)
{
	group_t* group;

	group = duk_require_sphere_obj(ctx, 0, "Group");
	group_free(group);
	return 0;
}

static duk_ret_t
js_Group_get_shader(duk_context* ctx)
{
	group_t*  group;
	shader_t* shader;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");

	shader = group_shader(group);
	duk_push_sphere_obj(ctx, "ShaderProgram", shader_ref(shader));
	return 1;
}

static duk_ret_t
js_Group_get_transform(duk_context* ctx)
{
	group_t*  group;
	matrix_t* matrix;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");

	matrix = group_transform(group);
	duk_push_sphere_obj(ctx, "Transform", matrix_ref(matrix));
	return 1;
}

static duk_ret_t
js_Group_set_shader(duk_context* ctx)
{
	group_t*  group;
	shader_t* old_shader;
	shader_t* shader;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	shader = duk_require_sphere_obj(ctx, 0, "ShaderProgram");

	old_shader = group->shader;
	group->shader = shader_ref(shader);
	shader_free(old_shader);
	return 0;
}

static duk_ret_t
js_Group_set_transform(duk_context* ctx)
{
	group_t*  group;
	matrix_t* matrix;
	matrix_t* old_matrix;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	matrix = duk_require_sphere_obj(ctx, 0, "Transform");

	old_matrix = group->transform;
	group->transform = matrix_ref(matrix);
	matrix_free(old_matrix);
	return 0;
}

static duk_ret_t
js_Group_draw(duk_context* ctx)
{
	group_t* group;
	int      num_args;
	image_t* surface;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	group = duk_require_sphere_obj(ctx, -1, "Group");
	surface = num_args >= 1 ? duk_require_sphere_obj(ctx, 0, "Surface")
		: NULL;

	if (!screen_is_skipframe(g_screen))
		group_draw(group, surface);
	return 0;
}

static duk_ret_t
js_Group_setFloat(duk_context* ctx)
{
	group_t*    group;
	const char* name;
	float       value;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	value = duk_require_number(ctx, 1);

	group_put_float(group, name, value);
	return 1;
}

static duk_ret_t
js_Group_setInt(duk_context* ctx)
{
	group_t*    group;
	const char* name;
	int         value;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	value = duk_require_int(ctx, 1);

	group_put_int(group, name, value);
	return 1;
}

static duk_ret_t
js_Group_setMatrix(duk_context* ctx)
{
	group_t*    group;
	matrix_t*   matrix;
	const char* name;

	duk_push_this(ctx);
	group = duk_require_sphere_obj(ctx, -1, "Group");
	name = duk_require_string(ctx, 0);
	matrix = duk_require_sphere_obj(ctx, 1, "Transform");

	group_put_matrix(group, name, matrix);
	return 1;
}

static duk_ret_t
js_GetDefaultShaderProgram(duk_context* ctx)
{
	shader_t* shader;
	
	if (!(shader = get_default_shader()))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "GetDefaultShaderProgram(): no default shader available or shader couldn't be built");
	duk_push_sphere_obj(ctx, "ShaderProgram", shader_ref(shader));
	return 1;
}

static duk_ret_t
js_new_Shape(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	duk_require_object_coercible(ctx, 0);
	image_t* texture = duk_is_null(ctx, 1) ? NULL : duk_require_sphere_obj(ctx, 1, "Image");
	shape_type_t type = n_args >= 3 ? duk_require_int(ctx, 2) : SHAPE_AUTO;

	bool      is_missing_uv = false;
	size_t    num_vertices;
	shape_t*  shape;
	duk_idx_t stack_idx;
	vertex_t  vertex;

	duk_uarridx_t i;

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Shape(): first argument must be an array");
	if (type < 0 || type >= SHAPE_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Shape(): invalid shape type constant");
	if (!(shape = shape_new(type, texture)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Shape(): unable to create shape object");
	num_vertices = duk_get_length(ctx, 0);
	for (i = 0; i < num_vertices; ++i) {
		duk_get_prop_index(ctx, 0, i); stack_idx = duk_normalize_index(ctx, -1);
		vertex.x = duk_get_prop_string(ctx, stack_idx, "x") ? duk_require_number(ctx, -1) : 0.0f;
		vertex.y = duk_get_prop_string(ctx, stack_idx, "y") ? duk_require_number(ctx, -1) : 0.0f;
		if (duk_get_prop_string(ctx, stack_idx, "z"))
			vertex.z = duk_require_number(ctx, -1);
		if (duk_get_prop_string(ctx, stack_idx, "u"))
			vertex.u = duk_require_number(ctx, -1);
		else
			is_missing_uv = true;
		if (duk_get_prop_string(ctx, stack_idx, "v"))
			vertex.v = duk_require_number(ctx, -1);
		else
			is_missing_uv = true;
		vertex.color = duk_get_prop_string(ctx, stack_idx, "color")
			? duk_require_sphere_color(ctx, -1)
			: color_new(255, 255, 255, 255);
		duk_pop_n(ctx, 6);
		if (!shape_add_vertex(shape, vertex))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Shape(): vertex list allocation failure");
	}
	if (is_missing_uv)
		assign_default_uv(shape);
	shape_upload(shape);
	duk_push_sphere_obj(ctx, "Shape", shape);
	return 1;
}

static duk_ret_t
js_Shape_finalize(duk_context* ctx)
{
	shape_t* shape;

	shape = duk_require_sphere_obj(ctx, 0, "Shape");
	shape_free(shape);
	return 0;
}

static duk_ret_t
js_Shape_get_texture(duk_context* ctx)
{
	shape_t* shape;

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	duk_pop(ctx);
	duk_push_sphere_obj(ctx, "Shape", ref_image(shape_texture(shape)));
	return 1;
}

static duk_ret_t
js_Shape_set_texture(duk_context* ctx)
{
	shape_t* shape;
	image_t* texture;

	duk_push_this(ctx);
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	texture = duk_require_sphere_obj(ctx, 0, "Image");

	shape_set_texture(shape, texture);
	return 0;
}

static duk_ret_t
js_Shape_draw(duk_context* ctx)
{
	int       num_args;
	shape_t*  shape;
	image_t*  surface = NULL;
	matrix_t* transform = NULL;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	shape = duk_require_sphere_obj(ctx, -1, "Shape");
	if (num_args >= 1)
		surface = duk_require_sphere_obj(ctx, 0, "Surface");
	if (num_args >= 2)
		transform = duk_require_sphere_obj(ctx, 1, "Transform");

	shape_draw(shape, transform, surface);
	return 0;
}

static duk_ret_t
js_new_Transform(duk_context* ctx)
{
	matrix_t* matrix;
	
	matrix = matrix_new();
	duk_push_sphere_obj(ctx, "Transform", matrix);
	return 1;
}

static duk_ret_t
js_Transform_finalize(duk_context* ctx)
{
	matrix_t* matrix;

	matrix = duk_require_sphere_obj(ctx, 0, "Transform");

	matrix_free(matrix);
	return 0;
}

static duk_ret_t
js_Transform_compose(duk_context* ctx)
{
	matrix_t* matrix;
	matrix_t* other;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	other = duk_require_sphere_obj(ctx, 0, "Transform");

	matrix_compose(matrix, other);
	return 1;
}

static duk_ret_t
js_Transform_identity(duk_context* ctx)
{
	matrix_t* matrix;

	duk_push_this(ctx);
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");

	matrix_identity(matrix);
	return 1;
}

static duk_ret_t
js_Transform_rotate(duk_context* ctx)
{
	matrix_t* matrix;
	int       num_args;
	float     theta;
	float     vx = 0.0;
	float     vy = 0.0;
	float     vz = 1.0;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	theta = duk_require_number(ctx, 0);
	if (num_args >= 2) {
		vx = duk_require_number(ctx, 1);
		vy = duk_require_number(ctx, 2);
		vz = duk_require_number(ctx, 3);
	}

	matrix_rotate(matrix, theta, vx, vy, vz);
	return 1;
}

static duk_ret_t
js_Transform_scale(duk_context* ctx)
{
	matrix_t* matrix;
	int       num_args;
	float     sx;
	float     sy;
	float     sz = 1.0;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	sx = duk_require_number(ctx, 0);
	sy = duk_require_number(ctx, 1);
	if (num_args >= 3)
		sz = duk_require_number(ctx, 2);

	matrix_scale(matrix, sx, sy, sz);
	return 1;
}

static duk_ret_t
js_Transform_translate(duk_context* ctx)
{
	matrix_t* matrix;
	int       num_args;
	float     dx;
	float     dy;
	float     dz = 0.0;

	duk_push_this(ctx);
	num_args = duk_get_top(ctx) - 1;
	matrix = duk_require_sphere_obj(ctx, -1, "Transform");
	dx = duk_require_number(ctx, 0);
	dy = duk_require_number(ctx, 1);
	if (num_args >= 3)
		dz = duk_require_number(ctx, 2);

	matrix_translate(matrix, dx, dy, dz);
	return 1;
}
