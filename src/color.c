#include "minisphere.h"
#include "api.h"
#include "color.h"

static duk_ret_t js_BlendColors           (duk_context* ctx);
static duk_ret_t js_BlendColorsWeighted   (duk_context* ctx);
static duk_ret_t js_CreateColor           (duk_context* ctx);
static duk_ret_t js_new_Color             (duk_context* ctx);
static duk_ret_t js_Color_toString        (duk_context* ctx);
static duk_ret_t js_Color_clone           (duk_context* ctx);
static duk_ret_t js_CreateColorMatrix     (duk_context* ctx);
static duk_ret_t js_new_ColorMatrix       (duk_context* ctx);
static duk_ret_t js_ColorMatrix_toString  (duk_context* ctx);

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

colormatrix_t
colormatrix(uint8_t rn, uint8_t rr, uint8_t rg, uint8_t rb,
            uint8_t gn, uint8_t gr, uint8_t gg, uint8_t gb,
			uint8_t bn, uint8_t br, uint8_t bg, uint8_t bb)
{
	colormatrix_t matrix = {
		rn, rr, rg, rb,
		gn, gr, gg, gb,
		bn, br, bg, bb,
	};
	return matrix;
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

color_t
transform_pixel(color_t pixel, colormatrix_t matrix)
{
	return rgba(
		fmin(fmax(matrix.rn + ((matrix.rr * pixel.r + matrix.rg * pixel.g + matrix.rb * pixel.b) / 255), 0), 255),
		fmin(fmax(matrix.gn + ((matrix.gr * pixel.r + matrix.gg * pixel.g + matrix.gb * pixel.b) / 255), 0), 255),
		fmin(fmax(matrix.bn + ((matrix.br * pixel.r + matrix.bg * pixel.g + matrix.bb * pixel.b) / 255), 0), 255),
		pixel.alpha);
}

void
init_color_api(void)
{
	register_api_function(g_duk, NULL, "BlendColors", js_BlendColors);
	register_api_function(g_duk, NULL, "BlendColorsWeighted", js_BlendColorsWeighted);
	register_api_function(g_duk, NULL, "CreateColor", js_CreateColor);
	register_api_function(g_duk, NULL, "CreateColorMatrix", js_CreateColorMatrix);

	// register Color methods and properties
	register_api_ctor(g_duk, "Color", js_new_Color, NULL);
	register_api_function(g_duk, "Color", "toString", js_Color_toString);
	register_api_function(g_duk, "Color", "clone", js_Color_clone);
	
	// register ColorMatrix methods and properties
	register_api_ctor(g_duk, "ColorMatrix", js_new_ColorMatrix, NULL);
	register_api_function(g_duk, "ColorMatrix", "toString", js_ColorMatrix_toString);
}

void
duk_push_sphere_color(duk_context* ctx, color_t color)
{
	duk_push_global_object(ctx);
	duk_get_prop_string(ctx, -1, "Color");
	duk_push_number(ctx, color.r);
	duk_push_number(ctx, color.g);
	duk_push_number(ctx, color.b);
	duk_push_number(ctx, color.alpha);
	duk_new(ctx, 4);
	duk_remove(ctx, -2);
}

color_t
duk_require_sphere_color(duk_context* ctx, duk_idx_t index)
{
	color_t color;
	
	duk_require_sphere_obj(ctx, index, "Color");
	duk_get_prop_string(ctx, index, "red"); color.r = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "green"); color.g = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "blue"); color.b = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "alpha"); color.alpha = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	return color;
}

colormatrix_t
duk_require_sphere_colormatrix(duk_context* ctx, duk_idx_t index)
{
	colormatrix_t matrix;

	duk_require_sphere_obj(ctx, index, "ColorMatrix");
	duk_get_prop_string(ctx, index, "rn"); matrix.rn = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rr"); matrix.rr = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rg"); matrix.rg = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rb"); matrix.rb = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gn"); matrix.gn = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gr"); matrix.gr = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gg"); matrix.gg = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gb"); matrix.gb = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bn"); matrix.bn = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "br"); matrix.br = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bg"); matrix.bg = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bb"); matrix.bb = fmin(fmax(duk_get_number(ctx, -1), 0), 255); duk_pop(ctx);
	return matrix;
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
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "BlendColorsWeighted(): weights cannot be negative ({ w1: %f, w2: %f })", w1, w2);
	duk_push_sphere_color(ctx, blend_colors(color1, color2, w1, w2));
	return 1;
}

