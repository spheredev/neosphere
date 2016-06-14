#include "minisphere.h"
#include "vanilla.h"

#include "animation.h"
#include "api.h"
#include "async.h"
#include "debugger.h"
#include "font.h"
#include "logger.h"

static duk_ret_t js_IsSkippedFrame            (duk_context* ctx);
static duk_ret_t js_GetClippingRectangle      (duk_context* ctx);
static duk_ret_t js_GetFrameRate              (duk_context* ctx);
static duk_ret_t js_GetGameList               (duk_context* ctx);
static duk_ret_t js_GetGameManifest           (duk_context* ctx);
static duk_ret_t js_GetMaxFrameSkips          (duk_context* ctx);
static duk_ret_t js_GetScreenHeight           (duk_context* ctx);
static duk_ret_t js_GetScreenWidth            (duk_context* ctx);
static duk_ret_t js_GetSystemFont             (duk_context* ctx);
static duk_ret_t js_GetTime                   (duk_context* ctx);
static duk_ret_t js_GetVersion                (duk_context* ctx);
static duk_ret_t js_GetVersionString          (duk_context* ctx);
static duk_ret_t js_SetClippingRectangle      (duk_context* ctx);
static duk_ret_t js_SetFrameRate              (duk_context* ctx);
static duk_ret_t js_SetMaxFrameSkips          (duk_context* ctx);
static duk_ret_t js_SetScreenSize             (duk_context* ctx);
static duk_ret_t js_Abort                     (duk_context* ctx);
static duk_ret_t js_Alert                     (duk_context* ctx);
static duk_ret_t js_ApplyColorMask            (duk_context* ctx);
static duk_ret_t js_Assert                    (duk_context* ctx);
static duk_ret_t js_CreateStringFromCode      (duk_context* ctx);
static duk_ret_t js_DebugPrint                (duk_context* ctx);
static duk_ret_t js_Delay                     (duk_context* ctx);
static duk_ret_t js_DispatchScript            (duk_context* ctx);
static duk_ret_t js_DoEvents                  (duk_context* ctx);
static duk_ret_t js_EvaluateScript            (duk_context* ctx);
static duk_ret_t js_EvaluateSystemScript      (duk_context* ctx);
static duk_ret_t js_ExecuteGame               (duk_context* ctx);
static duk_ret_t js_Exit                      (duk_context* ctx);
static duk_ret_t js_FlipScreen                (duk_context* ctx);
static duk_ret_t js_GarbageCollect            (duk_context* ctx);
static duk_ret_t js_GradientCircle            (duk_context* ctx);
static duk_ret_t js_GradientRectangle         (duk_context* ctx);
static duk_ret_t js_Line                      (duk_context* ctx);
static duk_ret_t js_LineSeries                (duk_context* ctx);
static duk_ret_t js_LoadAnimation             (duk_context* ctx);
static duk_ret_t js_LoadFont                  (duk_context* ctx);
static duk_ret_t js_OpenLog                   (duk_context* ctx);
static duk_ret_t js_OutlinedCircle            (duk_context* ctx);
static duk_ret_t js_OutlinedRectangle         (duk_context* ctx);
static duk_ret_t js_OutlinedRoundRectangle    (duk_context* ctx);
static duk_ret_t js_Point                     (duk_context* ctx);
static duk_ret_t js_PointSeries               (duk_context* ctx);
static duk_ret_t js_Print                     (duk_context* ctx);
static duk_ret_t js_Rectangle                 (duk_context* ctx);
static duk_ret_t js_RequireScript             (duk_context* ctx);
static duk_ret_t js_RequireSystemScript       (duk_context* ctx);
static duk_ret_t js_RestartGame               (duk_context* ctx);
static duk_ret_t js_RoundRectangle            (duk_context* ctx);
static duk_ret_t js_Triangle                  (duk_context* ctx);
static duk_ret_t js_UnskipFrame               (duk_context* ctx);
static duk_ret_t js_Animation_finalize        (duk_context* ctx);
static duk_ret_t js_Animation_get_height      (duk_context* ctx);
static duk_ret_t js_Animation_get_width       (duk_context* ctx);
static duk_ret_t js_Animation_getDelay        (duk_context* ctx);
static duk_ret_t js_Animation_getNumFrames    (duk_context* ctx);
static duk_ret_t js_Animation_drawFrame       (duk_context* ctx);
static duk_ret_t js_Animation_drawZoomedFrame (duk_context* ctx);
static duk_ret_t js_Animation_readNextFrame   (duk_context* ctx);
static duk_ret_t js_Font_finalize             (duk_context* ctx);
static duk_ret_t js_Font_toString             (duk_context* ctx);
static duk_ret_t js_Font_get_colorMask        (duk_context* ctx);
static duk_ret_t js_Font_set_colorMask        (duk_context* ctx);
static duk_ret_t js_Font_get_height           (duk_context* ctx);
static duk_ret_t js_Font_getCharacterImage    (duk_context* ctx);
static duk_ret_t js_Font_setCharacterImage    (duk_context* ctx);
static duk_ret_t js_Font_getStringHeight      (duk_context* ctx);
static duk_ret_t js_Font_getStringWidth       (duk_context* ctx);
static duk_ret_t js_Font_clone                (duk_context* ctx);
static duk_ret_t js_Font_drawText             (duk_context* ctx);
static duk_ret_t js_Font_drawTextBox          (duk_context* ctx);
static duk_ret_t js_Font_drawZoomedText       (duk_context* ctx);
static duk_ret_t js_Font_wordWrapString       (duk_context* ctx);
static duk_ret_t js_Logger_finalize           (duk_context* ctx);
static duk_ret_t js_Logger_toString           (duk_context* ctx);
static duk_ret_t js_Logger_beginBlock         (duk_context* ctx);
static duk_ret_t js_Logger_endBlock           (duk_context* ctx);
static duk_ret_t js_Logger_write              (duk_context* ctx);

