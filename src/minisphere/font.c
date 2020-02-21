/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2020, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "minisphere.h"
#include "font.h"

#include "color.h"
#include "image.h"
#include "unicode.h"

static bool do_multiline_text_line (int line_idx, const char* line, int size, void* userdata);
static void update_font_metrics    (font_t* font);

struct font
{
	unsigned int  refcount;
	unsigned int  id;
	color_t       color_mask;
	int           height;
	int           max_width;
	int           min_width;
	bool          modified;
	char*         path;
	uint32_t      num_glyphs;
	struct glyph* glyphs;
};

struct glyph
{
	int      width, height;
	image_t* image;
};

struct ttf
{
	unsigned int  refcount;
	unsigned int  id;
	char*         path;
	font_t*       rfn_font;
	ALLEGRO_FONT* ttf_font;
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

static unsigned int s_next_font_id = 1;

font_t*
font_load(const char* filename)
{
	image_t*                atlas = NULL;
	int                     atlas_x, atlas_y;
	int                     atlas_size_x, atlas_size_y;
	file_t*                 file;
	font_t*                 font = NULL;
	struct glyph*           glyph;
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

	console_log(2, "loading font #%u from '%s'", s_next_font_id, filename);

	memset(&rfn, 0, sizeof(struct rfn_header));

	if ((file = file_open(g_game, filename, "rb")) == NULL)
		goto on_error;
	if (!(font = calloc(1, sizeof(font_t))))
		goto on_error;
	if (file_read(file, &rfn, 1, sizeof(struct rfn_header)) != 1)
		goto on_error;
	pixel_size = (rfn.version == 1) ? 1 : 4;
	if (!(font->glyphs = calloc(rfn.num_chars, sizeof(struct glyph))))
		goto on_error;

	// pass 1: load glyph headers and find largest glyph
	glyph_start = file_position(file);
	for (i = 0; i < rfn.num_chars; ++i) {
		glyph = &font->glyphs[i];
		if (file_read(file, &glyph_hdr, 1, sizeof(struct rfn_glyph_header)) != 1)
			goto on_error;
		file_seek(file, glyph_hdr.width * glyph_hdr.height * pixel_size, WHENCE_CUR);
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
	if ((atlas = image_new(atlas_size_x, atlas_size_y, NULL)) == NULL)
		goto on_error;

	// pass 2: load glyph data
	file_seek(file, glyph_start, WHENCE_SET);
	if (!(lock = image_lock(atlas, true, false)))
		goto on_error;
	for (i = 0; i < rfn.num_chars; ++i) {
		glyph = &font->glyphs[i];
		if (file_read(file, &glyph_hdr, 1, sizeof(struct rfn_glyph_header)) != 1)
			goto on_error;
		atlas_x = i % n_glyphs_per_row * max_x;
		atlas_y = i / n_glyphs_per_row * max_y;
		switch (rfn.version) {
		case 1: // RFN v1: 8-bit grayscale glyphs
			if (!(glyph->image = image_new_slice(atlas, atlas_x, atlas_y, glyph_hdr.width, glyph_hdr.height)))
				goto on_error;
			grayscale = malloc(glyph_hdr.width * glyph_hdr.height);
			if (file_read(file, grayscale, 1, glyph_hdr.width * glyph_hdr.height) != 1)
				goto on_error;
			psrc = grayscale;
			pdest = lock->pixels + atlas_x + atlas_y * lock->pitch;
			for (y = 0; y < glyph_hdr.height; ++y) {
				for (x = 0; x < glyph_hdr.width; ++x)
					pdest[x] = mk_color(255, 255, 255, psrc[x]);
				pdest += lock->pitch;
				psrc += glyph_hdr.width;
			}
			break;
		case 2: // RFN v2: 32-bit truecolor glyphs
			if (!(glyph->image = fread_image_slice(file, atlas, atlas_x, atlas_y, glyph_hdr.width, glyph_hdr.height)))
				goto on_error;
			break;
		}
	}
	image_unlock(atlas, lock);
	file_close(file);
	image_unref(atlas);

	font->id = s_next_font_id++;
	font->color_mask = mk_color(255, 255, 255, 255);
	font->path = strdup(filename);
	return font_ref(font);

on_error:
	console_log(2, "failed to load font #%u", s_next_font_id++);
	file_close(file);
	if (font != NULL) {
		if (font->glyphs != NULL) {
			for (i = 0; i < rfn.num_chars; ++i)
				image_unref(font->glyphs[i].image);
		}
		free(font->glyphs);
	}
	free(font);
	if (lock != NULL) image_unlock(atlas, lock);
	if (atlas != NULL) image_unref(atlas);
	return NULL;
}

font_t*
font_clone(const font_t* it)
{
	font_t*             dolly = NULL;
	int                 max_x = 0;
	int                 max_y = 0;
	int                 min_width = INT_MAX;
	const struct glyph* src_glyph;

	uint32_t i;

	if (it == NULL)
		return NULL;
	
	console_log(2, "cloning font #%u from source font #%u", s_next_font_id, it->id);

	if (!(dolly = calloc(1, sizeof(font_t))))
		goto on_error;
	if (!(dolly->glyphs = calloc(it->num_glyphs, sizeof(struct glyph))))
		goto on_error;
	dolly->num_glyphs = it->num_glyphs;

	// perform the clone
	font_get_metrics(it, &min_width, &max_x, &max_y);
	dolly->color_mask = it->color_mask;
	dolly->height = max_y;
	dolly->min_width = min_width;
	dolly->max_width = max_x;
	for (i = 0; i < it->num_glyphs; ++i) {
		src_glyph = &it->glyphs[i];
		dolly->glyphs[i].image = image_ref(src_glyph->image);
		dolly->glyphs[i].width = src_glyph->width;
		dolly->glyphs[i].height = src_glyph->height;
	}

	dolly->id = s_next_font_id++;
	return font_ref(dolly);

on_error:
	if (dolly != NULL) {
		for (i = 0; i < dolly->num_glyphs; ++i) {
			if (dolly->glyphs[i].image != NULL)
				image_unref(dolly->glyphs[i].image);
		}
		free(dolly->glyphs);
		free(dolly);
	}
	return NULL;
}

font_t*
font_ref(font_t* it)
{
	++it->refcount;
	return it;
}

void
font_unref(font_t* it)
{
	uint32_t i;

	if (it == NULL || --it->refcount > 0)
		return;

	console_log(3, "disposing font #%u no longer in use", it->id);
	for (i = 0; i < it->num_glyphs; ++i)
		image_unref(it->glyphs[i].image);
	free(it->glyphs);
	free(it);
}

image_t*
font_glyph(const font_t* it, uint32_t cp)
{
	return it->glyphs[cp].image;
}

int
font_height(const font_t* it)
{
	return it->height;
}

const char*
font_path(const font_t* it)
{
	return it->path;
}

color_t
font_get_mask(const font_t* it)
{
	return it->color_mask;
}

void
font_set_glyph(font_t* it, uint32_t cp, image_t* image)
{
	image_t* old_image;

	old_image = it->glyphs[cp].image;
	it->glyphs[cp].image = image_ref(image);
	image_unref(old_image);
	it->modified = true;
}

void
font_set_mask(font_t* it, color_t color)
{
	it->color_mask = color;
}

void
font_draw_text(font_t* it, int x, int y, text_align_t alignment, const char* text)
{
	uint32_t       cp;
	utf8_ret_t     ret;
	int            tab_width;
	utf8_decode_t* utf8;

	if (it->modified)
		update_font_metrics(it);

	if (alignment == TEXT_ALIGN_CENTER)
		x -= font_get_width(it, text) / 2;
	else if (alignment == TEXT_ALIGN_RIGHT)
		x -= font_get_width(it, text);

	tab_width = it->glyphs[' '].width * 3;
	al_hold_bitmap_drawing(true);
	utf8 = utf8_decode_start(true);
	do {
		while ((ret = utf8_decode_next(utf8, *text++, &cp)) == UTF8_CONTINUE);
		if (ret == UTF8_RETRY)
			--text;
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
		cp = ret == UTF8_CODEPOINT
			? cp < it->num_glyphs ? cp : 0x1A
			: 0x1A;
		if (cp == '\t')
			x += tab_width;
		else if (cp != '\0') {
			image_draw_masked(it->glyphs[cp].image, it->color_mask, x, y);
			x += it->glyphs[cp].width;
		}
	} while (cp != '\0');
	utf8_decode_end(utf8);
	al_hold_bitmap_drawing(false);
}

void
font_get_metrics(const font_t* it, int* out_min_width, int* out_max_width, int* out_line_height)
{
	if (out_min_width != NULL)
		*out_min_width = it->min_width;
	if (out_max_width != NULL)
		*out_max_width = it->max_width;
	if (out_line_height != NULL)
		*out_line_height = it->height;
}

int
font_get_width(const font_t* it, const char* text)
{
	uint32_t       cp;
	utf8_ret_t     ret;
	utf8_decode_t* utf8;
	int            width = 0;

	utf8 = utf8_decode_start(true);
	do {
		while ((ret = utf8_decode_next(utf8, *text++, &cp)) == UTF8_CONTINUE);
		if (ret == UTF8_RETRY)
			--text;
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
		cp = ret == UTF8_CODEPOINT
			? cp < it->num_glyphs ? cp : 0x1A
			: 0x1A;
		if (cp != '\0')
			width += it->glyphs[cp].width;
	} while (cp != '\0');
	utf8_decode_end(utf8);
	return width;
}

ttf_t*
ttf_open(const char* path, int size, bool auto_kern, bool antialias)
{
	size_t        file_size;
	int           flags = 0x0;
	struct ttf*   font;
	ALLEGRO_FILE* memfile = NULL;
	font_t*       rfn_font;
	void*         slurp;

	// check if the font can be loaded as RFN first
	rfn_font = font_load(path);
	
	if (!(font = calloc(1, sizeof(ttf_t))))
		goto on_error;
	if (rfn_font == NULL) {
		if (!(slurp = game_read_file(g_game, path, &file_size)))
			goto on_error;
		memfile = al_open_memfile(slurp, file_size, "rb");
		if (!auto_kern)
			flags |= ALLEGRO_TTF_NO_KERNING;
		if (!antialias)
			flags |= ALLEGRO_TTF_MONOCHROME;
		if (!(font->ttf_font = al_load_ttf_font_f(memfile, NULL, size, flags)))
			goto on_error;
	}
	else {
		font->rfn_font = rfn_font;
	}
	return ttf_ref(font);

on_error:
	if (memfile != NULL)
		al_fclose(memfile);
	free(font);
	return NULL;
}

ttf_t*
ttf_ref(ttf_t* it)
{
	++it->refcount;
	return it;
}

void
ttf_unref(ttf_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;

	console_log(3, "disposing TTF font #%u no longer in use", it->id);
	if (it->ttf_font != NULL)
		al_destroy_font(it->ttf_font);
	else
		font_unref(it->rfn_font);
	free(it);
}

const char*
ttf_path(const ttf_t* it)
{
	return it->path;
}

void
ttf_draw_text(const ttf_t* it, int x, int y, const char* text, color_t color)
{
	if (it->ttf_font != NULL) {
		al_draw_text(it->ttf_font, nativecolor(color), x, y, 0x0, text);
	}
	else {
		// legacy RFN font mode
		font_set_mask(it->rfn_font, color);
		font_draw_text(it->rfn_font, x, y, TEXT_ALIGN_LEFT, text);
	}
}

wraptext_t*
ttf_wrap(const ttf_t* it, const char* text, int width)
{
	wraptext_t* wraptext;
	
	if (it->ttf_font != NULL) {
		wraptext = calloc(1, sizeof(wraptext_t));
		wraptext->pitch = 1024;
		wraptext->buffer = malloc(1024 * wraptext->pitch);
		al_do_multiline_text(it->ttf_font, width, text, do_multiline_text_line, wraptext);
		return wraptext;
	}
	else {
		return wraptext_new(text, it->rfn_font, width);
	}
}

wraptext_t*
wraptext_new(const char* text, const font_t* font, int width)
{
	char*          buffer = NULL;
	uint8_t        ch_byte;
	char*          carry;
	size_t         ch_size;
	uint32_t       cp;
	int            glyph_width;
	bool           is_line_end = false;
	int            line_idx;
	int            line_width;
	int            max_lines = 10;
	char*          last_break;
	char*          last_space;
	char*          last_tab;
	char*          line_buffer;
	size_t         line_length;
	char*          new_buffer;
	size_t         pitch;
	utf8_ret_t     ret;
	utf8_decode_t* utf8;
	wraptext_t*    wraptext;
	const char     *p, *start;

	if (!(wraptext = calloc(1, sizeof(wraptext_t))))
		goto on_error;

	// allocate initial buffer
	font_get_metrics(font, &glyph_width, NULL, NULL);
	pitch = 4 * (glyph_width > 0 ? width / glyph_width : width) + 3;
	if (!(buffer = malloc(max_lines * pitch)))
		goto on_error;
	carry = malloc(pitch);

	// run through one character at a time, carrying as necessary
	line_buffer = buffer; line_buffer[0] = '\0';
	line_idx = 0; line_width = 0; line_length = 0;
	memset(line_buffer, 0, pitch);  // fill line with NULs
	utf8 = utf8_decode_start(true);
	p = text;
	do {
		start = p;
		while ((ret = utf8_decode_next(utf8, ch_byte = *p++, &cp)) == UTF8_CONTINUE);
		if (ret == UTF8_RETRY)
			--p;
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
		cp = ret == UTF8_CODEPOINT
			? cp < (uint32_t)font->num_glyphs ? cp : 0x1A
			: 0x1A;
		switch (cp) {
		case '\n': case '\r':  // explicit newline
			if (cp == '\r' && *p == '\n')
				++text;  // CRLF
			is_line_end = true;
			break;
		case '\t':  // tab
			line_buffer[line_length++] = cp;
			line_width += font_get_width(font, "   ");
			is_line_end = false;
			break;
		case '\0':  // NUL terminator
			is_line_end = line_length > 0;  // commit last line on EOT
			break;
		default:  // default case, copy character as-is
			memcpy(line_buffer + line_length, start, ch_size);
			line_length += ch_size;
			line_width += font->glyphs[cp].width;
			is_line_end = false;
		}
		if (is_line_end)
			carry[0] = '\0';
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
			else {
				line_buffer += pitch;
			}

			memset(line_buffer, 0, pitch);  // fill line with NULs

			// copy carry text into new line
			line_width = font_get_width(font, carry);
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
wraptext_free(wraptext_t* it)
{
	free(it->buffer);
	free(it);
}

int
wraptext_len(const wraptext_t* it)
{
	return it->num_lines;
}

const char*
wraptext_line(const wraptext_t* it, int line_index)
{
	return it->buffer + line_index * it->pitch;
}

static bool
do_multiline_text_line(int line_idx, const char* line, int size, void* userdata)
{
	char*       line_buffer;
	wraptext_t* wraptext;

	wraptext = userdata;
	line_buffer = wraptext->buffer + line_idx * wraptext->pitch;
	memcpy(line_buffer, line, size);
	line_buffer[size] = '\0';  // NUL terminator
	++wraptext->num_lines;
	return true;
}

static void
update_font_metrics(font_t* font)
{
	int max_x = 0, max_y = 0;
	int min_width = INT_MAX;

	uint32_t i;

	for (i = 0; i < font->num_glyphs; ++i) {
		font->glyphs[i].width = image_width(font->glyphs[i].image);
		font->glyphs[i].height = image_height(font->glyphs[i].image);
		min_width = fmin(font->glyphs[i].width, min_width);
		max_x = fmax(font->glyphs[i].width, max_x);
		max_y = fmax(font->glyphs[i].height, max_y);
	}
	font->min_width = min_width;
	font->max_width = max_x;
	font->height = max_y;

	font->modified = false;
}
