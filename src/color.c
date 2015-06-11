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
static duk_ret_t js_ColorMatrix_apply     (duk_context* ctx);

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
colormatrix(int rn, int rr, int rg, int rb, int gn, int gr, int gg, int gb, int bn, int br, int bg, int bb)
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
	blend.r = (color1.r * w1 + color2.r * w2) / sigma;
	blend.g = (color1.g * w1 + color2.g * w2) / sigma;
	blend.b = (color1.b * w1 + color2.b * w2) / sigma;
	blend.alpha = (color1.alpha * w1 + color2.alpha * w2) / sigma;
	return blend;

	#undef INTERP
}

colormatrix_t
blend_color_matrices(colormatrix_t mat1, colormatrix_t mat2, int w1, int w2)
{
	colormatrix_t blend;
	int           sigma;

	sigma = w1 + w2;
	blend.rn = (mat1.rn * w1 + mat2.rn * w2) / sigma;
	blend.rr = (mat1.rr * w1 + mat2.rr * w2) / sigma;
	blend.rg = (mat1.rg * w1 + mat2.rg * w2) / sigma;
	blend.rb = (mat1.rb * w1 + mat2.rb * w2) / sigma;
	blend.gn = (mat1.gn * w1 + mat2.gn * w2) / sigma;
	blend.gr = (mat1.gr * w1 + mat2.gr * w2) / sigma;
	blend.gg = (mat1.gg * w1 + mat2.gg * w2) / sigma;
	blend.gb = (mat1.gb * w1 + mat2.gb * w2) / sigma;
	blend.bn = (mat1.bn * w1 + mat2.bn * w2) / sigma;
	blend.br = (mat1.br * w1 + mat2.br * w2) / sigma;
	blend.bg = (mat1.bg * w1 + mat2.bg * w2) / sigma;
	blend.bb = (mat1.bb * w1 + mat2.bb * w2) / sigma;
	return blend;
}

color_t
transform_pixel(color_t pixel, colormatrix_t matrix)
{
	int r, g, b;
	
	r = matrix.rn + (matrix.rr * pixel.r + matrix.rg * pixel.g + matrix.rb * pixel.b) / 255;
	g = matrix.gn + (matrix.gr * pixel.r + matrix.gg * pixel.g + matrix.gb * pixel.b) / 255;
	b = matrix.bn + (matrix.br * pixel.r + matrix.bg * pixel.g + matrix.bb * pixel.b) / 255;
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	return rgba(r, g, b, pixel.alpha);
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
	register_api_function(g_duk, "ColorMatrix", "apply", js_ColorMatrix_apply);
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
	int r, g, b;
	int alpha;
	
	duk_require_sphere_obj(ctx, index, "Color");
	duk_get_prop_string(ctx, index, "red"); r = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "green"); g = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "blue"); b = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "alpha"); alpha = duk_get_int(ctx, -1); duk_pop(ctx);
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
	return rgba(r, g, b, alpha);
}

colormatrix_t
duk_require_sphere_colormatrix(duk_context* ctx, duk_idx_t index)
{
	colormatrix_t matrix;

	duk_require_sphere_obj(ctx, index, "ColorMatrix");
	duk_get_prop_string(ctx, index, "rn"); matrix.rn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rr"); matrix.rr = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rg"); matrix.rg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "rb"); matrix.rb = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gn"); matrix.gn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gr"); matrix.gr = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gg"); matrix.gg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "gb"); matrix.gb = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bn"); matrix.bn = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "br"); matrix.br = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bg"); matrix.bg = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, index, "bb"); matrix.bb = duk_get_int(ctx, -1); duk_pop(ctx);
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
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;

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

static duk_ret_t
js_ColorMatrix_apply(duk_context* ctx)
{
	color_t color = duk_require_sphere_color(ctx, 0);
	
	colormatrix_t matrix;

	duk_push_this(ctx);
	matrix = duk_require_sphere_colormatrix(ctx, -1);
	duk_pop(ctx);
	duk_push_sphere_color(ctx, transform_pixel(color, matrix));
	return 1;
}
