#include "minisphere.h"
#include "api.h"
#include "person.h"

#include "map_engine.h"

struct rmp_header
{
	char    signature[4];
	int16_t version;
	uint8_t type;
	int8_t  num_layers;
	uint8_t reserved_1;
	int8_t  num_entities;
	int16_t start_x;
	int16_t start_y;
	int8_t  start_layer;
	int8_t  start_direction;
	int16_t num_strings;
	int16_t num_zones;
	uint8_t toric_map;
	uint8_t reserved[234];
};

static void render_map_engine (void);
static void update_map_engine (void);

static duk_ret_t _js_MapEngine             (duk_context* ctx);
static duk_ret_t _js_GetMapEngineFrameRate (duk_context* ctx);
static duk_ret_t _js_SetMapEngineFrameRate (duk_context* ctx);
static duk_ret_t _js_SetDefaultMapScript   (duk_context* ctx);
static duk_ret_t _js_SetRenderScript       (duk_context* ctx);
static duk_ret_t _js_SetUpdateScript       (duk_context* ctx);
static duk_ret_t _js_IsMapEngineRunning    (duk_context* ctx);
static duk_ret_t _js_AttachCamera          (duk_context* ctx);
static duk_ret_t _js_AttachInput           (duk_context* ctx);

static person_t* s_camera_person = NULL;
static int       s_cam_x         = 0;
static int       s_cam_y         = 0;
static bool      s_exiting       = false;
static int       s_framerate     = 0;
static person_t* s_input_person  = NULL;
static bool      s_running       = false;

enum mapscript
{
	MAPSCRIPT_ON_ENTER,
	MAPSCRIPT_ON_LEAVE,
	MAPSCRIPT_ON_LEAVE_NORTH,
	MAPSCRIPT_ON_LEAVE_EAST,
	MAPSCRIPT_ON_LEAVE_SOUTH,
	MAPSCRIPT_ON_LEAVE_WEST
};

map_t*
load_map(const char* path)
{
	ALLEGRO_FILE*     file;
	map_t*            map;
	struct rmp_header rmp;
	
	if ((map = al_calloc(1, sizeof(map_t))) == NULL) goto on_error;
	if ((file = al_fopen(path, "rb")) == NULL) goto on_error;
	if (al_fread(file, &rmp, sizeof(struct rmp_header)) != sizeof(struct rmp_header))
		goto on_error;
	if (memcmp(rmp.signature, ".rmp", 4) != 0) goto on_error;
	switch (rmp.version) {
	case 1:
		goto on_error;
		break;
	default:
		goto on_error;
		break;
	}

on_error:
	if (file != NULL) al_fclose(file);
	return NULL;
}

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
	register_api_func(ctx, NULL, "AttachInput", &_js_AttachInput);

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

static void
render_map_engine(void)
{
	render_persons(0, 0);
	duk_push_global_stash(g_duktape);
	duk_get_prop_string(g_duktape, -1, "render_script");
	if (duk_is_callable(g_duktape, -1)) duk_call(g_duktape, 0);
	duk_pop_2(g_duktape);
}

static void
update_map_engine(void)
{
	ALLEGRO_KEYBOARD_STATE kb_state;
	
	// check for player input
	if (s_input_person != NULL) {
		al_get_keyboard_state(&kb_state);
		if (al_key_down(&kb_state, ALLEGRO_KEY_UP))
			s_input_person->y -= s_input_person->speed;
		else if (al_key_down(&kb_state, ALLEGRO_KEY_DOWN))
			s_input_person->y += s_input_person->speed;
		else if (al_key_down(&kb_state, ALLEGRO_KEY_LEFT))
			s_input_person->x -= s_input_person->speed;
		else if (al_key_down(&kb_state, ALLEGRO_KEY_RIGHT))
			s_input_person->x += s_input_person->speed;
	}
	
	// update camera
	if (s_camera_person != NULL) {
		s_cam_x = s_camera_person->x;
		s_cam_y = s_camera_person->y;
	}

	// run update script
	duk_push_global_stash(g_duktape);
	duk_get_prop_string(g_duktape, -1, "update_script");
	if (duk_is_callable(g_duktape, -1)) duk_call(g_duktape, 0);
	duk_pop_2(g_duktape);
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
		if (!begin_frame(s_framerate)) duk_error(ctx, DUK_ERR_ERROR, "!exit");
		update_map_engine();
		al_draw_text(g_sys_font, al_map_rgb(255, 255, 255), 160, 114, ALLEGRO_ALIGN_CENTER, filename);
		render_map_engine();
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
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	if ((person = find_person(name)) != NULL) {
		s_camera_person = person;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "AttachCamera(): Person '%s' doesn't exist", name);
	}
	return 0;
}

static duk_ret_t
_js_AttachInput(duk_context* ctx)
{
	const char* name;
	person_t*   person;

	name = duk_to_string(ctx, 0);
	if ((person = find_person(name)) != NULL) {
		s_input_person = person;
	}
	else {
		duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "AttachInput(): Person '%s' doesn't exist", name);
	}
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
	render_map_engine();
	return 0;
}

static duk_ret_t
_js_UpdateMapEngine(duk_context* ctx)
{
	update_map_engine();
	return 0;
}