enum line_series_type
{
	LINE_MULTIPLE,
	LINE_STRIP,
	LINE_LOOP
};

static unsigned int s_next_async_id = 1;

void
initialize_vanilla_api(duk_context* ctx)
{
	console_log(1, "initializing Vanilla (v1.5)");
	
	// add polyfills for __defineGetter__/__defineSetter__.  Sphere 1.x predates ES5
	// and as a result there are a lot of games that expect these to exist.
	duk_eval_string(ctx, "Object.defineProperty(Object.prototype, '__defineGetter__', { value: function(name, func) {"
		"Object.defineProperty(this, name, { get: func, configurable: true }); } });");
	duk_eval_string(ctx, "Object.defineProperty(Object.prototype, '__defineSetter__', { value: function(name, func) {"
		"Object.defineProperty(this, name, { set: func, configurable: true }); } });");

	// set up a dictionary to track RequireScript() calls
	duk_push_global_stash(ctx);
	duk_push_object(ctx);
	duk_put_prop_string(ctx, -2, "RequireScript");
	duk_pop(ctx);

	// register the absolutely massive Sphere 1.x API
	api_register_static_func(ctx, NULL, "IsSkippedFrame", js_IsSkippedFrame);
	api_register_static_func(ctx, NULL, "GetClippingRectangle", js_GetClippingRectangle);
	api_register_static_func(ctx, NULL, "GetFrameRate", js_GetFrameRate);
	api_register_static_func(ctx, NULL, "GetGameManifest", js_GetGameManifest);
	api_register_static_func(ctx, NULL, "GetGameList", js_GetGameList);
	api_register_static_func(ctx, NULL, "GetMaxFrameSkips", js_GetMaxFrameSkips);
	api_register_static_func(ctx, NULL, "GetScreenHeight", js_GetScreenHeight);
	api_register_static_func(ctx, NULL, "GetScreenWidth", js_GetScreenWidth);
	api_register_static_func(ctx, NULL, "GetSystemFont", js_GetSystemFont);
	api_register_static_func(ctx, NULL, "GetTime", js_GetTime);
	api_register_static_func(ctx, NULL, "GetVersion", js_GetVersion);
	api_register_static_func(ctx, NULL, "GetVersionString", js_GetVersionString);
	api_register_static_func(ctx, NULL, "SetClippingRectangle", js_SetClippingRectangle);
	api_register_static_func(ctx, NULL, "SetFrameRate", js_SetFrameRate);
	api_register_static_func(ctx, NULL, "SetMaxFrameSkips", js_SetMaxFrameSkips);
	api_register_static_func(ctx, NULL, "SetScreenSize", js_SetScreenSize);
	api_register_static_func(ctx, NULL, "Abort", js_Abort);
	api_register_static_func(ctx, NULL, "Alert", js_Alert);
	api_register_static_func(ctx, NULL, "ApplyColorMask", js_ApplyColorMask);
	api_register_static_func(ctx, NULL, "Assert", js_Assert);
	api_register_static_func(ctx, NULL, "CreateStringFromCode", js_CreateStringFromCode);
	api_register_static_func(ctx, NULL, "DebugPrint", js_DebugPrint);
	api_register_static_func(ctx, NULL, "Delay", js_Delay);
	api_register_static_func(ctx, NULL, "DispatchScript", js_DispatchScript);
	api_register_static_func(ctx, NULL, "DoEvents", js_DoEvents);
	api_register_static_func(ctx, NULL, "EvaluateScript", js_EvaluateScript);
	api_register_static_func(ctx, NULL, "EvaluateSystemScript", js_EvaluateSystemScript);
	api_register_static_func(ctx, NULL, "Exit", js_Exit);
	api_register_static_func(ctx, NULL, "ExecuteGame", js_ExecuteGame);
	api_register_static_func(ctx, NULL, "FlipScreen", js_FlipScreen);
	api_register_static_func(ctx, NULL, "GarbageCollect", js_GarbageCollect);
	api_register_static_func(ctx, NULL, "GradientCircle", js_GradientCircle);
	api_register_static_func(ctx, NULL, "GradientRectangle", js_GradientRectangle);
	api_register_static_func(ctx, NULL, "Line", js_Line);
	api_register_static_func(ctx, NULL, "LineSeries", js_LineSeries);
	api_register_static_func(ctx, NULL, "LoadAnimation", js_LoadAnimation);
	api_register_static_func(ctx, NULL, "LoadFont", js_LoadFont);
	api_register_static_func(ctx, NULL, "OpenLog", js_OpenLog);
	api_register_static_func(ctx, NULL, "OutlinedCircle", js_OutlinedCircle);
	api_register_static_func(ctx, NULL, "OutlinedRectangle", js_OutlinedRectangle);
	api_register_static_func(ctx, NULL, "OutlinedRoundRectangle", js_OutlinedRoundRectangle);
	api_register_static_func(ctx, NULL, "Point", js_Point);
	api_register_static_func(ctx, NULL, "PointSeries", js_PointSeries);
	api_register_static_func(ctx, NULL, "Print", js_Print);
	api_register_static_func(ctx, NULL, "Rectangle", js_Rectangle);
	api_register_static_func(ctx, NULL, "RequireScript", js_RequireScript);
	api_register_static_func(ctx, NULL, "RequireSystemScript", js_RequireSystemScript);
	api_register_static_func(ctx, NULL, "RestartGame", js_RestartGame);
	api_register_static_func(ctx, NULL, "RoundRectangle", js_RoundRectangle);
	api_register_static_func(ctx, NULL, "Triangle", js_Triangle);
	api_register_static_func(ctx, NULL, "UnskipFrame", js_UnskipFrame);

	api_register_type(ctx, "Animation", js_Animation_finalize);
	api_register_prop(ctx, "Animation", "width", js_Animation_get_width, NULL);
	api_register_prop(ctx, "Animation", "height", js_Animation_get_height, NULL);
	api_register_method(ctx, "Animation", "getDelay", js_Animation_getDelay);
	api_register_method(ctx, "Animation", "getNumFrames", js_Animation_getNumFrames);
	api_register_method(ctx, "Animation", "drawFrame", js_Animation_drawFrame);
	api_register_method(ctx, "Animation", "drawZoomedFrame", js_Animation_drawZoomedFrame);
	api_register_method(ctx, "Animation", "readNextFrame", js_Animation_readNextFrame);

	api_register_type(ctx, "Font", js_Font_finalize);
	api_register_prop(ctx, "Font", "colorMask", js_Font_get_colorMask, js_Font_set_colorMask);
	api_register_prop(ctx, "Font", "height", js_Font_get_height, NULL);
	api_register_method(ctx, "Font", "getCharacterImage", js_Font_getCharacterImage);
	api_register_method(ctx, "Font", "getColorMask", js_Font_get_colorMask);
	api_register_method(ctx, "Font", "getHeight", js_Font_get_height);
	api_register_method(ctx, "Font", "getStringHeight", js_Font_getStringHeight);
	api_register_method(ctx, "Font", "getStringWidth", js_Font_getStringWidth);
	api_register_method(ctx, "Font", "setCharacterImage", js_Font_setCharacterImage);
	api_register_method(ctx, "Font", "setColorMask", js_Font_set_colorMask);
	api_register_method(ctx, "Font", "toString", js_Font_toString);
	api_register_method(ctx, "Font", "clone", js_Font_clone);
	api_register_method(ctx, "Font", "drawText", js_Font_drawText);
	api_register_method(ctx, "Font", "drawTextBox", js_Font_drawTextBox);
	api_register_method(ctx, "Font", "drawZoomedText", js_Font_drawZoomedText);
	api_register_method(ctx, "Font", "wordWrapString", js_Font_wordWrapString);
	
	api_register_type(ctx, "Logger", js_Logger_finalize);
	api_register_method(ctx, "Logger", "toString", js_Logger_toString);
	api_register_method(ctx, "Logger", "beginBlock", js_Logger_beginBlock);
	api_register_method(ctx, "Logger", "endBlock", js_Logger_endBlock);
	api_register_method(ctx, "Logger", "write", js_Logger_write);
	
	// line series types
	api_register_const(ctx, NULL, "LINE_MULTIPLE", LINE_MULTIPLE);
	api_register_const(ctx, NULL, "LINE_STRIP", LINE_STRIP);
	api_register_const(ctx, NULL, "LINE_LOOP", LINE_LOOP);
}

