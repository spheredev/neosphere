#include "minisphere.h"
#include "api.h"
#include "person.h"

#include "map_engine.h"

static duk_ret_t _js_MapEngine             (duk_context* ctx);
static duk_ret_t _js_GetMapEngineFrameRate (duk_context* ctx);
static duk_ret_t _js_SetMapEngineFrameRate (duk_context* ctx);
static duk_ret_t _js_SetDefaultMapScript   (duk_context* ctx);
static duk_ret_t _js_SetRenderScript       (duk_context* ctx);
static duk_ret_t _js_SetUpdateScript       (duk_context* ctx);
static duk_ret_t _js_IsMapEngineRunning    (duk_context* ctx);
static duk_ret_t _js_AttachCamera          (duk_context* ctx);

static char* s_camera_person = NULL;
static bool  s_exiting       = false;
static int   s_framerate     = 0;
static bool  s_running       = false;

enum mapscript
{
	MAPSCRIPT_ON_ENTER,
	MAPSCRIPT_ON_LEAVE,
	MAPSCRIPT_ON_LEAVE_NORTH,
	MAPSCRIPT_ON_LEAVE_EAST,
	MAPSCRIPT_ON_LEAVE_SOUTH,
	MAPSCRIPT_ON_LEAVE_WEST
};

void
init_map_engine_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "MapEngine", &_js_MapEngine);
	register_api_func(ctx, NULL, "GetMapEngineFrameRate", &_js_GetMapEngineFrameRate);
	register_api_func(ctx, NULL, "SetMapEngineFrameRate", &_js_SetMapEngineFrameRate);
	register_api_func(ctx, NULL, "SetDefaultMapScript", &_js_SetDefaultMapScript);
	register_api_func(ctx, NULL, "SetRenderScript", &_js_SetRenderScript);
	register_api_func(ctx, NULL, "SetUpdateScript", &_js_SetUpdateScript);
	register_api_func(ctx, NULL, "IsMapEngineRunning", &_js_IsMapEngineRunning);
	register_api_func(ctx, NULL, "AttachCamera", &_js_AttachCamera);

	// Map script types
	register_api_const(ctx, "SCRIPT_ON_ENTER_MAP", 0);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP", 1);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP_NORTH", 2);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP_EAST", 3);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP_SOUTH", 4);
	register_api_const(ctx, "SCRIPT_ON_LEAVE_MAP_WEST", 5);

	// initialize subcomponent APIs (persons, etc.)
	init_person_api();
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
_js_SetDefaultMapScript(duk_context* ctx)
{
	const char* script;
	size_t      script_size;
	const char* script_name;
	int         script_type;

	script_type = duk_require_int(ctx, 0);
	script = duk_require_lstring(ctx, 1, &script_size);
	script_name = (script_type == MAPSCRIPT_ON_ENTER) ? "map_def_enter_script"
		: (script_type == MAPSCRIPT_ON_LEAVE) ? "map_def_leave_script"
		: (script_type == MAPSCRIPT_ON_LEAVE_NORTH) ? "map_def_leave_north_script"
		: (script_type == MAPSCRIPT_ON_LEAVE_EAST) ? "map_def_leave_east_script"
		: (script_type == MAPSCRIPT_ON_LEAVE_SOUTH) ? "map_def_leave_south_script"
		: (script_type == MAPSCRIPT_ON_LEAVE_WEST) ? "map_def_leave_west_script"
		: NULL;
	if (script_name != NULL) {
		duk_push_global_stash(ctx);
		duk_push_string(ctx, "[def-mapscript]");
		duk_compile_lstring_filename(ctx, DUK_COMPILE_EVAL, script, script_size);
		duk_put_prop_string(ctx, -2, script_name);
		duk_pop(ctx);
		return 0;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "SetDefaultMapScript(): Invalid map script constant");
	}
}

static duk_ret_t
_js_SetRenderScript(duk_context* ctx)
{
	const char* script;
	size_t      script_size;

	script = duk_require_lstring(ctx, 0, &script_size);
	duk_push_global_stash(ctx);
	duk_push_string(ctx, "[renderscript]");
	duk_compile_lstring_filename(ctx, DUK_COMPILE_EVAL, script, script_size);
	duk_put_prop_string(ctx, -2, "render_script");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
_js_SetUpdateScript(duk_context* ctx)
{
	const char* script;
	size_t      script_size;

	script = duk_require_lstring(ctx, 0, &script_size);
	duk_push_global_stash(ctx);
	duk_push_string(ctx, "[updatescript]");
	duk_compile_lstring_filename(ctx, DUK_COMPILE_EVAL, script, script_size);
	duk_put_prop_string(ctx, -2, "update_script");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
_js_IsMapEngineRunning(duk_context* ctx)
{
	duk_push_boolean(ctx, s_running);
	return 1;
}

static duk_ret_t
_js_AttachCamera(duk_context* ctx)
{
	const char* person_name;
	
	person_name = duk_to_string(ctx, 0);
	return 0;
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
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "render_script");
	duk_call(ctx, 0);
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
_js_UpdateMapEngine(duk_context* ctx)
{
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "update_script");
	duk_call(ctx, 0);
	duk_pop_2(ctx);
	return 0;
}
