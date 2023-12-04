/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "neosphere.h"
#include "galileo.h"

#include "color.h"
#include "vector.h"

static void free_cached_uniform (shader_t* shader, const char* name);
static void render_shape        (shape_t* shape);

enum uniform_type
{
	UNIFORM_BOOL,
	UNIFORM_INT,
	UNIFORM_INT_ARR,
	UNIFORM_INT_VEC,
	UNIFORM_FLOAT,
	UNIFORM_FLOAT_ARR,
	UNIFORM_FLOAT_VEC,
	UNIFORM_MATRIX,
	UNIFORM_SAMPLER
};

struct uniform
{
	char              name[256];
	enum uniform_type type;
	int               num_values;
	union {
		bool              bool_value;
		int*              int_list;
		int               int_value;
		int               int_vec[4];
		float*            float_list;
		float             float_value;
		float             float_vec[4];
		ALLEGRO_TRANSFORM mat_value;
		struct sampler {
			image_t* texture;
			int      texture_unit;
		} sampler;
	};
};

struct ibo
{
	unsigned int          refcount;
	ALLEGRO_INDEX_BUFFER* buffer;
	vector_t*             indices;
};

struct model
{
	unsigned int refcount;
	unsigned int id;
	shader_t*    shader;
	vector_t*    shapes;
	transform_t* transform;
};

struct shader
{
	unsigned int    id;
	unsigned int    refcount;
	char*           fragment_path;
	ALLEGRO_SHADER* program;
	vector_t*       uniforms;
	char*           vertex_path;
};

struct shape
{
	unsigned int refcount;
	unsigned int id;
	ibo_t*       ibo;
	image_t*     texture;
	shape_type_t type;
	vbo_t*       vbo;
};

struct vbo
{
	unsigned int           refcount;
	ALLEGRO_VERTEX_BUFFER* buffer;
	vector_t*              vertices;
};

static shader_t*    s_def_shader;
static shader_t*    s_last_shader;
static unsigned int s_next_model_id = 1;
static unsigned int s_next_shader_id = 1;
static unsigned int s_next_shape_id = 1;

void
galileo_init(void)
{
	console_log(1, "initializing Galileo subsystem");
	s_def_shader = NULL;
	s_last_shader = NULL;
}

void
galileo_uninit(void)
{
	console_log(1, "shutting down Galileo subsystem");
	shader_unref(s_def_shader);
}

shader_t*
galileo_shader(void)
{
	if (s_def_shader == NULL) {
		console_log(3, "compiling Galileo default shaders");
		s_def_shader = shader_new(
			"#/shaders/default.vert.glsl",
			"#/shaders/default.frag.glsl");
	}
	return s_def_shader;
}

void
galileo_reset(void)
{
	// note: this resets internal render state to the default settings for Sphere v1.
	//       Sv1 APIs should call this before drawing anything; doing so avoids Galileo
	//       having to undo its own state changes all the time, keeping things snappy.

	image_render_to(screen_backbuffer(g_screen), NULL);
	shader_use(NULL, false);
}

ibo_t*
ibo_new(void)
{
	ibo_t* ibo;

	if (!(ibo = calloc(1, sizeof(ibo_t))))
		return NULL;
	ibo->indices = vector_new(sizeof(uint16_t));
	return ibo_ref(ibo);
}

ibo_t*
ibo_ref(ibo_t* it)
{
	if (it == NULL)
		return it;
	++it->refcount;
	return it;
}

void
ibo_unref(ibo_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	if (it->buffer != NULL)
		al_destroy_index_buffer(it->buffer);
	vector_free(it->indices);
	free(it);
}

ALLEGRO_INDEX_BUFFER*
ibo_buffer(const ibo_t* it)
{
	return it->buffer;
}

int
ibo_len(const ibo_t* it)
{
	if (it != NULL)
		return vector_len(it->indices);
	else
		return 0;
}

void
ibo_add_index(ibo_t* it, uint16_t index)
{
	vector_push(it->indices, &index);
}

