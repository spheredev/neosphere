#include "minisphere.h"
#include "api.h"

#include "shader.h"

static duk_ret_t js_new_ShaderProgram      (duk_context* ctx);
static duk_ret_t js_ShaderProgram_finalize (duk_context* ctx);

struct shader
{
	unsigned int    refcount;
#ifdef MINISPHERE_USE_SHADERS
	ALLEGRO_SHADER* program;
#endif
};

static bool s_have_shaders = false;

void
initialize_shaders(bool enable_shading)
{
	console_log(1, "Initializing shader support");
#ifdef MINISPHERE_USE_SHADERS
	s_have_shaders = enable_shading;
#endif
	reset_shader();
}

void
shutdown_shaders(void)
{
	console_log(1, "Shutting down shader manager");
}

bool
are_shaders_active(void)
{
	return s_have_shaders;
}

shader_t*
create_shader(const char* vs_filename, const char* fs_filename)
{
	char*      fs_source = NULL;
	char*      vs_source = NULL;
	shader_t*  shader;

	shader = calloc(1, sizeof(shader_t));
	
	if (!(vs_source = sfs_fslurp(g_fs, vs_filename, NULL, NULL))) goto on_error;
	if (!(fs_source = sfs_fslurp(g_fs, fs_filename, NULL, NULL))) goto on_error;
#ifdef MINISPHERE_USE_SHADERS
	if (!(shader->program = al_create_shader(ALLEGRO_SHADER_GLSL))) goto on_error;
	if (!al_attach_shader_source(shader->program, ALLEGRO_VERTEX_SHADER, vs_source)) {
		fprintf(stderr, "\nVertex shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_attach_shader_source(shader->program, ALLEGRO_PIXEL_SHADER, fs_source)) {
		fprintf(stderr, "\nFragment shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_build_shader(shader->program)) {
		fprintf(stderr, "\nError building shader program:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
#endif
	free(vs_source);
	free(fs_source);
	return ref_shader(shader);

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
ref_shader(shader_t* shader)
{
	if (shader == NULL)
		return shader;
	++shader->refcount;
	return shader;
}

void
free_shader(shader_t* shader)
{
	if (shader == NULL || --shader->refcount > 0)
		return;
#ifdef MINISPHERE_USE_SHADERS
	al_destroy_shader(shader->program);
#endif
	free(shader);
}

bool
apply_shader(shader_t* shader)
{
#ifdef MINISPHERE_USE_SHADERS
	if (are_shaders_active())
		return al_use_shader(shader != NULL ? shader->program : NULL);
	else
		// if shaders are not supported, degrade gracefully. this simplifies the rest
		// of the engine, which simply assumes shaders are always supported.
		return true;
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

void
init_shader_api(void)
{
	// ShaderProgram object
	register_api_ctor(g_duk, "ShaderProgram", js_new_ShaderProgram, js_ShaderProgram_finalize);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): Shaders not supported on this system");
	
	duk_get_prop_string(ctx, 0, "vertex");
	duk_get_prop_string(ctx, 0, "fragment");
	vs_filename = duk_require_path(ctx, -2, NULL);
	fs_filename = duk_require_path(ctx, -1, NULL);
	duk_pop_2(ctx);
	if (!(shader = create_shader(vs_filename, fs_filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): Failed to build shader from '%s', '%s'", vs_filename, fs_filename);
	duk_push_sphere_obj(ctx, "ShaderProgram", shader);
	return 1;
}

static duk_ret_t
js_ShaderProgram_finalize(duk_context* ctx)
{
	shader_t* shader = duk_require_sphere_obj(ctx, 0, "ShaderProgram");

	free_shader(shader);
	return 0;
}
