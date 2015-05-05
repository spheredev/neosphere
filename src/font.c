#include "minisphere.h"
#include "api.h"
#include "color.h"
#include "image.h"

#include "font.h"

static duk_ret_t js_GetSystemFont          (duk_context* ctx);
static duk_ret_t js_LoadFont               (duk_context* ctx);
static duk_ret_t js_new_Font               (duk_context* ctx);
static duk_ret_t js_Font_finalize          (duk_context* ctx);
static duk_ret_t js_Font_toString          (duk_context* ctx);
static duk_ret_t js_Font_get_colorMask     (duk_context* ctx);
static duk_ret_t js_Font_set_colorMask     (duk_context* ctx);
static duk_ret_t js_Font_get_height        (duk_context* ctx);
static duk_ret_t js_Font_getCharacterImage (duk_context* ctx);
static duk_ret_t js_Font_setCharacterImage (duk_context* ctx);
static duk_ret_t js_Font_getStringHeight   (duk_context* ctx);
static duk_ret_t js_Font_getStringWidth    (duk_context* ctx);
static duk_ret_t js_Font_clone             (duk_context* ctx);
static duk_ret_t js_Font_drawText          (duk_context* ctx);
static duk_ret_t js_Font_drawTextBox       (duk_context* ctx);
static duk_ret_t js_Font_drawZoomedText    (duk_context* ctx);
static duk_ret_t js_Font_wordWrapString    (duk_context* ctx);

struct font
{
	int                refcount;
	int                height;
	int                min_width;
	int                max_width;
	int                pitch;
	int                num_glyphs;
	struct font_glyph* glyphs;
};

struct font_glyph
{
	int      width, height;
	image_t* image;
};

struct wraptext
{
	int    num_lines;
	char*  buffer;
	size_t pitch;
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
	image_t*                atlas = NULL;
	int                     atlas_x, atlas_y;
	int                     atlas_size_x, atlas_size_y;
	FILE*                   file;
	font_t*                 font = NULL;
	struct font_glyph*      glyph;
	struct rfn_glyph_header glyph_hdr;
	long                    glyph_start;
	uint8_t*                grayscale;
	imagelock_t             lock;
	int                     max_x = 0, max_y = 0;
	int                     min_width = INT_MAX;
	int64_t                 n_glyphs_per_row;
	int                     pixel_size;
	struct rfn_header       rfn;
	uint8_t                 *psrc;
	color_t                 *pdest;

	int i, x, y;

	memset(&rfn, 0, sizeof(struct rfn_header));

	if ((file = fopen(path, "rb")) == NULL) goto on_error;
	if (!(font = calloc(1, sizeof(font_t)))) goto on_error;
	if (fread(&rfn, sizeof(struct rfn_header), 1, file) != 1)
		goto on_error;
	pixel_size = (rfn.version == 1) ? 1 : 4;
	if (!(font->glyphs = calloc(rfn.num_chars, sizeof(struct font_glyph))))
		goto on_error;

	// pass 1: load glyph headers and find largest glyph
	glyph_start = ftell(file);
	for (i = 0; i < rfn.num_chars; ++i) {
		glyph = &font->glyphs[i];
		if (fread(&glyph_hdr, sizeof(struct rfn_glyph_header), 1, file) != 1)
			goto on_error;
		fseek(file, glyph_hdr.width * glyph_hdr.height * pixel_size, SEEK_CUR);
		max_x = fmax(glyph_hdr.width, max_x);
		max_y = fmax(glyph_hdr.height, max_y);
		min_width = fmin(min_width, glyph_hdr.width);
		glyph->width = glyph_hdr.width;
		glyph->height = glyph_hdr.height;
	}
	font->min_width = min_width;
	font->max_width = max_x;
	font->height = max_y;

	// create glyph atlas
	n_glyphs_per_row = ceil(sqrt(rfn.num_chars));
	atlas_size_x = max_x * n_glyphs_per_row;
	atlas_size_y = max_y * n_glyphs_per_row;
	if ((atlas = create_image(atlas_size_x, atlas_size_y)) == NULL)
		goto on_error;

