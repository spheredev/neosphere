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

#include "ssj.h"
#include "listing.h"

struct listing
{
	vector_t* lines;
};

static char* read_line (const char** p_string);

listing_t*
listing_new(const char* text)
{
	char*       line_text;
	vector_t*   lines;
	listing_t*   source = NULL;
	const char* p_source;

	p_source = text;
	lines = vector_new(sizeof(char*));
	while ((line_text = read_line(&p_source)))
		vector_push(lines, &line_text);

	if (!(source = calloc(1, sizeof(listing_t))))
		return NULL;
	source->lines = lines;
	return source;
}

void
listing_free(listing_t* it)
{
	char** text_ptr;

	iter_t iter;

	if (it == NULL)
		return;
	iter = vector_enum(it->lines);
	while ((text_ptr = iter_next(&iter)))
		free(*text_ptr);
	vector_free(it->lines);
	free(it);
}

int
listing_cloc(const listing_t* it)
{
	return vector_len(it->lines);
}

const char*
listing_get_line(const listing_t* it, int line_index)
{
	if (line_index < 0 || line_index >= listing_cloc(it))
		return NULL;
	return *(char**)vector_get(it->lines, line_index);
}

void
listing_print(const listing_t* it, int line_number, int num_lines, int active_lineno)
{
	const char* arrow;
	int         start;
	const char* text;

	int i;

	start = line_number - 1;
	for (i = start; i < start + num_lines; ++i) {
		if (i >= listing_cloc(it))
			break;  // EOF
		text = listing_get_line(it, i);
		arrow = i + 1 == active_lineno ? "->" : "  ";
		if (num_lines == 1)
			printf("%d %s\n", i + 1, text);
		else {
			if (i + 1 == active_lineno)
				printf("\33[37;1m");
			printf("%s %4d %s\n", arrow, i + 1, text);
			printf("\33[m");
		}
	}
}

static char*
read_line(const char** p_string)
{
	char*  buffer;
	size_t buf_size;
	char   ch;
	bool   have_line = false;
	size_t length;
	char*  new_buffer;

	if (!(buffer = malloc(buf_size = 256)))
		goto on_error;
	length = 0;
	while (!have_line) {
		if (length + 1 >= buf_size) {
			if (!(new_buffer = realloc(buffer, buf_size *= 2)))
				goto on_error;
			buffer = new_buffer;
		}
		if ((ch = *(*p_string)) == '\0')
			goto hit_eof;
		++(*p_string);
		switch (ch) {
		case '\n': have_line = true; break;
		case '\r':
			have_line = true;
			if (*(*p_string) == '\n')  // CR LF?
				++(*p_string);
			break;
		default:
			buffer[length++] = ch;
		}
	}

hit_eof:
	buffer[length] = '\0';
	if (*(*p_string) == '\0' && length == 0) {
		free(buffer);
		return NULL;
	}
	else {
		return buffer;
	}

on_error:
	free(buffer);
	return NULL;
}
