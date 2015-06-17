#include "minisphere.h"
#include "api.h"

#include "shader.h"

static duk_ret_t js_new_ShaderProgram      (duk_context* ctx);
static duk_ret_t js_ShaderProgram_finalize (duk_context* ctx);

struct shader
{
	unsigned int    refcount;
	ALLEGRO_SHADER* program;
};

shader_t*
create_shader(const char* vs_filename, const char* fs_filename)
{
	lstring_t* fs_source;
	lstring_t* vs_source;
	shader_t*  shader;
	void*      slurp;
	size_t     slurp_size;

	if (!(shader = calloc(1, sizeof(shader_t)))) goto on_error;
	if (!(shader->program = al_create_shader(ALLEGRO_SHADER_GLSL)))
		goto on_error;
	if (!(slurp = sfs_fslurp(g_fs, vs_filename, "shaders", &slurp_size)))
		goto on_error;
	vs_source = lstring_from_buf(slurp_size, slurp);
	free(slurp);
	if (!(slurp = sfs_fslurp(g_fs, fs_filename, "shaders", &slurp_size)))
		goto on_error;
	fs_source = lstring_from_buf(slurp_size, slurp);
	free(slurp);
	if (!al_attach_shader_source(shader->program, ALLEGRO_VERTEX_SHADER, lstr_cstr(vs_source))) {
		fprintf(stderr, "\nVertex shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_attach_shader_source(shader->program, ALLEGRO_PIXEL_SHADER, lstr_cstr(fs_source))) {
		fprintf(stderr, "\nFragment shader compile log:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	if (!al_build_shader(shader->program)) {
		fprintf(stderr, "\nError building shader program:\n%s\n", al_get_shader_log(shader->program));
		goto on_error;
	}
	return ref_shader(shader);

on_error:
	if (shader != NULL) {
		if (shader->program != NULL)
			al_destroy_shader(shader->program);
		free(shader);
	}
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
	al_destroy_shader(shader->program);
	free(shader);
}

bool
apply_shader(shader_t* shader)
{
	return al_use_shader(shader->program);
}

void
reset_shader(void)
{
	al_use_shader(NULL);
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
	
	duk_get_prop_string(ctx, 0, "vertex");
	duk_get_prop_string(ctx, 0, "fragment");
	vs_filename = duk_require_string(ctx, -2);
	fs_filename = duk_require_string(ctx, -1);
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
