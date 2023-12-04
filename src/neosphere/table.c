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

#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "table.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct column
{
	int    max_rows;
	int    num_rows;
	char** rows;
	int    width;
};

struct table
{
	struct column* columns;
	bool           has_totals;
	int            max_columns;
	int            num_columns;
	char*          title;
};

#if !defined(__APPLE__)
static int asprintf(char* *out, const char* format, ...);
static int vasprintf(char* *out, const char* format, va_list ap);
#endif

table_t*
table_new(const char* title, bool has_totals)
{
	table_t* table;

	if (!(table = calloc(1, sizeof(table_t))))
		return NULL;
	table->has_totals = has_totals;
	table->num_columns = 0;
	table->title = strdup(title);
	return table;
}

void
table_free(table_t* it)
{
	int r, c;

	for (c = 0; c < it->num_columns; ++c) {
		for (r = 0; r < it->columns[c].num_rows; ++r)
			free(it->columns[c].rows[r]);
		free(it->columns[c].rows);
	}
	free(it->columns);
	free(it->title);
	free(it);
}

int
table_add_column(table_t* it, const char* name_fmt, ...)
{
	va_list args;
	int     index;
	char*   name;

	index = it->num_columns++;
	if (it->num_columns > it->max_columns) {
		if (it->max_columns > 0)
			it->max_columns *= 2;
		else
			it->max_columns = 8;
		it->columns = realloc(it->columns, it->max_columns * sizeof(struct column));
	}
	it->columns[index].num_rows = 0;
	it->columns[index].max_rows = 0;
	it->columns[index].rows = NULL;
	it->columns[index].width = 0;
	va_start(args, name_fmt);
	vasprintf(&name, name_fmt, args);
	va_end(args);
	table_add_text(it, index, name);
	free(name);
	return index;
}

void
table_add_number(table_t* it, int column_index, long long value)
{
	char* dest_ptr;
	char  source[64];
	char* src_ptr;
	int   state;
	char  text[64];

	// add commas for thousand separators
	sprintf(source, "%lld", value);
	src_ptr = source;
	dest_ptr = text;
	if (*src_ptr == '-')
		*dest_ptr++ = *src_ptr++;
	state = 2 - (int)strlen(src_ptr) % 3;
	while (*src_ptr != '\0') {
		*dest_ptr++ = *src_ptr++;
		state = (state + 1) % 3;
		if (state == 2 && *src_ptr != '\0')
			*dest_ptr++ = ',';
	}
	*dest_ptr = '\0';  // NUL terminator

	table_add_text(it, column_index, text);
}

void
table_add_percentage(table_t* it, int column_index, double value)
{
	char text[32];

	sprintf(text, "%.1f %%", value * 100.0);
	table_add_text(it, column_index, text);
}

void
table_add_text(table_t* it, int column_index, const char* text)
{
	struct column* column;
	int            index;

	column = &it->columns[column_index];
	index = column->num_rows++;
	if (column->num_rows > column->max_rows) {
		if (column->max_rows > 0)
			column->max_rows *= 2;
		else
			column->max_rows = 8;
		column->rows = realloc(column->rows, column->max_rows * sizeof(char*));
	}
	column->rows[index] = strdup(text);
}

void
table_print(const table_t* it)
{
	struct column* column;
	int            length;
	int            num_rows = 0;
	const char*    text;
	int            total_width = 0;

	int r, c, i;

	// calculate the size of the table
	for (c = 0; c < it->num_columns; ++c) {
		column = &it->columns[c];
		column->width = 0;
		if (column->num_rows > num_rows)
			num_rows = column->num_rows;
		for (r = 0; r < column->num_rows; ++r) {
			length = (int)strlen(column->rows[r]);
			if (length > column->width)
				column->width = length;
		}
		total_width += column->width;
	}
	total_width += 2 * (it->num_columns - 1) + 3;

	// print the table to the console
	for (i = (int)strlen(it->title) + 2; i >= 0; --i)
		putchar('=');
	printf("\n %s |\n", it->title);
	for (r = 0; r < num_rows; ++r) {
		// see if we need to print a divider line
		if (r == 0) {
			// top border should be no shorter than the heading
			length = total_width;
			if (length < (int)strlen(it->title) + 3)
				length = (int)strlen(it->title) + 3;
			for (i = 0; i < length; ++i)
				putchar('=');
			putchar('\n');
		}
		else if (r == 1 || (r == num_rows - 1 && it->has_totals)) {
			for (i = 0; i < total_width; ++i)
				putchar('-');
			putchar('\n');
		}

		for (c = 0; c < it->num_columns; ++c) {
			column = &it->columns[c];
			text = r < column->num_rows ? column->rows[r] : "";
			if (c == 0)
				printf(" %-*s", column->width, text);
			else if (c == it->num_columns - 1)
				printf("  %*s |", column->width, text);
			else
				printf("  %*s", column->width, text);
		}
		putchar('\n');
	}
	for (i = 0; i < total_width; ++i)
		putchar('-');
	putchar('\n');
}

#if !defined(__APPLE__)
static int
asprintf(char** out, const char* format, ...)
{
	va_list ap;
	int     buf_size;

	va_start(ap, format);
	buf_size = vasprintf(out, format, ap);
	va_end(ap);
	return buf_size;
}

static int
vasprintf(char** out, const char* format, va_list ap)
{
	va_list apc;
	int     buf_size;

	va_copy(apc, ap);
	buf_size = vsnprintf(NULL, 0, format, apc) + 1;
	va_end(apc);
	*out = malloc(buf_size);
	vsnprintf(*out, buf_size, format, ap);
	return buf_size;
}
#endif
