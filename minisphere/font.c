#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"

#include "font.h"

static duk_ret_t js_GetSystemFont          (duk_context* ctx);
static duk_ret_t js_LoadFont               (duk_context* ctx);
static duk_ret_t js_Font_finalize          (duk_context* ctx);
static duk_ret_t js_Font_toString          (duk_context* ctx);
static duk_ret_t js_Font_clone             (duk_context* ctx);
static duk_ret_t js_Font_getCharacterImage (duk_context* ctx);
static duk_ret_t js_Font_getColorMask      (duk_context* ctx);
static duk_ret_t js_Font_getHeight         (duk_context* ctx);
static duk_ret_t js_Font_setCharacterImage (duk_context* ctx);
static duk_ret_t js_Font_setColorMask      (duk_context* ctx);
static duk_ret_t js_Font_drawText          (duk_context* ctx);
static duk_ret_t js_Font_drawTextBox       (duk_context* ctx);
static duk_ret_t js_Font_drawZoomedText    (duk_context* ctx);
static duk_ret_t js_Font_getStringHeight   (duk_context* ctx);
static duk_ret_t js_Font_getStringWidth    (duk_context* ctx);
static duk_ret_t js_Font_wordWrapString    (duk_context* ctx);

struct font
{
	int                refcount;
	int                height;
	int                pitch;
	int                num_glyphs;
	struct font_glyph* glyphs;
};

struct font_glyph
{
	int      width, height;
	image_t* image;
};

#pragma pack(push, 1)
struct rfn_header
{
	char signature[4];
	int16_t version;
	int16_t num_chars;
	char reserved[248];
};

struct rfn_glyph_header
{
	int16_t width;
	int16_t height;
	char reserved[28];
};
#pragma pack(pop)

font_t*
load_font(const char* path)
{
	image_t*                atlas;
	int                     atlas_size_x, atlas_size_y;
	ALLEGRO_LOCKED_REGION*  bitmap_lock;
	ALLEGRO_FILE*           file;
	font_t*                 font = NULL;
	struct font_glyph*      glyph;
	struct rfn_glyph_header glyph_hdr;
	int64_t                 glyph_start;
	int                     max_x = 0, max_y = 0;
	int64_t                 n_glyphs_per_row;
	size_t                  pixel_size;
	struct rfn_header       rfn;
	uint8_t                 *src_ptr, *dest_ptr;

	int i, x, y;

	if ((file = al_fopen(path, "rb")) == NULL) goto on_error;
	if ((font = calloc(1, sizeof(font_t))) == NULL) goto on_error;
	if (al_fread(file, &rfn, sizeof(struct rfn_header)) != sizeof(struct rfn_header))
		goto on_error;
	pixel_size = (rfn.version == 1) ? 1 : 4;
	if (!(font->glyphs = calloc(rfn.num_chars, sizeof(struct font_glyph))))
		goto on_error;

	// pass 1: load glyph headers and find largest glyph
	glyph_start = al_ftell(file);
	for (i = 0; i < rfn.num_chars; ++i) {
		glyph = &font->glyphs[i];
		if (al_fread(file, &glyph_hdr, sizeof(struct rfn_glyph_header)) != sizeof(struct rfn_glyph_header))
			goto on_error;
		al_fseek(file, glyph_hdr.width * glyph_hdr.height * pixel_size, ALLEGRO_SEEK_CUR);
		max_x = fmax(glyph_hdr.width, max_x);
		max_y = fmax(glyph_hdr.height, max_y);
		glyph->width = glyph_hdr.width;
		glyph->height = glyph_hdr.height;
	}
	font->height = max_y;

	// create glyph atlas
	n_glyphs_per_row = ceil(sqrt(rfn.num_chars));
	atlas_size_x = max_x * n_glyphs_per_row;
	atlas_size_y = max_y * n_glyphs_per_row;
	if ((atlas = create_image(atlas_size_x, atlas_size_y)) == NULL)
		goto on_error;

	// pass 2: load glyph data
	al_fseek(file, glyph_start, ALLEGRO_SEEK_SET);
	for (i = 0; i < rfn.num_chars; ++i) {
		glyph = &font->glyphs[i];
		if (al_fread(file, &glyph_hdr, sizeof(struct rfn_glyph_header)) != sizeof(struct rfn_glyph_header))
			goto on_error;
		size_t data_size = glyph_hdr.width * glyph_hdr.height * pixel_size;
		void* data = malloc(data_size);
		if (al_fread(file, data, data_size) != data_size) goto on_error;
		glyph->image = create_subimage(atlas,
			i % n_glyphs_per_row * max_x, i / n_glyphs_per_row * max_y,
			glyph_hdr.width, glyph_hdr.height);
		if (glyph->image == NULL) goto on_error;
		if ((bitmap_lock = al_lock_bitmap(get_image_bitmap(glyph->image), ALLEGRO_PIXEL_FORMAT_ABGR_8888, ALLEGRO_LOCK_WRITEONLY)) == NULL)
			goto on_error;
		src_ptr = data; dest_ptr = bitmap_lock->data;
		switch (rfn.version) {
		case 1: // RFN v1: 8-bit grayscale glyphs
			for (y = 0; y < glyph_hdr.height; ++y) {
				for (x = 0; x < glyph_hdr.width; ++x) {
					dest_ptr[x] = src_ptr[x];
					dest_ptr[x + 1] = src_ptr[x];
					dest_ptr[x + 2] = src_ptr[x];
					dest_ptr[x + 3] = 255;
					dest_ptr += 4;
				}
				dest_ptr += bitmap_lock->pitch - (glyph_hdr.width * 4);
				src_ptr += glyph_hdr.width;
			}
			break;
		case 2: // RFN v2: 32-bit truecolor glyphs
			for (y = 0; y < glyph_hdr.height; ++y) {
				memcpy(dest_ptr, src_ptr, glyph_hdr.width * 4);
				dest_ptr += bitmap_lock->pitch;
				src_ptr += glyph_hdr.width * pixel_size;
			}
			break;
		}
		al_unlock_bitmap(get_image_bitmap(glyph->image));
		free(data);
	}
	al_fclose(file);
	free_image(atlas);
	return ref_font(font);

on_error:
	if (font != NULL) {
		for (i = 0; i < rfn.num_chars; ++i) {
			if (font->glyphs[i].image != NULL) free_image(font->glyphs[i].image);
		}
		free(font->glyphs);
		free(font);
	}
	if (atlas != NULL) free_image(atlas);
	return NULL;
}

