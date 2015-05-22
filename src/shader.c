#include "minisphere.h"
#include "api.h"

#include "shader.h"

static duk_ret_t js_new_ShaderProgram      (duk_context* ctx);
static duk_ret_t js_ShaderProgram_finalize (duk_context* ctx);
static duk_ret_t js_new_PixelShader        (duk_context* ctx);
static duk_ret_t js_PixelShader_finalize   (duk_context* ctx);
static duk_ret_t js_new_VertexShader       (duk_context* ctx);
static duk_ret_t js_VertexShader_finalize  (duk_context* ctx);

struct shader
{
	unsigned int    refcount;
	ALLEGRO_SHADER* program;
};

shader_t*
create_shader(const char* pixel_path, const char* vertex_path)
{
	shader_t* shader;

	if (!(shader = calloc(1, sizeof(shader_t)))) goto on_error;
	if (!(shader->program = al_create_shader(ALLEGRO_SHADER_GLSL)))
		goto on_error;
	if (!al_attach_shader_source_file(shader->program, ALLEGRO_PIXEL_SHADER, pixel_path))
		goto on_error;
	if (!al_attach_shader_source_file(shader->program, ALLEGRO_VERTEX_SHADER, vertex_path))
		goto on_error;
	if (!al_build_shader(shader->program)) goto on_error;
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
	const char* pixel_filename = duk_require_string(ctx, 0);
	const char* vertex_filename = duk_require_string(ctx, 1);

	char*     path_pixel;
	char*     path_vertex;
	shader_t* shader;

	path_pixel = get_asset_path(pixel_filename, "shaders", false);
	path_vertex = get_asset_path(vertex_filename, "shaders", false);
	if (!(shader = create_shader(path_pixel, path_vertex)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ShaderProgram(): Failed to build shader from '%s', '%s'", pixel_filename, vertex_filename);
	free(path_pixel);
	free(path_vertex);
	duk_push_sphere_obj(ctx, "ShaderProgram", shader);
	return 1;
}

static duk_ret_t
js_ShaderProgram_finalize(duk_context* ctx)
{
	return 0;
}
