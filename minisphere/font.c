#include "minisphere.h"
#include "api.h"
#include "font.h"

static duk_ret_t _js_LoadFont            (duk_context* ctx);
static duk_ret_t _js_Font_finalize       (duk_context* ctx);
static duk_ret_t _js_Font_getColorMask   (duk_context* ctx);
static duk_ret_t _js_Font_getHeight      (duk_context* ctx);
static duk_ret_t _js_Font_setColorMask   (duk_context* ctx);
static duk_ret_t _js_Font_drawText       (duk_context* ctx);
static duk_ret_t _js_Font_getStringWidth (duk_context* ctx);
static duk_ret_t _js_Font_wordWrapString (duk_context* ctx);

void
init_font_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "LoadFont", &_js_LoadFont);
}

void
duk_push_sphere_Font(duk_context* ctx, ALLEGRO_FONT* font)
{
	duk_push_object(ctx);
	duk_push_pointer(ctx, font); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_object(ctx);
	duk_push_int(ctx, 255); duk_put_prop_string(ctx, -2, "red");
	duk_push_int(ctx, 255); duk_put_prop_string(ctx, -2, "green");
	duk_push_int(ctx, 255); duk_put_prop_string(ctx, -2, "blue");
	duk_push_int(ctx, 255); duk_put_prop_string(ctx, -2, "alpha");
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
	duk_push_c_function(ctx, &_js_Font_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, &_js_Font_getColorMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getColorMask");
	duk_push_c_function(ctx, &_js_Font_getHeight, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getHeight");
	duk_push_c_function(ctx, &_js_Font_setColorMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setColorMask");
	duk_push_c_function(ctx, &_js_Font_drawText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawText");
	duk_push_c_function(ctx, &_js_Font_getStringWidth, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getStringWidth");
	duk_push_c_function(ctx, &_js_Font_wordWrapString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "wordWrapString");
}

static duk_ret_t
_js_LoadFont(duk_context* ctx)
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
_js_Font_finalize(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	al_destroy_font(font);
	return 0;
}

static duk_ret_t
_js_Font_getColorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
_js_Font_getHeight(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, al_get_font_line_height(font));
	return 1;
}

static duk_ret_t
_js_Font_setColorMask(duk_context* ctx)
{
	ALLEGRO_FONT* font;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "color_mask"); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
_js_Font_drawText(duk_context* ctx)
{
	ALLEGRO_FONT* font;
	ALLEGRO_COLOR mask;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	duk_get_prop_string(ctx, -1, "red"); mask.r = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "green"); mask.g = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "blue"); mask.b = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "alpha"); mask.a = duk_get_number(ctx, -1) / 255; duk_pop(ctx);
	duk_pop_2(ctx);
	float x = duk_get_number(ctx, 0);
	float y = duk_get_number(ctx, 1);
	const char* text = duk_to_string(ctx, 2);
	al_draw_text(font, mask, x, y, 0x0, text);
	return 0;
}

static duk_ret_t
_js_Font_getStringWidth(duk_context* ctx)
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
_js_Font_wordWrapString(duk_context* ctx)
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