static duk_ret_t
js_IsSkippedFrame(duk_context* ctx)
{
	duk_push_boolean(ctx, screen_is_skipframe(g_screen));
	return 1;
}

static duk_ret_t
js_GetClippingRectangle(duk_context* ctx)
{
	rect_t clip;

	clip = screen_get_clipping(g_screen);
	duk_push_object(ctx);
	duk_push_int(ctx, clip.x1); duk_put_prop_string(ctx, -2, "x");
	duk_push_int(ctx, clip.y1); duk_put_prop_string(ctx, -2, "y");
	duk_push_int(ctx, clip.x2 - clip.x1); duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, clip.y2 - clip.y1); duk_put_prop_string(ctx, -2, "height");
	return 1;
}

static duk_ret_t
js_GetFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, g_framerate);
	return 1;
}

static duk_ret_t
js_GetGameList(duk_context* ctx)
{
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_FS_ENTRY* fse;
	path_t*           path = NULL;
	path_t*           paths[2];
	sandbox_t*        sandbox;

	int i, j = 0;

	// build search paths
	paths[0] = path_rebase(path_new("games/"), enginepath());
	paths[1] = path_rebase(path_new("minisphere/games/"), homepath());

	// search for supported games
	duk_push_array(ctx);
	for (i = sizeof paths / sizeof(path_t*) - 1; i >= 0; --i) {
		fse = al_create_fs_entry(path_cstr(paths[i]));
		if (al_get_fs_entry_mode(fse) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fse)) {
			while (file_info = al_read_directory(fse)) {
				path = path_new(al_get_fs_entry_name(file_info));
				if (sandbox = new_sandbox(path_cstr(path))) {
					duk_push_lstring_t(ctx, get_game_manifest(sandbox));
					duk_json_decode(ctx, -1);
					duk_push_string(ctx, path_cstr(path));
					duk_put_prop_string(ctx, -2, "directory");
					duk_put_prop_index(ctx, -2, j++);
					free_sandbox(sandbox);
				}
				path_free(path);
			}
		}
		al_destroy_fs_entry(fse);
		path_free(paths[i]);
	}
	return 1;
}

