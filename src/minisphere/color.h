#ifndef MINISPHERE__COLOR_H__INCLUDED
#define MINISPHERE__COLOR_H__INCLUDED

typedef struct colormat colormat_t;

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

color_t       color_new         (uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
color_t       color_lerp        (color_t color1, color_t color2, float w1, float w2);
color_t       color_transform   (color_t pixel, colormatrix_t matrix);
colormat_t*   colormat_new      (void);
colormat_t*   colormat_clone    (const colormat_t* matrix);
colormat_t*   colormat_ref      (colormat_t* matrix);
void          colormat_free     (colormat_t* matrix);
color_t       colormat_apply    (const colormat_t* matrix, color_t color);
void          colormat_compose  (colormat_t* matrix, const colormat_t* other);
void          colormat_identity (colormat_t* matrix);
void          colormat_scale    (colormat_t* matrix, float sr, float sg, float sb, float sa);
void          colormat_shift    (colormat_t* matrix, float dr, float dg, float db, float da);
colormatrix_t colormatrix       (int rn, int rr, int rg, int rb, int gn, int gr, int gg, int gb, int bn, int br, int bg, int bb);
colormatrix_t colormatrix_lerp  (colormatrix_t mat1, colormatrix_t mat2, int w1, int w2);

ALLEGRO_COLOR nativecolor (color_t color);

void init_color_api (void);

void          duk_push_sphere_color          (duk_context* ctx, color_t color);
color_t       duk_require_sphere_color       (duk_context* ctx, duk_idx_t index);
colormatrix_t duk_require_sphere_colormatrix (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__COLOR_H__INCLUDED
