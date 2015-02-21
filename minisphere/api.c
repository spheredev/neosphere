#include "minisphere.h"
#include "api.h"
#include "color.h"

static duk_ret_t duk_RequireSystemScript(duk_context* ctx);
static duk_ret_t duk_RequireScript(duk_context* ctx);
static duk_ret_t duk_EvaluateSystemScript(duk_context* ctx);
static duk_ret_t duk_EvaluateScript(duk_context* ctx);
static duk_ret_t duk_GetVersion(duk_context* ctx);
static duk_ret_t duk_GetVersionString(duk_context* ctx);
static duk_ret_t duk_GetClippingRectangle(duk_context* ctx);
static duk_ret_t duk_GetFrameRate(duk_context* ctx);
static duk_ret_t duk_GetScreenHeight(duk_context* ctx);
static duk_ret_t duk_GetScreenWidth(duk_context* ctx);
static duk_ret_t duk_GetSystemFont(duk_context* ctx);
static duk_ret_t duk_GetTime(duk_context* ctx);
static duk_ret_t duk_SetClippingRectangle(duk_context* ctx);
static duk_ret_t duk_SetFrameRate(duk_context* ctx);
static duk_ret_t duk_Abort(duk_context* ctx);
static duk_ret_t duk_ApplyColorMask(duk_context* ctx);
static duk_ret_t duk_Exit(duk_context* ctx);
static duk_ret_t duk_FlipScreen(duk_context* ctx);
static duk_ret_t duk_GarbageCollect(duk_context* ctx);
static duk_ret_t duk_GetFileList(duk_context* ctx);
static duk_ret_t duk_GradientCircle(duk_context* ctx);
static duk_ret_t duk_GradientRectangle(duk_context* ctx);
static duk_ret_t duk_Line(duk_context* ctx);
static duk_ret_t duk_OutlinedRectangle(duk_context* ctx);
static duk_ret_t duk_Rectangle(duk_context* ctx);

static int s_framerate = 0;

void
init_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "GetVersion", &duk_GetVersion);
	register_api_func(ctx, NULL, "GetVersionString", &duk_GetVersionString);
	register_api_func(ctx, NULL, "Abort", &duk_Abort);
	register_api_func(ctx, NULL, "EvaluateScript", &duk_EvaluateScript);
	register_api_func(ctx, NULL, "EvaluateSystemScript", &duk_EvaluateSystemScript);
	register_api_func(ctx, NULL, "Exit", &duk_Exit);
	register_api_func(ctx, NULL, "GetFrameRate", &duk_GetFrameRate);
	register_api_func(ctx, NULL, "GetTime", &duk_GetTime);
	register_api_func(ctx, NULL, "RequireScript", &duk_RequireScript);
	register_api_func(ctx, NULL, "RequireSystemScript", &duk_RequireSystemScript);
	register_api_func(ctx, NULL, "SetFrameRate", &duk_SetFrameRate);
	register_api_func(ctx, NULL, "GetSystemFont", &duk_GetSystemFont);
	register_api_func(ctx, NULL, "GetClippingRectangle", &duk_GetClippingRectangle);
	register_api_func(ctx, NULL, "GetScreenHeight", &duk_GetScreenHeight);
	register_api_func(ctx, NULL, "GetScreenWidth", &duk_GetScreenWidth);
	register_api_func(ctx, NULL, "SetClippingRectangle", &duk_SetClippingRectangle);
	register_api_func(ctx, NULL, "ApplyColorMask", &duk_ApplyColorMask);
	register_api_func(ctx, NULL, "FlipScreen", &duk_FlipScreen);
	register_api_func(ctx, NULL, "GarbageCollect", &duk_GarbageCollect);
	register_api_func(ctx, NULL, "GetFileList", &duk_GetFileList);
	register_api_func(ctx, NULL, "GradientCircle", &duk_GradientCircle);
	register_api_func(ctx, NULL, "GradientRectangle", &duk_GradientRectangle);
	register_api_func(ctx, NULL, "Line", &duk_Line);
	register_api_func(ctx, NULL, "OutlinedRectangle", &duk_OutlinedRectangle);
	register_api_func(ctx, NULL, "Rectangle", &duk_Rectangle);
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

