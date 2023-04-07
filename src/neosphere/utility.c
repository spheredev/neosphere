/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2023, Fat Cerberus
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
#include "utility.h"

#include "geometry.h"
#include "image.h"
#include "jsal.h"
#include "lstring.h"
#include "md5.h"
#include "path.h"

const path_t*
app_data_path(void)
{
	static path_t* retval = NULL;

	ALLEGRO_PATH* al_path;

	if (retval == NULL) {
		al_path = al_get_standard_path(ALLEGRO_USER_DATA_PATH);
		retval = path_new_dir(al_path_cstr(al_path, '/'));
		al_destroy_path(al_path);
	}
	path_mkdir(retval);
	return retval;
}

const path_t*
assets_path(void)
{
	static path_t* retval = NULL;

	ALLEGRO_PATH* al_path;

	if (retval == NULL) {
		al_path = al_get_standard_path(ALLEGRO_RESOURCES_PATH);
		retval = path_new_dir(al_path_cstr(al_path, '/'));
		al_destroy_path(al_path);
	}
	return retval;
}

const path_t*
engine_path(void)
{
	static path_t* retval = NULL;

	ALLEGRO_PATH* al_path;

	if (retval == NULL) {
		al_path = al_get_standard_path(ALLEGRO_EXENAME_PATH);
		al_set_path_filename(al_path, NULL);
#if defined(__APPLE__) && !defined(NEOSPHERE_SPHERUN)
		// for macOS, we want the directory containing neoSphere.app and NOT one
		// of the subdirectories that's buried inside.  to get there, we need to
		// go 3 levels up:
		//     neoSphere.app/Contents/MacOS/neoSphere
		//     ^3             ^2       ^1    ^0
		al_drop_path_tail(al_path);
		al_drop_path_tail(al_path);
		al_drop_path_tail(al_path);
#endif
		retval = path_new_dir(al_path_cstr(al_path, '/'));
		al_destroy_path(al_path);
	}
	return retval;
}

const path_t*
home_path(void)
{
	static path_t* retval = NULL;

	ALLEGRO_PATH* al_path;

	if (retval == NULL) {
		al_path = al_get_standard_path(ALLEGRO_USER_DOCUMENTS_PATH);
		retval = path_new_dir(al_path_cstr(al_path, '/'));
		al_destroy_path(al_path);
	}
	path_mkdir(retval);
	return retval;
}

const char*
md5sum(const void* data, size_t size)
{
	// note: a static buffer is used here to store the last generated hash, so
	//       only one output can be used at a time.  be careful.

	static char output[33];

	MD5_CTX ctx;
	uint8_t hash_bytes[16];
	char    *p;

	int i;

	MD5_Init(&ctx);
	MD5_Update(&ctx, data, (unsigned long)size);
	MD5_Final(hash_bytes, &ctx);
	p = &output[0];
	for (i = 0; i < 16; ++i) {
		sprintf(p, "%.2x", (int)hash_bytes[i]);
		p += 2;
	}
	return output;
}

image_t*
fread_image(file_t* file, int width, int height)
{
	long long     file_pos;
	image_t*      image;
	size_t        line_size;
	image_lock_t* lock = NULL;
	color_t*      out_ptr;

	int i_y;

	console_log(3, "reading %dx%d image from open file", width, height);
	file_pos = file_position(file);
	if (!(image = image_new(width, height, NULL)))
		goto on_error;
	if (!(lock = image_lock(image, true, false)))
		goto on_error;
	line_size = width * sizeof(color_t);
	out_ptr = lock->pixels;
	for (i_y = 0; i_y < height; ++i_y) {
		if (file_read(file, out_ptr, 1, line_size) != 1)
			goto on_error;
		out_ptr += lock->pitch;
	}
	image_unlock(image, lock);
	return image;

on_error:
	file_seek(file, file_pos, WHENCE_SET);
	if (lock != NULL)
		image_unlock(image, lock);
	image_unref(image);
	return NULL;
}

image_t*
fread_image_slice(file_t* file, image_t* parent, int x, int y, int width, int height)
{
	long          file_pos;
	image_t*      image;
	size_t        line_size;
	image_lock_t* lock = NULL;
	color_t*      out_ptr;

	int i_y;

	file_pos = file_position(file);
	if (!(image = image_new_slice(parent, x, y, width, height)))
		goto on_error;
	file_pos = file_position(file);
	if (!(lock = image_lock(parent, true, false)))
		goto on_error;
	line_size = width * sizeof(color_t);
	out_ptr = lock->pixels + x + y * lock->pitch;
	for (i_y = 0; i_y < height; ++i_y) {
		if (file_read(file, out_ptr, 1, line_size) != 1)
			goto on_error;
		out_ptr += lock->pitch;
	}
	image_unlock(parent, lock);
	return image;

on_error:
	file_seek(file, file_pos, WHENCE_SET);
	if (lock != NULL)
		image_unlock(parent, lock);
	image_unref(image);
	return NULL;
}

