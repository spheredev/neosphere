#include "minisphere.h"
#include "galileo.h"

#include "color.h"
#include "shader.h"
#include "vector.h"

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
	console_log(1, "initializing Galileo subsystem");
}

void
shutdown_galileo(void)
{
	console_log(1, "shutting down Galileo subsystem");
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
		vs_pathname = strdup(system_path(vs_filename));
		fs_pathname = strdup(system_path(fs_filename));
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
group_get_shader(const group_t* group)
{
	return group->shader;
}

matrix_t*
group_get_transform(const group_t* group)
{
	return group->transform;
}

void
group_set_shader(group_t* group, shader_t* shader)
{
	shader_t* old_shader;

	old_shader = group->shader;
	group->shader = shader_ref(shader);
	shader_free(old_shader);
}

void
group_set_transform(group_t* group, matrix_t* transform)
{
	matrix_t* old_transform;

	old_transform = group->transform;
	group->transform = matrix_ref(transform);
	matrix_free(old_transform);
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
		al_set_target_bitmap(image_bitmap(surface));

#if defined(MINISPHERE_USE_SHADERS)
	if (are_shaders_active()) {
		shader_use(group->shader != NULL ? group->shader : get_default_shader());
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
#endif

	screen_transform(g_screen, group->transform);
	iter = vector_enum(group->shapes);
	while (vector_next(&iter))
		render_shape(*(shape_t**)iter.ptr);
	screen_transform(g_screen, NULL);

#if defined(MINISPHERE_USE_SHADERS)
	shader_use(NULL);
#endif

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

	type_name = type == SHAPE_POINTS ? "point list"
		: type == SHAPE_LINES ? "line list"
		: type == SHAPE_LINE_LOOP ? "line loop"
		: type == SHAPE_TRIANGLES ? "triangle list"
		: type == SHAPE_TRI_FAN ? "triangle fan"
		: type == SHAPE_TRI_STRIP ? "triangle strip"
		: "automatic";
	console_log(4, "creating shape #%u as %s", s_next_shape_id, type_name);

	shape = calloc(1, sizeof(shape_t));
	shape->texture = image_ref(texture);
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
	image_free(shape->texture);
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
	shape->texture = image_ref(texture);
	image_free(old_texture);
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
shape_calculate_uv(shape_t* shape)
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

void
shape_draw(shape_t* shape, matrix_t* matrix)
{
	screen_transform(g_screen, matrix);
	render_shape(shape);
	screen_transform(g_screen, NULL);
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
	bitmap = shape->texture != NULL ? image_bitmap(shape->texture) : NULL;

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
		vertices[i].z = shape->vertices[i].z;
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
		draw_mode = shape->type == SHAPE_LINES ? ALLEGRO_PRIM_LINE_LIST
			: shape->type == SHAPE_LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
			: shape->type == SHAPE_LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
			: shape->type == SHAPE_TRIANGLES ? ALLEGRO_PRIM_TRIANGLE_LIST
			: shape->type == SHAPE_TRI_STRIP ? ALLEGRO_PRIM_TRIANGLE_STRIP
			: shape->type == SHAPE_TRI_FAN ? ALLEGRO_PRIM_TRIANGLE_FAN
			: ALLEGRO_PRIM_POINT_LIST;

	bitmap = shape->texture != NULL ? image_bitmap(shape->texture) : NULL;
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (shape->vbuf != NULL)
		al_draw_vertex_buffer(shape->vbuf, bitmap, 0, shape->num_vertices, draw_mode);
	else
		al_draw_prim(shape->sw_vbuf, NULL, bitmap, 0, shape->num_vertices, draw_mode);
#else
	al_draw_prim(shape->sw_vbuf, NULL, bitmap, 0, shape->num_vertices, draw_mode);
#endif
}