static duk_ret_t
js_GetGameManifest(duk_context* ctx)
{
	duk_push_lstring_t(ctx, get_game_manifest(g_fs));
	duk_json_decode(ctx, -1);
	duk_push_string(ctx, path_cstr(get_game_path(g_fs)));
	duk_put_prop_string(ctx, -2, "directory");
	return 1;
}

static duk_ret_t
js_GetMaxFrameSkips(duk_context* ctx)
{
	duk_push_int(ctx, screen_get_frameskip(g_screen));
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
js_GetSystemFont(duk_context* ctx)
{
	duk_push_sphere_font(ctx, g_sys_font);
	return 1;
}

static duk_ret_t
js_GetTime(duk_context* ctx)
{
	duk_push_number(ctx, floor(al_get_time() * 1000));
	return 1;
}

static duk_ret_t
js_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, 1.5);
	return 1;
}

static duk_ret_t
js_GetVersionString(duk_context* ctx)
{
	duk_push_sprintf(ctx, "v1.5 (%s %s)", PRODUCT_NAME, VERSION_NAME);
	return 1;
}

static duk_ret_t
js_SetClippingRectangle(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int width = duk_require_int(ctx, 2);
	int height = duk_require_int(ctx, 3);

	screen_set_clipping(g_screen, new_rect(x, y, x + width, y + height));
	return 0;
}

static duk_ret_t
js_SetFrameRate(duk_context* ctx)
{
	int framerate = duk_require_int(ctx, 0);

	if (framerate < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetFrameRate(): framerate must be positive (got: %d)", framerate);
	g_framerate = framerate;
	return 0;
}

static duk_ret_t
js_SetMaxFrameSkips(duk_context* ctx)
{
	int max_skips = duk_require_int(ctx, 0);

	if (max_skips < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetMaxFrameSkips(): value cannot be negative (%d)", max_skips);
	screen_set_frameskip(g_screen, max_skips);
	return 0;
}

static duk_ret_t
js_SetScreenSize(duk_context* ctx)
{
	int  res_width;
	int  res_height;

	res_width = duk_require_int(ctx, 0);
	res_height = duk_require_int(ctx, 1);

	if (res_width < 0 || res_height < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "SetScreenSize(): dimensions cannot be negative (got X: %d, Y: %d)",
			res_width, res_height);
	screen_resize(g_screen, res_width, res_height);
	return 0;
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

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Alert(): stack offset must be negative");

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
	duk_get_prop_string(ctx, -1, "fileName"); filename = duk_get_string(ctx, -1); duk_pop(ctx);
	duk_pop_2(ctx);

	// show the message
	screen_show_mouse(g_screen, true);
	caller_info =
		duk_push_sprintf(ctx, "%s (line %i)", filename, line_number),
		duk_get_string(ctx, -1);
	al_show_native_message_box(screen_display(g_screen), "Alert from Sphere game", caller_info, text, NULL, 0x0);
	screen_show_mouse(g_screen, false);

	return 0;
}

static duk_ret_t
js_Abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* message = n_args >= 1 ? duk_to_string(ctx, 0) : "Some type of weird pig just ate your game!\n\n\n\n\n\n\n\n...and you*munch*";
	int stack_offset = n_args >= 2 ? duk_require_int(ctx, 1) : 0;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Abort(): stack offset must be negative");
	duk_error_ni(ctx, -1 + stack_offset, DUK_ERR_ERROR, "%s", message);
}

static duk_ret_t
js_ApplyColorMask(duk_context* ctx)
{
	color_t color;

	color = duk_require_sphere_color(ctx, 0);

	if (!screen_is_skipframe(g_screen))
		al_draw_filled_rectangle(0, 0, g_res_x, g_res_y, nativecolor(color));
	return 0;
}

static duk_ret_t
js_Assert(duk_context* ctx)
{
	const char* filename;
	int         line_number;
	const char* message;
	int         num_args;
	bool        result;
	int         stack_offset;
	lstring_t*  text;

	num_args = duk_get_top(ctx);
	result = duk_to_boolean(ctx, 0);
	message = duk_require_string(ctx, 1);
	stack_offset = num_args >= 3 ? duk_require_int(ctx, 2)
		: 0;

	if (stack_offset > 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Assert(): stack offset must be negative");

	if (!result) {
		// get the offending script and line number from the call stack
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
		duk_get_prop_string(ctx, -1, "fileName"); filename = duk_get_string(ctx, -1); duk_pop(ctx);
		duk_pop_2(ctx);
		fprintf(stderr, "ASSERT: `%s:%i` : %s\n", filename, line_number, message);

		// if an assertion fails in a game being debugged:
		//   - the user may choose to ignore it, in which case execution continues.  this is useful
		//     in some debugging scenarios.
		//   - if the user chooses not to continue, a prompt breakpoint will be triggered, turning
		//     over control to the attached debugger.
		if (is_debugger_attached()) {
			text = lstr_newf("%s (line: %i)\n%s\n\nYou can ignore the error, or pause execution, turning over control to the attached debugger.  If you choose to debug, execution will pause at the statement following the failed Assert().\n\nIgnore the error and continue?", filename, line_number, message);
			if (!al_show_native_message_box(screen_display(g_screen), "Script Error", "Assertion failed!",
				lstr_cstr(text), NULL, ALLEGRO_MESSAGEBOX_WARN | ALLEGRO_MESSAGEBOX_YES_NO))
			{
				duk_debugger_pause(ctx);
			}
			lstr_free(text);
		}
	}
	duk_dup(ctx, 0);
	return 1;
}

static duk_ret_t
js_CreateStringFromCode(duk_context* ctx)
{
	int code = duk_require_int(ctx, 0);

	char cstr[2];

	if (code < 0 || code > 255)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "CreateStringFromCode(): character code is out of ASCII range (%i)", code);
	cstr[0] = (char)code; cstr[1] = '\0';
	duk_push_string(ctx, cstr);
	return 1;
}

static duk_ret_t
js_DebugPrint(duk_context* ctx)
{
	int num_items;

	num_items = duk_get_top(ctx);

	// separate printed values with a space
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_NORMAL);
	return 0;
}

