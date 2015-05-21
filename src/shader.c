#include "minisphere.h"
#include "api.h"

#include "shader.h"

static duk_ret_t js_new_PixelShader(duk_context* ctx);
static duk_ret_t js_PixelShader_finalize  (duk_context* ctx);

static duk_ret_t js_new_VertexShader      (duk_context* ctx);
static duk_ret_t js_VertexShader_finalize (duk_context* ctx);

struct shader
{
	unsigned int    refcount;
	ALLEGRO_SHADER* ptr;
};

shader_t*
create_shader(const char* path, shader_type_t type)
{
	int       attach_type;
	shader_t* shader;

	if (!(shader = calloc(1, sizeof(shader_t)))) goto on_error;
	if (!(shader->ptr = al_create_shader(ALLEGRO_SHADER_GLSL)))
		goto on_error;
	attach_type = type == SHADER_TYPE_PIXEL ? ALLEGRO_PIXEL_SHADER
		: type == SHADER_TYPE_VERTEX ? ALLEGRO_VERTEX_SHADER
		: 0x0;
	if (!al_attach_shader_source_file(shader->ptr, attach_type, path)) goto on_error;
	if (!al_build_shader(shader->ptr)) goto on_error;
	return ref_shader(shader);

on_error:
	if (shader != NULL) {
		if (shader->ptr != NULL)
			al_destroy_shader(shader->ptr);
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
	al_destroy_shader(shader->ptr);
	free(shader);
}

bool
apply_shader(shader_t* shader)
{
	return al_use_shader(shader->ptr);
}

void
reset_shaders(void)
{
	al_use_shader(NULL);
}

void
init_shader_api(void)
{
	register_api_ctor(g_duk, "PixelShader", js_new_PixelShader, js_PixelShader_finalize);
	register_api_ctor(g_duk, "VertexShader", js_new_VertexShader, js_VertexShader_finalize);
}

static duk_ret_t
js_new_PixelShader(duk_context* ctx)
{
	shader_t* shader;
	const char* filename = duk_require_string(ctx, 0);

	char* path;
	
	path = get_asset_path(filename, "shaders", false);
	if (!(shader = create_shader(path, SHADER_TYPE_PIXEL)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "PixelShader(): Failed to compile pixel shader '%s'", filename);
	free(path);
	duk_push_sphere_obj(ctx, "PixelShader", shader);
	return 1;
}

static duk_ret_t
js_PixelShader_finalize(duk_context* ctx)
{
	shader_t* shader;

	shader = duk_require_sphere_obj(ctx, 0, "PixelShader");
	free_shader(shader);
	return 0;
}

static duk_ret_t
js_new_VertexShader(duk_context* ctx)
{
	shader_t* shader;
	const char* filename = duk_require_string(ctx, 0);

	char* path;

	path = get_asset_path(filename, "shaders", false);
	if (!(shader = create_shader(path, SHADER_TYPE_VERTEX)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "VertexShader(): Failed to compile vertex shader '%s'", filename);
	free(path);
	duk_push_sphere_obj(ctx, "VertexShader", shader);
	return 1;
}

static duk_ret_t
js_VertexShader_finalize(duk_context* ctx)
{
	shader_t* shader;

	shader = duk_require_sphere_obj(ctx, 0, "VertexShader");
	free_shader(shader);
	return 0;
}
