#include "minisphere.h"
#include "color.h"

#include "api.h"

static duk_ret_t js_CreateColor          (duk_context* ctx);
static duk_ret_t js_Color_get_Color      (duk_context* ctx);
static duk_ret_t js_new_Color            (duk_context* ctx);
static duk_ret_t js_Color_mix            (duk_context* ctx);
static duk_ret_t js_Color_toString       (duk_context* ctx);
static duk_ret_t js_Color_clone          (duk_context* ctx);
static duk_ret_t js_Color_fade           (duk_context* ctx);
static duk_ret_t js_CreateColorMatrix    (duk_context* ctx);
static duk_ret_t js_new_ColorMatrix      (duk_context* ctx);
static duk_ret_t js_ColorMatrix_toString (duk_context* ctx);
static duk_ret_t js_ColorMatrix_apply    (duk_context* ctx);

static add_js_color_const (const char* name, color_t color);

ALLEGRO_COLOR
nativecolor(color_t color)
{
	return al_map_rgba(color.r, color.g, color.b, color.alpha);
}

color_t
color_new(uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
{
	color_t color;

	color.r = r;
	color.g = g;
	color.b = b;
	color.alpha = alpha;
	return color;
}

color_t
color_lerp(color_t color, color_t other, float w1, float w2)
{
	color_t blend;
	float   sigma;

	sigma = w1 + w2;
	blend.r = (color.r * w1 + other.r * w2) / sigma;
	blend.g = (color.g * w1 + other.g * w2) / sigma;
	blend.b = (color.b * w1 + other.b * w2) / sigma;
	blend.alpha = (color.alpha * w1 + other.alpha * w2) / sigma;
	return blend;
}

colormatrix_t
colormatrix_new(int rn, int rr, int rg, int rb, int gn, int gr, int gg, int gb, int bn, int br, int bg, int bb)
{
	colormatrix_t matrix = {
		rn, rr, rg, rb,
		gn, gr, gg, gb,
		bn, br, bg, bb,
	};
	return matrix;
}

colormatrix_t
colormatrix_lerp(colormatrix_t mat, colormatrix_t other, int w1, int w2)
{
	colormatrix_t blend;
	int           sigma;

	sigma = w1 + w2;
	blend.rn = (mat.rn * w1 + other.rn * w2) / sigma;
	blend.rr = (mat.rr * w1 + other.rr * w2) / sigma;
	blend.rg = (mat.rg * w1 + other.rg * w2) / sigma;
	blend.rb = (mat.rb * w1 + other.rb * w2) / sigma;
	blend.gn = (mat.gn * w1 + other.gn * w2) / sigma;
	blend.gr = (mat.gr * w1 + other.gr * w2) / sigma;
	blend.gg = (mat.gg * w1 + other.gg * w2) / sigma;
	blend.gb = (mat.gb * w1 + other.gb * w2) / sigma;
	blend.bn = (mat.bn * w1 + other.bn * w2) / sigma;
	blend.br = (mat.br * w1 + other.br * w2) / sigma;
	blend.bg = (mat.bg * w1 + other.bg * w2) / sigma;
	blend.bb = (mat.bb * w1 + other.bb * w2) / sigma;
	return blend;
}

color_t
color_transform(color_t color, colormatrix_t mat)
{
	int r, g, b;
	
	r = mat.rn + (mat.rr * color.r + mat.rg * color.g + mat.rb * color.b) / 255;
	g = mat.gn + (mat.gr * color.r + mat.gg * color.g + mat.gb * color.b) / 255;
	b = mat.bn + (mat.br * color.r + mat.bg * color.g + mat.bb * color.b) / 255;
	r = r < 0 ? 0 : r > 255 ? 255 : r;
	g = g < 0 ? 0 : g > 255 ? 255 : g;
	b = b < 0 ? 0 : b > 255 ? 255 : b;
	return color_new(r, g, b, color.alpha);
}

void
init_color_api(void)
{
	api_register_method(g_duk, NULL, "BlendColors", js_Color_mix);
	api_register_method(g_duk, NULL, "BlendColorsWeighted", js_Color_mix);
	api_register_method(g_duk, NULL, "CreateColor", js_CreateColor);
	api_register_method(g_duk, NULL, "CreateColorMatrix", js_CreateColorMatrix);

	api_register_ctor(g_duk, "Color", js_new_Color, NULL);
	api_register_static_func(g_duk, "Color", "mix", js_Color_mix);
	api_register_method(g_duk, "Color", "toString", js_Color_toString);
	api_register_method(g_duk, "Color", "clone", js_Color_clone);
	api_register_method(g_duk, "Color", "fade", js_Color_fade);

	api_register_ctor(g_duk, "ColorMatrix", js_new_ColorMatrix, NULL);
	api_register_method(g_duk, "ColorMatrix", "toString", js_ColorMatrix_toString);
	api_register_method(g_duk, "ColorMatrix", "apply", js_ColorMatrix_apply);

	add_js_color_const("AliceBlue", color_new(240, 248, 255, 255));
	add_js_color_const("AntiqueWhite", color_new(250, 235, 215, 255));
	add_js_color_const("Aqua", color_new(0, 255, 255, 255));
	add_js_color_const("Aquamarine", color_new(127, 255, 212, 255));
	add_js_color_const("Azure", color_new(240, 255, 255, 255));
	add_js_color_const("Beige", color_new(245, 245, 220, 255));
	add_js_color_const("Bisque", color_new(255, 228, 196, 255));
	add_js_color_const("Black", color_new(0, 0, 0, 255));
	add_js_color_const("BlanchedAlmond", color_new(255, 235, 205, 255));
	add_js_color_const("Blue", color_new(0, 0, 255, 255));
	add_js_color_const("BlueViolet", color_new(138, 43, 226, 255));
	add_js_color_const("Brown", color_new(165, 42, 42, 255));
	add_js_color_const("BurlyWood", color_new(222, 184, 135, 255));
	add_js_color_const("CadetBlue", color_new(95, 158, 160, 255));
	add_js_color_const("Chartreuse", color_new(127, 255, 0, 255));
	add_js_color_const("Chocolate", color_new(210, 105, 30, 255));
	add_js_color_const("Coral", color_new(255, 127, 80, 255));
	add_js_color_const("CornflowerBlue", color_new(100, 149, 237, 255));
	add_js_color_const("CornSilk", color_new(255, 248, 220, 255));
	add_js_color_const("Crimson", color_new(220, 20, 60, 255));
	add_js_color_const("Cyan", color_new(0, 255, 255, 255));
	add_js_color_const("DarkBlue", color_new(0, 0, 139, 255));
	add_js_color_const("DarkCyan", color_new(0, 139, 139, 255));
	add_js_color_const("DarkGoldenrod", color_new(184, 134, 11, 255));
	add_js_color_const("DarkGray", color_new(169, 169, 169, 255));
	add_js_color_const("DarkGreen", color_new(0, 100, 0, 255));
	add_js_color_const("DarkKhaki", color_new(189, 183, 107, 255));
	add_js_color_const("DarkMagenta", color_new(139, 0, 139, 255));
	add_js_color_const("DarkOliveGreen", color_new(85, 107, 47, 255));
	add_js_color_const("DarkOrange", color_new(255, 140, 0, 255));
	add_js_color_const("DarkOrchid", color_new(153, 50, 204, 255));
	add_js_color_const("DarkRed", color_new(139, 0, 0, 255));
	add_js_color_const("DarkSalmon", color_new(233, 150, 122, 255));
	add_js_color_const("DarkSeaGreen", color_new(143, 188, 143, 255));
	add_js_color_const("DarkSlateBlue", color_new(72, 61, 139, 255));
	add_js_color_const("DarkSlateGray", color_new(47, 79, 79, 255));
	add_js_color_const("DarkTurquoise", color_new(0, 206, 209, 255));
	add_js_color_const("DarkViolet", color_new(148, 0, 211, 255));
	add_js_color_const("DeepPink", color_new(255, 20, 147, 255));
	add_js_color_const("DeepSkyBlue", color_new(0, 191, 255, 255));
	add_js_color_const("DimGray", color_new(105, 105, 105, 255));
	add_js_color_const("DodgerBlue", color_new(30, 144, 255, 255));
	add_js_color_const("FireBrick", color_new(178, 34, 34, 255));
	add_js_color_const("FloralWhite", color_new(255, 250, 240, 255));
	add_js_color_const("ForestGreen", color_new(34, 139, 34, 255));
	add_js_color_const("Fuchsia", color_new(255, 0, 255, 255));
	add_js_color_const("Gainsboro", color_new(220, 220, 220, 255));
	add_js_color_const("GhostWhite", color_new(248, 248, 255, 255));
	add_js_color_const("Gold", color_new(255, 215, 0, 255));
	add_js_color_const("Goldenrod", color_new(218, 165, 32, 255));
	add_js_color_const("Gray", color_new(128, 128, 128, 255));
	add_js_color_const("Green", color_new(0, 128, 0, 255));
	add_js_color_const("GreenYellow", color_new(173, 255, 47, 255));
	add_js_color_const("Honeydew", color_new(240, 255, 240, 255));
	add_js_color_const("HotPink", color_new(255, 105, 180, 255));
	add_js_color_const("IndianRed", color_new(205, 92, 92, 255));
	add_js_color_const("Indigo", color_new(75, 0, 130, 255));
	add_js_color_const("Ivory", color_new(255, 255, 240, 255));
	add_js_color_const("Khaki", color_new(240, 230, 140, 255));
	add_js_color_const("Lavender", color_new(230, 230, 250, 255));
	add_js_color_const("LavenderBlush", color_new(255, 240, 245, 255));
	add_js_color_const("LawnGreen", color_new(124, 252, 0, 255));
	add_js_color_const("LemonChiffon", color_new(255, 250, 205, 255));
	add_js_color_const("LightBlue", color_new(173, 216, 230, 255));
	add_js_color_const("LightCoral", color_new(240, 128, 128, 255));
	add_js_color_const("LightCyan", color_new(224, 255, 255, 255));
	add_js_color_const("LightGoldenrodYellow", color_new(250, 250, 210, 255));
	add_js_color_const("LightGray", color_new(211, 211, 211, 255));
	add_js_color_const("LightGreen", color_new(144, 238, 144, 255));
	add_js_color_const("LightPink", color_new(255, 182, 193, 255));
	add_js_color_const("LightSalmon", color_new(255, 160, 122, 255));
	add_js_color_const("LightSeaGreen", color_new(32, 178, 170, 255));
	add_js_color_const("LightSkyBlue", color_new(135, 206, 250, 255));
	add_js_color_const("LightSlateGray", color_new(119, 136, 153, 255));
	add_js_color_const("LightSteelBlue", color_new(176, 196, 222, 255));
	add_js_color_const("LightYellow", color_new(255, 255, 224, 255));
	add_js_color_const("Lime", color_new(0, 255, 0, 255));
	add_js_color_const("LimeGreen", color_new(50, 205, 50, 255));
	add_js_color_const("Linen", color_new(250, 240, 230, 255));
	add_js_color_const("Magenta", color_new(255, 0, 255, 255));
	add_js_color_const("Maroon", color_new(128, 0, 0, 255));
	add_js_color_const("MediumAquamarine", color_new(102, 205, 170, 255));
	add_js_color_const("MediumBlue", color_new(0, 0, 205, 255));
	add_js_color_const("MediumOrchid", color_new(186, 85, 211, 255));
	add_js_color_const("MediumPurple", color_new(147, 112, 219, 255));
	add_js_color_const("MediumSeaGreen", color_new(60, 179, 113, 255));
	add_js_color_const("MediumSlateBlue", color_new(123, 104, 238, 255));
	add_js_color_const("MediumSpringGreen", color_new(0, 250, 154, 255));
	add_js_color_const("MediumTurquoise", color_new(72, 209, 204, 255));
	add_js_color_const("MediumVioletRed", color_new(199, 21, 133, 255));
	add_js_color_const("MidnightBlue", color_new(25, 25, 112, 255));
	add_js_color_const("MintCream", color_new(245, 255, 250, 255));
	add_js_color_const("MistyRose", color_new(255, 228, 225, 255));
	add_js_color_const("Moccasin", color_new(255, 228, 181, 255));
	add_js_color_const("NavajoWhite", color_new(255, 222, 173, 255));
	add_js_color_const("Navy", color_new(0, 0, 128, 255));
	add_js_color_const("OldLace", color_new(253, 245, 230, 255));
	add_js_color_const("Olive", color_new(128, 128, 0, 255));
	add_js_color_const("OliveDrab", color_new(107, 142, 35, 255));
	add_js_color_const("Orange", color_new(255, 165, 0, 255));
	add_js_color_const("OrangeRed", color_new(255, 69, 0, 255));
	add_js_color_const("Orchid", color_new(218, 112, 214, 255));
	add_js_color_const("PaleGoldenrod", color_new(238, 232, 170, 255));
	add_js_color_const("PaleGreen", color_new(152, 251, 152, 255));
	add_js_color_const("PaleTurquoise", color_new(175, 238, 238, 255));
	add_js_color_const("PaleVioletRed", color_new(219, 112, 147, 255));
	add_js_color_const("PapayaWhip", color_new(225, 239, 213, 255));
	add_js_color_const("PeachPuff", color_new(255, 218, 185, 255));
	add_js_color_const("Peru", color_new(205, 133, 63, 255));
	add_js_color_const("Pink", color_new(255, 192, 203, 255));
	add_js_color_const("Plum", color_new(221, 160, 221, 255));
	add_js_color_const("PowderBlue", color_new(176, 224, 230, 255));
	add_js_color_const("Purple", color_new(128, 0, 128, 255));
	add_js_color_const("Red", color_new(255, 0, 0, 255));
	add_js_color_const("RosyBrown", color_new(188, 143, 143, 255));
	add_js_color_const("RoyalBlue", color_new(65, 105, 225, 255));
	add_js_color_const("SaddleBrown", color_new(139, 69, 19, 255));
	add_js_color_const("Salmon", color_new(250, 128, 114, 255));
	add_js_color_const("SandyBrown", color_new(244, 164, 96, 255));
	add_js_color_const("SeaGreen", color_new(46, 139, 87, 255));
	add_js_color_const("Seashell", color_new(255, 245, 238, 255));
	add_js_color_const("Sienna", color_new(160, 82, 45, 255));
	add_js_color_const("Silver", color_new(192, 192, 192, 255));
	add_js_color_const("SkyBlue", color_new(135, 206, 235, 255));
	add_js_color_const("SlateBlue", color_new(106, 90, 205, 255));
	add_js_color_const("SlateGray", color_new(112, 128, 144, 255));
	add_js_color_const("Snow", color_new(255, 250, 250, 255));
	add_js_color_const("SpringGreen", color_new(0, 255, 127, 255));
	add_js_color_const("SteelBlue", color_new(70, 130, 180, 255));
	add_js_color_const("Tan", color_new(210, 180, 140, 255));
	add_js_color_const("Teal", color_new(0, 128, 128, 255));
	add_js_color_const("Thistle", color_new(216, 191, 216, 255));
	add_js_color_const("Tomato", color_new(255, 99, 71, 255));
	add_js_color_const("Transparent", color_new(0, 0, 0, 0));
	add_js_color_const("Turquoise", color_new(64, 224, 208, 255));
	add_js_color_const("Violet", color_new(238, 130, 238, 255));
	add_js_color_const("Wheat", color_new(245, 222, 179, 255));
	add_js_color_const("White", color_new(255, 255, 255, 255));
	add_js_color_const("WhiteSmoke", color_new(245, 245, 245, 255));
	add_js_color_const("Yellow", color_new(255, 255, 0, 255));
	add_js_color_const("YellowGreen", color_new(154, 205, 50, 255));
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
	return color_new(r, g, b, alpha);
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

static
add_js_color_const(const char* name, color_t color)
{
	duk_get_global_string(g_duk, "Color");
	duk_push_string(g_duk, name);
	duk_push_c_function(g_duk, js_Color_get_Color, DUK_VARARGS);
	duk_push_sphere_color(g_duk, color);
	duk_put_prop_string(g_duk, -2, "\xFF" "color");
	duk_def_prop(g_duk, -3, DUK_DEFPROP_HAVE_GETTER
		| DUK_DEFPROP_CLEAR_ENUMERABLE
		| DUK_DEFPROP_SET_CONFIGURABLE);
	duk_pop(g_duk);
}

static duk_ret_t
js_Color_get_Color(duk_context* ctx)
{
	color_t color;
	
	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color");
	color = duk_require_sphere_color(ctx, -1);
	duk_push_sphere_color(ctx, color);
	return 1;
}

static duk_ret_t
js_Color_mix(duk_context* ctx)
{
	color_t color1;
	color_t color2;
	int     num_args;
	float   w1 = 1.0;
	float   w2 = 1.0;

	num_args = duk_get_top(ctx);
	color1 = duk_require_sphere_color(ctx, 0);
	color2 = duk_require_sphere_color(ctx, 1);
	if (num_args > 2) {
		w1 = duk_require_number(ctx, 2);
		w2 = duk_require_number(ctx, 3);
	}
	
	if (w1 < 0.0 || w2 < 0.0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "weight cannot be negative", w1, w2);
	
	duk_push_sphere_color(ctx, color_lerp(color1, color2, w1, w2));
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

	duk_push_sphere_color(ctx, color);
	return 1;
}

static duk_ret_t
js_Color_fade(duk_context* ctx)
{
	int     alpha;
	color_t color;

	duk_push_this(ctx);
	color = duk_require_sphere_color(ctx, -1);
	alpha = duk_require_int(ctx, 0);

	alpha = alpha < 0 ? 0 : alpha > 255 ? 255 : alpha;
	color.alpha = color.alpha * alpha / 255;
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
	duk_push_sphere_color(ctx, color_transform(color, matrix));
	return 1;
}