static duk_ret_t
js_CreateColor(duk_context* ctx)
{
	return js_new_Color(ctx);
}

static duk_ret_t
js_new_Color(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int r = duk_require_int(ctx, 0);
	int g = duk_require_int(ctx, 1);
	int b = duk_require_int(ctx, 2);
	int alpha = n_args >= 4 ? duk_require_int(ctx, 3) : 255;

	// clamp components to 8-bit [0-255]
	r = fmin(fmax(r, 0), 255);
	g = fmin(fmax(g, 0), 255);
	b = fmin(fmax(b, 0), 255);
	alpha = fmin(fmax(alpha, 0), 255);
	
	// construct a Color object
	duk_push_sphere_obj(ctx, "Color", NULL);
	duk_push_int(ctx, r); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, g); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, b); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, alpha); duk_put_prop_string(ctx, -2, "alpha");
	return 1;
}

static duk_ret_t
js_Color_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object color]");
	return 1;
}

static duk_ret_t
js_Color_clone(duk_context* ctx)
{
	color_t color;

	duk_push_this(ctx);
	color = duk_require_sphere_color(ctx, -1);
	duk_pop(ctx);
	duk_push_sphere_color(ctx, color);
	return 1;
}

static duk_ret_t
js_CreateColorMatrix(duk_context* ctx)
{
	return js_new_ColorMatrix(ctx);
}

static duk_ret_t
js_new_ColorMatrix(duk_context* ctx)
{
	int rn = duk_require_int(ctx, 0);
	int rr = duk_require_int(ctx, 1);
	int rg = duk_require_int(ctx, 2);
	int rb = duk_require_int(ctx, 3);
	int gn = duk_require_int(ctx, 4);
	int gr = duk_require_int(ctx, 5);
	int gg = duk_require_int(ctx, 6);
	int gb = duk_require_int(ctx, 7);
	int bn = duk_require_int(ctx, 8);
	int br = duk_require_int(ctx, 9);
	int bg = duk_require_int(ctx, 10);
	int bb = duk_require_int(ctx, 11);

	// clamp entries to 8-bit [0-255]
	rn = fmin(fmax(rn, 0), 255);
	rr = fmin(fmax(rr, 0), 255);
	rg = fmin(fmax(rg, 0), 255);
	rb = fmin(fmax(rb, 0), 255);
	gn = fmin(fmax(gn, 0), 255);
	gr = fmin(fmax(gr, 0), 255);
	gg = fmin(fmax(gg, 0), 255);
	gb = fmin(fmax(gb, 0), 255);
	bn = fmin(fmax(bn, 0), 255);
	br = fmin(fmax(br, 0), 255);
	bg = fmin(fmax(bg, 0), 255);
	bb = fmin(fmax(bb, 0), 255);

	// construct a ColorMatrix object
	duk_push_sphere_obj(ctx, "ColorMatrix", NULL);
	duk_push_int(ctx, rn); duk_put_prop_string(ctx, -2, "rn");
	duk_push_int(ctx, rr); duk_put_prop_string(ctx, -2, "rr");
	duk_push_int(ctx, rg); duk_put_prop_string(ctx, -2, "rg");
	duk_push_int(ctx, rb); duk_put_prop_string(ctx, -2, "rb");
	duk_push_int(ctx, gn); duk_put_prop_string(ctx, -2, "gn");
	duk_push_int(ctx, gr); duk_put_prop_string(ctx, -2, "gr");
	duk_push_int(ctx, gg); duk_put_prop_string(ctx, -2, "gg");
	duk_push_int(ctx, gb); duk_put_prop_string(ctx, -2, "gb");
	duk_push_int(ctx, bn); duk_put_prop_string(ctx, -2, "bn");
	duk_push_int(ctx, br); duk_put_prop_string(ctx, -2, "br");
	duk_push_int(ctx, bg); duk_put_prop_string(ctx, -2, "bg");
	duk_push_int(ctx, bb); duk_put_prop_string(ctx, -2, "bb");
	return 1;
}

static duk_ret_t
js_ColorMatrix_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object colormatrix]");
	return 1;
}
