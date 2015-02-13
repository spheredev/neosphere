#include "minisphere.h"
#include "api.h"

// Engine functions
static duk_ret_t duk_GetVersion(duk_context* ctx);
static duk_ret_t duk_GetVersionString(duk_context* ctx);
static duk_ret_t duk_GarbageCollect(duk_context* ctx);
static duk_ret_t duk_Abort(duk_context* ctx);
static duk_ret_t duk_EvaluateScript(duk_context* ctx);
static duk_ret_t duk_EvaluateSystemScript(duk_context* ctx);
static duk_ret_t duk_Exit(duk_context* ctx);
static duk_ret_t duk_GetFrameRate(duk_context* ctx);
static duk_ret_t duk_GetTime(duk_context* ctx);
static duk_ret_t duk_RequireScript(duk_context* ctx);
static duk_ret_t duk_RequireSystemScript(duk_context* ctx);
static duk_ret_t duk_SetFrameRate(duk_context* ctx);

// Logging functions
static duk_ret_t duk_OpenLog(duk_context* ctx);
static duk_ret_t duk_Log_finalize(duk_context* ctx);
static duk_ret_t duk_Log_beginBlock(duk_context* ctx);
static duk_ret_t duk_Log_endBlock(duk_context* ctx);
static duk_ret_t duk_Log_write(duk_context* ctx);

// Color management functions
static duk_ret_t duk_CreateColor(duk_context* ctx);
static duk_ret_t duk_BlendColors(duk_context* ctx);
static duk_ret_t duk_BlendColorsWeighted(duk_context* ctx);

// Font functions
static duk_ret_t duk_GetSystemFont(duk_context* ctx);
static duk_ret_t duk_LoadFont(duk_context* ctx);
static duk_ret_t duk_Font_finalize(duk_context* ctx);
static duk_ret_t duk_Font_drawText(duk_context* ctx);

// Image functions
static duk_ret_t duk_LoadImage(duk_context* ctx);
static duk_ret_t duk_GrabImage(duk_context* ctx);
static duk_ret_t duk_Image_finalize(duk_context* ctx);
static duk_ret_t duk_Image_blit(duk_context* ctx);

// Spriteset functions
static duk_ret_t duk_LoadSpriteset(duk_context* ctx);

// Graphics/rendering functions
static duk_ret_t duk_GetClippingRectangle(duk_context* ctx);
static duk_ret_t duk_GetScreenHeight(duk_context* ctx);
static duk_ret_t duk_GetScreenWidth(duk_context* ctx);
static duk_ret_t duk_SetClippingRectangle(duk_context* ctx);
static duk_ret_t duk_ApplyColorMask(duk_context* ctx);
static duk_ret_t duk_FlipScreen(duk_context* ctx);
static duk_ret_t duk_GradientRectangle(duk_context* ctx);
static duk_ret_t duk_OutlinedRectangle(duk_context* ctx);
static duk_ret_t duk_Rectangle(duk_context* ctx);

void
init_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "GetVersion", &duk_GetVersion);
	register_api_func(ctx, NULL, "GetVersionString", &duk_GetVersionString);
	register_api_func(ctx, NULL, "GarbageCollect", &duk_GarbageCollect);
	register_api_func(ctx, NULL, "Abort", &duk_Abort);
	register_api_func(ctx, NULL, "EvaluateScript", &duk_EvaluateScript);
	register_api_func(ctx, NULL, "EvaluateSystemScript", &duk_EvaluateSystemScript);
	register_api_func(ctx, NULL, "Exit", &duk_Exit);
	register_api_func(ctx, NULL, "GetFrameRate", &duk_GetFrameRate);
	register_api_func(ctx, NULL, "GetTime", &duk_GetTime);
	register_api_func(ctx, NULL, "RequireScript", &duk_RequireScript);
	register_api_func(ctx, NULL, "RequireSystemScript", &duk_RequireSystemScript);
	register_api_func(ctx, NULL, "SetFrameRate", &duk_SetFrameRate);
	register_api_func(ctx, NULL, "OpenLog", &duk_OpenLog);
	register_api_func(ctx, NULL, "CreateColor", &duk_CreateColor);
	register_api_func(ctx, NULL, "BlendColors", &duk_BlendColors);
	register_api_func(ctx, NULL, "BlendColorsWeighted", &duk_BlendColorsWeighted);
	register_api_func(ctx, NULL, "GetSystemFont", &duk_GetSystemFont);
	register_api_func(ctx, NULL, "LoadFont", &duk_LoadFont);
	register_api_func(ctx, NULL, "LoadImage", &duk_LoadImage);
	register_api_func(ctx, NULL, "GrabImage", &duk_GrabImage);
	register_api_func(ctx, NULL, "GetClippingRectangle", &duk_GetClippingRectangle);
	register_api_func(ctx, NULL, "GetScreenHeight", &duk_GetScreenHeight);
	register_api_func(ctx, NULL, "GetScreenWidth", &duk_GetScreenWidth);
	register_api_func(ctx, NULL, "SetClippingRectangle", &duk_SetClippingRectangle);
	register_api_func(ctx, NULL, "ApplyColorMask", &duk_ApplyColorMask);
	register_api_func(ctx, NULL, "FlipScreen", &duk_FlipScreen);
	register_api_func(ctx, NULL, "GradientRectangle", &duk_GradientRectangle);
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

