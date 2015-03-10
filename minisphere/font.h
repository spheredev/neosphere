#ifndef MINISPHERE__FONT_H__INCLUDED
#define MINISPHERE__FONT_H__INCLUDED

typedef enum text_align text_align_t;
enum text_align
{
	TEXT_ALIGN_LEFT,
	TEXT_ALIGN_CENTER,
	TEXT_ALIGN_RIGHT
};

typedef struct font font_t;

font_t*             load_font       (const char* path, int size);
font_t*             ref_font        (font_t* font);
void                free_font       (font_t* font);
const ALLEGRO_FONT* get_font_object (const font_t* font);
void                draw_text       (const font_t* font, ALLEGRO_COLOR mask, int x, int y, text_align_t alignment, const char* text);

extern void    init_font_api           (duk_context* ctx);
extern void    duk_push_sphere_font    (duk_context* ctx, font_t* font);
extern font_t* duk_require_sphere_font (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__FONT_H__INCLUDED
