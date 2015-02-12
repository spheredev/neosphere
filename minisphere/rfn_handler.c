#include "minisphere.h"

typedef struct {
	char signature[4];
	int16_t version;
	int16_t num_chars;
	char reserved[248];
} rfn_header_t;

typedef struct {
	int16_t width;
	int16_t height;
	char reserved[28];
} rfn_glyph_header_t;

typedef struct {
	rfn_glyph_header_t header;
	ALLEGRO_BITMAP* bitmap;
} rfn_glyph_t;

typedef struct {
	rfn_header_t header;
	rfn_glyph_t* glyphs;
} rfn_font_t;

static int  rfn_get_height          (const ALLEGRO_FONT* f);
static int  rfn_get_ascent          (const ALLEGRO_FONT* f);
static int  rfn_get_descent         (const ALLEGRO_FONT* f);
static int  rfn_get_char_length     (const ALLEGRO_FONT* f, int ch);
static int  rfn_get_length          (const ALLEGRO_FONT* f, const ALLEGRO_USTR* text);
static int  rfn_render_char         (const ALLEGRO_FONT* f, ALLEGRO_COLOR color, int ch, float x, float y);
static int  rfn_render              (const ALLEGRO_FONT* f, ALLEGRO_COLOR color, const ALLEGRO_USTR* text, float x, float y);
static void rfn_destroy_font        (ALLEGRO_FONT* f);
static void rfn_get_text_dimensions (const ALLEGRO_FONT* f, const ALLEGRO_USTR* text, int* bbx, int* bby, int* bbw, int* bbh);

static ALLEGRO_FONT_VTABLE rfn_font_vtable =
{
	rfn_get_height,
	rfn_get_ascent,
	rfn_get_descent,
	rfn_get_char_length,
	rfn_get_length,
	rfn_render_char,
	rfn_render,
	rfn_destroy_font,
	rfn_get_text_dimensions
};

ALLEGRO_FONT*
al_load_rfn_font(const char* filename, int size, int flags)
{
	ALLEGRO_FILE* file = al_fopen(filename, "rb");
	rfn_font_t* rfn = al_calloc(1, sizeof(rfn_font_t));
	al_fread(file, &rfn->header, 256);
	size_t pixel_size = (rfn->header.version == 1) ? 1 : 4;
	rfn->glyphs = calloc(rfn->header.num_chars, sizeof(rfn_glyph_t));
	for (int i = 0; i < rfn->header.num_chars; ++i) {
		rfn_glyph_t* glyph = &rfn->glyphs[i];
		al_fread(file, &glyph->header, sizeof glyph->header);
		size_t data_size = glyph->header.width * glyph->header.height * pixel_size;
		void* data = al_malloc(data_size);
		al_fread(file, data, data_size);
		glyph->bitmap = al_create_bitmap(glyph->header.width, glyph->header.height);
		ALLEGRO_LOCKED_REGION* bitmap_lock = al_lock_bitmap(glyph->bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY);
		uint8_t* src_ptr = data;
		uint8_t* dest_ptr = bitmap_lock->data;
		switch (rfn->header.version) {
		case 1: // version 1: 8-bit grayscale glyphs
			for (int y = 0; y < glyph->header.height; ++y) {
				for (int x = 0; x < glyph->header.width; ++x) {
					dest_ptr[x] = src_ptr[x];
					dest_ptr[x + 1] = src_ptr[x];
					dest_ptr[x + 2] = src_ptr[x];
					dest_ptr[x + 3] = 255;
					dest_ptr += 4;
				}
				dest_ptr += bitmap_lock->pitch - (glyph->header.width * 4);
				src_ptr += glyph->header.width;
			}
			break;
		case 2: // version 2: 32-bit truecolor glyphs
			for (int y = 0; y < glyph->header.height; ++y) {
				memcpy(dest_ptr, src_ptr, glyph->header.width * 4);
				dest_ptr += bitmap_lock->pitch;
				src_ptr += glyph->header.width * pixel_size;
			}
			break;
		}
		al_unlock_bitmap(glyph->bitmap);
		al_free(data);
	}
	ALLEGRO_FONT* font = al_calloc(1, sizeof *font);
	font->vtable = &rfn_font_vtable;
	font->data = rfn;
	font->height = rfn->glyphs[0].header.height;
	al_fclose(file);
	return font;
}

static int
rfn_get_height(const ALLEGRO_FONT* f)
{
	return f->height;
}

static int
rfn_get_ascent(const ALLEGRO_FONT* f)
{
	return rfn_get_height(f);
}

static int
rfn_get_descent(const ALLEGRO_FONT* f)
{
	(void)f;
	return 0;
}

static int
rfn_get_char_length(const ALLEGRO_FONT* f, int ch)
{
	rfn_font_t* rfn = f->data;
	return rfn->glyphs[ch].header.width;
}

static int
rfn_get_length(const ALLEGRO_FONT* f, const ALLEGRO_USTR* text)
{
	int length = 0;
	int ch, pos = 0;
	while ((ch = al_ustr_get_next(text, &pos)) >= 0) {
		length += rfn_get_char_length(f, ch);
	}
	return length;
}

static int
rfn_render_char(const ALLEGRO_FONT* f, ALLEGRO_COLOR color, int ch, float x, float y)
{
	rfn_font_t* rfn = f->data;
	rfn_glyph_t* glyph = &rfn->glyphs[ch];
	al_draw_tinted_bitmap(glyph->bitmap, color, x, y, 0x0);
	return glyph->header.width;
}

static int
rfn_render(const ALLEGRO_FONT* f, ALLEGRO_COLOR color, const ALLEGRO_USTR* text, float x, float y)
{
	int32_t ch;
	int pos = 0, width = 0;
	bool was_draw_held = al_is_bitmap_drawing_held();
	al_hold_bitmap_drawing(true);
	while ((ch = al_ustr_get_next(text, &pos)) >= 0) {
		width += rfn_render_char(f, color, ch, x + width, y);
	}
	al_hold_bitmap_drawing(was_draw_held);
	return width;
}

static
void rfn_destroy_font(ALLEGRO_FONT* f)
{
	rfn_font_t* rfn = f->data;
	for (int i = 0; i < rfn->header.num_chars; ++i) {
		al_destroy_bitmap(rfn->glyphs[i].bitmap);
	}
	al_free(rfn->glyphs);
	al_free(f->data);
	al_free(f);
}

static
void rfn_get_text_dimensions(const ALLEGRO_FONT* f, const ALLEGRO_USTR* text, int* bbx, int* bby, int* bbw, int* bbh)
{
	rfn_font_t* rfn = f->data;
	if (bbx != NULL) *bbx = 0;
	if (bby != NULL) *bby = 0;
	if (bbw != NULL) *bbw = rfn_get_length(f, text);
	if (bbh != NULL) *bbh = al_get_font_line_height(f);
}