static duk_ret_t
duk_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, 1.5);
	return 1;
}

static duk_ret_t
duk_GetVersionString(duk_context* ctx)
{
	duk_push_sprintf(ctx, "%s (compatible; minisphere %s)", SPHERE_API_VER, ENGINE_VER);
	return 1;
}

static duk_ret_t
duk_Abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* err_msg = n_args > 0 ? duk_to_string(ctx, 0) : "Abort() called by script";
	duk_error(ctx, DUK_ERR_ERROR, "%s", err_msg);
	return 0;
}

duk_ret_t
duk_EvaluateScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_asset_path(script_file, "scripts", false);
	duk_eval_file_noresult(ctx, script_path);
	free(script_path);
	return 0;
}

duk_ret_t
duk_EvaluateSystemScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_sys_asset_path(script_file, "scripts");
	duk_eval_file_noresult(ctx, script_path);
	free(script_path);
	return 0;
}

duk_ret_t
duk_Exit(duk_context* ctx)
{
	duk_error(ctx, DUK_ERR_ERROR, "!exit");
	return 0;
}

static duk_ret_t
duk_GetFrameRate(duk_context* ctx)
{
	duk_push_int(ctx, s_framerate);
	return 1;
}

duk_ret_t
duk_GetTime(duk_context* ctx)
{
	int ms = floor(al_get_time() * 1000);
	duk_push_int(ctx, ms);
	return 1;
}

duk_ret_t
duk_RequireScript(duk_context* ctx)
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

duk_ret_t
duk_RequireSystemScript(duk_context* ctx)
{
	const char* script_file = duk_get_string(ctx, 0);
	char* script_path = get_sys_asset_path(script_file, "scripts");
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
duk_SetFrameRate(duk_context* ctx)
{
	s_framerate = duk_to_int(ctx, 0);
	return 0;
}

duk_ret_t
duk_GetSystemFont(duk_context* ctx)
{
	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "system_font");
	duk_remove(ctx, -2);
	return 1;
}

duk_ret_t
duk_GetClippingRectangle(duk_context* ctx)
{
	int x, y, width, height;
	al_get_clipping_rectangle(&x, &y, &width, &height);
	duk_push_object(ctx);
	duk_push_int(ctx, x); duk_put_prop_string(ctx, -2, "x");
	duk_push_int(ctx, y); duk_put_prop_string(ctx, -2, "y");
	duk_push_int(ctx, width); duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, height); duk_put_prop_string(ctx, -2, "height");
	return 1;
}

duk_ret_t
duk_GetScreenHeight(duk_context* ctx)
{
	duk_push_int(ctx, g_res_y);
	return 1;
}

duk_ret_t
duk_GetScreenWidth(duk_context* ctx)
{
	duk_push_int(ctx, g_res_x);
	return 1;
}

duk_ret_t
duk_SetClippingRectangle(duk_context* ctx)
{
	int x = duk_to_int(ctx, 0);
	int y = duk_to_int(ctx, 1);
	int width = duk_to_int(ctx, 2);
	int height = duk_to_int(ctx, 3);
	// HACK: Allegro doesn't transform the clipping rect, so we have to scale it manually.
	al_set_clipping_rectangle(x * g_render_scale, y * g_render_scale, width * g_render_scale, height * g_render_scale);
	return 0;
}

duk_ret_t
duk_ApplyColorMask(duk_context* ctx)
{
	ALLEGRO_COLOR mask_color;
	float         rect_w, rect_h;
	
	mask_color = duk_get_sphere_color(ctx, 0);
	rect_w = al_get_display_width(g_display);
	rect_h = al_get_display_height(g_display);
	if (!g_skip_frame) al_draw_filled_rectangle(0, 0, rect_w, rect_h, mask_color);
	return 0;
}

duk_ret_t
duk_FlipScreen(duk_context* ctx)
{
	if (!begin_frame(s_framerate)) duk_error(ctx, DUK_ERR_ERROR, "!exit");
	return 0;
}

static duk_ret_t
duk_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}

