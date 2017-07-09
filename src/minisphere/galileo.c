#include "minisphere.h"
#include "galileo.h"

#include "color.h"
#include "vector.h"

static void free_cached_uniform (model_t* model, const char* name);
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
	int               vector_size;
	union {
		ALLEGRO_TRANSFORM mat_value;
		int               int_value;
		int               intvec[4];
		float             float_value;
		float             floatvec[4];
	};
};

struct model
{
	unsigned int refcount;
	unsigned int id;
	shader_t*    shader;
	vector_t*    shapes;
	transform_t* transform;
	vector_t*    uniforms;
};

struct shader
{
	unsigned int   id;
	unsigned int   refcount;
	ALLEGRO_SHADER* program;
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

static shader_t*    s_def_shader = NULL;
static bool         s_have_shaders = false;
static unsigned int s_next_group_id = 0;
static unsigned int s_next_shader_id = 1;
static unsigned int s_next_shape_id = 0;

void
galileo_init(bool programmable)
{
	console_log(1, "initializing Galileo subsystem");
	s_have_shaders = programmable;
}

void
galileo_uninit(void)
{
	console_log(1, "shutting down Galileo subsystem");
	shader_free(s_def_shader);
}

bool
galileo_programmable(void)
{
	return s_have_shaders;
}

shader_t*
galileo_shader(void)
{
	const char* fs_filename;
	char*       fs_pathname;
	const char* vs_filename;
	char*       vs_pathname;

	if (s_def_shader == NULL) {
		console_log(3, "compiling Galileo default shaders");
		vs_filename = kev_read_string(g_sys_conf, "GalileoVertShader", "shaders/galileo.vert.glsl");
		fs_filename = kev_read_string(g_sys_conf, "GalileoFragShader", "shaders/galileo.frag.glsl");
		vs_pathname = strdup(system_path(vs_filename));
		fs_pathname = strdup(system_path(fs_filename));
		s_def_shader = shader_new(vs_pathname, fs_pathname);
		free(vs_pathname);
		free(fs_pathname);
	}
	return s_def_shader;
}

model_t*
model_new(shader_t* shader)
{
	model_t* model;

	console_log(4, "creating new model #%u", s_next_group_id);

	model = calloc(1, sizeof(model_t));
	model->shapes = vector_new(sizeof(shape_t*));
	model->transform = transform_new();
	model->shader = shader_ref(shader);
	model->uniforms = vector_new(sizeof(struct uniform));

	model->id = s_next_group_id++;
	return model_ref(model);
}

model_t*
model_ref(model_t* it)
{
	if (it != NULL)
		++it->refcount;
	return it;
}

void
model_free(model_t* it)
{
	shape_t** i_shape;

	iter_t iter;

	if (it == NULL || --it->refcount > 0)
		return;

	console_log(4, "disposing model #%u no longer in use", it->id);

	iter = vector_enum(it->shapes);
	while (i_shape = vector_next(&iter))
		shape_free(*i_shape);
	vector_free(it->shapes);
	shader_free(it->shader);
	transform_free(it->transform);
	vector_free(it->uniforms);
	free(it);
}

shader_t*
model_get_shader(const model_t* it)
{
	return it->shader;
}

transform_t*
model_get_transform(const model_t* it)
{
	return it->transform;
}

void
model_set_shader(model_t* it, shader_t* shader)
{
	shader_t* old_shader;

	old_shader = it->shader;
	it->shader = shader_ref(shader);
	shader_free(old_shader);
}

void
model_set_transform(model_t* it, transform_t* transform)
{
	transform_t* old_transform;

	old_transform = it->transform;
	it->transform = transform_ref(transform);
	transform_free(old_transform);
}

bool
model_add_shape(model_t* it, shape_t* shape)
{
	console_log(4, "adding it #%u to model #%u", shape->id, it->id);

	shape = shape_ref(shape);
	vector_push(it->shapes, &shape);
	return true;
}

void
model_draw(const model_t* it, image_t* surface)
{
	iter_t iter;
	struct uniform* p;

	if (surface != NULL)
		image_render_to(surface, it->transform);
	else
		screen_render_to(g_screen, it->transform);

	if (s_have_shaders) {
		shader_use(it->shader != NULL ? it->shader : galileo_shader());
		iter = vector_enum(it->uniforms);
		while (p = vector_next(&iter)) {
			switch (p->type) {
			case UNIFORM_FLOAT:
				al_set_shader_float(p->name, p->float_value);
				break;
			case UNIFORM_FLOAT_VEC:
				al_set_shader_float_vector(p->name, p->vector_size, p->floatvec, 1);
				break;
			case UNIFORM_INT:
				al_set_shader_int(p->name, p->int_value);
				break;
			case UNIFORM_INT_VEC:
				al_set_shader_int_vector(p->name, p->vector_size, p->intvec, 1);
				break;
			case UNIFORM_MATRIX:
				al_set_shader_matrix(p->name, &p->mat_value);
				break;
			}
		}
	}

	iter = vector_enum(it->shapes);
	while (vector_next(&iter))
		render_shape(*(shape_t**)iter.ptr);
	screen_render_to(g_screen, NULL);

	shader_use(NULL);

	if (surface != NULL)
		al_set_target_bitmap(screen_backbuffer(g_screen));
}

void
model_put_float(model_t* it, const char* name, float value)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_FLOAT;
	unif.float_value = value;
	vector_push(it->uniforms, &unif);
}

