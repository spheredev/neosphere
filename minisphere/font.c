#include "minisphere.h"
#include "api.h"
#include "color.h"

#include "font.h"

static duk_ret_t js_LoadFont             (duk_context* ctx);
static duk_ret_t js_Font_finalize        (duk_context* ctx);
static duk_ret_t js_Font_getColorMask    (duk_context* ctx);
static duk_ret_t js_Font_getHeight       (duk_context* ctx);
static duk_ret_t js_Font_setColorMask    (duk_context* ctx);
static duk_ret_t js_Font_drawText        (duk_context* ctx);
static duk_ret_t js_Font_drawTextBox     (duk_context* ctx);
static duk_ret_t js_Font_drawZoomedText  (duk_context* ctx);
static duk_ret_t js_Font_getStringHeight (duk_context* ctx);
static duk_ret_t js_Font_getStringWidth  (duk_context* ctx);
static duk_ret_t js_Font_wordWrapString  (duk_context* ctx);

void
init_font_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "LoadFont", &js_LoadFont);
}

void
duk_push_sphere_Font(duk_context* ctx, ALLEGRO_FONT* font)
{
	duk_push_object(ctx);
	duk_push_c_function(ctx, js_Font_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_Font_getColorMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getColorMask");
	duk_push_c_function(ctx, js_Font_getHeight, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getHeight");
	duk_push_c_function(ctx, js_Font_setColorMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setColorMask");
	duk_push_c_function(ctx, js_Font_drawText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawText");
	duk_push_c_function(ctx, js_Font_drawTextBox, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawTextBox");
	duk_push_c_function(ctx, js_Font_drawZoomedText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawZoomedText");
	duk_push_c_function(ctx, js_Font_getStringHeight, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getStringHeight");
	duk_push_c_function(ctx, js_Font_getStringWidth, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getStringWidth");
	duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "wordWrapString");
	
	duk_push_pointer(ctx, font); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_sphere_color(ctx, al_map_rgba(255, 255, 255, 255)); duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
}

static duk_ret_t
js_LoadFont(duk_context* ctx)
{
	const char* filename = duk_get_string(ctx, 0);
	char* path = get_asset_path(filename, "fonts", false);
	ALLEGRO_FONT* font = al_load_font(path, 0, 0x0);
	free(path);
	if (font != NULL) {
		duk_push_sphere_Font(ctx, font);
		return 1;
	}
	else {
		duk_error(ctx, DUK_ERR_ERROR, "LoadFont(): Unable to load font file '%s'", filename);
	}
}

static duk_ret_t
js_Font_finalize(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_destroy_font(font);
	return 0;
}

static duk_ret_t
js_Font_getColorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_Font_getHeight(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, al_get_font_line_height(font));
	return 1;
}

static duk_ret_t
js_Font_setColorMask(duk_context* ctx)
{
	ALLEGRO_FONT* font;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "color_mask"); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Font_drawText(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	ALLEGRO_COLOR mask;
	int           x, y;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_get_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	x = duk_to_int(ctx, 0);
	y = duk_to_int(ctx, 1);
	const char* text = duk_to_string(ctx, 2);
	if (!g_skip_frame) al_draw_text(font, mask, x, y, 0x0, text);
	return 0;
}

static duk_ret_t
js_Font_drawZoomedText(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	float scale = duk_require_number(ctx, 2);
	const char* text = duk_to_string(ctx, 3);
	
	ALLEGRO_BITMAP* bitmap;
	ALLEGRO_FONT*   font;
	ALLEGRO_COLOR   mask;
	int             text_w, text_h;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_get_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!g_skip_frame) {
		text_w = al_get_text_width(font, text);
		text_h = al_get_font_line_height(font);
		bitmap = al_create_bitmap(text_w, text_h);
		al_set_target_bitmap(bitmap);
		al_draw_text(font, mask, 0, 0, 0x0, text);
		al_set_target_backbuffer(g_display);
		al_draw_scaled_bitmap(bitmap, 0, 0, text_w, text_h, x, y, text_w * scale, text_h * scale, 0x0);
		al_destroy_bitmap(bitmap);
	}
	return 0;
}

static duk_ret_t
js_Font_drawTextBox(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	int w = duk_require_int(ctx, 2);
	int h = duk_require_int(ctx, 3);
	int pad = duk_require_int(ctx, 4);
	const char* text = duk_to_string(ctx, 5);

	ALLEGRO_FONT* font;
	const char*   line_text;
	ALLEGRO_COLOR mask;
	int           num_lines;

	int i;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_get_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!g_skip_frame) {
		duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
		duk_push_this(ctx);
		duk_push_string(ctx, text);
		duk_push_int(ctx, w - pad * 2);
		duk_call_method(ctx, 2);
		duk_get_prop_string(ctx, -1, "length"); num_lines = duk_get_int(ctx, -1); duk_pop(ctx);
		for (i = 0; i < num_lines; ++i) {
			duk_get_prop_index(ctx, -1, i); line_text = duk_get_string(ctx, -1); duk_pop(ctx);
			al_draw_text(font, mask, x + pad, y, 0x0, line_text);
			y += al_get_font_line_height(font);
		}
		duk_pop(ctx);
	}
	return 0;
}

static duk_ret_t
js_Font_getStringHeight(duk_context* ctx)
{
	const char* text = duk_require_string(ctx, 0);
	int width = duk_require_int(ctx, 1);
	
	ALLEGRO_FONT* font;
	int           num_lines;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
	duk_push_this(ctx);
	duk_push_string(ctx, text);
	duk_push_int(ctx, width);
	duk_call_method(ctx, 2);
	duk_get_prop_string(ctx, -1, "length"); num_lines = duk_get_int(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, al_get_font_line_height(font) * num_lines);
	return 1;
}

static duk_ret_t
js_Font_getStringWidth(duk_context* ctx)
{
	ALLEGRO_FONT* font;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	const char* text = duk_to_string(ctx, 0);
	duk_push_int(ctx, al_get_text_width(font, text));
	return 1;
}

static duk_ret_t
js_Font_wordWrapString(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	int           line_width;
	int           line_idx;
	int           num_words;
	int           space_width;
	char*         text;
	float         targ_width;
	char*         word;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	text = strdup(duk_to_string(ctx, 0));
	targ_width = duk_to_number(ctx, 1);
	space_width = al_get_text_width(font, " ");
	duk_push_array(ctx);
	line_idx = 0;
	line_width = 0;
	num_words = 0;
	word = strtok(text, " ");
	while (word != NULL) {
		line_width += al_get_text_width(font, word);
		if (line_width > targ_width) {
			duk_concat(ctx, num_words);
			duk_put_prop_index(ctx, -2, line_idx);
			line_width = al_get_text_width(font, word);
			num_words = 0;
			++line_idx;
		}
		++num_words;
		duk_push_string(ctx, word);
		word = strtok(NULL, " ");
		if (word != NULL) {
			duk_push_string(ctx, " ");
			line_width += space_width;
			++num_words;
		}
	}
	duk_concat(ctx, num_words);
	duk_put_prop_index(ctx, -2, line_idx);
	free(text);
	return 1;
}
