#ifndef MINISPHERE__FONT_H__INCLUDED
#define MINISPHERE__FONT_H__INCLUDED

#include "color.h"
#include "image.h"

typedef struct font     font_t;
typedef enum text_align text_align_t;

font_t*  load_font            (const char* path);
font_t*  ref_font             (font_t* font);
void     free_font            (font_t* font);
int      get_font_line_height (const font_t* font);
image_t* get_glyph_image      (const font_t* font, int codepoint);
int      get_text_width       (const font_t* font, const char* text);
void     set_glyph_image      (font_t* font, int codepoint, image_t* image);
void     draw_text            (const font_t* font, color_t mask, int x, int y, text_align_t alignment, const char* text);

extern void    init_font_api           (duk_context* ctx);
extern void    duk_push_sphere_font    (duk_context* ctx, font_t* font);
extern font_t* duk_require_sphere_font (duk_context* ctx, duk_idx_t index);

enum text_align
{
	TEXT_ALIGN_LEFT,
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_RIGHT
};

#endif // MINISPHERE__FONT_H__INCLUDED