void
duk_push_sphere_Font(duk_context* ctx, ALLEGRO_FONT* font)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, font); duk_put_prop_string(ctx, -2, "\xFF" "font_ptr");
	duk_push_c_function(ctx, &duk_Font_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &duk_Font_drawText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawText");
}

void
duk_push_sphere_Image(duk_context* ctx, ALLEGRO_BITMAP* bitmap, bool allow_free)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, bitmap); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_boolean(ctx, allow_free); duk_put_prop_string(ctx, -2, "\xFF" "allow_free");
	duk_push_c_function(ctx, &duk_Image_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &duk_Image_blit, DUK_VARARGS); duk_put_prop_string(ctx, -2, "blit");
	duk_push_string(ctx, "width"); duk_push_int(ctx, al_get_bitmap_width(bitmap));
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0 |
		DUK_DEFPROP_HAVE_VALUE |
		DUK_DEFPROP_HAVE_WRITABLE | 0);
	duk_push_string(ctx, "height"); duk_push_int(ctx, al_get_bitmap_height(bitmap));
	duk_def_prop(ctx, -3,
		DUK_DEFPROP_HAVE_CONFIGURABLE | 0
		| DUK_DEFPROP_HAVE_WRITABLE | 0
		| DUK_DEFPROP_HAVE_VALUE);
}

void
duk_push_sphere_Log(duk_context* ctx, ALLEGRO_FILE* file)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, file); duk_put_prop_string(ctx, -2, "\xFF" "file_ptr");
	duk_push_c_function(ctx, &duk_Log_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &duk_Log_beginBlock, DUK_VARARGS); duk_put_prop_string(ctx, -2, "beginBlock");
	duk_push_c_function(ctx, &duk_Log_endBlock, DUK_VARARGS); duk_put_prop_string(ctx, -2, "endBlock");
	duk_push_c_function(ctx, &duk_Log_write, DUK_VARARGS); duk_put_prop_string(ctx, -2, "write");
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
	duk_push_sprintf(ctx, "minisphere %s (API: sphere-%s)", ENGINE_VER, SPHERE_API_VER);
	return 1;
}

static duk_ret_t
duk_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
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
	duk_push_int(ctx, 60);
	return 1;
}

duk_ret_t
duk_GetTime(duk_context* ctx)
{
	clock_t c_ticks = clock();
	double ms = (double)c_ticks / CLOCKS_PER_SEC * 1000;
	duk_push_number(ctx, ms);
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
	return 0;
}

static duk_ret_t
duk_OpenLog(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);
	char* path = get_asset_path(filename, "logs", true);
	ALLEGRO_FILE* file = al_fopen(path, "a");
	free(path);
	if (file != NULL) {
		duk_push_sphere_Log(ctx, file);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "OpenLog(): Unable to open log file '%s'", filename);
	}
}

static duk_ret_t
duk_Log_finalize(duk_context* ctx)
{
	ALLEGRO_FILE* file;
	duk_get_prop_string(ctx, 0, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_fclose(file);
	return 0;
}

static duk_ret_t
duk_Log_beginBlock(duk_context* ctx)
{
	const char* name = duk_to_string(ctx, 0);
	ALLEGRO_FILE* file = NULL;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	// TODO: implement beginBlock for logs
	return 0;
}

static duk_ret_t
duk_Log_endBlock(duk_context* ctx)
{
	ALLEGRO_FILE* file = NULL;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	// TODO: implement endBlock for logs
	return 0;
}

static duk_ret_t
duk_Log_write(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	ALLEGRO_FILE* file = NULL;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "file_ptr"); file = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_fputs(file, text); al_fputc(file, '\n');
	return 0;
}