static duk_ret_t
js_Delay(duk_context* ctx)
{
	double millisecs = floor(duk_require_number(ctx, 0));

	if (millisecs < 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Delay(): delay must be positive (got: %.0f)", millisecs);
	delay(millisecs / 1000);
	return 0;
}

static duk_ret_t
js_DispatchScript(duk_context* ctx)
{
	script_t* script;
	char*     script_name;

	script_name = strnewf("synth:async~%u.js", s_next_async_id++);
	script = duk_require_sphere_script(ctx, 0, script_name);
	free(script_name);

	if (!queue_async_script(script))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to dispatch async script");
	return 0;
}

static duk_ret_t
js_DoEvents(duk_context* ctx)
{
	do_events();
	duk_push_boolean(ctx, true);
	return 1;
}

static duk_ret_t
js_EvaluateScript(duk_context* ctx)
{
	const char* filename;

	filename = duk_require_path(ctx, 0, "scripts", true);
	if (!sfs_fexist(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "EvaluateScript(): file `%s` not found", filename);
	if (!evaluate_script(filename, false))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_EvaluateSystemScript(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	char path[SPHERE_PATH_MAX];

	sprintf(path, "scripts/lib/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		sprintf(path, "#/scripts/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "EvaluateSystemScript(): system script `%s` not found", filename);
	if (!evaluate_script(path, false))
		duk_throw(ctx);
	return 1;
}

static duk_ret_t
js_ExecuteGame(duk_context* ctx)
{
	path_t*     games_path;
	const char* filename;

	filename = duk_require_string(ctx, 0);

	// store the old game path so we can relaunch when the chained game exits
	g_last_game_path = path_dup(get_game_path(g_fs));

	// if the passed-in path is relative, resolve it relative to <engine>/games.
	// this is done for compatibility with Sphere 1.x.
	g_game_path = path_new(filename);
	games_path = path_rebase(path_new("games/"), enginepath());
	path_rebase(g_game_path, games_path);
	path_free(games_path);

	restart_engine();
}

static duk_ret_t
js_Exit(duk_context* ctx)
{
	exit_game(false);
}

static duk_ret_t
js_FlipScreen(duk_context* ctx)
{
	screen_flip(g_screen, g_framerate);
	return 0;
}

static duk_ret_t
js_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}

static duk_ret_t
js_GradientCircle(duk_context* ctx)
{
	static ALLEGRO_VERTEX s_vbuf[128];

	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	int radius = duk_require_number(ctx, 2);
	color_t in_color = duk_require_sphere_color(ctx, 3);
	color_t out_color = duk_require_sphere_color(ctx, 4);

	double phi;
	int    vcount;

	int i;

	if (screen_is_skipframe(g_screen))
		return 0;
	vcount = fmin(radius, 126);
	s_vbuf[0].x = x; s_vbuf[0].y = y; s_vbuf[0].z = 0;
	s_vbuf[0].color = nativecolor(in_color);
	for (i = 0; i < vcount; ++i) {
		phi = 2 * M_PI * i / vcount;
		s_vbuf[i + 1].x = x + cos(phi) * radius;
		s_vbuf[i + 1].y = y - sin(phi) * radius;
		s_vbuf[i + 1].z = 0;
		s_vbuf[i + 1].color = nativecolor(out_color);
	}
	s_vbuf[i + 1].x = x + cos(0) * radius;
	s_vbuf[i + 1].y = y - sin(0) * radius;
	s_vbuf[i + 1].z = 0;
	s_vbuf[i + 1].color = nativecolor(out_color);
	al_draw_prim(s_vbuf, NULL, NULL, 0, vcount + 2, ALLEGRO_PRIM_TRIANGLE_FAN);
	return 0;
}

static duk_ret_t
js_GradientRectangle(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = x1 + duk_require_int(ctx, 2);
	int y2 = y1 + duk_require_int(ctx, 3);
	color_t color_ul = duk_require_sphere_color(ctx, 4);
	color_t color_ur = duk_require_sphere_color(ctx, 5);
	color_t color_lr = duk_require_sphere_color(ctx, 6);
	color_t color_ll = duk_require_sphere_color(ctx, 7);

	if (!screen_is_skipframe(g_screen)) {
		ALLEGRO_VERTEX verts[] = {
			{ x1, y1, 0, 0, 0, nativecolor(color_ul) },
			{ x2, y1, 0, 0, 0, nativecolor(color_ur) },
			{ x1, y2, 0, 0, 0, nativecolor(color_ll) },
			{ x2, y2, 0, 0, 0, nativecolor(color_lr) }
		};
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	}
	return 0;
}

static duk_ret_t
js_Line(duk_context* ctx)
{
	float x1 = duk_require_int(ctx, 0) + 0.5;
	float y1 = duk_require_int(ctx, 1) + 0.5;
	float x2 = duk_require_int(ctx, 2) + 0.5;
	float y2 = duk_require_int(ctx, 3) + 0.5;
	color_t color = duk_require_sphere_color(ctx, 4);

	if (!screen_is_skipframe(g_screen))
		al_draw_line(x1, y1, x2, y2, nativecolor(color), 1);
	return 0;
}

static duk_ret_t
js_LineSeries(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	color_t color = duk_require_sphere_color(ctx, 1);
	int type = n_args >= 3 ? duk_require_int(ctx, 2) : LINE_MULTIPLE;

	size_t          num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	size_t i;

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LineSeries(): first argument must be an array");
	duk_get_prop_string(ctx, 0, "length"); num_points = duk_get_uint(ctx, 0); duk_pop(ctx);
	if (num_points < 2)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "LineSeries(): two or more vertices required");
	if (num_points > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "LineSeries(): too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LineSeries(): unable to allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
		duk_get_prop_string(ctx, 0, "x"); x = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, 0, "y"); y = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points,
		type == LINE_STRIP ? ALLEGRO_PRIM_LINE_STRIP
		: type == LINE_LOOP ? ALLEGRO_PRIM_LINE_LOOP
		: ALLEGRO_PRIM_LINE_LIST
	);
	free(vertices);
	return 0;
}

static duk_ret_t
js_LoadAnimation(duk_context* ctx)
{
	animation_t* anim;
	const char*  filename;

	filename = duk_require_path(ctx, 0, "animations", true);
	if (!(anim = animation_new(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LoadAnimation(): unable to load animation file `%s`", filename);
	duk_push_sphere_obj(ctx, "Animation", anim);
	return 1;
}

static duk_ret_t
js_LoadFont(duk_context* ctx)
{
	const char* filename;
	font_t*     font;

	filename = duk_require_path(ctx, 0, "fonts", true);
	if (!(font = font_load(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to load font `%s`", filename);
	duk_push_sphere_font(ctx, font);
	font_free(font);
	return 1;
}

static duk_ret_t
js_OpenLog(duk_context* ctx)
{
	const char* filename;
	logger_t*   logger;

	filename = duk_require_path(ctx, 0, "logs", true);
	if (!(logger = log_open(filename)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to open log `%s`", filename);
	duk_push_sphere_obj(ctx, "Logger", logger);
	return 1;
}

static duk_ret_t
js_OutlinedCircle(duk_context* ctx)
{
	float x = duk_require_int(ctx, 0) + 0.5;
	float y = duk_require_int(ctx, 1) + 0.5;
	float radius = duk_require_int(ctx, 2);
	color_t color = duk_require_sphere_color(ctx, 3);

	if (!screen_is_skipframe(g_screen))
		al_draw_circle(x, y, radius, nativecolor(color), 1);
	return 0;
}

static duk_ret_t
js_OutlinedRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x1 = duk_require_int(ctx, 0) + 0.5;
	float y1 = duk_require_int(ctx, 1) + 0.5;
	float x2 = x1 + duk_require_int(ctx, 2) - 1;
	float y2 = y1 + duk_require_int(ctx, 3) - 1;
	color_t color = duk_require_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_require_int(ctx, 5) : 1;

	if (!screen_is_skipframe(g_screen))
		al_draw_rectangle(x1, y1, x2, y2, nativecolor(color), thickness);
	return 0;
}

static duk_ret_t
js_OutlinedRoundRectangle(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	float x = duk_require_int(ctx, 0) + 0.5;
	float y = duk_require_int(ctx, 1) + 0.5;
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	float radius = duk_require_number(ctx, 4);
	color_t color = duk_require_sphere_color(ctx, 5);
	int thickness = n_args >= 7 ? duk_require_int(ctx, 6) : 1;

	if (!screen_is_skipframe(g_screen))
		al_draw_rounded_rectangle(x, y, x + w - 1, y + h - 1, radius, radius, nativecolor(color), thickness);
	return 0;
}

static duk_ret_t
js_Point(duk_context* ctx)
{
	float x = duk_require_int(ctx, 0) + 0.5;
	float y = duk_require_int(ctx, 1) + 0.5;
	color_t color = duk_require_sphere_color(ctx, 2);

	if (!screen_is_skipframe(g_screen))
		al_draw_pixel(x, y, nativecolor(color));
	return 0;
}

static duk_ret_t
js_PointSeries(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 1);

	size_t          num_points;
	int             x, y;
	ALLEGRO_VERTEX* vertices;
	ALLEGRO_COLOR   vtx_color;

	size_t i;

	if (!duk_is_array(ctx, 0))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "PointSeries(): first argument must be an array");
	duk_get_prop_string(ctx, 0, "length"); num_points = duk_get_uint(ctx, 0); duk_pop(ctx);
	if (num_points < 1)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "PointSeries(): one or more vertices required");
	if (num_points > INT_MAX)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "PointSeries(): too many vertices");
	if ((vertices = calloc(num_points, sizeof(ALLEGRO_VERTEX))) == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "PointSeries(): unable to allocate vertex buffer");
	vtx_color = nativecolor(color);
	for (i = 0; i < num_points; ++i) {
		duk_get_prop_index(ctx, 0, (duk_uarridx_t)i);
		duk_get_prop_string(ctx, 0, "x"); x = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_get_prop_string(ctx, 0, "y"); y = duk_require_int(ctx, -1); duk_pop(ctx);
		duk_pop(ctx);
		vertices[i].x = x + 0.5; vertices[i].y = y + 0.5;
		vertices[i].color = vtx_color;
	}
	al_draw_prim(vertices, NULL, NULL, 0, (int)num_points, ALLEGRO_PRIM_POINT_LIST);
	free(vertices);
	return 0;
}

static duk_ret_t
js_Print(duk_context* ctx)
{
	int num_items;

	num_items = duk_get_top(ctx);

	// separate printed values with a space
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	printf("%s\n", duk_get_string(ctx, -1));
	return 0;
}

static duk_ret_t
js_Rectangle(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	color_t color = duk_require_sphere_color(ctx, 4);

	if (!screen_is_skipframe(g_screen))
		al_draw_filled_rectangle(x, y, x + w, y + h, nativecolor(color));
	return 0;
}

static duk_ret_t
js_RequireScript(duk_context* ctx)
{
	const char* filename;
	bool        is_required;

	filename = duk_require_path(ctx, 0, "scripts", true);
	if (!sfs_fexist(g_fs, filename, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RequireScript(): file `%s` not found", filename);
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, filename);
	is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx);
		duk_put_prop_string(ctx, -2, filename);
		if (!evaluate_script(filename, false))
			duk_throw(ctx);
	}
	duk_pop_3(ctx);
	return 0;
}

static duk_ret_t
js_RequireSystemScript(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	bool is_required;
	char path[SPHERE_PATH_MAX];

	sprintf(path, "scripts/lib/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		sprintf(path, "#/scripts/%s", filename);
	if (!sfs_fexist(g_fs, path, NULL))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "RequireSystemScript(): system script `%s` not found", filename);

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "RequireScript");
	duk_get_prop_string(ctx, -1, path);
	is_required = duk_get_boolean(ctx, -1);
	duk_pop(ctx);
	if (!is_required) {
		duk_push_true(ctx);
		duk_put_prop_string(ctx, -2, path);
		if (!evaluate_script(path, false))
			duk_throw(ctx);
	}
	duk_pop_2(ctx);
	return 0;
}

static duk_ret_t
js_RestartGame(duk_context* ctx)
{
	restart_engine();
}

static duk_ret_t
js_RoundRectangle(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	float radius = duk_require_number(ctx, 4);
	color_t color = duk_require_sphere_color(ctx, 5);

	if (!screen_is_skipframe(g_screen))
		al_draw_filled_rounded_rectangle(x, y, x + w, y + h, radius, radius, nativecolor(color));
	return 0;
}

static duk_ret_t
js_Triangle(duk_context* ctx)
{
	int x1 = duk_require_int(ctx, 0);
	int y1 = duk_require_int(ctx, 1);
	int x2 = duk_require_int(ctx, 2);
	int y2 = duk_require_int(ctx, 3);
	int x3 = duk_require_int(ctx, 4);
	int y3 = duk_require_int(ctx, 5);
	color_t color = duk_require_sphere_color(ctx, 6);

	if (!screen_is_skipframe(g_screen))
		al_draw_filled_triangle(x1, y1, x2, y2, x3, y3, nativecolor(color));
	return 0;
}

static duk_ret_t
js_UnskipFrame(duk_context* ctx)
{
	screen_unskip_frame(g_screen);
	return 0;
}

static duk_ret_t
js_Animation_finalize(duk_context* ctx)
{
	animation_t* anim;

	anim = duk_require_sphere_obj(ctx, 0, "Animation");
	animation_free(anim);
	return 0;
}

static duk_ret_t
js_Animation_get_height(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");

	duk_push_int(ctx, animation_height(anim));
	return 1;
}

static duk_ret_t
js_Animation_get_width(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");

	duk_push_int(ctx, animation_width(anim));
	return 1;
}

static duk_ret_t
js_Animation_getDelay(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_int(ctx, animation_delay(anim));
	return 1;
}

static duk_ret_t
js_Animation_getNumFrames(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");

	duk_push_int(ctx, animation_num_frames(anim));
	return 1;
}

static duk_ret_t
js_Animation_drawFrame(duk_context* ctx)
{
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);

	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");

	image_draw(animation_frame(anim), x, y);
	return 0;
}

static duk_ret_t
js_Animation_drawZoomedFrame(duk_context* ctx)
{
	animation_t* anim;
	int          height;
	double       scale;
	int          width;
	int          x;
	int          y;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	x = duk_require_number(ctx, 0);
	y = duk_require_number(ctx, 1);
	scale = duk_require_number(ctx, 2);

	if (scale < 0.0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "zoom must be positive");
	width = animation_width(anim);
	height = animation_height(anim);
	image_draw_scaled(animation_frame(anim), x, y, width * scale, height * scale);
	return 0;
}

static duk_ret_t
js_Animation_readNextFrame(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	animation_update(anim);
	return 0;
}

static duk_ret_t
js_Font_finalize(duk_context* ctx)
{
	font_t* font;

	font = duk_require_sphere_obj(ctx, 0, "Font");
	font_free(font);
	return 0;
}

static duk_ret_t
js_Font_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object font]");
	return 1;
}