lstring_t*
read_lstring(file_t* file, bool trim_null)
{
	long     file_pos;
	uint16_t length;

	file_pos = file_position(file);
	if (file_read(file, &length, 1, 2) != 1)
		goto on_error;
	return read_lstring_raw(file, length, trim_null);

on_error:
	file_seek(file, file_pos, WHENCE_CUR);
	return NULL;
}

lstring_t*
read_lstring_raw(file_t* file, size_t length, bool trim_null)
{
	char*      buffer = NULL;
	long       file_pos;
	lstring_t* string;

	file_pos = file_position(file);
	if (!(buffer = malloc(length + 1)))
		goto on_error;
	if (file_read(file, buffer, length, 1) != length)
		goto on_error;
	buffer[length] = '\0';
	if (trim_null)
		length = strchr(buffer, '\0') - buffer;
	string = lstr_from_cp1252(buffer, length);
	free(buffer);
	return string;

on_error:
	free(buffer);
	file_seek(file, file_pos, WHENCE_CUR);
	return NULL;
}

bool
write_lstring(file_t* file, const lstring_t* string, bool include_nul)
{
	uint16_t length;

	length = (uint16_t)lstr_len(string);
	if (include_nul)
		++length;
	if (file_write(file, &length, 1, 2) != 1)
		return false;
	if (file_write(file, lstr_cstr(string), 1, length) != 1)
		return false;
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

int
jsal_push_lstring_t(const lstring_t* string)
{
	return jsal_push_lstring(lstr_cstr(string), lstr_len(string));
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
jsal_require_pathname(int index, const char* origin_name, bool v1_mode, bool need_write)
{
	// note: for compatibility with Sphere 1.x, if 'v1_mode' is true, then the game package
	//       is treated as writable.

	static int     s_index = 0;
	static path_t* s_paths[10];

	const char* first_hop = "";
	const char* pathname;
	const char* prefix;
	path_t*     path;

	pathname = jsal_require_string(index);
	path = game_full_path(g_game, pathname, origin_name, v1_mode);
	prefix = path_hop(path, 0);  // safe, prefix is always present
	if (path_num_hops(path) > 1)
		first_hop = path_hop(path, 1);
	if (strcmp(first_hop, "..") == 0 || path_rooted(path))
		jsal_error(JS_URI_ERROR, "SphereFS sandbox violation '%s'", path_cstr(path));
	if (strcmp(prefix, "%") == 0)
		jsal_error(JS_REF_ERROR, "SphereFS prefix '%%/' is reserved for future use");
	if (strcmp(prefix, "~") == 0 && game_save_id(g_game) == NULL)
		jsal_error(JS_REF_ERROR, "SphereFS prefix '~/' requires a save ID");
	if (need_write && !game_is_writable(g_game, path_cstr(path), v1_mode))
		jsal_error(JS_TYPE_ERROR, "File or directory is not writable '%s'", path_cstr(path));
	if (s_paths[s_index] != NULL)
		path_free(s_paths[s_index]);
	s_paths[s_index] = path;
	s_index = (s_index + 1) % 10;
	return path_cstr(path);
}

bool
fread_rect16(file_t* file, rect_t* out_rect)
{
	int16_t x1;
	int16_t x2;
	int16_t y1;
	int16_t y2;

	if (file_read(file, &x1, 1, 2) != 1)
		return false;
	if (file_read(file, &y1, 1, 2) != 1)
		return false;
	if (file_read(file, &x2, 1, 2) != 1)
		return false;
	if (file_read(file, &y2, 1, 2) != 1)
		return false;
	*out_rect = mk_rect(x1, y1, x2, y2);
	return true;
}

bool
fread_rect32(file_t* file, rect_t* out_rect)
{
	int32_t x1;
	int32_t x2;
	int32_t y1;
	int32_t y2;

	if (file_read(file, &x1, 1, 4) != 1)
		return false;
	if (file_read(file, &y1, 1, 4) != 1)
		return false;
	if (file_read(file, &x2, 1, 4) != 1)
		return false;
	if (file_read(file, &y2, 1, 4) != 1)
		return false;
	*out_rect = mk_rect(x1, y1, x2, y2);
	return true;
}

bool
fwrite_image(file_t* file, image_t* image)
{
	size_t        line_size;
	image_lock_t* lock;
	color_t*      out_ptr;

	int y;

	console_log(3, "writing %dx%d image to open file", image_width(image), image_height(image));

	if (!(lock = image_lock(image, false, true)))
		goto on_error;
	line_size = image_width(image) * sizeof(color_t);
	out_ptr = lock->pixels;
	for (y = 0; y < lock->num_lines; ++y) {
		if (file_write(file, out_ptr, 1, line_size) != 1)
			goto on_error;
		out_ptr += lock->pitch;
	}
	image_unlock(image, lock);
	return true;

on_error:
	console_log(3, "    couldn't write image to file");
	image_unlock(image, lock);
	return false;
}
