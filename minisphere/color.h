#ifndef MINISPHERE__COLOR_H__INCLUDED
#define MINISPHERE__COLOR_H__INCLUDED

typedef struct color color_t;

extern color_t rgba         (uint8_t r, uint8_t g, uint8_t b, uint8_t alpha);
extern color_t blend_colors (color_t color1, color_t color2, float w1, float w2);

extern void    init_color_api           (void);
extern void    duk_push_sphere_color    (duk_context* ctx, color_t color);
extern color_t duk_require_sphere_color (duk_context* ctx, duk_idx_t index);

struct color
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t alpha;
};

ALLEGRO_COLOR to_native_color (color_t color);

#endif // MINISPHERE__COLOR_H__INCLUDED
