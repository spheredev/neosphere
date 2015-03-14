#include "minisphere.h"

#pragma pack(push, 1)
typedef struct rfn_header {
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
#pragma pack(pop)

typedef struct {
	rfn_glyph_header_t header;
	ALLEGRO_BITMAP*    bitmap;
} rfn_glyph_t;

typedef struct {
	ALLEGRO_BITMAP* atlas;
	rfn_header_t    header;
	rfn_glyph_t*    glyphs;
	int             pitch;
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
	int                    atlas_size_x, atlas_size_y;
	ALLEGRO_LOCKED_REGION* bitmap_lock;
	ALLEGRO_FILE*          file;
	ALLEGRO_FONT*          font = NULL;
	int64_t                glyph_start;
	int                    max_x = 0, max_y = 0;
	int64_t                n_glyphs_per_row;
	size_t                 pixel_size;
	rfn_font_t*            rfn = NULL;
	uint8_t                *src_ptr, *dest_ptr;
	int                    i, x, y;

	if ((file = al_fopen(filename, "rb")) == NULL) goto on_error;
	if ((rfn = calloc(1, sizeof(rfn_font_t))) == NULL) goto on_error;
	if (al_fread(file, &rfn->header, sizeof(rfn_header_t)) != sizeof(rfn_header_t))
		goto on_error;
	pixel_size = (rfn->header.version == 1) ? 1 : 4;
	if ((rfn->glyphs = calloc(rfn->header.num_chars, sizeof(rfn_glyph_t))) == NULL)
		goto on_error;
	
	// pass 1: load glyph headers and find largest glyph
	glyph_start = al_ftell(file);
	for (i = 0; i < rfn->header.num_chars; ++i) {
		rfn_glyph_t* glyph = &rfn->glyphs[i];
		if (al_fread(file, &glyph->header, sizeof(rfn_glyph_header_t)) != sizeof(rfn_glyph_header_t))
			goto on_error;
		al_fseek(file, glyph->header.width * glyph->header.height * pixel_size, ALLEGRO_SEEK_CUR);
		max_x = fmax(rfn->glyphs[i].header.width, max_x);
		max_y = fmax(rfn->glyphs[i].header.height, max_y);
	}
	
	// create glyph atlas
	n_glyphs_per_row = ceil(sqrt(rfn->header.num_chars));
	atlas_size_x = max_x * n_glyphs_per_row;
	atlas_size_y = max_y * n_glyphs_per_row;
	if ((rfn->atlas = al_create_bitmap(atlas_size_x, atlas_size_y)) == NULL)
		goto on_error;

	// pass 2: load glyph data
	al_fseek(file, glyph_start, ALLEGRO_SEEK_SET);
	for (i = 0; i < rfn->header.num_chars; ++i) {
		rfn_glyph_t* glyph = &rfn->glyphs[i];
		al_fseek(file, sizeof(rfn_glyph_header_t), ALLEGRO_SEEK_CUR); // already have glyph header from pass 1
		size_t data_size = glyph->header.width * glyph->header.height * pixel_size;
		void* data = malloc(data_size);
		if (al_fread(file, data, data_size) != data_size) goto on_error;
		glyph->bitmap = al_create_sub_bitmap(rfn->atlas,
			i % n_glyphs_per_row * max_x, i / n_glyphs_per_row * max_y,
			glyph->header.width, glyph->header.height);
		if (glyph->bitmap == NULL) goto on_error;
		if ((bitmap_lock = al_lock_bitmap(glyph->bitmap, ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)) == NULL)
			goto on_error;
		src_ptr = data; dest_ptr = bitmap_lock->data;
		switch (rfn->header.version) {
		case 1: // version 1: 8-bit grayscale glyphs
			for (y = 0; y < glyph->header.height; ++y) {
				for (x = 0; x < glyph->header.width; ++x) {
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
			for (y = 0; y < glyph->header.height; ++y) {
				memcpy(dest_ptr, src_ptr, glyph->header.width * 4);
				dest_ptr += bitmap_lock->pitch;
				src_ptr += glyph->header.width * pixel_size;
			}
			break;
		}
		al_unlock_bitmap(glyph->bitmap);
		free(data);
	}
	if ((font = al_calloc(1, sizeof *font)) == NULL) goto on_error;
	font->vtable = &rfn_font_vtable;
	font->data = rfn;
	font->height = max_y;
	al_fclose(file);
	return font;

on_error:
	al_free(font);
	if (rfn != NULL) {
		for (i = 0; i < rfn->header.num_chars; ++i) {
			if (rfn->glyphs[i].bitmap != NULL) al_destroy_bitmap(rfn->glyphs[i].bitmap);
		}
		al_free(rfn->glyphs);
		if (rfn->atlas != NULL) al_destroy_bitmap(rfn->atlas);
		al_free(rfn);
	}
	return NULL;
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
	free(rfn->glyphs);
	al_destroy_bitmap(rfn->atlas);
	free(f->data);
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