void
model_put_float_vector(model_t* it, const char* name, float values[], int size)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_FLOAT_VEC;
	unif.vector_size = size;
	memcpy(unif.floatvec, values, sizeof(float) * size);
	vector_push(it->uniforms, &unif);
}

void
model_put_int(model_t* it, const char* name, int value)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_INT;
	unif.int_value = value;
	vector_push(it->uniforms, &unif);
}

void
model_put_int_vector(model_t* it, const char* name, int values[], int size)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_INT_VEC;
	unif.vector_size = size;
	memcpy(unif.intvec, values, sizeof(int) * size);
	vector_push(it->uniforms, &unif);
}

void
model_put_matrix(model_t* it, const char* name, const transform_t* matrix)
{
	struct uniform unif;

	free_cached_uniform(it, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_MATRIX;
	al_copy_transform(&unif.mat_value, transform_matrix(matrix));
	vector_push(it->uniforms, &unif);
}

shader_t*
shader_new(const char* vs_filename, const char* fs_filename)
{
	char*      fs_source = NULL;
	char*      vs_source = NULL;
	shader_t*  shader;

	shader = calloc(1, sizeof(shader_t));

	console_log(2, "compiling new shader program #%u", s_next_shader_id);

	if (!(vs_source = sfs_fslurp(g_fs, vs_filename, NULL, NULL)))
		goto on_error;
	if (!(fs_source = sfs_fslurp(g_fs, fs_filename, NULL, NULL)))
		goto on_error;
	if (!(shader->program = al_create_shader(ALLEGRO_SHADER_GLSL)))
		goto on_error;
	if (!al_attach_shader_source(shader->program, ALLEGRO_VERTEX_SHADER, vs_source)) {
		fprintf(stderr, "\nvertex shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_attach_shader_source(shader->program, ALLEGRO_PIXEL_SHADER, fs_source)) {
		fprintf(stderr, "\nfragment shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_build_shader(shader->program)) {
		fprintf(stderr, "\nerror building shader program:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	free(vs_source);
	free(fs_source);

	shader->id = s_next_shader_id++;
	return shader_ref(shader);

on_error:
	free(vs_source);
	free(fs_source);
	if (shader->program != NULL)
		al_destroy_shader(shader->program);
	free(shader);
	return NULL;
}

shader_t*
shader_ref(shader_t* shader)
{
	if (shader == NULL)
		return shader;
	++shader->refcount;
	return shader;
}

void
shader_free(shader_t* shader)
{
	if (shader == NULL || --shader->refcount > 0)
		return;

	console_log(3, "disposing shader program #%u no longer in use", shader->id);
	al_destroy_shader(shader->program);
	free(shader);
}

bool
shader_use(shader_t* shader)
{
	ALLEGRO_SHADER* al_shader;

	if (shader != NULL)
		console_log(4, "activating shader program #%u", shader->id);
	else
		console_log(4, "activating null shader");
	if (s_have_shaders) {
		al_shader = shader != NULL ? shader->program : NULL;
		if (!al_use_shader(al_shader))
			return false;
		return true;
	}
	else {
		// if shaders are not supported, degrade gracefully. this simplifies the rest
		// of the engine, which simply assumes shaders are always supported.
		return true;
	}
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
	console_log(4, "creating it #%u as %s", s_next_shape_id, type_name);

	shape = calloc(1, sizeof(shape_t));
	shape->texture = image_ref(texture);
	shape->type = type;

	shape->id = s_next_shape_id++;
	return shape_ref(shape);
}

shape_t*
shape_ref(shape_t* it)
{
	if (it != NULL)
		++it->refcount;
	return it;
}

void
shape_free(shape_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	console_log(4, "disposing it #%u no longer in use", it->id);
	image_free(it->texture);
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (it->vbuf != NULL)
		al_destroy_vertex_buffer(it->vbuf);
#endif
	free(it->sw_vbuf);
	free(it->vertices);
	free(it);
}

float_rect_t
shape_bounds(const shape_t* it)
{
	float_rect_t bounds;

	int i;

	if (it->num_vertices < 1)
		return new_float_rect(0.0, 0.0, 0.0, 0.0);
	bounds = new_float_rect(
		it->vertices[0].x, it->vertices[0].y,
		it->vertices[0].x, it->vertices[0].y);
	for (i = 1; i < it->num_vertices; ++i) {
		bounds.x1 = fmin(it->vertices[i].x, bounds.x1);
		bounds.y1 = fmin(it->vertices[i].y, bounds.y1);
		bounds.x2 = fmax(it->vertices[i].x, bounds.x2);
		bounds.y2 = fmax(it->vertices[i].y, bounds.y2);
	}
	return bounds;
}

image_t*
shape_texture(const shape_t* it)
{
	return it->texture;
}

void
shape_set_texture(shape_t* it, image_t* texture)
{
	image_t* old_texture;

	old_texture = it->texture;
	it->texture = image_ref(texture);
	image_free(old_texture);
	shape_upload(it);
}

bool
shape_add_vertex(shape_t* it, vertex_t vertex)
{
	int      new_max;
	vertex_t *new_buffer;

	if (it->num_vertices + 1 > it->max_vertices) {
		new_max = (it->num_vertices + 1) * 2;
		if (!(new_buffer = realloc(it->vertices, new_max * sizeof(vertex_t))))
			return false;
		it->vertices = new_buffer;
		it->max_vertices = new_max;
	}
	++it->num_vertices;
	it->vertices[it->num_vertices - 1] = vertex;
	return true;
}

void
shape_calculate_uv(shape_t* it)
{
	// this assigns default UV coordinates to a shape's vertices. note that clockwise
	// winding from top left is assumed; if the shape is wound any other way, the
	// texture will be rotated accordingly. if this is not what you want, explicit U/V
	// coordinates should be supplied.

	double phi;

	int i;

	console_log(4, "auto-calculating U/V for it #%u", it->id);

	for (i = 0; i < it->num_vertices; ++i) {
		// circumscribe the UV coordinate space.
		// the circumcircle is rotated 135 degrees counterclockwise, which ensures
		// that the top-left corner of a clockwise quad is mapped to (0,0).
		phi = 2 * M_PI * i / it->num_vertices - M_PI_4 * 3;
		it->vertices[i].u = cos(phi) * M_SQRT1_2 + 0.5;
		it->vertices[i].v = sin(phi) * -M_SQRT1_2 + 0.5;
	}
}

void
shape_draw(shape_t* it, transform_t* matrix)
{
	screen_render_to(g_screen, matrix);
	render_shape(it);
	screen_render_to(g_screen, NULL);
}

void
shape_upload(shape_t* it)
{
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_VERTEX* vertices = NULL;

	int i;

	console_log(3, "uploading it #%u vertices to GPU", it->id);
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (it->vbuf != NULL)
		al_destroy_vertex_buffer(it->vbuf);
#endif
	free(it->sw_vbuf); it->sw_vbuf = NULL;
	bitmap = it->texture != NULL ? image_bitmap(it->texture) : NULL;

	// create a vertex buffer
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (it->vbuf = al_create_vertex_buffer(NULL, NULL, it->num_vertices, ALLEGRO_PRIM_BUFFER_STATIC))
		vertices = al_lock_vertex_buffer(it->vbuf, 0, it->num_vertices, ALLEGRO_LOCK_WRITEONLY);
#endif
	if (vertices == NULL) {
		// hardware buffer couldn't be created, fall back to software
		console_log(3, "unable to create a VBO for it #%u", it->id);
		if (!(it->sw_vbuf = malloc(it->num_vertices * sizeof(ALLEGRO_VERTEX))))
			return;
		vertices = it->sw_vbuf;
	}

	// upload vertices
	for (i = 0; i < it->num_vertices; ++i) {
		vertices[i].x = it->vertices[i].x;
		vertices[i].y = it->vertices[i].y;
		vertices[i].z = it->vertices[i].z;
		vertices[i].color = nativecolor(it->vertices[i].color);
		vertices[i].u = it->vertices[i].u;
		vertices[i].v = it->vertices[i].v;
	}

	// unlock hardware buffer, if applicable
#ifdef MINISPHERE_USE_VERTEX_BUF
	if (vertices != it->sw_vbuf)
		al_unlock_vertex_buffer(it->vbuf);
	else if (it->vbuf != NULL) {
		al_destroy_vertex_buffer(it->vbuf);
		it->vbuf = NULL;
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
free_cached_uniform(model_t* group, const char* name)
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