	// pass 2: load glyph data
	fseek(file, glyph_start, SEEK_SET);
	lock_image(atlas, &lock);
	for (i = 0; i < rfn.num_chars; ++i) {
		glyph = &font->glyphs[i];
		if (fread(&glyph_hdr, sizeof(struct rfn_glyph_header), 1, file) != 1)
			goto on_error;
		atlas_x = i % n_glyphs_per_row * max_x;
		atlas_y = i / n_glyphs_per_row * max_y;
		switch (rfn.version) {
		case 1: // RFN v1: 8-bit grayscale glyphs
			if (!(glyph->image = create_subimage(atlas, atlas_x, atlas_y, glyph_hdr.width, glyph_hdr.height)))
				goto on_error;
			grayscale = malloc(glyph_hdr.width * glyph_hdr.height);
			if (fread(grayscale, glyph_hdr.width * glyph_hdr.height, 1, file) != 1)
				goto on_error;
			psrc = grayscale;
			pdest = lock.pixels + atlas_x + atlas_y * lock.pitch;
			for (y = 0; y < glyph_hdr.height; ++y) {
				for (x = 0; x < glyph_hdr.width; ++x)
					pdest[x] = rgba(psrc[x], psrc[x], psrc[x], 255);
				pdest += lock.pitch;
				psrc += glyph_hdr.width;
			}
			break;
		case 2: // RFN v2: 32-bit truecolor glyphs
			if (!(glyph->image = read_subimage(file, atlas, atlas_x, atlas_y, glyph_hdr.width, glyph_hdr.height)))
				goto on_error;
			break;
		}
	}
	unlock_image(atlas, lock);
	fclose(file);
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
	int i;
	
