#include "minisphere.h"
#include "color.h"

#include "api.h"

struct x11_color
{
	const char* name;
	uint8_t     r;
	uint8_t     g;
	uint8_t     b;
	uint8_t     a;
};

static const struct x11_color X11_COLOR[] =
{
	{ "AliceBlue", 240, 248, 255, 255 },
	{ "AntiqueWhite", 250, 235, 215, 255 },
	{ "Aqua", 0, 255, 255, 255 },
	{ "Aquamarine", 127, 255, 212, 255 },
	{ "Azure", 240, 255, 255, 255 },
	{ "Beige", 245, 245, 220, 255 },
	{ "Bisque", 255, 228, 196, 255 },
	{ "Black", 0, 0, 0, 255 },
	{ "BlanchedAlmond", 255, 235, 205, 255 },
	{ "Blue", 0, 0, 255, 255 },
	{ "BlueViolet", 138, 43, 226, 255 },
	{ "Brown", 165, 42, 42, 255 },
	{ "BurlyWood", 222, 184, 135, 255 },
	{ "CadetBlue", 95, 158, 160, 255 },
	{ "Chartreuse", 127, 255, 0, 255 },
	{ "Chocolate", 210, 105, 30, 255 },
	{ "Coral", 255, 127, 80, 255 },
	{ "CornflowerBlue", 100, 149, 237, 255 },
	{ "Cornsilk", 255, 248, 220, 255 },
	{ "Crimson", 220, 20, 60, 255 },
	{ "Cyan", 0, 255, 255, 255 },
	{ "DarkBlue", 0, 0, 139, 255 },
	{ "DarkCyan", 0, 139, 139, 255 },
	{ "DarkGoldenrod", 184, 134, 11, 255 },
	{ "DarkGray", 169, 169, 169, 255 },
	{ "DarkGreen", 0, 100, 0, 255 },
	{ "DarkKhaki", 189, 183, 107, 255 },
	{ "DarkMagenta", 139, 0, 139, 255 },
	{ "DarkOliveGreen", 85, 107, 47, 255 },
	{ "DarkOrange", 255, 140, 0, 255 },
	{ "DarkOrchid", 153, 50, 204, 255 },
	{ "DarkRed", 139, 0, 0, 255 },
	{ "DarkSalmon", 233, 150, 122, 255 },
	{ "DarkSeaGreen", 143, 188, 143, 255 },
	{ "DarkSlateBlue", 72, 61, 139, 255 },
	{ "DarkSlateGray", 47, 79, 79, 255 },
	{ "DarkTurquoise", 0, 206, 209, 255 },
	{ "DarkViolet", 148, 0, 211, 255 },
	{ "DeepPink", 255, 20, 147, 255 },
	{ "DeepSkyBlue", 0, 191, 255, 255 },
	{ "DimGray", 105, 105, 105, 255 },
	{ "DodgerBlue", 30, 144, 255, 255 },
	{ "FireBrick", 178, 34, 34, 255 },
	{ "FloralWhite", 255, 250, 240, 255 },
	{ "ForestGreen", 34, 139, 34, 255 },
	{ "Fuchsia", 255, 0, 255, 255 },
	{ "Gainsboro", 220, 220, 220, 255 },
	{ "GhostWhite", 248, 248, 255, 255 },
	{ "Gold", 255, 215, 0, 255 },
	{ "Goldenrod", 218, 165, 32, 255 },
	{ "Gray", 128, 128, 128, 255 },
	{ "Green", 0, 128, 0, 255 },
	{ "GreenYellow", 173, 255, 47, 255 },
	{ "Honeydew", 240, 255, 240, 255 },
	{ "HotPink", 255, 105, 180, 255 },
	{ "IndianRed", 205, 92, 92, 255 },
	{ "Indigo", 75, 0, 130, 255 },
	{ "Ivory", 255, 255, 240, 255 },
	{ "Khaki", 240, 230, 140, 255 },
	{ "Lavender", 230, 230, 250, 255 },
	{ "LavenderBlush", 255, 240, 245, 255 },
	{ "LawnGreen", 124, 252, 0, 255 },
	{ "LemonChiffon", 255, 250, 205, 255 },
	{ "LightBlue", 173, 216, 230, 255 },
	{ "LightCoral", 240, 128, 128, 255 },
	{ "LightCyan", 224, 255, 255, 255 },
	{ "LightGoldenrodYellow", 250, 250, 210, 255 },
	{ "LightGray", 211, 211, 211, 255 },
	{ "LightGreen", 144, 238, 144, 255 },
	{ "LightPink", 255, 182, 193, 255 },
	{ "LightSalmon", 255, 160, 122, 255 },
	{ "LightSeaGreen", 32, 178, 170, 255 },
	{ "LightSkyBlue", 135, 206, 250, 255 },
	{ "LightSlateGray", 119, 136, 153, 255 },
	{ "LightSteelBlue", 176, 196, 222, 255 },
	{ "LightYellow", 255, 255, 224, 255 },
	{ "Lime", 0, 255, 0, 255 },
	{ "LimeGreen", 50, 205, 50, 255 },
	{ "Linen", 250, 240, 230, 255 },
	{ "Magenta", 255, 0, 255, 255 },
	{ "Maroon", 128, 0, 0, 255 },
	{ "MediumAquamarine", 102, 205, 170, 255 },
	{ "MediumBlue", 0, 0, 205, 255 },
	{ "MediumOrchid", 186, 85, 211, 255 },
	{ "MediumPurple", 147, 112, 219, 255 },
	{ "MediumSeaGreen", 60, 179, 113, 255 },
	{ "MediumSlateBlue", 123, 104, 238, 255 },
	{ "MediumSpringGreen", 0, 250, 154, 255 },
	{ "MediumTurquoise", 72, 209, 204, 255 },
	{ "MediumVioletRed", 199, 21, 133, 255 },
	{ "MidnightBlue", 25, 25, 112, 255 },
	{ "MintCream", 245, 255, 250, 255 },
	{ "MistyRose", 255, 228, 225, 255 },
	{ "Moccasin", 255, 228, 181, 255 },
	{ "NavajoWhite", 255, 222, 173, 255 },
	{ "Navy", 0, 0, 128, 255 },
	{ "OldLace", 253, 245, 230, 255 },
	{ "Olive", 128, 128, 0, 255 },
	{ "OliveDrab", 107, 142, 35, 255 },
	{ "Orange", 255, 165, 0, 255 },
	{ "OrangeRed", 255, 69, 0, 255 },
	{ "Orchid", 218, 112, 214, 255 },
	{ "PaleGoldenrod", 238, 232, 170, 255 },
	{ "PaleGreen", 152, 251, 152, 255 },
	{ "PaleTurquoise", 175, 238, 238, 255 },
	{ "PaleVioletRed", 219, 112, 147, 255 },
	{ "PapayaWhip", 225, 239, 213, 255 },
	{ "PeachPuff", 255, 218, 185, 255 },
	{ "Peru", 205, 133, 63, 255 },
	{ "Pink", 255, 192, 203, 255 },
	{ "Plum", 221, 160, 221, 255 },
	{ "PowderBlue", 176, 224, 230, 255 },
	{ "Purple", 128, 0, 128, 255 },
	{ "Red", 255, 0, 0, 255 },
	{ "RosyBrown", 188, 143, 143, 255 },
	{ "RoyalBlue", 65, 105, 225, 255 },
	{ "SaddleBrown", 139, 69, 19, 255 },
	{ "Salmon", 250, 128, 114, 255 },
	{ "SandyBrown", 244, 164, 96, 255 },
	{ "SeaGreen", 46, 139, 87, 255 },
	{ "Seashell", 255, 245, 238, 255 },
	{ "Sienna", 160, 82, 45, 255 },
	{ "Silver", 192, 192, 192, 255 },
	{ "SkyBlue", 135, 206, 235, 255 },
	{ "SlateBlue", 106, 90, 205, 255 },
	{ "SlateGray", 112, 128, 144, 255 },
	{ "Snow", 255, 250, 250, 255 },
	{ "SpringGreen", 0, 255, 127, 255 },
	{ "SteelBlue", 70, 130, 180, 255 },
	{ "Tan", 210, 180, 140, 255 },
	{ "Teal", 0, 128, 128, 255 },
	{ "Thistle", 216, 191, 216, 255 },
	{ "Tomato", 255, 99, 71, 255 },
	{ "Transparent", 255, 255, 255, 0 },
	{ "Turquoise", 64, 224, 208, 255 },
	{ "Violet", 238, 130, 238, 255 },
	{ "Wheat", 245, 222, 179, 255 },
	{ "White", 255, 255, 255, 255 },
	{ "WhiteSmoke", 245, 245, 245, 255 },
	{ "Yellow", 255, 255, 0, 255 },
	{ "YellowGreen", 154, 205, 50, 255 },
	{ NULL, 0, 0, 0, 0 }
};

