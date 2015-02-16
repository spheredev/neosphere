#include "minisphere.h"
#include "api.h"
#include "color.h"

static duk_ret_t _js_CreateColor(duk_context* ctx);
static duk_ret_t _js_BlendColors(duk_context* ctx);
static duk_ret_t _js_BlendColorsWeighted(duk_context* ctx);

void
init_color_api(void)
{
	register_api_func(g_duktape, NULL, "CreateColor", &_js_CreateColor);
	register_api_func(g_duktape, NULL, "BlendColors", &_js_BlendColors);
	register_api_func(g_duktape, NULL, "BlendColorsWeighted", &_js_BlendColorsWeighted);
}

ALLEGRO_COLOR
duk_get_sphere_color(duk_context* ctx, duk_idx_t index)
{
	ALLEGRO_COLOR color;
	
	duk_get_prop_string(ctx, index, "red"); color.r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, index, "green"); color.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, index, "blue"); color.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, index, "alpha"); color.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	return color;
}

void
duk_push_sphere_color(duk_context* ctx, ALLEGRO_COLOR color)
{
	duk_push_object(ctx);
	duk_push_number(ctx, color.r * 255); duk_put_prop_string(ctx, -2, "red");
	duk_push_number(ctx, color.g * 255); duk_put_prop_string(ctx, -2, "green");
	duk_push_number(ctx, color.b * 255); duk_put_prop_string(ctx, -2, "blue");
	duk_push_number(ctx, color.a * 255); duk_put_prop_string(ctx, -2, "alpha");
}

static duk_ret_t
_js_CreateColor(duk_context* ctx)
{
	ALLEGRO_COLOR color;
	int           n_args;
	
	n_args = duk_get_top(ctx);
	color.r = duk_to_number(ctx, 0) / 255;
	color.g = duk_to_number(ctx, 1) / 255;
	color.b = duk_to_number(ctx, 2) / 255;
	color.a = n_args > 3 ? duk_to_number(ctx, 3) / 255 : 255;
	duk_push_sphere_color(ctx, color);
	return 1;
}

static duk_ret_t
_js_BlendColors(duk_context* ctx)
{
	ALLEGRO_COLOR color1, color2;
	ALLEGRO_COLOR result;
	
	color1 = duk_get_sphere_color(ctx, 0);
	color2 = duk_get_sphere_color(ctx, 1);
	result.r = (color1.r + color2.r) / 2;
	result.g = (color1.g + color2.g) / 2;
	result.b = (color1.b + color2.b) / 2;
	result.a = (color1.a + color2.a) / 2;
	duk_push_sphere_color(ctx, result);
	return 1;
}

static duk_ret_t
_js_BlendColorsWeighted(duk_context* ctx)
{
	ALLEGRO_COLOR color1, color2;
	ALLEGRO_COLOR result;
	float         sigma;
	float         w1, w2;

	color1 = duk_get_sphere_color(ctx, 0);
	color2 = duk_get_sphere_color(ctx, 1);
	w1 = duk_to_number(ctx, 2);
	w2 = duk_to_number(ctx, 3);
	sigma = w1 + w2;
	result.r = (color1.r * w1 + color2.r * w2) / sigma;
	result.g = (color1.g * w1 + color2.g * w2) / sigma;
	result.b = (color1.b * w1 + color2.b * w2) / sigma;
	result.a = (color1.a * w1 + color2.a * w2) / sigma;
	duk_push_sphere_color(ctx, result);
	return 1;
}