duk_ret_t
duk_CreateColor(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int red_val = duk_get_int(ctx, 0);
	int green_val = duk_get_int(ctx, 1);
	int blue_val = duk_get_int(ctx, 2);
	int alpha_val = n_args > 3 ? duk_get_int(ctx, 3) : 255;
	duk_push_object(ctx);
	duk_push_int(ctx, red_val); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, green_val); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, blue_val); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, alpha_val); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

duk_ret_t
duk_BlendColors(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	duk_int_t r1, g1, b1, a1;
	duk_int_t r2, g2, b2, a2;
	duk_get_prop_string(ctx, 0, "red"); r1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "green"); g1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "blue"); b1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "alpha"); a1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "red"); r2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "green"); g2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "blue"); b2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "alpha"); a2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_push_object(ctx);
	duk_push_int(ctx, (r1 + r2) / 2); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, (g1 + g2) / 2); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, (b1 + b2) / 2); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, (a1 + a2) / 2); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

duk_ret_t
duk_BlendColorsWeighted(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	duk_int_t r1, g1, b1, a1;
	duk_int_t r2, g2, b2, a2;
	duk_get_prop_string(ctx, 0, "red"); r1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "green"); g1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "blue"); b1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "alpha"); a1 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "red"); r2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "green"); g2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "blue"); b2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 1, "alpha"); a2 = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_double_t w1 = duk_get_number(ctx, 2);
	duk_double_t w2 = duk_get_number(ctx, 3);
	duk_double_t sigma = w1 + w2;
	duk_push_object(ctx);
	duk_push_int(ctx, (r1 * w1 + r2 * w2) / sigma); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, (g1 * w1 + g2 * w2) / sigma); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, (b1 * w1 + b2 * w2) / sigma); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, (a1 * w1 + a2 * w2) / sigma); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
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
duk_LoadFont(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);
	char* path = get_asset_path(filename, "fonts", false);
	ALLEGRO_FONT* font = al_load_font(path, 0, 0x0);
	free(path);
	if (font != NULL) {
		duk_push_sphere_Font(ctx, font);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "LoadFont(): Unable to load font file '%s'", filename);
	}
}

duk_ret_t
duk_LoadImage(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);
	char* path = get_asset_path(filename, "images", false);
	ALLEGRO_BITMAP* bitmap = al_load_bitmap(path);
	free(path);
	if (bitmap != NULL) {
		duk_push_sphere_Image(ctx, bitmap, true);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "LoadImage(): Unable to load image file '%s'", filename);
	}
}

duk_ret_t
duk_Font_finalize(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	duk_get_prop_string(ctx, 0, "\xFF" "font_ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_destroy_font(font);
	return 0;
}

duk_ret_t
duk_Font_drawText(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "font_ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	float x = duk_get_number(ctx, 0);
	float y = duk_get_number(ctx, 1);
	const char* text = duk_get_string(ctx, 2);
	al_draw_text(font, al_map_rgb(255, 255, 255), x, y, 0x0, text);
	return 0;
}

duk_ret_t
duk_GrabImage(duk_context* ctx)
{
	int x_res = al_get_display_width(g_display);
	int y_res = al_get_display_height(g_display);
	ALLEGRO_BITMAP* bitmap = al_create_bitmap(x_res, y_res);
	if (bitmap != NULL) {
		duk_push_sphere_Image(ctx, bitmap, true);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "GrabImage(): Unable to create new bitmap");
	}
}

duk_ret_t
duk_Image_finalize(duk_context* ctx)
{
	bool            allow_free;
	ALLEGRO_BITMAP* bitmap;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 0, "\xFF" "allow_free"); allow_free = duk_get_boolean(ctx, -1); duk_pop(ctx);
	if (allow_free) {
		al_destroy_bitmap(bitmap);
	}
	return 0;
}

duk_ret_t
duk_Image_blit(duk_context* ctx)
{
	float x = (float)duk_get_number(ctx, 0);
	float y = (float)duk_get_number(ctx, 1);
	ALLEGRO_BITMAP* bitmap;
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); bitmap = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	al_draw_bitmap(bitmap, x, y, 0x0);
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
	duk_push_int(ctx, (duk_int_t)al_get_display_height(g_display));
	return 1;
}

