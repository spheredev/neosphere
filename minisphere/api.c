#include "minisphere.h"
#include "api.h"
#include "color.h"

static duk_ret_t js_GetVersion           (duk_context* ctx);
static duk_ret_t js_GetVersionString     (duk_context* ctx);
static duk_ret_t js_GetExtensions        (duk_context* ctx);
static duk_ret_t js_RequireSystemScript  (duk_context* ctx);
static duk_ret_t js_RequireScript        (duk_context* ctx);
static duk_ret_t js_EvaluateSystemScript (duk_context* ctx);
static duk_ret_t js_EvaluateScript       (duk_context* ctx);
static duk_ret_t js_IsSkippedFrame       (duk_context* ctx);
static duk_ret_t js_GetFileList          (duk_context* ctx);
static duk_ret_t js_GetFrameRate         (duk_context* ctx);
static duk_ret_t js_GetGameList          (duk_context* ctx);
static duk_ret_t js_GetScreenHeight      (duk_context* ctx);
static duk_ret_t js_GetScreenWidth       (duk_context* ctx);
static duk_ret_t js_GetTime              (duk_context* ctx);
static duk_ret_t js_SetFrameRate         (duk_context* ctx);
static duk_ret_t js_Abort                (duk_context* ctx);
static duk_ret_t js_Alert                (duk_context* ctx);
static duk_ret_t js_Delay                (duk_context* ctx);
static duk_ret_t js_ExecuteGame          (duk_context* ctx);
static duk_ret_t js_Exit                 (duk_context* ctx);
static duk_ret_t js_FlipScreen           (duk_context* ctx);
static duk_ret_t js_GarbageCollect       (duk_context* ctx);
static duk_ret_t js_UnskipFrame          (duk_context* ctx);

static int s_framerate = 0;

void
init_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "GetVersion", js_GetVersion);
	register_api_func(ctx, NULL, "GetVersionString", js_GetVersionString);
	register_api_func(ctx, NULL, "GetExtensions", js_GetExtensions);
	register_api_func(ctx, NULL, "EvaluateScript", js_EvaluateScript);
	register_api_func(ctx, NULL, "EvaluateSystemScript", js_EvaluateSystemScript);
	register_api_func(ctx, NULL, "RequireScript", js_RequireScript);
	register_api_func(ctx, NULL, "RequireSystemScript", js_RequireSystemScript);
	register_api_func(ctx, NULL, "IsSkippedFrame", js_IsSkippedFrame);
	register_api_func(ctx, NULL, "GetFileList", js_GetFileList);
	register_api_func(ctx, NULL, "GetFrameRate", js_GetFrameRate);
	register_api_func(ctx, NULL, "GetGameList", js_GetGameList);
	register_api_func(ctx, NULL, "GetScreenHeight", js_GetScreenHeight);
	register_api_func(ctx, NULL, "GetScreenWidth", js_GetScreenWidth);
	register_api_func(ctx, NULL, "GetTime", js_GetTime);
	register_api_func(ctx, NULL, "SetFrameRate", js_SetFrameRate);
	register_api_func(ctx, NULL, "Abort", js_Abort);
	register_api_func(ctx, NULL, "Alert", js_Alert);
	register_api_func(ctx, NULL, "Delay", js_Delay);
	register_api_func(ctx, NULL, "Exit", js_Exit);
	register_api_func(ctx, NULL, "ExecuteGame", js_ExecuteGame);
	register_api_func(ctx, NULL, "FlipScreen", js_FlipScreen);
	register_api_func(ctx, NULL, "GarbageCollect", js_GarbageCollect);
	register_api_func(ctx, NULL, "UnskipFrame", js_UnskipFrame);
	duk_push_global_stash(ctx);
	duk_push_object(ctx); duk_put_prop_string(ctx, -2, "RequireScript");
	duk_pop(ctx);
}

void
register_api_const(duk_context* ctx, const char* name, double value)
{
	duk_push_global_object(ctx);
	duk_push_string(ctx, name); duk_push_number(ctx, value);
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_ENUMERABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);
	duk_pop(ctx);
}

void
register_api_func(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn)
{
	duk_push_global_object(ctx);
	if (ctor_name != NULL) {
		duk_get_prop_string(ctx, -1, ctor_name);
		duk_get_prop_string(ctx, -1, "prototype");
	}
	duk_push_c_function(ctx, fn, DUK_VARARGS);
	duk_put_prop_string(ctx, -2, name);
	if (ctor_name != NULL) {
		duk_pop_2(ctx);
	}
	duk_pop(ctx);
}

