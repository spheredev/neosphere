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
#include "backtrace.h"

struct frame
{
	char* name;
	char* filename;
	int   lineno;
};

struct backtrace
{
	int           num_frames;
	struct frame* frames;
};

backtrace_t*
backtrace_new(void)
{
	backtrace_t* obj;

	obj = calloc(1, sizeof(backtrace_t));
	return obj;
}

void
backtrace_free(backtrace_t* obj)
{
	int i;

	if (obj == NULL)
		return;

	for (i = 0; i < obj->num_frames; ++i) {
		free(obj->frames[i].name);
		free(obj->frames[i].filename);
	}
	free(obj->frames);
	free(obj);
}

int
backtrace_len(const backtrace_t* obj)
{
	return obj->num_frames;
}

const char*
backtrace_get_call_name(const backtrace_t* obj, int index)
{
	return obj->frames[index].name;
}

const char*
backtrace_get_filename(const backtrace_t* obj, int index)
{
	return obj->frames[index].filename;
}

int
backtrace_get_linenum(const backtrace_t* obj, int index)
{
	return obj->frames[index].lineno;
}

bool
backtrace_add(backtrace_t* obj, const char* call_name, const char* filename, int line_no)
{
	struct frame* frames;
	int           index;

	if (!(frames = realloc(obj->frames, (obj->num_frames + 1) * sizeof(struct frame))))
		return false;
	index = obj->num_frames++;
	frames[index].name = strdup(call_name);
	frames[index].filename = strdup(filename);
	frames[index].lineno = line_no;
	obj->frames = frames;
	return true;
}

void
backtrace_print(const backtrace_t* obj, int active_frame, bool show_all)
{
	const char* arrow;
	int         name_len;
	const char* filename;
	int         line_no;
	int         max_name_len = 0;
	const char* name;

	int i;

	for (i = 0; i < backtrace_len(obj); ++i) {
		name = backtrace_get_call_name(obj, i);
		filename = backtrace_get_filename(obj, i);
		line_no = backtrace_get_linenum(obj, i);
		name_len = (int)strlen(name);
		if (name_len > max_name_len)
			max_name_len = name_len;
	}

	for (i = 0; i < backtrace_len(obj); ++i) {
		name = backtrace_get_call_name(obj, i);
		filename = backtrace_get_filename(obj, i);
		line_no = backtrace_get_linenum(obj, i);
		if (i == active_frame || show_all) {
			arrow = i == active_frame ? "->" : "  ";
			if (i == active_frame)
				printf("\33[37;1m");
			if (line_no > 0)
				printf("%s #%2d  %-*s  %s:%d\n", arrow, i, max_name_len, name, filename, line_no);
			else
				printf("%s #%2d: %s <system call>\n", arrow, i, name);
			printf("\33[m");
		}
	}
}