static duk_ret_t js_CreateColor          (duk_context* ctx);
static duk_ret_t js_Color_get_Color      (duk_context* ctx);
static duk_ret_t js_new_Color            (duk_context* ctx);
static duk_ret_t js_Color_mix            (duk_context* ctx);
static duk_ret_t js_Color_of             (duk_context* ctx);
static duk_ret_t js_Color_get_name       (duk_context* ctx);
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
	const struct x11_color* p;
	
	api_register_method(g_duk, NULL, "BlendColors", js_Color_mix);
	api_register_method(g_duk, NULL, "BlendColorsWeighted", js_Color_mix);
	api_register_method(g_duk, NULL, "CreateColor", js_CreateColor);
	api_register_method(g_duk, NULL, "CreateColorMatrix", js_CreateColorMatrix);

	api_register_ctor(g_duk, "Color", js_new_Color, NULL);
	api_register_static_func(g_duk, "Color", "mix", js_Color_mix);
	api_register_static_func(g_duk, "Color", "of", js_Color_of);
	api_register_prop(g_duk, "Color", "name", js_Color_get_name, NULL);
	api_register_method(g_duk, "Color", "toString", js_Color_toString);
	api_register_method(g_duk, "Color", "clone", js_Color_clone);
	api_register_method(g_duk, "Color", "fade", js_Color_fade);

	api_register_ctor(g_duk, "ColorMatrix", js_new_ColorMatrix, NULL);
	api_register_method(g_duk, "ColorMatrix", "toString", js_ColorMatrix_toString);
	api_register_method(g_duk, "ColorMatrix", "apply", js_ColorMatrix_apply);

	p = &X11_COLOR[0];
	while (p->name != NULL) {
		duk_get_global_string(g_duk, "Color");
		duk_push_string(g_duk, p->name);
		duk_push_c_function(g_duk, js_Color_get_Color, DUK_VARARGS);
		duk_push_sphere_color(g_duk, color_new(p->r, p->g, p->b, p->a));
		duk_put_prop_string(g_duk, -2, "\xFF" "color");
		duk_def_prop(g_duk, -3, DUK_DEFPROP_HAVE_GETTER
			| DUK_DEFPROP_CLEAR_ENUMERABLE
			| DUK_DEFPROP_SET_CONFIGURABLE);
		duk_pop(g_duk);
		++p;
	}
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
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "weights cannot be negative", w1, w2);
	
	duk_push_sphere_color(ctx, color_lerp(color1, color2, w1, w2));
	return 1;
}