font_t*
ref_font(font_t* font)
{
	++font->refcount;
	return font;
}

void
free_font(font_t* font)
{
	if (font == NULL || --font->refcount > 0)
		return;
	for (int i = 0; i < font->num_glyphs; ++i) {
		free_image(font->glyphs[i].image);
	}
	free(font->glyphs);
	free(font);
}

int
get_font_line_height(const font_t* font)
{
	return font->height;
}

image_t*
get_glyph_image(const font_t* font, int codepoint)
{
	return font->glyphs[codepoint].image;
}

int
get_text_width(const font_t* font, const char* text)
{
	int cp;
	int width = 0;
	
	while ((cp = *text++) != '\0') {
		width += font->glyphs[cp].width;
	}
	return width;
}

void
set_glyph_image(font_t* font, int codepoint, image_t* image)
{
	image_t*           old_image;
	struct font_glyph* p_glyph;
	
	p_glyph = &font->glyphs[codepoint];
	old_image = p_glyph->image;
	p_glyph->image = ref_image(image);
	p_glyph->width = get_image_width(image);
	p_glyph->height = get_image_height(image);
	free_image(old_image);
}

void
draw_text(const font_t* font, ALLEGRO_COLOR mask, int x, int y, text_align_t alignment, const char* text)
{
	bool is_draw_held;
	int  cp;
	
	if (alignment == TEXT_ALIGN_CENTER)
		x -= get_text_width(font, text) / 2;
	else if (alignment == TEXT_ALIGN_RIGHT)
		x -= get_text_width(font, text);
	is_draw_held = al_is_bitmap_drawing_held();
	al_hold_bitmap_drawing(true);
	while ((cp = *text++) != '\0') {
		draw_image_masked(font->glyphs[cp].image, mask, x, y);
		x += font->glyphs[cp].width;
	}
	al_hold_bitmap_drawing(is_draw_held);
}

void
init_font_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "GetSystemFont", js_GetSystemFont);
	register_api_func(ctx, NULL, "LoadFont", js_LoadFont);
}