static duk_ret_t
js_Font_getCharacterImage(duk_context* ctx)
{
	uint32_t cp;
	font_t*  font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	cp = duk_require_uint(ctx, 0);

	duk_push_sphere_image(ctx, font_glyph(font, cp));
	return 1;
}

static duk_ret_t
js_Font_get_colorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_Font_set_colorMask(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "color_mask"); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Font_get_height(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	duk_push_int(ctx, font_height(font));
	return 1;
}

static duk_ret_t
js_Font_setCharacterImage(duk_context* ctx)
{
	int cp = duk_require_int(ctx, 0);
	image_t* image = duk_require_sphere_image(ctx, 1);

	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");

	font_set_glyph(font, cp, image);
	return 0;
}

static duk_ret_t
js_Font_clone(duk_context* ctx)
{
	font_t* dolly_font;
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	if (!(dolly_font = font_clone(font)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Font:clone(): unable to clone font");
	duk_push_sphere_font(ctx, dolly_font);
	return 1;
}

static duk_ret_t
js_Font_drawText(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	const char* text = duk_to_string(ctx, 2);

	font_t* font;
	color_t mask;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen))
		font_draw_text(font, mask, x, y, TEXT_ALIGN_LEFT, text);
	return 0;
}

static duk_ret_t
js_Font_drawZoomedText(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	float scale = duk_require_number(ctx, 2);
	const char* text = duk_to_string(ctx, 3);

	ALLEGRO_BITMAP* bitmap;
	font_t*         font;
	color_t         mask;
	int             text_w, text_h;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen)) {
		text_w = font_get_width(font, text);
		text_h = font_height(font);
		bitmap = al_create_bitmap(text_w, text_h);
		al_set_target_bitmap(bitmap);
		font_draw_text(font, mask, 0, 0, TEXT_ALIGN_LEFT, text);
		al_set_target_backbuffer(screen_display(g_screen));
		al_draw_scaled_bitmap(bitmap, 0, 0, text_w, text_h, x, y, text_w * scale, text_h * scale, 0x0);
		al_destroy_bitmap(bitmap);
	}
	return 0;
}