bool
ibo_upload(ibo_t* it)
{
	ALLEGRO_INDEX_BUFFER* buffer;
	uint16_t*             entries;

	iter_t iter;

	if (it->buffer != NULL) {
		al_destroy_index_buffer(it->buffer);
		it->buffer = NULL;
	}

	// create the index buffer object
	if (!(buffer = al_create_index_buffer(2, NULL, vector_len(it->indices), ALLEGRO_PRIM_BUFFER_STATIC)))
		return false;

	// upload indices to the GPU
	if (!(entries = al_lock_index_buffer(buffer, 0, vector_len(it->indices), ALLEGRO_LOCK_WRITEONLY))) {
		al_destroy_index_buffer(buffer);
		return false;
	}
	iter = vector_enum(it->indices);
	while (iter_next(&iter))
		entries[iter.index] = *(uint16_t*)iter.ptr;
	al_unlock_index_buffer(buffer);

	it->buffer = buffer;
	return true;
}

model_t*
model_new(shader_t* shader)
{
	model_t* model;

	console_log(4, "creating new model #%u", s_next_model_id);

	if (!(model = calloc(1, sizeof(model_t))))
		return NULL;
	model->shapes = vector_new(sizeof(shape_t*));
	model->transform = transform_new();
	model->shader = shader_ref(shader);

	model->id = s_next_model_id++;
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
model_unref(model_t* it)
{
	shape_t** shape_ptr;

	iter_t iter;

	if (it == NULL || --it->refcount > 0)
		return;

	console_log(4, "disposing model #%u no longer in use", it->id);

	iter = vector_enum(it->shapes);
	while ((shape_ptr = iter_next(&iter)))
		shape_unref(*shape_ptr);
	vector_free(it->shapes);
	shader_unref(it->shader);
	transform_unref(it->transform);
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
	shader_unref(old_shader);
}

void
model_set_transform(model_t* it, transform_t* transform)
{
	transform_t* old_transform;

	old_transform = it->transform;
	it->transform = transform_ref(transform);
	transform_unref(old_transform);
}

bool
model_add_shape(model_t* it, shape_t* shape)
{
	console_log(4, "adding shape #%u to model #%u", shape->id, it->id);

	shape = shape_ref(shape);
	vector_push(it->shapes, &shape);
	return true;
}

void
model_draw(const model_t* it, image_t* surface)
{
	iter_t iter;

	image_render_to(surface, it->transform);
	shader_use(it->shader != NULL ? it->shader : galileo_shader(), false);

	iter = vector_enum(it->shapes);
	while (iter_next(&iter))
		render_shape(*(shape_t**)iter.ptr);
}

shader_t*
shader_new(const char* vert_filename, const char* frag_filename)
{
	char*      frag_source = NULL;
	char*      vert_source = NULL;
	shader_t*  shader;

	if (!(shader = calloc(1, sizeof(shader_t))))
		goto on_error;

	console_log(2, "compiling new shader program #%u", s_next_shader_id);

	if (!(vert_source = game_read_file(g_game, vert_filename, NULL)))
		goto on_error;
	if (!(frag_source = game_read_file(g_game, frag_filename, NULL)))
		goto on_error;
	if (!(shader->program = al_create_shader(ALLEGRO_SHADER_GLSL)))
		goto on_error;
	if (!al_attach_shader_source(shader->program, ALLEGRO_VERTEX_SHADER, vert_source)) {
		fprintf(stderr, "\nvertex shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_attach_shader_source(shader->program, ALLEGRO_PIXEL_SHADER, frag_source)) {
		fprintf(stderr, "\nfragment shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_build_shader(shader->program)) {
		fprintf(stderr, "\nerror building shader program:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	free(vert_source);
	free(frag_source);

	shader->id = s_next_shader_id++;
	shader->fragment_path = strdup(frag_filename);
	shader->vertex_path = strdup(vert_filename);
	shader->uniforms = vector_new(sizeof(struct uniform));
	return shader_ref(shader);

on_error:
	free(vert_source);
	free(frag_source);
	if (shader != NULL) {
		if (shader->program != NULL)
			al_destroy_shader(shader->program);
		free(shader);
	}
	return NULL;
}

shader_t*
shader_dup(const shader_t* it)
{
	shader_t* dolly;

	dolly = shader_new(it->vertex_path, it->fragment_path);
	return dolly;
}

shader_t*
shader_ref(shader_t* it)
{
	if (it == NULL)
		return it;
	++it->refcount;
	return it;
}

void
shader_unref(shader_t* it)
{
	struct uniform* uniform;

	iter_t          iter;

	if (it == NULL || --it->refcount > 0)
		return;

	console_log(3, "disposing shader program #%u no longer in use", it->id);

	iter = vector_enum(it->uniforms);
	while ((uniform = iter_next(&iter))) {
		switch (uniform->type) {
		case UNIFORM_SAMPLER:
			image_unref(uniform->sampler.texture);
			break;
		case UNIFORM_FLOAT_ARR:
			free(uniform->float_list);
			break;
		case UNIFORM_INT_ARR:
			free(uniform->int_list);
			break;
		}
	}

	al_destroy_shader(it->program);
	vector_free(it->uniforms);
	free(it);
}

ALLEGRO_SHADER*
shader_program(const shader_t* it)
{
	return it->program;
}

void
shader_put_bool(shader_t* it, const char* name, bool value)
{
	struct uniform unif;

	if (s_last_shader == it) {
		al_set_shader_bool(name, value);
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_BOOL;
		unif.bool_value = value;
		vector_push(it->uniforms, &unif);
	}
}

void
shader_put_float(shader_t* it, const char* name, float value)
{
	struct uniform unif;

	if (s_last_shader == it) {
		al_set_shader_float(name, value);
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_FLOAT;
		unif.float_value = value;
		vector_push(it->uniforms, &unif);
	}
}

void
shader_put_float_array(shader_t* it, const char* name, float values[], int size)
{
	struct uniform unif;

	if (s_last_shader == it) {
		al_set_shader_float_vector(name, 1, values, size);
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_FLOAT_ARR;
		unif.float_list = malloc(size * sizeof(float));
		unif.num_values = size;
		memcpy(unif.float_list, values, size * sizeof(float));
		vector_push(it->uniforms, &unif);
	}
}

void
shader_put_float_vector(shader_t* it, const char* name, float values[], int size)
{
	struct uniform unif;

	if (s_last_shader == it) {
		al_set_shader_float_vector(name, size, values, 1);
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_FLOAT_VEC;
		unif.num_values = size;
		memcpy(unif.float_vec, values, sizeof(float) * size);
		vector_push(it->uniforms, &unif);
	}
}

void
shader_put_int(shader_t* it, const char* name, int value)
{
	struct uniform unif;

	if (s_last_shader == it) {
		al_set_shader_int(name, value);
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_INT;
		unif.int_value = value;
		vector_push(it->uniforms, &unif);
	}
}

void
shader_put_int_array(shader_t* it, const char* name, int values[], int size)
{
	struct uniform unif;

	if (s_last_shader == it) {
		al_set_shader_int_vector(name, 1, values, size);
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_INT_ARR;
		unif.int_list = malloc(size * sizeof(int));
		unif.num_values = size;
		memcpy(unif.int_list, values, size * sizeof(int));
		vector_push(it->uniforms, &unif);
	}
}

void
shader_put_int_vector(shader_t* it, const char* name, int values[], int size)
{
	struct uniform unif;

	if (s_last_shader == it) {
		al_set_shader_int_vector(name, size, values, 1);
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_INT_VEC;
		unif.num_values = size;
		memcpy(unif.int_vec, values, sizeof(int) * size);
		vector_push(it->uniforms, &unif);
	}
}

void
shader_put_matrix(shader_t* it, const char* name, const transform_t* matrix)
{
	struct uniform unif;

	if (s_last_shader == it) {
		al_set_shader_matrix(name, transform_matrix(matrix));
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_MATRIX;
		al_copy_transform(&unif.mat_value, transform_matrix(matrix));
		vector_push(it->uniforms, &unif);
	}
}

void
shader_put_sampler(shader_t* it, const char* name, image_t* texture, int texture_unit)
{
	struct uniform unif;
	ALLEGRO_BITMAP* bitmap;

	bitmap = image_bitmap(texture);

	if (s_last_shader == it) {
		al_set_shader_sampler(name, bitmap, texture_unit);
	}
	else {
		free_cached_uniform(it, name);
		strncpy(unif.name, name, 255);
		unif.name[255] = '\0';
		unif.type = UNIFORM_SAMPLER;

		unif.sampler.texture = image_ref(texture);
		unif.sampler.texture_unit = texture_unit;
		vector_push(it->uniforms, &unif);
	}
}

bool
shader_use(shader_t* it, bool force_set)
{
	ALLEGRO_SHADER* al_shader;
	ALLEGRO_BITMAP* bitmap;

	struct uniform* uniform;

	iter_t iter;

	if (it == s_last_shader && !force_set)
		return true;

	if (it != NULL)
		console_log(4, "activating shader program #%u", it->id);
	else
		console_log(4, "activating legacy shaders");

	al_shader = it != NULL ? it->program : NULL;
	if (!al_use_shader(al_shader))
		return false;

	// set any uniforms defined while we were inactive
	if (it != NULL) {
		iter = vector_enum(it->uniforms);
		while ((uniform = iter_next(&iter))) {
			switch (uniform->type) {
			case UNIFORM_BOOL:
				al_set_shader_bool(uniform->name, uniform->bool_value);
				break;
			case UNIFORM_FLOAT:
				al_set_shader_float(uniform->name, uniform->float_value);
				break;
			case UNIFORM_FLOAT_ARR:
				al_set_shader_float_vector(uniform->name, 1, uniform->float_list, uniform->num_values);
				free(uniform->float_list);
				break;
			case UNIFORM_FLOAT_VEC:
				al_set_shader_float_vector(uniform->name, uniform->num_values, uniform->float_vec, 1);
				break;
			case UNIFORM_INT:
				al_set_shader_int(uniform->name, uniform->int_value);
				break;
			case UNIFORM_INT_ARR:
				al_set_shader_int_vector(uniform->name, 1, uniform->int_list, uniform->num_values);
				free(uniform->int_list);
				break;
			case UNIFORM_INT_VEC:
				al_set_shader_int_vector(uniform->name, uniform->num_values, uniform->int_vec, 1);
				break;
			case UNIFORM_MATRIX:
				al_set_shader_matrix(uniform->name, &uniform->mat_value);
				break;
			case UNIFORM_SAMPLER:
				bitmap = image_bitmap(uniform->sampler.texture);

				al_set_shader_sampler(uniform->name, bitmap, uniform->sampler.texture_unit);
				image_unref(uniform->sampler.texture);
				break;
			}
		}
		vector_clear(it->uniforms);
	}

	s_last_shader = it;
	return true;
}

shape_t*
shape_new(vbo_t* vertices, ibo_t* indices, shape_type_t type, image_t* texture)
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

	if (!(shape = calloc(1, sizeof(shape_t))))
		return NULL;
	shape->texture = image_ref(texture);
	shape->type = type;
	shape->vbo = vbo_ref(vertices);
	shape->ibo = ibo_ref(indices);

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
shape_unref(shape_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	console_log(4, "disposing shape #%u no longer in use", it->id);
	image_unref(it->texture);
	vbo_unref(it->vbo);
	ibo_unref(it->ibo);
	free(it);
}

ibo_t*
shape_get_ibo(const shape_t* it)
{
	return it->ibo;
}

image_t*
shape_get_texture(const shape_t* it)
{
	return it->texture;
}

vbo_t*
shape_get_vbo(const shape_t* it)
{
	return it->vbo;
}

void
shape_set_ibo(shape_t* it, ibo_t* ibo)
{
	ibo_t* old_ibo;

	old_ibo = it->ibo;
	it->ibo = ibo_ref(ibo);
	ibo_unref(old_ibo);
}

void
shape_set_texture(shape_t* it, image_t* texture)
{
	image_t* old_texture;

	old_texture = it->texture;
	it->texture = image_ref(texture);
	image_unref(old_texture);
}

void
shape_set_vbo(shape_t* it, vbo_t* vbo)
{
	vbo_t* old_vbo;

	old_vbo = it->vbo;
	it->vbo = vbo_ref(vbo);
	vbo_unref(old_vbo);
}

void
shape_draw(shape_t* it, image_t* surface, transform_t* transform)
{
	image_render_to(surface, transform);
	shader_use(galileo_shader(), false);
	render_shape(it);
}

vbo_t*
vbo_new(void)
{
	vbo_t* vbo;

	if (!(vbo = calloc(1, sizeof(vbo_t))))
		return NULL;
	vbo->vertices = vector_new(sizeof(vertex_t));
	return vbo_ref(vbo);
}

vbo_t*
vbo_ref(vbo_t* it)
{
	++it->refcount;
	return it;
}

void
vbo_unref(vbo_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	if (it->buffer != NULL)
		al_destroy_vertex_buffer(it->buffer);
	vector_free(it->vertices);
	free(it);
}

ALLEGRO_VERTEX_BUFFER*
vbo_buffer(const vbo_t* it)
{
	return it->buffer;
}

int
vbo_len(const vbo_t* it)
{
	return vector_len(it->vertices);
}

void
vbo_add_vertex(vbo_t* it, vertex_t vertex)
{
	vector_push(it->vertices, &vertex);
}

bool
vbo_upload(vbo_t* it)
{
	ALLEGRO_VERTEX_BUFFER* buffer;
	ALLEGRO_VERTEX*        entries;
	vertex_t*              vertex;

	iter_t iter;

	if (it->buffer != NULL) {
		al_destroy_vertex_buffer(it->buffer);
		it->buffer = NULL;
	}

	// create the vertex buffer object
	if (!(buffer = al_create_vertex_buffer(NULL, NULL, vector_len(it->vertices), ALLEGRO_PRIM_BUFFER_STATIC)))
		return false;

	// upload indices to the GPU
	if (!(entries = al_lock_vertex_buffer(buffer, 0, vector_len(it->vertices), ALLEGRO_LOCK_WRITEONLY))) {
		al_destroy_vertex_buffer(buffer);
		return false;
	}
	iter = vector_enum(it->vertices);
	while (iter_next(&iter)) {
		vertex = iter.ptr;
		entries[iter.index].x = vertex->x;
		entries[iter.index].y = vertex->y;
		entries[iter.index].z = vertex->z;
		entries[iter.index].u = vertex->u;
		entries[iter.index].v = vertex->v;
		entries[iter.index].color = nativecolor(vertex->color);
	}
	al_unlock_vertex_buffer(buffer);

	it->buffer = buffer;
	return true;
}

static void
free_cached_uniform(shader_t* shader, const char* name)
{
	struct uniform* uniform;

	iter_t iter;

	iter = vector_enum(shader->uniforms);
	while ((uniform = iter_next(&iter))) {
		if (strcmp(uniform->name, name) == 0) {
			switch (uniform->type) {
			case UNIFORM_SAMPLER:
				image_unref(uniform->sampler.texture);
				break;
			case UNIFORM_FLOAT_ARR:
				free(uniform->float_list);
				break;
			case UNIFORM_INT_ARR:
				free(uniform->int_list);
				break;
			}

			iter_remove(&iter);
		}
	}
}

static void
render_shape(shape_t* shape)
{
	ALLEGRO_BITMAP* bitmap;
	int             draw_mode;
	int             num_indices;
	int             num_vertices;

	if (shape->vbo == NULL)
		return;

	num_vertices = vbo_len(shape->vbo);
	num_indices = ibo_len(shape->ibo);
	draw_mode = shape->type == SHAPE_LINES ? ALLEGRO_PRIM_LINE_LIST
		: shape->type == SHAPE_LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
		: shape->type == SHAPE_LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
		: shape->type == SHAPE_TRIANGLES ? ALLEGRO_PRIM_TRIANGLE_LIST
		: shape->type == SHAPE_TRI_STRIP ? ALLEGRO_PRIM_TRIANGLE_STRIP
		: shape->type == SHAPE_TRI_FAN ? ALLEGRO_PRIM_TRIANGLE_FAN
		: ALLEGRO_PRIM_POINT_LIST;

	bitmap = shape->texture != NULL ? image_bitmap(shape->texture) : NULL;
	if (shape->ibo != NULL)
		al_draw_indexed_buffer(vbo_buffer(shape->vbo), bitmap, ibo_buffer(shape->ibo), 0, num_indices, draw_mode);
	else
		al_draw_vertex_buffer(vbo_buffer(shape->vbo), bitmap, 0, num_vertices, draw_mode);
}
