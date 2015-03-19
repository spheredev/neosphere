#include "minisphere.h"
#include "api.h"
#include "color.h"

static duk_ret_t js_CreateColor         (duk_context* ctx);
static duk_ret_t js_BlendColors         (duk_context* ctx);
static duk_ret_t js_BlendColorsWeighted (duk_context* ctx);
static duk_ret_t js_Color_toString      (duk_context* ctx);

ALLEGRO_COLOR
blend_colors(ALLEGRO_COLOR color1, ALLEGRO_COLOR color2, float w1, float w2)
{
	ALLEGRO_COLOR blend;
	float         sigma;
	
	sigma = w1 + w2;
	blend.r = (color1.r * w1 + color2.r * w2) / sigma;
	blend.g = (color1.g * w1 + color2.g * w2) / sigma;
	blend.b = (color1.b * w1 + color2.b * w2) / sigma;
	blend.a = (color1.a * w1 + color2.a * w2) / sigma;
	return blend;
}

void
init_color_api(void)
{
	register_api_func(g_duktape, NULL, "CreateColor", js_CreateColor);
	register_api_func(g_duktape, NULL, "BlendColors", js_BlendColors);
	register_api_func(g_duktape, NULL, "BlendColorsWeighted", js_BlendColorsWeighted);
}

ALLEGRO_COLOR
duk_require_sphere_color(duk_context* ctx, duk_idx_t index)
{
	ALLEGRO_COLOR color;
	const char*   type;
	
	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	if (!duk_get_prop_string(ctx, index, "\xFF" "sphere_type"))
		goto on_error;
	type = duk_get_string(ctx, -1); duk_pop(ctx);
	if (strcmp(type, "color") != 0) goto on_error;
	duk_get_prop_string(ctx, index, "red"); color.r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, index, "green"); color.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, index, "blue"); color.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, index, "alpha"); color.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	return color;

on_error:
	duk_error(ctx, DUK_ERR_TYPE_ERROR, "Not a Sphere color object");
}

void
duk_push_sphere_color(duk_context* ctx, ALLEGRO_COLOR color)
{
	duk_push_object(ctx);
	duk_push_string(ctx, "color"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_c_function(ctx, js_Color_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_number(ctx, color.r * 255); duk_put_prop_string(ctx, -2, "red");
	duk_push_number(ctx, color.g * 255); duk_put_prop_string(ctx, -2, "green");
	duk_push_number(ctx, color.b * 255); duk_put_prop_string(ctx, -2, "blue");
	duk_push_number(ctx, color.a * 255); duk_put_prop_string(ctx, -2, "alpha");
}

static duk_ret_t
js_CreateColor(duk_context* ctx)
{
	ALLEGRO_COLOR color;
	int           n_args;
	
	n_args = duk_get_top(ctx);
	color.r = duk_to_int(ctx, 0) / 255.0; color.r = fmin(fmax(color.r, 0.0), 1.0);
	color.g = duk_to_int(ctx, 1) / 255.0; color.g = fmin(fmax(color.g, 0.0), 1.0);
	color.b = duk_to_int(ctx, 2) / 255.0; color.b = fmin(fmax(color.b, 0.0), 1.0);
	color.a = n_args > 3 ? duk_to_int(ctx, 3) / 255.0 : 1.0; color.a = fmin(fmax(color.a, 0.0), 1.0);
	duk_push_sphere_color(ctx, color);
	return 1;
}

static duk_ret_t
js_BlendColors(duk_context* ctx)
{
	ALLEGRO_COLOR color1 = duk_require_sphere_color(ctx, 0);
	ALLEGRO_COLOR color2 = duk_require_sphere_color(ctx, 1);
	
	duk_push_sphere_color(ctx, blend_colors(color1, color2, 1, 1));
	return 1;
}

static duk_ret_t
js_BlendColorsWeighted(duk_context* ctx)
{
	ALLEGRO_COLOR color1 = duk_require_sphere_color(ctx, 0);
	ALLEGRO_COLOR color2 = duk_require_sphere_color(ctx, 1);
	float w1 = duk_require_number(ctx, 2);
	float w2 = duk_require_number(ctx, 3);
	
	if (w1 < 0.0 || w2 < 0.0)
		duk_error(ctx, DUK_ERR_RANGE_ERROR, "BlendColorsWeighted(): Weights cannot be negative ({ w1: %f, w2: %f })", w1, w2);
	duk_push_sphere_color(ctx, blend_colors(color1, color2, w1, w2));
	return 1;
}

static duk_ret_t
js_Color_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object color]");
	return 1;
}
