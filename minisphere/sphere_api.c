#include "minisphere.h"

static void reg_script_func(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);

static duk_ret_t duk_GetVersion(duk_context* ctx);
static duk_ret_t duk_GetVersionString(duk_context* ctx);
static duk_ret_t duk_Abort(duk_context* ctx);
static duk_ret_t duk_Exit(duk_context* ctx);
static duk_ret_t duk_GarbageCollect(duk_context* ctx);

static duk_ret_t duk_CreateColor(duk_context* ctx);
static duk_ret_t duk_BlendColors(duk_context* ctx);
static duk_ret_t duk_BlendColorsWeighted(duk_context* ctx);

static duk_ret_t duk_GetClippingRectangle(duk_context* ctx);
static duk_ret_t duk_GetScreenHeight(duk_context* ctx);
static duk_ret_t duk_GetScreenWidth(duk_context* ctx);
static duk_ret_t duk_SetClippingRectangle(duk_context* ctx);
static duk_ret_t duk_ApplyColorMask(duk_context* ctx);
static duk_ret_t duk_FlipScreen(duk_context* ctx);
static duk_ret_t duk_OutlinedRectangle(duk_context* ctx);
static duk_ret_t duk_Rectangle(duk_context* ctx);

void
init_sphere_api(duk_context* ctx)
{
	reg_script_func(ctx, NULL, "GetVersion", &duk_GetVersion);
	reg_script_func(ctx, NULL, "GetVersionString", &duk_GetVersionString);
	reg_script_func(ctx, NULL, "Abort", &duk_Abort);
	reg_script_func(ctx, NULL, "Exit", &duk_Exit);
	reg_script_func(ctx, NULL, "GarbageCollect", &duk_GarbageCollect);
	reg_script_func(ctx, NULL, "CreateColor", &duk_CreateColor);
	reg_script_func(ctx, NULL, "BlendColors", &duk_BlendColors);
	reg_script_func(ctx, NULL, "BlendColorsWeighted", &duk_BlendColorsWeighted);
	reg_script_func(ctx, NULL, "GetClippingRectangle", &duk_GetClippingRectangle);
	reg_script_func(ctx, NULL, "GetScreenHeight", &duk_GetScreenHeight);
	reg_script_func(ctx, NULL, "GetScreenWidth", &duk_GetScreenWidth);
	reg_script_func(ctx, NULL, "SetClippingRectangle", &duk_SetClippingRectangle);
	reg_script_func(ctx, NULL, "ApplyColorMask", &duk_ApplyColorMask);
	reg_script_func(ctx, NULL, "FlipScreen", &duk_FlipScreen);
	reg_script_func(ctx, NULL, "OutlinedRectangle", &duk_OutlinedRectangle);
	reg_script_func(ctx, NULL, "Rectangle", &duk_Rectangle);
}

void
reg_script_func(duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn)
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

duk_ret_t
duk_GetVersion(duk_context* ctx)
{
	duk_push_number(ctx, 1.5);
	return 1;
}

duk_ret_t
duk_GetVersionString(duk_context* ctx)
{
	duk_push_sprintf(ctx, "minisphere %s (API: sphere-%s)", ENGINE_VER, SPHERE_API_VER);
	return 1;
}

duk_ret_t
duk_GarbageCollect(duk_context* ctx)
{
	duk_gc(ctx, 0x0);
	duk_gc(ctx, 0x0);
	return 0;
}

duk_ret_t
duk_Abort(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	const char* err_msg = n_args > 0 ? duk_to_string(ctx, 0) : "Abort() called by script";
	duk_error(ctx, DUK_ERR_ERROR, "%s ", err_msg);
	return 0;
}

duk_ret_t
duk_Exit(duk_context* ctx)
{
	duk_error(ctx, DUK_ERR_ERROR, "!exit");
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
duk_GetClippingRectangle(duk_context* ctx)
{
	int x, y, width, height;
	al_get_clipping_rectangle(&x, &y, &width, &height);
	duk_push_object(ctx);
	duk_push_int(ctx, x);
	duk_put_prop_string(ctx, -2, "x");
	duk_push_int(ctx, y);
	duk_put_prop_string(ctx, -2, "y");
	duk_push_int(ctx, width);
	duk_put_prop_string(ctx, -2, "width");
	duk_push_int(ctx, height);
	duk_put_prop_string(ctx, -2, "height");
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
