/**
 *  Cell, the Sphere packaging compiler
 *  Copyright (c) 2015-2018, Fat Cerberus
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

#include "cell.h"
#include "utility.h"

#include "api.h"
#include "fs.h"

static bool do_decode_json (void* udata);

void
jsal_push_lstring_t(const lstring_t* string)
{
	jsal_push_lstring(lstr_cstr(string), lstr_len(string));
}

lstring_t*
jsal_require_lstring_t(int index)
{
	const char* buffer;
	size_t      length;

	buffer = jsal_require_lstring(index, &length);
	return lstr_from_cp1252(buffer, length);
}

const char*
jsal_require_pathname(int index, const char* origin_name)
{
	static int     s_index = 0;
	static path_t* s_paths[10];

	const char* first_hop = "";
	const char* pathname;
	path_t*     path;
	const char* prefix;

	pathname = jsal_require_string(index);
	path = fs_full_path(pathname, origin_name);
	prefix = path_hop(path, 0);  // note: fs_full_path() *always* prefixes
	if (path_num_hops(path) > 1)
		first_hop = path_hop(path, 1);
	if (strcmp(first_hop, "..") == 0 || path_is_rooted(path))
		jsal_error(JS_TYPE_ERROR, "illegal path '%s'", pathname);
	if (strcmp(prefix, "%") == 0)
		jsal_error(JS_REF_ERROR, "SphereFS prefix '%%/' is reserved for future use");
	if (strcmp(prefix, "~") == 0)
		jsal_error(JS_TYPE_ERROR, "no save directory in Cell");
	if (s_paths[s_index] != NULL)
		path_free(s_paths[s_index]);
	s_paths[s_index] = path;
	s_index = (s_index + 1) % 10;
	return path_cstr(path);
}

bool
fexist(const char* filename)
{
	struct stat sb;

	if (stat(filename, &sb) != 0)
		return false;
	return true;
}

void*
fslurp(const char* filename, size_t *out_size)
{
	void* buffer;
	FILE* file = NULL;

	if (!(file = fopen(filename, "rb")))
		return false;
	*out_size = (fseek(file, 0, SEEK_END), ftell(file));
	if (!(buffer = malloc(*out_size))) goto on_error;
	fseek(file, 0, SEEK_SET);
	if (fread(buffer, 1, *out_size, file) != *out_size)
		goto on_error;
	fclose(file);
	return buffer;

on_error:
	return NULL;
}

bool
fspew(const void* buffer, size_t size, const char* filename)
{
	FILE* file = NULL;

	if (!(file = fopen(filename, "wb")))
		return false;
	fwrite(buffer, size, 1, file);
	fclose(file);
	return true;
}

char*
strnewf(const char* fmt, ...)
{
	va_list ap, apc;
	char* buffer;
	int   buf_size;

	va_start(ap, fmt);
	va_copy(apc, ap);
	buf_size = vsnprintf(NULL, 0, fmt, apc) + 1;
	va_end(apc);
	buffer = malloc(buf_size);
	va_copy(apc, ap);
	vsnprintf(buffer, buf_size, fmt, apc);
	va_end(apc);
	va_end(ap);
	return buffer;
}

bool
wildcmp(const char* filename, const char* pattern)
{
	const char* cp = NULL;
	const char* mp = NULL;
	bool        is_match = 0;

	// check filename against the specified filter string
	while (*filename != '\0' && *pattern != '*') {
		if (*pattern != *filename && *pattern != '?')
			return false;
		++pattern;
		++filename;
	}
	while (*filename != '\0') {
		if (*pattern == '*') {
			if (*++pattern == '\0') return true;
			mp = pattern;
			cp = filename + 1;
		}
		else if (*pattern == *filename || *pattern == '?') {
			pattern++;
			filename++;
		}
		else {
			pattern = mp;
			filename = cp++;
		}
	}
	while (*pattern == '*')
		pattern++;
	return *pattern == '\0';
}