static duk_ret_t
js_Font_drawTextBox(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	int offset = duk_require_int(ctx, 4);
	const char* text = duk_to_string(ctx, 5);

	font_t*     font;
	int         line_height;
	const char* line_text;
	color_t     mask;
	int         num_lines;

	int i;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!screen_is_skipframe(g_screen)) {
		duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
		duk_push_this(ctx);
		duk_push_string(ctx, text);
		duk_push_int(ctx, w);
		duk_call_method(ctx, 2);
		duk_get_prop_string(ctx, -1, "length"); num_lines = duk_get_int(ctx, -1); duk_pop(ctx);
		line_height = font_height(font);
		for (i = 0; i < num_lines; ++i) {
			duk_get_prop_index(ctx, -1, i); line_text = duk_get_string(ctx, -1); duk_pop(ctx);
			font_draw_text(font, mask, x + offset, y, TEXT_ALIGN_LEFT, line_text);
			y += line_height;
		}
		duk_pop(ctx);
	}
	return 0;
}

static duk_ret_t
js_Font_getStringHeight(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int width = duk_require_int(ctx, 1);

	font_t* font;
	int     num_lines;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
	duk_push_this(ctx);
	duk_push_string(ctx, text);
	duk_push_int(ctx, width);
	duk_call_method(ctx, 2);
	duk_get_prop_string(ctx, -1, "length"); num_lines = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, font_height(font) * num_lines);
	return 1;
}

