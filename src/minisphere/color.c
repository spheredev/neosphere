#include "minisphere.h"
#include "color.h"

ALLEGRO_COLOR
nativecolor(color_t color)
{
	return al_map_rgba(color.r, color.g, color.b, color.a);
}

color_t
color_new(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	color_t color;

	color.r = r;
	color.g = g;
	color.b = b;
	color.a = a;
	return color;
}

color_t
color_mix(color_t color, color_t other, float w1, float w2)
{
	color_t blend;
	float   sigma;

	sigma = w1 + w2;
	blend.r = (color.r * w1 + other.r * w2) / sigma;
	blend.g = (color.g * w1 + other.g * w2) / sigma;
	blend.b = (color.b * w1 + other.b * w2) / sigma;
	blend.a = (color.a * w1 + other.a * w2) / sigma;
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
	return color_new(r, g, b, color.a);
}