duk_ret_t
duk_GetScreenWidth(duk_context* ctx)
{
	duk_push_int(ctx, (duk_int_t)al_get_display_width(g_display));
	return 1;
}

duk_ret_t
duk_SetClippingRectangle(duk_context* ctx)
{
	float x = (float)duk_get_number(ctx, 0);
	float y = (float)duk_get_number(ctx, 1);
	float width = (float)duk_get_number(ctx, 2);
	float height = (float)duk_get_number(ctx, 3);
	al_set_clipping_rectangle(x, y, width, height);
	return 0;
}

duk_ret_t
duk_ApplyColorMask(duk_context* ctx)
{
	duk_int_t r, g, b, a;
	duk_get_prop_string(ctx, -1, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	float width = (float)al_get_display_width(g_display);
	float height = (float)al_get_display_height(g_display);
	al_draw_filled_rectangle(0, 0, width, height, al_map_rgba(r, g, b, a));
	return 0;
}

duk_ret_t
duk_FlipScreen(duk_context* ctx)
{
	ALLEGRO_EVENT event;
	ALLEGRO_TIMEOUT timeout;
	al_init_timeout(&timeout, 0.05);
	bool got_event = al_wait_for_event_until(g_events, &event, &timeout);
	if (got_event && event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
		duk_error(ctx, DUK_ERR_ERROR, "!exit");
	}
	al_flip_display();
	al_clear_to_color(al_map_rgb(0, 0, 0));
	return 0;
}

duk_ret_t
duk_GradientRectangle(duk_context* ctx)
{
	float x1 = (float)duk_get_number(ctx, 0);
	float y1 = (float)duk_get_number(ctx, 1);
	float x2 = x1 + (float)duk_get_number(ctx, 2);
	float y2 = y1 + (float)duk_get_number(ctx, 3);
	duk_int_t r, g, b, a;
	duk_get_prop_string(ctx, 4, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	ALLEGRO_COLOR color_ul = al_map_rgba(r, g, b, a);
	duk_get_prop_string(ctx, 5, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 5, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	ALLEGRO_COLOR color_ur = al_map_rgba(r, g, b, a);
	duk_get_prop_string(ctx, 6, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 6, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	ALLEGRO_COLOR color_lr = al_map_rgba(r, g, b, a);
	duk_get_prop_string(ctx, 7, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 7, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	ALLEGRO_COLOR color_ll = al_map_rgba(r, g, b, a);
	ALLEGRO_VERTEX verts[] = {
		{ x1, y1, 0, 0, 0, color_ul },
		{ x2, y1, 0, 0, 0, color_ur },
		{ x1, y2, 0, 0, 0, color_ll },
		{ x2, y2, 0, 0, 0, color_lr }
	};
	al_draw_prim(verts, NULL, NULL, 0, 4, ALLEGRO_PRIM_TRIANGLE_STRIP);
	return 0;
}

duk_ret_t
duk_OutlinedRectangle(duk_context* ctx)
{
	duk_idx_t n_args = duk_get_top(ctx);
	float x1 = (float)duk_get_number(ctx, 0) + 0.5;
	float y1 = (float)duk_get_number(ctx, 1) + 0.5;
	float x2 = x1 + (float)duk_get_number(ctx, 2) - 1;
	float y2 = y1 + (float)duk_get_number(ctx, 3) - 1;
	float thickness = n_args >= 6 ? (float)floor((double)duk_get_number(ctx, 5)) : 1;
	duk_int_t r, g, b, a;
	duk_get_prop_string(ctx, 4, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	al_draw_rectangle(x1, y1, x2, y2, al_map_rgba(r, g, b, a), thickness);
	return 0;
}

duk_ret_t
duk_Rectangle(duk_context* ctx)
{
	float x = (float)duk_get_number(ctx, 0);
	float y = (float)duk_get_number(ctx, 1);
	float width = (float)duk_get_number(ctx, 2);
	float height = (float)duk_get_number(ctx, 3);
	duk_int_t r, g, b, a;
	duk_get_prop_string(ctx, 4, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, 4, "alpha"); a = duk_get_int(ctx, -1); duk_pop(ctx);
	al_draw_filled_rectangle(x, y, x + width, y + height, al_map_rgba(r, g, b, a));
	return 0;
}
