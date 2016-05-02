#include "minisphere.h"
#include "shader.h"

#include "api.h"
#include "matrix.h"

static duk_ret_t js_new_ShaderProgram       (duk_context* ctx);
static duk_ret_t js_ShaderProgram_finalize  (duk_context* ctx);
static duk_ret_t js_ShaderProgram_setFloat  (duk_context* ctx);
static duk_ret_t js_ShaderProgram_setInt    (duk_context* ctx);
static duk_ret_t js_ShaderProgram_setMatrix (duk_context* ctx);

enum uniform_type
{
	UNIFORM_INT,
	UNIFORM_FLOAT,
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

struct shader
{
	unsigned int   refcount;
	vector_t*      uniforms;
#ifdef MINISPHERE_USE_SHADERS
	ALLEGRO_SHADER* program;
#endif
};

static void free_cached_uniform (shader_t* shader, const char* name);

static bool s_have_shaders = false;

void
initialize_shaders(bool enable_shading)
{
	console_log(1, "initializing shader support");
#ifdef MINISPHERE_USE_SHADERS
	s_have_shaders = enable_shading;
#endif
	reset_shader();
}

void
shutdown_shaders(void)
{
	console_log(1, "shutting down shader manager");
}

bool
are_shaders_active(void)
{
	return s_have_shaders;
}

shader_t*
shader_new(const char* vs_filename, const char* fs_filename)
{
	char*      fs_source = NULL;
	char*      vs_source = NULL;
	shader_t*  shader;

	shader = calloc(1, sizeof(shader_t));
	
	if (!(vs_source = sfs_fslurp(g_fs, vs_filename, NULL, NULL)))
		goto on_error;
	if (!(fs_source = sfs_fslurp(g_fs, fs_filename, NULL, NULL)))
		goto on_error;
#ifdef MINISPHERE_USE_SHADERS
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
#endif
	free(vs_source);
	free(fs_source);
	shader->uniforms = vector_new(sizeof(struct uniform));
	return shader_ref(shader);

on_error:
	free(vs_source);
	free(fs_source);
#ifdef MINISPHERE_USE_SHADERS
	if (shader->program != NULL)
		al_destroy_shader(shader->program);
#endif
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
#ifdef MINISPHERE_USE_SHADERS
	al_destroy_shader(shader->program);
#endif
	vector_free(shader->uniforms);
	free(shader);
}

void
shader_set_float(shader_t* shader, const char* name, float value)
{
	struct uniform unif;
	
	free_cached_uniform(shader, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_FLOAT;
	unif.float_value = value;
	vector_push(shader->uniforms, &unif);
}

void
shader_set_int(shader_t* shader, const char* name, int value)
{
	struct uniform unif;

	free_cached_uniform(shader, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_INT;
	unif.int_value = value;
	vector_push(shader->uniforms, &unif);
}

void
shader_set_matrix(shader_t* shader, const char* name, const matrix_t* matrix)
{
	struct uniform unif;

	free_cached_uniform(shader, name);
	strncpy(unif.name, name, 255);
	unif.name[255] = '\0';
	unif.type = UNIFORM_MATRIX;
	al_copy_transform(&unif.mat_value, matrix_transform(matrix));
	vector_push(shader->uniforms, &unif);
}

bool
apply_shader(shader_t* shader)
{
#ifdef MINISPHERE_USE_SHADERS
	iter_t iter;
	struct uniform* p;
	
	if (are_shaders_active()) {
		if (!al_use_shader(shader != NULL ? shader->program : NULL))
			return false;
		iter = vector_enum(shader->uniforms);
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
		return true;
	}
	else {
		// if shaders are not supported, degrade gracefully. this simplifies the rest
		// of the engine, which simply assumes shaders are always supported.
		return true;
	}
#else
	return true;
#endif
}

void
reset_shader(void)
{
#ifdef MINISPHERE_USE_SHADERS
	if (s_have_shaders) al_use_shader(NULL);
#endif
}

static void
free_cached_uniform(shader_t* shader, const char* name)
{
	iter_t iter;
	struct uniform* p;

	iter = vector_enum(shader->uniforms);
	while (p = vector_next(&iter)) {
		if (strcmp(p->name, name) == 0)
			iter_remove(&iter);
	}
}

void
init_shader_api(void)
{
	register_api_ctor(g_duk, "ShaderProgram", js_new_ShaderProgram, js_ShaderProgram_finalize);
	register_api_method(g_duk, "ShaderProgram", "setFloat", js_ShaderProgram_setFloat);
	register_api_method(g_duk, "ShaderProgram", "setInt", js_ShaderProgram_setInt);
	register_api_method(g_duk, "ShaderProgram", "setMatrix", js_ShaderProgram_setMatrix);
}

static duk_ret_t
js_new_ShaderProgram(duk_context* ctx)
{
	const char* fs_filename;
	const char* vs_filename;
	shader_t*   shader;

	if (!duk_is_object(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): JS object expected as argument");
	if (duk_get_prop_string(ctx, 0, "vertex"), !duk_is_string(ctx, -1))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): 'vertex' property, string required");
	if (duk_get_prop_string(ctx, 0, "fragment"), !duk_is_string(ctx, -1))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "ShaderProgram(): 'fragment' property, string required");
	duk_pop_2(ctx);

	if (!are_shaders_active())
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): shaders not supported on this system");
	
	duk_get_prop_string(ctx, 0, "vertex");
	duk_get_prop_string(ctx, 0, "fragment");
	vs_filename = duk_require_path(ctx, -2, NULL, false);
	fs_filename = duk_require_path(ctx, -1, NULL, false);
	duk_pop_2(ctx);
	if (!(shader = shader_new(vs_filename, fs_filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): failed to build shader from `%s`, `%s`", vs_filename, fs_filename);
	duk_push_sphere_obj(ctx, "ShaderProgram", shader);
	return 1;
}

static duk_ret_t
js_ShaderProgram_finalize(duk_context* ctx)
{
	shader_t* shader = duk_require_sphere_obj(ctx, 0, "ShaderProgram");

	shader_free(shader);
	return 0;
}

static duk_ret_t
js_ShaderProgram_setFloat(duk_context* ctx)
{
	float       value;
	const char* name;
	shader_t*   shader;

	duk_push_this(ctx);
	shader = duk_require_sphere_obj(ctx, -1, "ShaderProgram");
	name = duk_require_string(ctx, 0);
	value = duk_require_number(ctx, 1);

	shader_set_float(shader, name, value);
	return 0;
}

static duk_ret_t
js_ShaderProgram_setInt(duk_context* ctx)
{
	int         value;
	const char* name;
	shader_t*   shader;

	duk_push_this(ctx);
	shader = duk_require_sphere_obj(ctx, -1, "ShaderProgram");
	name = duk_require_string(ctx, 0);
	value = duk_require_int(ctx, 1);

	shader_set_int(shader, name, value);
	return 0;
}

static duk_ret_t
js_ShaderProgram_setMatrix(duk_context* ctx)
{
	matrix_t*   matrix;
	const char* name;
	shader_t*   shader;

	duk_push_this(ctx);
	shader = duk_require_sphere_obj(ctx, -1, "ShaderProgram");
	name = duk_require_string(ctx, 0);
	matrix = duk_require_sphere_obj(ctx, 1, "Transform");

	shader_set_matrix(shader, name, matrix);
	return 0;
}
