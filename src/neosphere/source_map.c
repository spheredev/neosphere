/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

#include "neosphere.h"
#include "source_map.h"

#include "font.h"

struct alias
{
	char* name;
	char* url;
};

struct map
{
	wraptext_t* filenames;
	wraptext_t* identifiers;
	vector_t*   segments;
	char*       url;
};

struct segment
{
	int column;
	int file_index;
	int line;
	int src_column;
	int src_line;
};

struct source
{
	char* filename;
	char* text;
};

static int compare_segments (const void* a, const void* b);
static int vlq_decode_next  (const char* start, int* out_array, int max_values);

static vector_t* s_aliases;
static bool      s_enabled;
static vector_t* s_maps;
static vector_t* s_sources;

void
source_map_init(void)
{
	s_sources = vector_new(sizeof(struct source));
	s_maps = vector_new(sizeof(struct map));
	s_aliases = vector_new(sizeof(struct alias));
	s_enabled = true;
}

void
source_map_uninit(void)
{
	struct alias*  alias;
	struct map*    map;
	struct source* source;

	iter_t iter;

	if (!s_enabled)
		return;
	
	iter = vector_enum(s_aliases);
	while ((alias = iter_next(&iter))) {
		free(alias->name);
		free(alias->url);
	}
	vector_free(s_aliases);

	iter = vector_enum(s_maps);
	while ((map = iter_next(&iter))) {
		wraptext_free(map->filenames);
		wraptext_free(map->identifiers);
		vector_free(map->segments);
		free(map->url);
	}
	vector_free(s_maps);

	iter = vector_enum(s_sources);
	while ((source = iter_next(&iter))) {
		free(source->filename);
		free(source->text);
	}
	vector_free(s_sources);
	
	s_enabled = false;
}

const char*
source_map_alias_of(const char* url)
{
	struct alias* alias;

	iter_t iter;

	if (s_aliases == NULL)
		return url;
	iter = vector_enum(s_aliases);
	while ((alias = iter_next(&iter))) {
		if (strcmp(url, alias->url) == 0)
			return alias->name;
	}
	return url;
}

const char*
source_map_source_text(const char* filename)
{
	struct source* source;

	iter_t iter;

	if (s_sources == NULL)
		return NULL;
	iter = vector_enum(s_sources);
	while ((source = iter_next(&iter))) {
		if (strcmp(filename, source->filename) == 0)
			return source->text;
	}
	return NULL;
}

const char*
source_map_url_of(const char* name)
{
	struct alias* alias;

	iter_t iter;

	if (s_aliases == NULL)
		return name;
	iter = vector_enum(s_aliases);
	while ((alias = iter_next(&iter))) {
		if (strcmp(name, alias->name) == 0)
			return alias->url;
	}
	return name;
}

bool
source_map_add_alias(const char* url, const char* name)
{
	struct alias alias_obj;
	char*        name_copy = NULL;
	char*        url_copy = NULL;

	if (!(url_copy = strdup(url)))
		goto on_error;
	if (!(name_copy = strdup(name)))
		goto on_error;

	alias_obj.url = url_copy;
	alias_obj.name = name_copy;
	if (!vector_push(s_aliases, &alias_obj))
		goto on_error;
	return true;

on_error:
	free(name_copy);
	free(url_copy);
	return false;
}

bool
source_map_add_map(const char* url, js_ref_t* content)
{
	// note: because of the way this function locates sources named in a source map, this works
	//       best if all source text is provided first (using `source_map_add_source`).

	const char*    filename;
	int            line = 0;
	struct map     map;
	int            num_values;
	path_t*        path;
	struct segment segment;
	int            stack_top;
	int            values[5];
	const char*    p_in;
	struct map*    p_map;

	iter_t iter;

	iter = vector_enum(s_maps);
	while ((p_map = iter_next(&iter))) {
		if (strcmp(url, p_map->url) == 0)
			return true;
	}

	stack_top = jsal_get_top();

	jsal_push_ref_weak(content);

	memset(&map, 0, sizeof(struct map));
	map.url = strdup(url);

	map.filenames = wraptext_new(256);
	jsal_get_prop_string(-1, "sources");
	if (jsal_is_array(-1)) {
		jsal_push_new_iterator(-1);
		while (jsal_next(-1)) {
			filename = jsal_get_string(-1);
			if (filename != NULL) {
				// find the longest path suffix that matches a file in `@/ssj`.  this gives us a bit
				// of valuable fuzziness in finding source files, since most tools (including TypeScript)
				// don't understand how SphereFS prefixes work.
				path = path_new(filename);
				path_collapse(path, true);
				while (path_hop_is(path, 0, ".."))
					path_remove_hop(path, 0);
				for (;;) {
					if (source_map_source_text(path_cstr(path)) != NULL) {
						filename = path_cstr(path);
						wraptext_add_line(map.filenames, filename, strlen(filename));
						break;  // found a match; we're done here
					}
					if (path_num_hops(path) > 0)
						path_remove_hop(path, 0);
					else if (path_filename(path) != NULL)
						path_strip(path);
					else
						break;
				}
				path_free(path);
			}
			jsal_pop(1);
		}
		jsal_pop(1);
	}

	map.segments = vector_new(sizeof(struct segment));
	jsal_get_prop_string(-2, "mappings");
	p_in = jsal_get_string(-1);
	if (p_in == NULL)
		goto on_error;
	memset(values, 0, sizeof values);
	while (p_in != NULL) {
		num_values = vlq_decode_next(p_in, values, 5);
		if (num_values == 1 || num_values == 4 || num_values == 5) {
			segment.line = line;
			segment.column = values[0];
			segment.file_index = values[1];
			segment.src_line = values[2];
			segment.src_column = values[3];
			if (!vector_push(map.segments, &segment))
				goto on_error;
		}
		p_in = strpbrk(p_in, ",;");
		if (p_in != NULL && *p_in++ == ';') {
			++line;
			values[0] = 0;
		}
	}
	vector_sort(map.segments, compare_segments);

	vector_push(s_maps, &map);
	jsal_set_top(stack_top);
	return true;

on_error:
	jsal_set_top(stack_top);
	return false;
}