static duk_ret_t
js_Font_getStringWidth(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);

	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	duk_push_int(ctx, font_get_width(font, text));
	return 1;
}

static duk_ret_t
js_Font_wordWrapString(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int         width = duk_require_int(ctx, 1);

	font_t*     font;
	int         num_lines;
	wraptext_t* wraptext;

	int i;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	wraptext = wraptext_new(text, font, width);
	num_lines = wraptext_len(wraptext);
	duk_push_array(ctx);
	for (i = 0; i < num_lines; ++i) {
		duk_push_string(ctx, wraptext_line(wraptext, i));
		duk_put_prop_index(ctx, -2, i);
	}
	wraptext_free(wraptext);
	return 1;
}

static duk_ret_t
js_Logger_finalize(duk_context* ctx)
{
	logger_t* logger;

	logger = duk_require_sphere_obj(ctx, 0, "Logger");
	log_close(logger);
	return 0;
}

static duk_ret_t
js_Logger_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object log]");
	return 1;
}

static duk_ret_t
js_Logger_beginBlock(duk_context* ctx)
{
	const char* title = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	if (!log_begin_block(logger, title))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Log:beginBlock(): unable to create new log block");
	return 0;
}

static duk_ret_t
js_Logger_endBlock(duk_context* ctx)
{
	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	log_end_block(logger);
	return 0;
}

static duk_ret_t
js_Logger_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);

	logger_t* logger;

	duk_push_this(ctx);
	logger = duk_require_sphere_obj(ctx, -1, "Logger");
	log_write(logger, NULL, text);
	return 0;
}