static duk_ret_t
js_Color_of(duk_context* ctx)
{
	color_t     color;
	size_t      hex_length;
	const char* name;
	uint32_t    value;

	const struct x11_color* p;

	duk_push_this(ctx);
	name = duk_require_string(ctx, 0);
	
	// check if caller gave an X11 color name
	p = &X11_COLOR[0];
	while (p->name != NULL) {
		if (strcasecmp(name, p->name) == 0) {
			duk_push_sphere_color(ctx, color_new(p->r, p->g, p->b, p->a));
			return 1;
		}
		++p;
	}

	// is `name` an RGB or ARGB signature?
	if (name[0] != '#')
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "unknown color name `%s`", name);
	hex_length = strspn(&name[1], "0123456789ABCDEFabcdef");
	if (hex_length != strlen(name) - 1 || (hex_length != 6 && hex_length != 8))
		duk_error_ni(ctx, -1, DUK_ERR_TYPE_ERROR, "invalid RGB signature `%s`", name);
	value = strtoul(&name[1], NULL, 16);
	color.alpha = hex_length == 8 ? (value >> 24) & 0xFF : 255;
	color.r = (value >> 16) & 0xFF;
	color.g = (value >> 8) & 0xFF;
	color.b = value & 0xFF;
	duk_push_sphere_color(ctx, color);
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
js_Color_get_name(duk_context* ctx)
{
	color_t color;

	const struct x11_color* p;

	duk_push_this(ctx);
	color = duk_require_sphere_color(ctx, -1);

	p = &X11_COLOR[0];
	while (p->name != NULL) {
		if (color.r == p->r && color.g == p->g && color.b == p->b && color.alpha == p->a) {
			duk_eval_string(ctx, "''.toLowerCase");
			duk_push_string(ctx, p->name);
			duk_call_method(ctx, 0);
			return 1;
		}
		++p;
	}
	
	duk_push_sprintf(ctx, "#%.2x%.2x%.2x%.2x", color.alpha, color.r, color.g, color.b);
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
