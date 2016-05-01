#include "minisphere.h"
#include "api.h"
#include "color.h"

struct colormat
{
	unsigned int refcount;
	float        m[5][5];
};

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
color_new(uint8_t r, uint8_t g, uint8_t b, uint8_t alpha)
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

colormat_t*
colormat_new(void)
{
	colormat_t* matrix;

	matrix = calloc(1, sizeof(colormat_t));
	matrix->m[0][0] = 1.0;
	matrix->m[1][1] = 1.0;
	matrix->m[2][2] = 1.0;
	matrix->m[3][3] = 1.0;
	matrix->m[4][4] = 1.0;
	return colormat_ref(matrix);
}

colormat_t*
colormat_clone(const colormat_t* mat)
{
	colormat_t* new_mat;

	new_mat = malloc(sizeof(colormat_t));
	memcpy(new_mat, mat, sizeof(colormat_t));
	return new_mat;
}

colormat_t*
colormat_ref(colormat_t* mat)
{
	++mat->refcount;
	return mat;
}

void
colormat_free(colormat_t* mat)
{
	if (--mat->refcount > 0)
		return;
	free(mat);
}

color_t
colormat_apply(const colormat_t* mat, color_t color)
{
	float v[4];

#define E(i) (fmin(fmax((0 \
		+ mat->m[i][0] * v[0] \
		+ mat->m[1][1] * v[1] \
		+ mat->m[i][2] * v[2] \
		+ mat->m[i][3] * v[3] \
		+ mat->m[i][4] \
	), 0), 255))

	v[0] = color.r;
	v[1] = color.g;
	v[2] = color.b;
	v[3] = color.alpha;

	return color_new(E(0), E(1), E(2), E(3));

#undef E
}

void
colormat_compose(colormat_t* mat, const colormat_t* other)
{
#define E(i, j) ( 0 \
		+ other->m[i][0] * mat->m[0][j] \
		+ other->m[i][1] * mat->m[1][j] \
		+ other->m[i][2] * mat->m[2][j] \
		+ other->m[i][3] * mat->m[3][j] \
		+ other->m[i][4] * mat->m[4][j] \
	)

	float m[5][5] = {
		E(0, 0), E(0, 1), E(0, 2), E(0, 3), E(0, 4),
		E(1, 0), E(1, 1), E(1, 2), E(1, 3), E(1, 4),
		E(2, 0), E(2, 1), E(2, 2), E(2, 3), E(2, 4),
		E(3, 0), E(3, 1), E(3, 2), E(3, 3), E(3, 4),
		E(4, 0), E(4, 1), E(4, 2), E(4, 3), E(4, 4),
	};
	memcpy(mat->m, &m, sizeof(float[5][5]));

#undef E
}

void
colormat_identity(colormat_t* mat)
{
	memset(mat, 0, sizeof(colormat_t));
	mat->m[0][0] = 1.0;
	mat->m[1][1] = 1.0;
	mat->m[2][2] = 1.0;
	mat->m[3][3] = 1.0;
	mat->m[4][4] = 1.0;
}

void
colormat_scale(colormat_t* mat, float sr, float sg, float sb, float sa)
{
	int j;

	for (j = 0; j < 5; ++j) {
		mat->m[0][j] *= sr;
		mat->m[1][j] *= sg;
		mat->m[2][j] *= sb;
		mat->m[3][j] *= sa;
	}
}

void
colormat_shift(colormat_t* mat, float dr, float dg, float db, float da)
{
	mat->m[0][4] += dr;
	mat->m[1][4] += dg;
	mat->m[2][4] += db;
	mat->m[3][4] += da;
}

void
init_color_api(void)
{
	register_api_method(g_duk, NULL, "BlendColors", js_BlendColors);
	register_api_method(g_duk, NULL, "BlendColorsWeighted", js_BlendColorsWeighted);
	register_api_method(g_duk, NULL, "CreateColor", js_CreateColor);
	register_api_method(g_duk, NULL, "CreateColorMatrix", js_CreateColorMatrix);

	// register Color methods and properties
	register_api_ctor(g_duk, "Color", js_new_Color, NULL);
	register_api_method(g_duk, "Color", "toString", js_Color_toString);
	register_api_method(g_duk, "Color", "clone", js_Color_clone);
	
	// register ColorMatrix methods and properties
	register_api_ctor(g_duk, "ColorMatrix", js_new_ColorMatrix, NULL);
	register_api_method(g_duk, "ColorMatrix", "toString", js_ColorMatrix_toString);
	register_api_method(g_duk, "ColorMatrix", "apply", js_ColorMatrix_apply);
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

static duk_ret_t
js_BlendColors(duk_context* ctx)
{
	color_t color1 = duk_require_sphere_color(ctx, 0);
	color_t color2 = duk_require_sphere_color(ctx, 1);

	duk_push_sphere_color(ctx, color_lerp(color1, color2, 1, 1));
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
	duk_push_sphere_color(ctx, color_transform(color, matrix));
	return 1;
}
