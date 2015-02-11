#include "minisphere.h"

static int rfn_font_ascent  (const ALLEGRO_FONT* f);
static int rfn_font_descent (const ALLEGRO_FONT* f);
static int rfn_font_height  (const ALLEGRO_FONT* f);

static ALLEGRO_FONT_VTABLE rfn_font_vtable =
{
	rfn_font_height,
	rfn_font_ascent,
	rfn_font_descent
};

ALLEGRO_FONT*
al_load_rfn_font(const char* filename, int size, int flags)
{
	ALLEGRO_FILE* file = NULL;
	ALLEGRO_FONT* font = NULL;
	file = al_fopen(filename, "rb");
	font = calloc(1, sizeof *font);
	font->vtable = &rfn_font_vtable;
	al_fclose(file);
	return font;
}

static int
rfn_font_height(const ALLEGRO_FONT* f)
{
	return f->height;
}

static int
rfn_font_ascent(const ALLEGRO_FONT* f)
{
	return rfn_font_height(f);
}

static int
rfn_font_descent(const ALLEGRO_FONT* f)
{
	(void) f;
	return 0;
}
