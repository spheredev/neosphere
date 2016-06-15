#ifndef MINISPHERE__COLOR_H__INCLUDED
#define MINISPHERE__COLOR_H__INCLUDED

typedef
struct color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t alpha;
} color_t;

typedef
struct colormatrix
{
	int rn, rr, rg, rb;
	int gn, gr, gg, gb;
	int bn, br, bg, bb;
} colormatrix_t;

ALLEGRO_COLOR nativecolor      (color_t color);
color_t       color_new        (uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
color_t       color_mix        (color_t color, color_t other, float w1, float w2);
color_t       color_transform  (color_t color, colormatrix_t matrix);
colormatrix_t colormatrix_new  (int rn, int rr, int rg, int rb, int gn, int gr, int gg, int gb, int bn, int br, int bg, int bb);
colormatrix_t colormatrix_lerp (colormatrix_t mat1, colormatrix_t mat2, int w1, int w2);

#endif // MINISPHERE__COLOR_H__INCLUDED