void
duk_push_sphere_font(duk_context* ctx, font_t* font)
{
	ref_font(font);
	
	duk_push_object(ctx);
	duk_push_c_function(ctx, js_Font_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_Font_toString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "toString");
	duk_push_c_function(ctx, js_Font_clone, DUK_VARARGS); duk_put_prop_string(ctx, -2, "clone");
	duk_push_c_function(ctx, js_Font_getCharacterImage, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getCharacterImage");
	duk_push_c_function(ctx, js_Font_getColorMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getColorMask");
	duk_push_c_function(ctx, js_Font_getHeight, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getHeight");
	duk_push_c_function(ctx, js_Font_setCharacterImage, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setCharacterImage");
	duk_push_c_function(ctx, js_Font_setColorMask, DUK_VARARGS); duk_put_prop_string(ctx, -2, "setColorMask");
	duk_push_c_function(ctx, js_Font_drawText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawText");
	duk_push_c_function(ctx, js_Font_drawTextBox, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawTextBox");
	duk_push_c_function(ctx, js_Font_drawZoomedText, DUK_VARARGS); duk_put_prop_string(ctx, -2, "drawZoomedText");
	duk_push_c_function(ctx, js_Font_getStringHeight, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getStringHeight");
	duk_push_c_function(ctx, js_Font_getStringWidth, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getStringWidth");
	duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS); duk_put_prop_string(ctx, -2, "wordWrapString");
	
	duk_push_string(ctx, "font"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_pointer(ctx, font); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_sphere_color(ctx, al_map_rgba(255, 255, 255, 255)); duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
}

font_t*
duk_require_sphere_font(duk_context* ctx, duk_idx_t index)
{
	font_t*     font;
	const char* type;

	index = duk_require_normalize_index(ctx, index);
	duk_require_object_coercible(ctx, index);
	if (!duk_get_prop_string(ctx, index, "\xFF" "sphere_type"))
		goto on_error;
	type = duk_get_string(ctx, -1); duk_pop(ctx);
	if (strcmp(type, "font") != 0) goto on_error;
	duk_get_prop_string(ctx, index, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	return font;

on_error:
	duk_error(ctx, DUK_ERR_TYPE_ERROR, "Not a Sphere font");
}

static duk_ret_t
js_GetSystemFont(duk_context* ctx)
{
	duk_push_sphere_font(ctx, g_sys_font);
	return 1;
}

static duk_ret_t
js_LoadFont(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);
	
	font_t* font;
	
	char* path = get_asset_path(filename, "fonts", false);
	font = load_font(path);
	free(path);
	if (font == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "LoadFont(): Failed to load font file '%s'", filename);
	duk_push_sphere_font(ctx, font);
	free_font(font);
	return 1;
}

static duk_ret_t
js_Font_finalize(duk_context* ctx)
{
	font_t* font;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_font(font);
	return 0;
}

static duk_ret_t
js_Font_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object font]");
	return 1;
}

static duk_ret_t
js_Font_clone(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	// TODO: actually clone font in Font:clone()
	duk_push_sphere_font(ctx, font);
	return 1;
}

static duk_ret_t
js_Font_getCharacterImage(duk_context* ctx)
{
	int cp = duk_require_int(ctx, 0);

	font_t* font;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_sphere_image(ctx, get_glyph_image(font, cp));
	return 1;
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
	font_t* font;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, get_font_line_height(font));
	return 1;
}

static duk_ret_t
js_Font_setCharacterImage(duk_context* ctx)
{
	int cp = duk_require_int(ctx, 0);
	image_t* image = duk_require_sphere_image(ctx, 1);

	font_t* font;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	set_glyph_image(font, cp, image);
	return 0;
}

static duk_ret_t
js_Font_setColorMask(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "color_mask"); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Font_drawText(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	const char* text = duk_to_string(ctx, 2);
	
	font_t*       font;
	ALLEGRO_COLOR mask;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame()) draw_text(font, mask, x, y, TEXT_ALIGN_LEFT, text);
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
	font_t*         font;
	ALLEGRO_COLOR   mask;
	int             text_w, text_h;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame()) {
		text_w = get_text_width(font, text);
		text_h = get_font_line_height(font);
		bitmap = al_create_bitmap(text_w, text_h);
		al_set_target_bitmap(bitmap);
		draw_text(font, mask, 0, 0, TEXT_ALIGN_LEFT, text);
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
	int offset = duk_require_int(ctx, 4);
	const char* text = duk_to_string(ctx, 5);

	font_t*       font;
	int           line_height;
	const char*   line_text;
	ALLEGRO_COLOR mask;
	int           num_lines;

	int i;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask"); mask = duk_require_sphere_color(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (!is_skipped_frame()) {
		duk_push_c_function(ctx, js_Font_wordWrapString, DUK_VARARGS);
		duk_push_this(ctx);
		duk_push_string(ctx, text);
		duk_push_int(ctx, w);
		duk_call_method(ctx, 2);
		duk_get_prop_string(ctx, -1, "length"); num_lines = duk_get_int(ctx, -1); duk_pop(ctx);
		line_height = get_font_line_height(font);
		for (i = 0; i < num_lines; ++i) {
			duk_get_prop_index(ctx, -1, i); line_text = duk_get_string(ctx, -1); duk_pop(ctx);
			draw_text(font, mask, x + offset, y, TEXT_ALIGN_LEFT, line_text);
			y += line_height;
		}
		duk_pop(ctx);
	}
	return 0;
}

static duk_ret_t
js_Font_getStringHeight(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int width = duk_require_int(ctx, 1);
	
	font_t* font;
	int     num_lines;

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
	duk_push_int(ctx, get_font_line_height(font) * num_lines);
	return 1;
}

static duk_ret_t
js_Font_getStringWidth(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	
	font_t* font;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	duk_push_int(ctx, get_text_width(font, text));
	return 1;
}

static duk_ret_t
js_Font_wordWrapString(duk_context* ctx)
{
	char* text = strdup(duk_to_string(ctx, 0));
	int   width = duk_require_int(ctx, 1);
	
	font_t* font;
	int     line_width;
	int     line_idx;
	int     num_words;
	int     space_width;
	char*   word;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); font = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	space_width = get_text_width(font, " ");
	duk_push_array(ctx);
	line_idx = 0;
	line_width = 0;
	num_words = 0;
	text = strdup(text);
	word = strtok(text, " ");
	while (word != NULL) {
		line_width += get_text_width(font, word);
		if (line_width > width) {
			duk_concat(ctx, num_words);
			duk_put_prop_index(ctx, -2, line_idx);
			line_width = get_text_width(font, word);
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
