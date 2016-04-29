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

color_t       rgba         (uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
ALLEGRO_COLOR nativecolor  (color_t color);
color_t       blend_colors (color_t color1, color_t color2, float w1, float w2);

colormatrix_t colormatrix          (int rn, int rr, int rg, int rb, int gn, int gr, int gg, int gb, int bn, int br, int bg, int bb);
colormatrix_t blend_color_matrices (colormatrix_t mat1, colormatrix_t mat2, int w1, int w2);
color_t       transform_pixel      (color_t pixel, colormatrix_t matrix);

void          init_color_api (void);

void          duk_push_sphere_color          (duk_context* ctx, color_t color);
color_t       duk_require_sphere_color       (duk_context* ctx, duk_idx_t index);
colormatrix_t duk_require_sphere_colormatrix (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__COLOR_H__INCLUDED