static duk_ret_t
duk_GetFileList(duk_context* ctx)
{
	const char*       directory_name;
	ALLEGRO_FS_ENTRY* file_info;
	ALLEGRO_PATH*     file_path;
	ALLEGRO_FS_ENTRY* fs;
	int               n_args;
	char*             path;
	int               i;

	n_args = duk_get_top(ctx);
	directory_name = n_args >= 1 ? duk_require_string(ctx, 0) : NULL;
	path = get_asset_path(directory_name, NULL, false);
	fs = al_create_fs_entry(path);
	free(path);
	i = 0;
	if (al_get_fs_entry_mode(fs) & ALLEGRO_FILEMODE_ISDIR && al_open_directory(fs)) {
		duk_push_array(ctx);
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

duk_ret_t
duk_GradientCircle(duk_context* ctx)
{
	ALLEGRO_COLOR inner_color;
	ALLEGRO_COLOR outer_color;
	float         radius;
	float         x, y;
	
	x = (float)duk_require_number(ctx, 0);
	y = (float)duk_require_number(ctx, 1);
	radius = (float)duk_require_number(ctx, 2);
	inner_color = duk_get_sphere_color(ctx, 3);
	outer_color = duk_get_sphere_color(ctx, 4);
	// TODO: actually draw a gradient circle instead of a solid one
	if (!g_skip_frame) al_draw_filled_circle(x, y, radius, inner_color);
	return 0;
}

duk_ret_t
duk_GradientRectangle(duk_context* ctx)
{
	ALLEGRO_COLOR color_ul, color_ur, color_lr, color_ll;
	int           x1, y1, x2, y2;
	
	x1 = duk_to_int(ctx, 0);
	y1 = duk_to_int(ctx, 1);
	x2 = x1 + duk_to_int(ctx, 2);
	y2 = y1 + duk_to_int(ctx, 3);
	color_ul = duk_get_sphere_color(ctx, 4);
	color_ur = duk_get_sphere_color(ctx, 5);
	color_lr = duk_get_sphere_color(ctx, 6);
	color_ll = duk_get_sphere_color(ctx, 7);
	if (!g_skip_frame) {
		ALLEGRO_VERTEX verts[] = {
			{ x1, y1, 0, 0, 0, color_ul },
			{ x2, y1, 0, 0, 0, color_ur },
			{ x1, y2, 0, 0, 0, color_ll },
			{ x2, y2, 0, 0, 0, color_lr }
		};
		al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	}
	return 0;
}

duk_ret_t
duk_Line(duk_context* ctx)
{
	ALLEGRO_COLOR color;
	int           x1, y1, x2, y2;

	x1 = duk_to_int(ctx, 0);
	y1 = duk_to_int(ctx, 1);
	x2 = duk_to_int(ctx, 2);
	y2 = duk_to_int(ctx, 3);
	color = duk_get_sphere_color(ctx, 4);
	if (!g_skip_frame) al_draw_line(x1, y1, x2, y2, color, 1);
	return 0;
}

duk_ret_t
duk_OutlinedRectangle(duk_context* ctx)
{
	ALLEGRO_COLOR color;
	int           n_args;
	float         x1, y1, x2, y2;
	
	n_args = duk_get_top(ctx);
	x1 = duk_to_int(ctx, 0) + 0.5;
	y1 = duk_to_int(ctx, 1) + 0.5;
	x2 = x1 + duk_to_int(ctx, 2) - 1;
	y2 = y1 + duk_to_int(ctx, 3) - 1;
	color = duk_get_sphere_color(ctx, 4);
	int thickness = n_args >= 6 ? duk_to_int(ctx, 5) : 1;
	if (!g_skip_frame) al_draw_rectangle(x1, y1, x2, y2, color, thickness);
	return 0;
}

duk_ret_t
duk_Rectangle(duk_context* ctx)
{
	ALLEGRO_COLOR color;
	int           x, y, w, h;
	
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	w = duk_to_int(ctx, 2);
	h = duk_to_int(ctx, 3);
	color = duk_get_sphere_color(ctx, 4);
	if (!g_skip_frame) al_draw_filled_rectangle(x, y, x + w, y + h, color);
	return 0;
}
