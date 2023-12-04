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
#include "utility.h"

#if defined(_WIN32)
#define _WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501
#include <windows.h>
#endif
#include "api.h"
#include "fs.h"

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
	return lstr_from_utf8(buffer, length, false);
}

const char*
jsal_require_pathname(int index, const char* origin_name, bool need_write)
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
	if (strcmp(first_hop, "..") == 0 || path_rooted(path))
		jsal_error(JS_URI_ERROR, "SphereFS sandbox violation '%s'", pathname);
	if (strcmp(prefix, "%") == 0)
		jsal_error(JS_REF_ERROR, "SphereFS prefix '%%/' is reserved for future use");
	if (strcmp(prefix, "~") == 0)
		jsal_error(JS_TYPE_ERROR, "Cell doesn't support the '~/' SphereFS prefix");
	if (need_write && (strcmp(prefix, "$") == 0 || strcmp(prefix, "#") == 0))
		jsal_error(JS_ERROR, "SphereFS pathname '%s' is not writable", pathname);
	if (s_paths[s_index] != NULL)
		path_free(s_paths[s_index]);
	s_paths[s_index] = path;
	s_index = (s_index + 1) % 10;
	return path_cstr(path);
}

int
fcopy(const char* src_filename, const char* dst_filename, int skip_if_exists)
{
#if defined(_WIN32)
	return CopyFileA(src_filename, dst_filename, (BOOL)skip_if_exists) ? 0 : -1;
#else
	char        buffer[32768];
	FILE*       f1;
	FILE*       f2;
	size_t      num_bytes;
	struct stat sb;

	if (skip_if_exists && stat(dst_filename, &sb) == 0) {
		errno = EEXIST;
		return -1;
	}
	if (!(f1 = fopen(src_filename, "rb")))
		return -1;
	if (!(f2 = fopen(dst_filename, "wb")))
		return -1;
	while ((num_bytes = fread(buffer, sizeof(char), sizeof(buffer), f1))) {
		if (fwrite(buffer, sizeof(char), num_bytes, f2) != num_bytes)
			goto on_error;
	}
	fclose(f1);
	fclose(f2);
	return 0;

on_error:
	fclose(f1);
	fclose(f2);
	return -1;
#endif
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
	char* buffer;
	FILE* file = NULL;
	size_t read_size;
	if (!(file = fopen(filename, "rb")))
		return false;
	fseek(file, 0, SEEK_END);
	read_size = (size_t)ftell(file);
	if (out_size != NULL)
		*out_size = read_size;
	if (!(buffer = (char*)malloc(read_size + 1)))
		goto on_error;
	fseek(file, 0, SEEK_SET);
	if (fread(buffer, 1, read_size, file) != read_size)
		goto on_error;
	buffer[read_size] = '\0';
	fclose(file);
	return buffer;

on_error:
	fclose(file);
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
strescq(const char* input, char quote_char)
{
	char*       buffer;
	const char* escapables;
	size_t      len;
	size_t      out_len = 0;
	const char* p_in;
	const char* p_next_in;
	char*       p_out;

	if (quote_char == '"')
		escapables = "\"\r\n\\";
	else if (quote_char == '\'')
		escapables = "'\r\n\\";
	else
		escapables = "`$\\";

	p_in = input;
	while ((p_next_in = strpbrk(p_in, escapables))) {
		out_len += p_next_in - p_in + 2;
		p_in = p_next_in + 1;
	}
	out_len += strlen(p_in);

	if (!(buffer = malloc(out_len + 1)))
		return NULL;
	p_in = input;
	p_out = buffer;
	while ((p_next_in = strpbrk(p_in, escapables))) {
		len = p_next_in - p_in;
		memcpy(p_out, p_in, len);
		p_out += len;
		*p_out++ = '\\';
		*p_out++ = *p_next_in == '\r' ? 'r'
			: *p_next_in == '\n' ? 'n'
			: *p_next_in;
		p_in = p_next_in + 1;
	}
	strcpy(p_out, p_in);  // copy remainder of string
	return buffer;
}

char*
strfmt(const char* format, ...)
{
	va_list ap;
	
	char*       buffer = NULL;
	size_t      in_len;
	int         index;
	const char* items[9];
	size_t      item_len[9];
	int         num_items = 0;
	size_t      out_len = 0;
	const char* p_in;
	const char* p_next_in;
	char*       p_out;

	// get replacement strings from argument list (up to 9)
	va_start(ap, format);
	while (num_items < 9) {
		if ((items[num_items] = va_arg(ap, const char*))) {
			item_len[num_items] = strlen(items[num_items]);
			++num_items;
		}
		else {
			item_len[num_items] = 0;
			break;
		}
	}
	va_end(ap);
	
	// find out how big a buffer we need so we don't have to resize it as we go.
	// while we're at it we can validate the input string to avoid some extra checks during the
	// replacement phase.
	p_in = format;
	while ((p_next_in = strpbrk(p_in, "{}"))) {
		out_len += p_next_in - p_in;
		if (p_next_in[1] == p_next_in[0]) {  // "{{" or "}}"
			out_len += 1;
			p_next_in += 2;
		}
		else if (p_next_in[0] == '{' && p_next_in[1] >= '0' && p_next_in[1] <= '9' && p_next_in[2] == '}') {
			// anything in brackets {...} is a format item and should be replaced with the
			// corresponding replacement string.
			index = p_next_in[1] - '0';
			if (index >= 0 && index < num_items) {
				out_len += item_len[index];
				p_next_in += 3;
			}
			else {
				// if a format item has no corresponding replacement string, that's an error.
				goto syntax_error;
			}
		}
		else {
			goto syntax_error;
		}
		p_in = p_next_in;
	}
	out_len += strlen(p_in);
	if (!(buffer = malloc(out_len + 1)))
		return NULL;

	// ready, set, GO!
	p_in = format;
	p_out = buffer;
	while ((p_next_in = strpbrk(p_in, "{}"))) {
		in_len = p_next_in - p_in;
		memcpy(p_out, p_in, in_len);
		p_out += in_len;
		if (p_next_in[1] == p_next_in[0]) {  // "{{" or "}}"
			memcpy(p_out, p_next_in, 1);
			p_out += 1;
			p_next_in += 2;
		}
		else if (p_next_in[0] == '{') {  // format item "{n}"
			// note: `index` is guaranteed to be in range as we checked for that above.
			index = p_next_in[1] - '0';
			memcpy(p_out, items[index], item_len[index]);
			p_out += item_len[index];
			p_next_in += 3;
		}
		p_in = p_next_in;
	}
	strcpy(p_out, p_in);
	return buffer;

syntax_error:
	free(buffer);
	return NULL;
}

char*
strnewf(const char* fmt, ...)
{
	va_list ap;

	char* buffer;
	int   buf_size;

	va_start(ap, fmt);
	buf_size = vsnprintf(NULL, 0, fmt, ap) + 1;
	va_end(ap);
	buffer = malloc(buf_size);

	va_start(ap, fmt);
	vsnprintf(buffer, buf_size, fmt, ap);
	va_end(ap);
	return buffer;
}

bool
wildcmp(const char* filename, const char* pattern)
{
	const char* cp = NULL;
	const char* mp = NULL;

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