void
bail_out_game(void)
{
	duk_error(g_duktape, DUK_ERR_ERROR, "@exit");
}

static duk_ret_t
js_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, SPHERE_API_VERSION);
	return 1;
}

static duk_ret_t
js_GetVersionString(duk_context* ctx)
{
	duk_push_string(ctx, SPHERE_API_VERSION_STRING);
	return 1;
}

static duk_ret_t
js_GetExtensions(duk_context* ctx)
{
	int i = 0;

	duk_push_array(ctx);
	duk_push_string(ctx, "sphere-legacy"); duk_put_prop_index(ctx, -2, i++);
	duk_push_string(ctx, "minisphere"); duk_put_prop_index(ctx, -2, i++);
	return 1;
}

static duk_ret_t
js_Alert(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* text = n_args >= 1 && !duk_is_null_or_undefined(ctx, 0)
		? duk_to_string(ctx, 0) : "It's 8:12... do you know where the pig is?\n\nIt's...\n\n\n\n\n\n\nBEHIND YOU! *MUNCH*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	const char* caller_info;
	const char* filename;
	int         line_number;
	const char* full_path;
	
	if (stack_offset > 0)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "Alert(): Stack offset cannot be positive");

	// get filename and line number of Alert() call
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3 + stack_offset); duk_call(ctx, 1);
	if (!duk_is_object(ctx, -1)) {
		duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3); duk_call(ctx, 1);
	}
	duk_remove(ctx, -2);
	duk_get_prop_string(ctx, -1, "lineNumber"); line_number = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "function");
	duk_get_prop_string(ctx, -1, "fileName"); full_path = duk_get_string(ctx, -1); duk_pop(ctx);
	duk_pop_2(ctx);

	// show the message
	filename = strrchr(full_path, ALLEGRO_NATIVE_PATH_SEP);
	filename = filename != NULL ? filename + 1 : full_path;
	caller_info =
		duk_push_sprintf(ctx, "%s (line %i)", filename, line_number),
		duk_get_string(ctx, -1);
	al_show_native_message_box(g_display, "Alert from Sphere game", caller_info, text, NULL, 0x0);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* error_text = n_args >= 1 ? duk_to_string(ctx, 0) : "Game terminated prematurely";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;
	
	const char* filename;
	int         line_number;
	const char* full_path;
	
	if (stack_offset > 0)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "Abort(): Stack offset cannot be positive");
	
	// get filename and line number of Abort() call
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Duktape");
	duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3 + stack_offset); duk_call(ctx, 1);
	if (!duk_is_object(ctx, -1)) {
		duk_pop(ctx);
		duk_get_prop_string(ctx, -1, "act"); duk_push_int(ctx, -3); duk_call(ctx, 1);
	}
	duk_remove(ctx, -2);
	duk_get_prop_string(ctx, -1, "lineNumber"); line_number = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "function");
	duk_get_prop_string(ctx, -1, "fileName"); full_path = duk_get_string(ctx, -1); duk_pop(ctx);
	duk_pop_2(ctx);
	
	// throw the exception
	filename = strrchr(full_path, ALLEGRO_NATIVE_PATH_SEP);
	filename = filename != NULL ? filename + 1 : full_path;
	duk_error_raw(ctx, DUK_ERR_ERROR, filename, line_number, "%s", error_text);
}

static duk_ret_t
js_EvaluateScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "scripts", false);
	duk_eval_file_noresult(ctx, script_path);
	free(script_path);
	return 0;
}

static duk_ret_t
js_EvaluateSystemScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_sys_asset_path(script_file, "system/scripts");
	duk_eval_file_noresult(ctx, script_path);
	free(script_path);
	return 0;
}

static duk_ret_t
js_RequireScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "scripts", false);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, script_path);
	bool is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_eval_file_noresult(ctx, script_path);
		duk_push_true(ctx); duk_put_prop_string(ctx, -2, script_path);
	}
	duk_pop_2(ctx);
	free(script_path);
	return 0;
}

static duk_ret_t
js_RequireSystemScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_sys_asset_path(script_file, "system/scripts");
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, script_path);
	bool is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_eval_file_noresult(ctx, script_path);
		duk_push_true(ctx); duk_put_prop_string(ctx, -2, script_path);
	}
	duk_pop_2(ctx);
	free(script_path);
	return 0;
}

static duk_ret_t
js_IsSkippedFrame(duk_context* ctx)
{
	duk_push_boolean(ctx, is_skipped_frame());
	return 1;
}

