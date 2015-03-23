#include "minisphere.h"
#include "api.h"
#include "color.h"

static duk_ret_t js_CreateColor         (duk_context* ctx);
static duk_ret_t js_BlendColors         (duk_context* ctx);
static duk_ret_t js_BlendColorsWeighted (duk_context* ctx);
static duk_ret_t js_Color_toString      (duk_context* ctx);

color_t
rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
	color_t color;

	color.r = r;
	color.g = g;
	color.b = b;
	color.alpha = alpha;
	return color;
}

ALLEGRO_COLOR
nativecolor(color_t color)
{
	return al_map_rgba(color.r, color.g, color.b, color.alpha);
}

color_t
blend_colors(color_t color1, color_t color2, float w1, float w2)
{
	color_t blend;
	float   sigma;
	
	sigma = w1 + w2;
	blend.r = fmin(fmax((color1.r * w1 + color2.r * w2) / sigma, 0), 255);
	blend.g = fmin(fmax((color1.g * w1 + color2.g * w2) / sigma, 0), 255);
	blend.b = fmin(fmax((color1.b * w1 + color2.b * w2) / sigma, 0), 255);
	blend.alpha = fmin(fmax((color1.alpha * w1 + color2.alpha * w2) / sigma, 0), 255);
	return blend;
}

void
init_color_api(void)
{
	register_api_func(g_duktape, NULL, "CreateColor", js_CreateColor);
	register_api_func(g_duktape, NULL, "BlendColors", js_BlendColors);
	register_api_func(g_duktape, NULL, "BlendColorsWeighted", js_BlendColorsWeighted);
}

color_t
duk_require_sphere_color(duk_context* ctx, duk_idx_t index)
{
	color_t     color;
	const char* type;
	
	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	if (!duk_get_prop_string(ctx, index, "\xFF" "sphere_type"))
		goto on_error;
	type = duk_get_string(ctx, -1); duk_pop(ctx);
	if (strcmp(type, "color") != 0) goto on_error;
	duk_get_prop_string(ctx, index, "red"); color.r = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "green"); color.g = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "blue"); color.b = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "alpha"); color.alpha = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	return color;

on_error:
	duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "Object is not a Sphere color");
}

void
duk_push_sphere_color(duk_context* ctx, color_t color)
{
	duk_push_object(ctx);
	duk_push_string(ctx, "color"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_c_function(ctx, js_Color_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_number(ctx, color.r); duk_put_prop_string(ctx, -2, "red");
	duk_push_number(ctx, color.g); duk_put_prop_string(ctx, -2, "green");
	duk_push_number(ctx, color.b); duk_put_prop_string(ctx, -2, "blue");
	duk_push_number(ctx, color.alpha); duk_put_prop_string(ctx, -2, "alpha");
}

static duk_ret_t
js_CreateColor(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int r = duk_require_int(ctx, 0);
	int g = duk_require_int(ctx, 1);
	int b = duk_require_int(ctx, 2);
	int alpha = n_args >= 4 ? duk_require_int(ctx, 3) : 255;

	r = fmin(fmax(r, 0), 255);
	g = fmin(fmax(g, 0), 255);
	b = fmin(fmax(b, 0), 255);
	alpha = fmin(fmax(alpha, 0), 255);
	duk_push_sphere_color(ctx, rgba(r, g, b, alpha));
	return 1;
}

static duk_ret_t
js_BlendColors(duk_context* ctx)
{
	color_t color1 = duk_require_sphere_color(ctx, 0);
	color_t color2 = duk_require_sphere_color(ctx, 1);
	
	duk_push_sphere_color(ctx, blend_colors(color1, color2, 1, 1));
	return 1;
}

static duk_ret_t
js_BlendColorsWeighted(duk_context* ctx)
{
	color_t color1 = duk_require_sphere_color(ctx, 0);
	color_t color2 = duk_require_sphere_color(ctx, 1);
	float w1 = duk_require_number(ctx, 2);
	float w2 = duk_require_number(ctx, 3);
	
	if (w1 < 0.0 || w2 < 0.0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BlendColorsWeighted(): Weights cannot be negative ({ w1: %f, w2: %f })", w1, w2);
	duk_push_sphere_color(ctx, blend_colors(color1, color2, w1, w2));
	return 1;
}

static duk_ret_t
js_Color_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object color]");
	return 1;
}