bool
source_map_add_source(const char* filename, const char* text)
{
	char*         filename_copy = NULL;
	struct source source_obj;
	char*         text_copy = NULL;

	if (s_sources == NULL)
		return true;

	if (!(filename_copy = strdup(filename)))
		goto on_error;
	if (!(text_copy = strdup(text)))
		goto on_error;

	source_obj.filename = filename_copy;
	source_obj.text = text_copy;
	if (!vector_push(s_sources, &source_obj))
		goto on_error;
	return true;

on_error:
	free(filename_copy);
	free(text_copy);
	return false;
}

mapping_t
source_map_lookup(const char* url, int line, int column)
{
	int             file_index = -1;
	struct map*     map;
	int             mapped_column = column;
	int             mapped_line = line;
	mapping_t       retval;
	struct segment* segment;

	iter_t iter;

	iter = vector_enum(s_maps);
	while ((map = iter_next(&iter))) {
		if (strcmp(map->url, url) == 0)
			break;
	}

	retval.filename = url;
	retval.line = line;
	retval.column = column;
	if (map != NULL) {
		iter = vector_enum(map->segments);
		while ((segment = iter_next(&iter))) {
			if (segment->line > line || (segment->line == line && segment->column > column))
				break;
			file_index = segment->file_index;
			mapped_line = segment->src_line;
			mapped_column = segment->src_column;
		}
		retval.filename = file_index >= 0
			? wraptext_line(map->filenames, file_index)
			: NULL;
		retval.line = mapped_line;
		retval.column = mapped_column;
	}
	return retval;
}

mapping_t
source_map_reverse(const char* filename, int line, int column)
{
	int             file_index = -1;
	bool            found_file = false;
	struct map*     map;
	int             mapped_column = column;
	int             mapped_line = line;
	mapping_t       retval;
	struct segment* segment;

	iter_t iter;
	int i;

	iter = vector_enum(s_maps);
	while ((map = iter_next(&iter))) {
		for (i = 0; i < wraptext_len(map->filenames); ++i) {
			if (strcmp(wraptext_line(map->filenames, i), filename) == 0)
				found_file = true;
		}
		if (found_file)
			break;
	}

	retval.filename = filename;
	retval.line = line;
	retval.column = column;
	if (map != NULL) {
		iter = vector_enum(map->segments);
		while ((segment = iter_next(&iter))) {
			if (segment->src_line > line || (segment->src_line == line && segment->src_column > column))
				break;
			file_index = segment->file_index;
			mapped_line = segment->line;
			mapped_column = segment->column;
		}
		retval.filename = map->url;
		retval.line = mapped_line;
		retval.column = mapped_column;
	}
	return retval;
}

static int
compare_segments(const void* in_a, const void* in_b)
{
	const struct segment* seg_a = in_a;
	const struct segment* seg_b = in_b;

	return seg_a->file_index < seg_b->file_index ? -1 : seg_a->file_index > seg_b->file_index ? +1
		: seg_a->line < seg_b->line ? -1 : seg_a->line > seg_b->line ? +1
		: seg_a->column < seg_b->column ? -1 : seg_a->column > seg_b->column ? +1
		: 0;
}

static int
vlq_decode_next(const char* start, int* values, int max_values)
{
	const char* const BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
	
	bool        have_c_bit;
	int         count = 0;
	int         digit;
	bool        negative;
	int         shift = 0;
	int         value = 0;
	const char* p_digit;
	const char* p_in;

	p_in = start;
	while (*p_in != ',' && *p_in != ';' && *p_in != '\0') {
		p_digit = strchr(BASE64, *p_in++);
		if (p_digit == NULL)
			return 0;  // invalid digit
		digit = (int)(p_digit - BASE64);
		have_c_bit = (digit & 0x20) != 0;
		value += (digit & 0x1F) << shift;
		if (have_c_bit) {
			shift += 5;
		}
		else {
			negative = (value & 0x01) != 0;
			value >>= 1;  // shift out the sign bit
			if (negative)
				value = value == 0 ? INT32_MIN : -(value);
			values[count++] += value;
			value = 0;
			shift = 0;
			if (count >= max_values)
				return 0;  // too many values
		}
	}
	if (shift != 0)
		return 0;  // incomplete value
	return count;
}