static duk_ret_t
js_GetFileList(duk_context* ctx)
{
	const char*       directory_name;
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_PATH*     file_path;
	ALLEGRO_FS_ENTRY* fs;
	char*             path;

	int i;

	directory_name = duk_require_string(ctx, 0);
	path = get_asset_path(directory_name, NULL, false);
	fs = al_create_fs_entry(path);
	free(path);
	duk_push_array(ctx);
	i = 0;
	if (al_get_fs_entry_mode(fs) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fs)) {
		while (file_info = al_read_directory(fs)) {
			if (al_get_fs_entry_mode(file_info) & ALLEGRO_FILEMODE_ISFILE) {
				file_path = al_create_path(al_get_fs_entry_name(file_info));
				duk_push_string(ctx, al_get_path_filename(file_path)); duk_put_prop_index(ctx, -2, i);
				al_destroy_path(file_path);
				++i;
			}
		}
	}
	al_destroy_fs_entry(fs);
	return 1;
}

static duk_ret_t
js_GetFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, s_framerate);
	return 1;
}

static duk_ret_t
js_GetGameList(duk_context* ctx)
{
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_PATH*     file_path;
	ALLEGRO_FS_ENTRY* fs;
	char*             path;
	ALLEGRO_CONFIG*   sgm;

	int i;

	path = get_sys_asset_path("games", NULL);
	fs = al_create_fs_entry(path);
	free(path);
	duk_push_array(ctx);
	i = 0;
	if (al_get_fs_entry_mode(fs) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fs)) {
		while (file_info = al_read_directory(fs)) {
			if (al_get_fs_entry_mode(file_info) & ALLEGRO_FILEMODE_ISDIR) {
				file_path = al_create_path_for_directory(al_get_fs_entry_name(file_info));
				al_set_path_filename(file_path, "game.sgm");
				if ((sgm = al_load_config_file(al_path_cstr(file_path, ALLEGRO_NATIVE_PATH_SEP))) != NULL) {
					duk_push_object(ctx);
					duk_push_string(ctx, al_get_path_component(file_path, -1)); duk_put_prop_string(ctx, -2, "directory");
					duk_push_string(ctx, al_get_config_value(sgm, NULL, "name")); duk_put_prop_string(ctx, -2, "name");
					duk_push_string(ctx, al_get_config_value(sgm, NULL, "author")); duk_put_prop_string(ctx, -2, "author");
					duk_push_string(ctx, al_get_config_value(sgm, NULL, "description")); duk_put_prop_string(ctx, -2, "description");
					duk_put_prop_index(ctx, -2, i);
					al_destroy_config(sgm);
				}
				al_destroy_path(file_path);
				++i;
			}
		}
	}
	al_destroy_fs_entry(fs);
	return 1;
}

static duk_ret_t
js_GetScreenHeight(duk_context* ctx)
{
	duk_push_int(ctx, g_res_y);
	return 1;
}

static duk_ret_t
js_GetScreenWidth(duk_context* ctx)
{
	duk_push_int(ctx, g_res_x);
	return 1;
}

static duk_ret_t
js_GetTime(duk_context* ctx)
{
	int ms = floor(al_get_time() * 1000);
	duk_push_int(ctx, ms);
	return 1;
}

static duk_ret_t
js_SetFrameRate(duk_context* ctx)
{
	s_framerate = duk_to_int(ctx, 0);
	return 0;
}

static duk_ret_t
js_Delay(duk_context* ctx)
{
	double millisecs = floor(duk_require_number(ctx, 0));
	
	double start_time;

	if (millisecs < 0)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "Delay(): Negative delay not allowed (%i)", millisecs);
	start_time = al_get_time();
	while (al_get_time() < start_time + millisecs / 1000) {
		if (!do_events()) bail_out_game();
	}
	return 0;
}

static duk_ret_t
js_ExecuteGame(duk_context* ctx)
{
	const char* path = duk_require_string(ctx, 0);
	
	duk_error(ctx, DUK_ERR_ERROR, "@exec %s", path);
	return 0;
}

static duk_ret_t
js_Exit(duk_context* ctx)
{
	bail_out_game();
	return 0;
}

static duk_ret_t
js_FlipScreen(duk_context* ctx)
{
	if (!flip_screen(s_framerate)) bail_out_game();
	return 0;
}

static duk_ret_t
js_UnskipFrame(duk_context* ctx)
{
	unskip_frame();
	return 0;
}

static duk_ret_t
js_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}
