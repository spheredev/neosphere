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

#include "cell.h"
#include "visor.h"

static void print_indent (int level);

struct visor
{
	vector_t* filenames;
	int       indent_level;
	int       num_errors;
	int       num_warns;
};

visor_t*
visor_new(void)
{
	visor_t* visor;

	if (!(visor = calloc(1, sizeof(visor_t))))
		return NULL;
	visor->filenames = vector_new(sizeof(char*));
	return visor;
}

void
visor_free(visor_t* visor)
{
	int i;

	for (i = 0; i < vector_len(visor->filenames); ++i)
		free(*(char**)vector_get(visor->filenames, i));
	vector_free(visor->filenames);
	free(visor);
}

vector_t*
visor_filenames(const visor_t* visor)
{
	return visor->filenames;
}

int
visor_num_errors(const visor_t* visor)
{
	return visor->num_errors;
}

int
visor_num_warns(const visor_t* visor)
{
	return visor->num_warns;
}

void
visor_add_file(visor_t* visor, const char* filename)
{
	char* filename_copy;

	filename_copy = strdup(filename);
	vector_push(visor->filenames, &filename_copy);
}

void
visor_begin_op(visor_t* visor, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	print_indent(visor->indent_level);
	vprintf(fmt, ap);
	printf("...\n");
	fflush(stdout);
	va_end(ap);

	++visor->indent_level;
}

void
visor_end_op(visor_t* visor)
{
	--visor->indent_level;
}

void
visor_error(visor_t* visor, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	print_indent(visor->indent_level);
	printf("E: ");
	vprintf(fmt, ap);
	printf("\n");
	fflush(stdout);
	va_end(ap);

	++visor->num_errors;
}

void
visor_print(visor_t* visor, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	print_indent(visor->indent_level);
	printf("i: ");
	vprintf(fmt, ap);
	printf("\n");
	fflush(stdout);
	va_end(ap);
}

void
visor_prompt(visor_t* visor, const char* prompt, char* buffer, size_t bufsize)
{
	char  ch;
	char* p_out;
	
	print_indent(visor->indent_level);
	printf("%s ", prompt);
	p_out = buffer;
	while ((ch = getchar()) != '\n') {
		if (p_out >= buffer + bufsize - 1) {
			while (getchar() != '\n');
			break;
		}
		*p_out++ = ch;
	}
	*p_out = '\0';
}

void
visor_warn(visor_t* visor, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	print_indent(visor->indent_level);
	printf("W: ");
	vprintf(fmt, ap);
	printf("\n");
	fflush(stdout);
	va_end(ap);

	++visor->num_warns;
}

void
print_indent(int level)
{
	int i;

	for (i = 0; i < level; ++i)
		printf("   ");
}
