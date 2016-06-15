#ifndef MINISPHERE__FONT_H__INCLUDED
#define MINISPHERE__FONT_H__INCLUDED

#include "color.h"
#include "image.h"

typedef struct font     font_t;
typedef struct wraptext wraptext_t;

typedef
enum text_align
{
	TEXT_ALIGN_LEFT,
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_RIGHT
} text_align_t;

font_t*     font_load        (const char* path);
font_t*     font_clone       (const font_t* font);
font_t*     font_ref         (font_t* font);
void        font_free        (font_t* font);
image_t*    font_glyph       (const font_t* font, uint32_t cp);
int         font_height      (const font_t* font);
void        font_set_glyph   (font_t* font, uint32_t cp, image_t* image);
void        font_draw_text   (const font_t* font, color_t mask, int x, int y, text_align_t alignment, const char* text);
void        font_get_metrics (const font_t* font, int* min_width, int* max_width, int* out_line_height);
int         font_get_width   (const font_t* font, const char* text);
wraptext_t* wraptext_new     (const char* text, const font_t* font, int width);
void        wraptext_free    (wraptext_t* wraptext);
int         wraptext_len     (const wraptext_t* wraptext);
const char* wraptext_line    (const wraptext_t* wraptext, int line_index);

#endif // MINISPHERE__FONT_H__INCLUDED
