#ifndef MINISPHERE__COLOR_H__INCLUDED
#define MINISPHERE__COLOR_H__INCLUDED

typedef struct color       color_t;
typedef struct colormatrix colormatrix_t;

extern color_t       rgba         (uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
extern ALLEGRO_COLOR nativecolor  (color_t color);
extern color_t       blend_colors (color_t color1, color_t color2, float w1, float w2);

extern colormatrix_t colormatrix     (uint8_t rn, uint8_t rr, uint8_t rg, uint8_t rb,
                                      uint8_t gn, uint8_t gr, uint8_t gg, uint8_t gb,
                                      uint8_t bn, uint8_t br, uint8_t bg, uint8_t bb);
extern color_t       transform_pixel (color_t pixel, colormatrix_t matrix);

extern void          init_color_api (void);

extern void          duk_push_sphere_color          (duk_context* ctx, color_t color);
extern color_t       duk_require_sphere_color       (duk_context* ctx, duk_idx_t index);
extern colormatrix_t duk_require_sphere_colormatrix (duk_context* ctx, duk_idx_t index);

struct color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t alpha;
};

struct colormatrix
{
	uint8_t rn, rr, rg, rb;
	uint8_t gn, gr, gg, gb;
	uint8_t bn, br, bg, bb;
};

#endif // MINISPHERE__COLOR_H__INCLUDED
