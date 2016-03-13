#include "minisphere.h"
#include "api.h"
#include "atlas.h"
#include "color.h"
#include "image.h"
#include "unicode.h"

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

static void update_font_metrics (font_t* font);

struct font
{
	unsigned int       refcount;
	unsigned int       id;
	int                height;
	int                min_width;
	int                max_width;
	uint32_t           num_glyphs;
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
	char     signature[4];
	uint16_t version;
	uint16_t num_chars;
	char     reserved[248];
};

struct rfn_glyph_header
{
	uint16_t width;
	uint16_t height;
	char     reserved[28];
};
#pragma pack(pop)

static unsigned int s_next_font_id = 0;

font_t*
load_font(const char* filename)
{
	image_t*                atlas = NULL;
	int                     atlas_x, atlas_y;
	int                     atlas_size_x, atlas_size_y;
	sfs_file_t*             file;
	font_t*                 font = NULL;
	struct font_glyph*      glyph;
	struct rfn_glyph_header glyph_hdr;
	long                    glyph_start;
	uint8_t*                grayscale;
	image_lock_t*           lock = NULL;
	int                     max_x = 0, max_y = 0;
	int                     min_width = INT_MAX;
	int64_t                 n_glyphs_per_row;
	int                     pixel_size;
	struct rfn_header       rfn;
	uint8_t                 *psrc;
	color_t                 *pdest;

	int i, x, y;
	
	console_log(2, "loading font #%u as '%s'", s_next_font_id, filename);
	
	memset(&rfn, 0, sizeof(struct rfn_header));

	if ((file = sfs_fopen(g_fs, filename, NULL, "rb")) == NULL) goto on_error;
	if (!(font = calloc(1, sizeof(font_t)))) goto on_error;
	if (sfs_fread(&rfn, sizeof(struct rfn_header), 1, file) != 1)
		goto on_error;
	pixel_size = (rfn.version == 1) ? 1 : 4;
	if (!(font->glyphs = calloc(rfn.num_chars, sizeof(struct font_glyph))))
		goto on_error;

	// pass 1: load glyph headers and find largest glyph
	glyph_start = sfs_ftell(file);
	for (i = 0; i < rfn.num_chars; ++i) {
		glyph = &font->glyphs[i];
		if (sfs_fread(&glyph_hdr, sizeof(struct rfn_glyph_header), 1, file) != 1)
			goto on_error;
		sfs_fseek(file, glyph_hdr.width * glyph_hdr.height * pixel_size, SFS_SEEK_CUR);
		max_x = fmax(glyph_hdr.width, max_x);
		max_y = fmax(glyph_hdr.height, max_y);
		min_width = fmin(min_width, glyph_hdr.width);
		glyph->width = glyph_hdr.width;
		glyph->height = glyph_hdr.height;
	}
	font->num_glyphs = rfn.num_chars;
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
	sfs_fseek(file, glyph_start, SFS_SEEK_SET);
	if (!(lock = lock_image(atlas))) goto on_error;
	for (i = 0; i < rfn.num_chars; ++i) {
		glyph = &font->glyphs[i];
		if (sfs_fread(&glyph_hdr, sizeof(struct rfn_glyph_header), 1, file) != 1)
			goto on_error;
		atlas_x = i % n_glyphs_per_row * max_x;
		atlas_y = i / n_glyphs_per_row * max_y;
		switch (rfn.version) {
		case 1: // RFN v1: 8-bit grayscale glyphs
			if (!(glyph->image = create_subimage(atlas, atlas_x, atlas_y, glyph_hdr.width, glyph_hdr.height)))
				goto on_error;
			grayscale = malloc(glyph_hdr.width * glyph_hdr.height);
			if (sfs_fread(grayscale, glyph_hdr.width * glyph_hdr.height, 1, file) != 1)
				goto on_error;
			psrc = grayscale;
			pdest = lock->pixels + atlas_x + atlas_y * lock->pitch;
			for (y = 0; y < glyph_hdr.height; ++y) {
				for (x = 0; x < glyph_hdr.width; ++x)
					pdest[x] = rgba(psrc[x], psrc[x], psrc[x], 255);
				pdest += lock->pitch;
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
	sfs_fclose(file);
	free_image(atlas);
	
	font->id = s_next_font_id++;
	return ref_font(font);

on_error:
	console_log(2, "failed to load font #%u", s_next_font_id++);
	sfs_fclose(file);
	if (font != NULL) {
		for (i = 0; i < rfn.num_chars; ++i) {
			if (font->glyphs[i].image != NULL) free_image(font->glyphs[i].image);
		}
		free(font->glyphs);
		free(font);
	}
	if (lock != NULL) unlock_image(atlas, lock);
	if (atlas != NULL) free_image(atlas);
	return NULL;
}

font_t*
clone_font(const font_t* src_font)
{
	font_t*                  font = NULL;
	int                      max_x = 0, max_y = 0;
	int                      min_width = INT_MAX;
	const struct font_glyph* src_glyph;

	uint32_t i;

	console_log(2, "cloning font #%u from source font #%u", s_next_font_id, src_font->id);
	
	if (!(font = calloc(1, sizeof(font_t)))) goto on_error;
	if (!(font->glyphs = calloc(src_font->num_glyphs, sizeof(struct font_glyph))))
		goto on_error;
	font->num_glyphs = src_font->num_glyphs;

	// perform the clone
	get_font_metrics(src_font, &min_width, &max_x, &max_y);
	font->height = max_y;
	font->min_width = min_width;
	font->max_width = max_x;
	for (i = 0; i < src_font->num_glyphs; ++i) {
		src_glyph = &src_font->glyphs[i];
		font->glyphs[i].image = ref_image(src_glyph->image);
		font->glyphs[i].width = src_glyph->width;
		font->glyphs[i].height = src_glyph->height;
	}

	font->id = s_next_font_id++;
	return ref_font(font);

on_error:
	if (font != NULL) {
		for (i = 0; i < font->num_glyphs; ++i) {
			if (font->glyphs[i].image != NULL)
				free_image(font->glyphs[i].image);
		}
		free(font->glyphs);
		free(font);
	}
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
	uint32_t i;
	
	if (font == NULL || --font->refcount > 0)
		return;
	
	console_log(3, "disposing font #%u no longer in use", font->id);
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
get_glyph_width(const font_t* font, int codepoint)
{
	return font->glyphs[codepoint].width;
}

int
get_text_width(const font_t* font, const char* text)
{
	uint8_t  ch_byte;
	uint32_t cp;
	uint32_t utf8state;
	int      width = 0;
	
	for (;;) {
		utf8state = UTF8_ACCEPT;
		while (utf8decode(&utf8state, &cp, ch_byte = *text++) > UTF8_REJECT);
		if (utf8state == UTF8_REJECT && ch_byte == '\0')
			--text;  // don't eat NUL terminator
		cp = cp == 0x20AC ? 128
			: cp == 0x201A ? 130
			: cp == 0x0192 ? 131
			: cp == 0x201E ? 132
			: cp == 0x2026 ? 133
			: cp == 0x2020 ? 134
			: cp == 0x2021 ? 135
			: cp == 0x02C6 ? 136
			: cp == 0x2030 ? 137
			: cp == 0x0160 ? 138
			: cp == 0x2039 ? 139
			: cp == 0x0152 ? 140
			: cp == 0x017D ? 142
			: cp == 0x2018 ? 145
			: cp == 0x2019 ? 146
			: cp == 0x201C ? 147
			: cp == 0x201D ? 148
			: cp == 0x2022 ? 149
			: cp == 0x2013 ? 150
			: cp == 0x2014 ? 151
			: cp == 0x02DC ? 152
			: cp == 0x2122 ? 153
			: cp == 0x0161 ? 154
			: cp == 0x203A ? 155
			: cp == 0x0153 ? 156
			: cp == 0x017E ? 158
			: cp == 0x0178 ? 159
			: cp;
		cp = utf8state == UTF8_ACCEPT
			? cp < (uint32_t)font->num_glyphs ? cp : 0x1A
			: 0x1A;
		if (cp == '\0') break;
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
	update_font_metrics(font);
	free_image(old_image);
}

void
draw_text(const font_t* font, color_t color, int x, int y, text_align_t alignment, const char* text)
{
	bool     is_draw_held;
	uint8_t  ch_byte;
	uint32_t cp;
	int      tab_width;
	uint32_t utf8state;
	
	if (alignment == TEXT_ALIGN_CENTER)
		x -= get_text_width(font, text) / 2;
	else if (alignment == TEXT_ALIGN_RIGHT)
		x -= get_text_width(font, text);
	
	tab_width = font->glyphs[' '].width * 3;
	is_draw_held = al_is_bitmap_drawing_held();
	al_hold_bitmap_drawing(true);
	for (;;) {
		utf8state = UTF8_ACCEPT;
		while (utf8decode(&utf8state, &cp, ch_byte = *text++) > UTF8_REJECT);
		if (utf8state == UTF8_REJECT && ch_byte == '\0')
			--text;  // don't eat NUL terminator
		cp = cp == 0x20AC ? 128
			: cp == 0x201A ? 130
			: cp == 0x0192 ? 131
			: cp == 0x201E ? 132
			: cp == 0x2026 ? 133
			: cp == 0x2020 ? 134
			: cp == 0x2021 ? 135
			: cp == 0x02C6 ? 136
			: cp == 0x2030 ? 137
			: cp == 0x0160 ? 138
			: cp == 0x2039 ? 139
			: cp == 0x0152 ? 140
			: cp == 0x017D ? 142
			: cp == 0x2018 ? 145
			: cp == 0x2019 ? 146
			: cp == 0x201C ? 147
			: cp == 0x201D ? 148
			: cp == 0x2022 ? 149
			: cp == 0x2013 ? 150
			: cp == 0x2014 ? 151
			: cp == 0x02DC ? 152
			: cp == 0x2122 ? 153
			: cp == 0x0161 ? 154
			: cp == 0x203A ? 155
			: cp == 0x0153 ? 156
			: cp == 0x017E ? 158
			: cp == 0x0178 ? 159
			: cp;
		cp = utf8state == UTF8_ACCEPT
			? cp < (uint32_t)font->num_glyphs ? cp : 0x1A
			: 0x1A;
		if (cp == '\0')
			break;
		else if (cp == '\t')
			x += tab_width;
		else {
			draw_image_masked(font->glyphs[cp].image, color, x, y);
			x += font->glyphs[cp].width;
		}
	}
	al_hold_bitmap_drawing(is_draw_held);
}

wraptext_t*
word_wrap_text(const font_t* font, const char* text, int width)
{
	char*       buffer = NULL;
	uint8_t     ch_byte;
	char*		carry;
	size_t      ch_size;
	uint32_t    cp;
	int         glyph_width;
	bool        is_line_end = false;
	int         line_idx;
	int         line_width;
	int         max_lines = 10;
	char*       last_break;
	char*       last_space;
	char*       last_tab;
	char*       line_buffer;
	size_t      line_length;
	char*       new_buffer;
	size_t      pitch;
	uint32_t    utf8state;
	wraptext_t* wraptext;
	const char  *p, *start;

	if (!(wraptext = calloc(1, sizeof(wraptext_t)))) goto on_error;
	
	// allocate initial buffer
	get_font_metrics(font, &glyph_width, NULL, NULL);
	pitch = 4 * (glyph_width > 0 ? width / glyph_width : width) + 3;
	if (!(buffer = malloc(max_lines * pitch))) goto on_error;
	if (!(carry = malloc(pitch))) goto on_error;

	// run through one character at a time, carrying as necessary
	line_buffer = buffer; line_buffer[0] = '\0';
	line_idx = 0; line_width = 0; line_length = 0;
	memset(line_buffer, 0, pitch);  // fill line with NULs
	p = text;
	do {
		utf8state = UTF8_ACCEPT; start = p;
		while (utf8decode(&utf8state, &cp, ch_byte = *p++) > UTF8_REJECT);
		if (utf8state == UTF8_REJECT && ch_byte == '\0')
			--p;  // don't eat NUL terminator
		ch_size = p - start;
		cp = cp == 0x20AC ? 128
			: cp == 0x201A ? 130
			: cp == 0x0192 ? 131
			: cp == 0x201E ? 132
			: cp == 0x2026 ? 133
			: cp == 0x2020 ? 134
			: cp == 0x2021 ? 135
			: cp == 0x02C6 ? 136
			: cp == 0x2030 ? 137
			: cp == 0x0160 ? 138
			: cp == 0x2039 ? 139
			: cp == 0x0152 ? 140
			: cp == 0x017D ? 142
			: cp == 0x2018 ? 145
			: cp == 0x2019 ? 146
			: cp == 0x201C ? 147
			: cp == 0x201D ? 148
			: cp == 0x2022 ? 149
			: cp == 0x2013 ? 150
			: cp == 0x2014 ? 151
			: cp == 0x02DC ? 152
			: cp == 0x2122 ? 153
			: cp == 0x0161 ? 154
			: cp == 0x203A ? 155
			: cp == 0x0153 ? 156
			: cp == 0x017E ? 158
			: cp == 0x0178 ? 159
			: cp;
		cp = utf8state == UTF8_ACCEPT
			? cp < (uint32_t)font->num_glyphs ? cp : 0x1A
			: 0x1A;
		switch (cp) {
		case '\n': case '\r':  // explicit newline
			if (cp == '\r' && *p == '\n') ++text;  // CRLF
			is_line_end = true;
			break;
		case '\t':  // tab
			line_buffer[line_length++] = cp;
			line_width += get_text_width(font, "   ");
			is_line_end = false;
			break;
		case '\0':  // NUL terminator
			is_line_end = line_length > 0;  // commit last line on EOT
			break;
		default:  // default case, copy character as-is
			memcpy(line_buffer + line_length, start, ch_size);
			line_length += ch_size;
			line_width += get_glyph_width(font, cp);
			is_line_end = false;
		}
		if (is_line_end) carry[0] = '\0';
		if (line_width > width || line_length >= pitch - 1) {
			// wrap width exceeded, carry current word to next line
			is_line_end = true;
			last_space = strrchr(line_buffer, ' ');
			last_tab = strrchr(line_buffer, '\t');
			last_break = last_space > last_tab ? last_space : last_tab;
			if (last_break != NULL)  // word break (space or tab) found
				strcpy(carry, last_break + 1);
			else  // no word break, so just carry last character
				sprintf(carry, "%c", line_buffer[line_length - 1]);
			line_buffer[line_length - strlen(carry)] = '\0';
		}
		if (is_line_end) {
			// do we need to enlarge the buffer?
			if (++line_idx >= max_lines) {
				max_lines *= 2;
				if (!(new_buffer = realloc(buffer, max_lines * pitch)))
					goto on_error;
				buffer = new_buffer;
				line_buffer = buffer + line_idx * pitch;
			}
			else
				line_buffer += pitch;
			
			memset(line_buffer, 0, pitch);  // fill line with NULs

			// copy carry text into new line
			line_width = get_text_width(font, carry);
			line_length = strlen(carry);
			strcpy(line_buffer, carry);
		}
	} while (cp != '\0');
	free(carry);
	wraptext->num_lines = line_idx;
	wraptext->buffer = buffer;
	wraptext->pitch = pitch;
	return wraptext;

on_error:
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

static void
update_font_metrics(font_t* font)
{
	int max_x = 0, max_y = 0;
	int min_width = INT_MAX;

	uint32_t i;

	for (i = 0; i < font->num_glyphs; ++i) {
		font->glyphs[i].width = get_image_width(font->glyphs[i].image);
		font->glyphs[i].height = get_image_height(font->glyphs[i].image);
		min_width = fmin(font->glyphs[i].width, min_width);
		max_x = fmax(font->glyphs[i].width, max_x);
		max_y = fmax(font->glyphs[i].height, max_y);
	}
	font->min_width = min_width;
	font->max_width = max_x;
	font->height = max_y;
}

void
init_font_api(duk_context* ctx)
{
	// Font API functions
	register_api_method(ctx, NULL, "GetSystemFont", js_GetSystemFont);
	
	// Font object
	register_api_method(ctx, NULL, "LoadFont", js_LoadFont);
	register_api_ctor(ctx, "Font", js_new_Font, js_Font_finalize);
	register_api_prop(ctx, "Font", "colorMask", js_Font_get_colorMask, js_Font_set_colorMask);
	register_api_prop(ctx, "Font", "height", js_Font_get_height, NULL);
	register_api_method(ctx, "Font", "getCharacterImage", js_Font_getCharacterImage);
	register_api_method(ctx, "Font", "getColorMask", js_Font_get_colorMask);
	register_api_method(ctx, "Font", "getHeight", js_Font_get_height);
	register_api_method(ctx, "Font", "getStringHeight", js_Font_getStringHeight);
	register_api_method(ctx, "Font", "getStringWidth", js_Font_getStringWidth);
	register_api_method(ctx, "Font", "setCharacterImage", js_Font_setCharacterImage);
	register_api_method(ctx, "Font", "setColorMask", js_Font_set_colorMask);
	register_api_method(ctx, "Font", "toString", js_Font_toString);
	register_api_method(ctx, "Font", "clone", js_Font_clone);
	register_api_method(ctx, "Font", "drawText", js_Font_drawText);
	register_api_method(ctx, "Font", "drawTextBox", js_Font_drawTextBox);
	register_api_method(ctx, "Font", "drawZoomedText", js_Font_drawZoomedText);
	register_api_method(ctx, "Font", "wordWrapString", js_Font_wordWrapString);
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
	const char* filename;
	font_t*     font;

	filename = duk_require_path(ctx, 0, "fonts", false);
	font = load_font(filename);
	if (font == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "LoadFont(): unable to load font file '%s'", filename);
	duk_push_sphere_font(ctx, font);
	free_font(font);
	return 1;
}

static duk_ret_t
js_new_Font(duk_context* ctx)
{
	const char* filename;
	font_t*     font;

	filename = duk_require_path(ctx, 0, NULL, false);
	font = load_font(filename);
	if (font == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Font(): unable to load font file '%s'", filename);
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
	font_t* dolly_font;
	font_t* font;

	duk_push_this(ctx);
	font = duk_require_sphere_obj(ctx, -1, "Font");
	duk_pop(ctx);
	if (!(dolly_font = clone_font(font)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Font:clone(): unable to clone font");
	duk_push_sphere_font(ctx, dolly_font);
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
	if (!screen_is_skipframe(g_screen))
		draw_text(font, mask, x, y, TEXT_ALIGN_LEFT, text);
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
	if (!screen_is_skipframe(g_screen)) {
		text_w = get_text_width(font, text);
		text_h = get_font_line_height(font);
		bitmap = al_create_bitmap(text_w, text_h);
		al_set_target_bitmap(bitmap);
		draw_text(font, mask, 0, 0, TEXT_ALIGN_LEFT, text);
		al_set_target_backbuffer(screen_display(g_screen));
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
	if (!screen_is_skipframe(g_screen)) {
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
