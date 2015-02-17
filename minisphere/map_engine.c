#include "minisphere.h"
#include "api.h"

#include "map_engine.h"


static duk_ret_t _js_MapEngine(duk_context* ctx);
static duk_ret_t _js_IsMapEngineRunning(duk_context* ctx);

static bool s_exiting   = false;
static int  s_framerate = 0;
static bool s_running   = false;

void
init_map_engine_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "IsMapEngineRunning", &_js_IsMapEngineRunning);
	register_api_func(ctx, NULL, "MapEngine", &_js_MapEngine);
}

static duk_ret_t
_js_MapEngine(duk_context* ctx)
{
	const char* filename;
	
	filename = duk_to_string(ctx, 0);
	s_running = true;
	s_exiting = false;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
	s_framerate = duk_to_int(ctx, 1);
	while (!s_exiting) {
		al_draw_text(g_sys_font, al_map_rgb(255, 255, 255), 160, 114, ALLEGRO_ALIGN_CENTER, filename);
		if (!end_frame(s_framerate)) duk_error(ctx, DUK_ERR_ERROR, "!exit");
	}
	s_running = false;
	return 0;
}

static duk_ret_t
_js_GetMapEngineFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, s_framerate);
	return 1;
}

static duk_ret_t
_js_SetMapEngineFrameRate(duk_context* ctx)
{
	s_framerate = duk_to_int(ctx, 0);
	return 0;
}

static duk_ret_t
_js_IsMapEngineRunning(duk_context* ctx)
{
	duk_push_boolean(ctx, s_running);
	return 1;
}

static duk_ret_t
_js_ExitMapEngine(duk_context* ctx)
{
	s_exiting = true;
	return 0;
}

static duk_ret_t
_js_RenderMap(duk_context* ctx)
{
	return 0;
}

static duk_ret_t
_js_UpdateMapEngine(duk_context* ctx)
{
	return 0;
}