	if (font == NULL || --font->refcount > 0)
		return;
	for (i = 0; i < font->num_glyphs; ++i) {
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

void
get_font_metrics(const font_t* font, int* out_min_width, int* out_max_width, int* out_line_height)
{
	if (out_min_width) *out_min_width = font->min_width;
	if (out_max_width) *out_max_width = font->max_width;
	if (out_line_height) *out_line_height = font->height;
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
draw_text(const font_t* font, color_t color, int x, int y, text_align_t alignment, const char* text)
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
		draw_image_masked(font->glyphs[cp].image, color, x, y);
		x += font->glyphs[cp].width;
	}
	al_hold_bitmap_drawing(is_draw_held);
}

wraptext_t*
word_wrap_text(const font_t* font, const char* text, int width)
{
	char*       buffer = NULL;
	int         glyph_width;
	int         line_idx;
	int         line_width;
	int         max_lines = 10;
	char*       line_buffer;
	char*       new_buffer;
	size_t      pitch;
	int         space_width = get_text_width(font, " ");
	char*       string = NULL;
	char*       word;
	wraptext_t* wraptext;

	if (!(wraptext = calloc(1, sizeof(wraptext_t)))) goto on_error;
	
	// allocate initial buffer
	get_font_metrics(font, &glyph_width, NULL, NULL);
	pitch = glyph_width > 0 ? width / glyph_width + 2 : width;
	if (!(buffer = malloc(max_lines * pitch))) goto on_error;
	
	// run through string one word at a time, wrapping as necessary
	line_buffer = buffer; line_buffer[0] = '\0';
	line_idx = 0; line_width = 0;
	string = strdup(text);
	word = strtok(string, " ");
	while (word != NULL) {
		line_width += get_text_width(font, word);
		if (line_width > width) {  // time for a new line?
			if (++line_idx >= max_lines) {  // enlarge the buffer?
				max_lines *= 2;
				if (!(new_buffer = realloc(buffer, max_lines * pitch)))
					goto on_error;
				buffer = new_buffer;
				line_buffer = buffer + line_idx * pitch;
			}
			else
				line_buffer += pitch;
			line_width = get_text_width(font, word);
			line_buffer[0] = '\0';
		}
		strcat(line_buffer, word);
		word = strtok(NULL, " ");
		if (word != NULL) {
			strcat(line_buffer, " ");
			line_width += space_width;
		}
	}
	free(string);
	wraptext->num_lines = line_idx + 1;
	wraptext->buffer = buffer;
	wraptext->pitch = pitch;
	return wraptext;

on_error:
	free(string);
	free(buffer);
	free(wraptext);
	return NULL;
}

void
free_wraptext(wraptext_t* wraptext)
{
	free(wraptext->buffer);
	free(wraptext);
}

const char*
get_wraptext_line(const wraptext_t* wraptext, int line_index)
{
	return wraptext->buffer + line_index * wraptext->pitch;
}

int
get_wraptext_line_count(const wraptext_t* wraptext)
{
	return wraptext->num_lines;
}

void
init_font_api(duk_context* ctx)
{
	// Font API functions
	register_api_function(ctx, NULL, "GetSystemFont", js_GetSystemFont);
	
	// Font object
	register_api_function(ctx, NULL, "LoadFont", js_LoadFont);
	register_api_ctor(ctx, "Font", js_new_Font, js_Font_finalize);
	register_api_prop(ctx, "Font", "colorMask", js_Font_get_colorMask, js_Font_set_colorMask);
	register_api_prop(ctx, "Font", "height", js_Font_get_height, NULL);
	register_api_function(ctx, "Font", "getCharacterImage", js_Font_getCharacterImage);
	register_api_function(ctx, "Font", "getColorMask", js_Font_get_colorMask);
	register_api_function(ctx, "Font", "getHeight", js_Font_get_height);
	register_api_function(ctx, "Font", "getStringHeight", js_Font_getStringHeight);
	register_api_function(ctx, "Font", "getStringWidth", js_Font_getStringWidth);
	register_api_function(ctx, "Font", "setCharacterImage", js_Font_setCharacterImage);
	register_api_function(ctx, "Font", "setColorMask", js_Font_set_colorMask);
	register_api_function(ctx, "Font", "toString", js_Font_toString);
	register_api_function(ctx, "Font", "clone", js_Font_clone);
	register_api_function(ctx, "Font", "drawText", js_Font_drawText);
	register_api_function(ctx, "Font", "drawTextBox", js_Font_drawTextBox);
	register_api_function(ctx, "Font", "drawZoomedText", js_Font_drawZoomedText);
	register_api_function(ctx, "Font", "wordWrapString", js_Font_wordWrapString);
}

void
duk_push_sphere_font(duk_context* ctx, font_t* font)
{
	duk_push_sphere_obj(ctx, "Font", ref_font(font));
	duk_push_sphere_color(ctx, rgba(255, 255, 255, 255));
	duk_put_prop_string(ctx, -2, "\xFF" "color_mask");
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
	duk_require_string(ctx, 0);
	
	js_new_Font(ctx);
	return 1;
}

static duk_ret_t
js_new_Font(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	font_t* font;

	char* path = get_asset_path(filename, "fonts", false);
	font = load_font(path);
	free(path);
	if (font == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Font(): Failed to load font file '%s'", filename);
	duk_push_sphere_font(ctx, font);
	free_font(font);
	return 1;
}

static duk_ret_t
js_Font_finalize(duk_context* ctx)
{
	font_t* font;

	font = duk_require_sphere_obj(ctx, 0, "Font");
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
js_Font_getCharacterImage(duk_context* ctx)
{
	int cp = duk_require_int(ctx, 0);

	font_t* font;
	
	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	duk_push_sphere_image(ctx, get_glyph_image(font, cp));
	return 1;
}

static duk_ret_t
js_Font_get_colorMask(duk_context* ctx)
{
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "color_mask");
	duk_remove(ctx, -2);
	return 1;
}

static duk_ret_t
js_Font_set_colorMask(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_dup(ctx, 0); duk_put_prop_string(ctx, -2, "\xFF" "color_mask"); duk_pop(ctx);
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Font_get_height(duk_context* ctx)
{
	font_t* font;
	
	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
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
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	set_glyph_image(font, cp, image);
	return 0;
}

static duk_ret_t
js_Font_clone(duk_context* ctx)
{
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	// TODO: actually clone font in Font:clone()
	duk_push_sphere_font(ctx, font);
	return 1;
}

static duk_ret_t
js_Font_drawText(duk_context* ctx)
{
	int x = duk_require_int(ctx, 0);
	int y = duk_require_int(ctx, 1);
	const char* text = duk_to_string(ctx, 2);
	
	font_t* font;
	color_t mask;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
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
	color_t         mask;
	int             text_w, text_h;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
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

	font_t*     font;
	int         line_height;
	const char* line_text;
	color_t     mask;
	int         num_lines;

	int i;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
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
	font = duk_require_sphere_obj(ctx, -1, "Font");
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
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	duk_push_int(ctx, get_text_width(font, text));
	return 1;
}

static duk_ret_t
js_Font_wordWrapString(duk_context* ctx)
{
	const char* text = duk_to_string(ctx, 0);
	int         width = duk_require_int(ctx, 1);
	
	font_t*     font;
	int         num_lines;
	wraptext_t* wraptext;

	int i;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	wraptext = word_wrap_text(font, text, width);
	num_lines = get_wraptext_line_count(wraptext);
	duk_push_array(ctx);
	for (i = 0; i < num_lines; ++i) {
		duk_push_string(ctx, get_wraptext_line(wraptext, i));
		duk_put_prop_index(ctx, -2, i);
	}
	free_wraptext(wraptext);
	return 1;
}
