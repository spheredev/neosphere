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
#include "fs.h"

#include "tinydir.h"

#if defined(_WIN32)
#include <direct.h>
#else
#include <unistd.h>
#endif

struct fs
{
	path_t* root_path;
	path_t* game_path;
	path_t* system_path;
	path_t* user_path;
};

struct directory
{
	vector_t* entries;
	fs_t*     fs;
	int       position;
	path_t*   path;
	bool      recursive;
};

static bool  help_list_dir (vector_t* list, const char* dirname, const path_t* origin_path, bool recursive);
static char* resolve       (const fs_t* fs, const char* filename);

fs_t*
fs_new(const char* root_dir, const char* game_dir, const char* home_dir)
{
	path_t* app_path;
	fs_t*   fs;

	if (!(fs = calloc(1, sizeof(fs_t))))
		return NULL;
	fs->root_path = path_new_dir(root_dir);
	fs->game_path = path_new_dir(game_dir);
	app_path = path_new_self();
	fs->system_path = path_rebase(path_new("system/"), app_path);
	if (!path_resolve(fs->system_path, NULL)) {
		path_free(fs->system_path);
		fs->system_path = path_rebase(path_new("../share/sphere/system/"), app_path);
		path_resolve(fs->system_path, NULL);
	}
	if (home_dir != NULL)
		fs->user_path = path_new_dir(home_dir);
	path_free(app_path);
	return fs;
}

void
fs_free(fs_t* fs)
{
	path_free(fs->root_path);
	path_free(fs->game_path);
	path_free(fs->system_path);
	path_free(fs->user_path);
	free(fs);
}

bool
fs_dir_exists(const fs_t* fs, const char* dirname)
{
	struct stat sb;

	if (fs_stat(fs, dirname, &sb) != 0)
		return false;
	return (sb.st_mode & S_IFDIR) == S_IFDIR;
}

path_t*
fs_full_path(const char* filename, const char* base_dir_name)
{
	// note: fs_full_path() collapses '../' path hops unconditionally, as per
	//       SphereFS spec. this ensures an unpackaged game can't subvert the
	//       sandbox by navigating outside of its directory via a symbolic link.

	path_t* base_path = NULL;
	path_t* path;
	char*   prefix;

	path = path_new(filename);
	if (path_rooted(path))  // absolute path?
		return path;

	if (base_dir_name != NULL) {
		base_path = fs_full_path(base_dir_name, NULL);
		path_to_dir(base_path);
	}
	if (path_num_hops(path) > 0)
		prefix = strdup(path_hop(path, 0));
	else
		prefix = strdup("");
	if (!strpbrk(prefix, "@#~$%") || strlen(prefix) != 1) {
		if (base_path != NULL)
			path_rebase(path, base_path);
		else
			path_insert_hop(path, 0, "$");
		free(prefix);
		prefix = strdup(path_hop(path, 0));
	}
	path_remove_hop(path, 0);
	path_collapse(path, true);
	path_insert_hop(path, 0, prefix);
	free(prefix);
	path_free(base_path);
	return path;
}

path_t*
fs_relative_path(const char* filename, const char* base_dir_name)
{
	path_t* base_path;
	path_t* path;

	path = fs_full_path(filename, NULL);
	if (path_rooted(path))
		return path;
	base_path = fs_full_path(base_dir_name, NULL);
	path_to_dir(base_path);
	if (path_hop_is(path, 0, path_hop(base_path, 0)))
		path_relativize(path, base_path);
	path_free(base_path);
	return path;
}

int
fs_fcopy(const fs_t* fs, const char* destination, const char* source, int overwrite)
{
	path_t* dest_path;
	char*   resolved_dest;
	char*   resolved_src;
	int     result;

	resolved_dest = resolve(fs, destination);
	resolved_src = resolve(fs, source);
	if (resolved_dest == NULL || resolved_src == NULL) {
		errno = EACCES;  // sandboxing violation
		goto on_error;
	}

	dest_path = path_new(resolved_dest);
	path_mkdir(dest_path);
	path_free(dest_path);
	result = fcopy(resolved_src, resolved_dest, !overwrite);
	free(resolved_src);
	free(resolved_dest);
	return result;

on_error:
	free(resolved_src);
	free(resolved_dest);
	return -1;
}

bool
fs_fexist(const fs_t* fs, const char* filename)
{
	struct stat sb;

	if (fs_stat(fs, filename, &sb) != 0)
		return false;
	return (sb.st_mode & S_IFREG) == S_IFREG;
}

FILE*
fs_fopen(const fs_t* fs, const char* filename, const char* mode)
{
	FILE* file;
	char* resolved_name;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return NULL;
	}

	file = fopen(resolved_name, mode);
	free(resolved_name);
	return file;
}

void*
fs_fslurp(const fs_t* fs, const char* filename, size_t* out_size)
{
	void* buffer;
	char* resolved_name;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return NULL;
	}

	buffer = fslurp(resolved_name, out_size);
	free(resolved_name);
	return buffer;
}

bool
fs_fspew(const fs_t* fs, const char* filename, const void* data, size_t size)
{
	char* resolved_name;
	bool  retval;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return false;
	}

	retval = fspew(data, size, resolved_name);
	free(resolved_name);
	return retval;
}

bool
fs_is_prefix_path(const fs_t* it, const char* pathname)
{
	// this may look unsafe but it actually isn't: if the `strpbrk()` check passes,
	// then we know the string is at least one character long, and accessing `pathname[1]`
	// is thus safe.
	return strpbrk(pathname, "@#~$%") == pathname
		&& (pathname[1] == '/' || pathname[1] == '\\');
}

bool
fs_is_game_dir(const fs_t* fs, const char* dirname)
{
	path_t* full_path;
	bool    retval;
	char*   resolved_name;

	if (!(resolved_name = resolve(fs, dirname)))
		return false;
	full_path = path_new_dir(resolved_name);
	retval = path_is(full_path, fs->game_path);
	path_free(full_path);
	free(resolved_name);
	return retval;
}

vector_t*
fs_list_dir(const fs_t* fs, const char* dirname, bool recursive)
{
	vector_t* list = NULL;
	path_t*   origin_path = NULL;
	char*     resolved_name;

	iter_t iter;

	if (!(resolved_name = resolve(fs, dirname)))
		goto on_error;

	origin_path = path_new_dir(dirname);
	list = vector_new(sizeof(path_t*));
	if (!help_list_dir(list, resolved_name, origin_path, recursive))
		goto on_error;
	path_free(origin_path);
	free(resolved_name);
	return list;

on_error:
	if (list != NULL) {
		iter = vector_enum(list);
		while (iter_next(&iter))
			path_free(*(path_t**)iter.ptr);
		vector_free(list);
	}
	free(resolved_name);
	return NULL;
}

int
fs_mkdir(const fs_t* fs, const char* dirname)
{
	path_t* path;
	char*   resolved_name;
	int     retval;

	if (!(resolved_name = resolve(fs, dirname))) {
		errno = EACCES;
		return -1;
	}
	path = path_new_dir(resolved_name);
	retval = path_mkdir(path) ? 0 : -1;
	path_free(path);
	free(resolved_name);
	return retval;
}

int
fs_rename(const fs_t* fs, const char* old_name, const char* new_name)
{
	char* resolved_old;
	char* resolved_new;
	int   retval;

	resolved_old = resolve(fs, old_name);
	resolved_new = resolve(fs, new_name);
	if (resolved_old == NULL || resolved_new == NULL)
		goto access_denied;
	retval = rename(resolved_old, resolved_new);
	free(resolved_old);
	free(resolved_new);
	return retval;

access_denied:
	free(resolved_old);
	free(resolved_new);
	errno = EACCES;
	return -1;
}

int
fs_rmdir(const fs_t* fs, const char* dirname)
{
	char* resolved_name;
	int   retval;

	if (!(resolved_name = resolve(fs, dirname))) {
		errno = EACCES;
		return -1;
	}
	retval = rmdir(resolved_name);
	free(resolved_name);
	return retval;
}

int
fs_stat(const fs_t* fs, const char* filename, struct stat* p_stat)
{
	char* resolved_name;
	int   result;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return -1;
	}

	result = stat(resolved_name, p_stat);
	free(resolved_name);
	return result;
}

int
fs_unlink(const fs_t* fs, const char* filename)
{
	char* resolved_name;
	int   result;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return -1;
	}

	result = unlink(resolved_name);
	free(resolved_name);
	return result;
}

int
fs_utime(const fs_t* fs, const char* filename, struct utimbuf* in_times)
{
	char* resolved_name;
	int   result;

	if (!(resolved_name = resolve(fs, filename))) {
		errno = EACCES;  // sandboxing violation
		return -1;
	}

	result = utime(resolved_name, in_times);
	free(resolved_name);
	return result;
}

directory_t*
directory_open(fs_t* fs, const char* dirname, bool recursive)
{
	directory_t* directory;

	if (!fs_dir_exists(fs, dirname))
		return NULL;

	if (!(directory = calloc(1, sizeof(directory_t))))
		return NULL;
	directory->fs = fs;
	directory->path = path_new_dir(dirname);
	directory->recursive = recursive;
	return directory;
}

void
directory_close(directory_t* it)
{
	iter_t iter;

	if (it->entries != NULL) {
		iter = vector_enum(it->entries);
		while (iter_next(&iter))
			path_free(*(path_t**)iter.ptr);
		vector_free(it->entries);
	}
	path_free(it->path);
	free(it);
}

int
directory_num_files(directory_t* it)
{
	if (it->entries == NULL)
		directory_rewind(it);
	return vector_len(it->entries);
}

const path_t*
directory_path(const directory_t* it)
{
	return it->path;
}

const char*
directory_pathname(const directory_t* it)
{
	return path_cstr(it->path);
}

int
directory_position(const directory_t* it)
{
	return it->position;
}

const path_t*
directory_next(directory_t* it)
{
	int     num_entries;
	path_t* path;

	if (it->entries == NULL)
		directory_rewind(it);
	num_entries = vector_len(it->entries);
	do {
		if (it->position >= num_entries) {
			path = NULL;
			break;
		}
		path = *(path_t**)vector_get(it->entries, it->position++);
	} while (it->recursive && !path_is_file(path));
	return path;
}

void
directory_rewind(directory_t* it)
{
	vector_t*  path_list;

	iter_t iter;

	path_list = fs_list_dir(it->fs, path_cstr(it->path), it->recursive);

	if (it->entries != NULL) {
		iter = vector_enum(it->entries);
		while (iter_next(&iter))
			path_free(*(path_t**)iter.ptr);
		vector_free(it->entries);
	}
	it->entries = path_list;
	it->position = 0;
}

bool
directory_seek(directory_t* it, int position)
{
	if (it->entries == NULL)
		directory_rewind(it);

	if (position > vector_len(it->entries))
		return false;
	it->position = position;
	return true;
}

static bool
help_list_dir(vector_t* list, const char* dirname, const path_t* origin_path, bool recursive)
{
	tinydir_dir  dir_info;
	tinydir_file file_info;
	path_t*      path;
	path_t*      subdir_origin;
	path_t*      subdir_path;

	size_t i;

	if (tinydir_open_sorted(&dir_info, dirname) != 0)
		return false;
	for (i = 0; i < dir_info.n_files; ++i) {
		tinydir_readfile_n(&dir_info, &file_info, i);
		if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0)
			continue;
		path = file_info.is_dir
			? path_new_dir(file_info.name)
			: path_new(file_info.name);
		path_rebase(path, origin_path);
		vector_push(list, &path);
		if (file_info.is_dir && recursive) {
			subdir_path = path_new_dir(dirname);
			subdir_origin = path_dup(origin_path);
			path_append_dir(subdir_path, file_info.name);
			path_append_dir(subdir_origin, file_info.name);
			if (!help_list_dir(list, path_cstr(subdir_path), subdir_origin, recursive))
				goto on_error;
			path_free(subdir_path);
			path_free(subdir_origin);
		}
	}
	tinydir_close(&dir_info);
	return true;

on_error:
	tinydir_close(&dir_info);
	return false;
}

static char*
resolve(const fs_t* fs, const char* filename)
{
	char*   resolved_name;
	path_t* path;

	path = path_new(filename);
	if (path_rooted(path))
		goto on_error;

	if (path_num_hops(path) == 0)
		path_rebase(path, fs->root_path);
	else if (path_hop_is(path, 0, "$")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->root_path);
	}
	else if (path_hop_is(path, 0, "@")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->game_path);
	}
	else if (path_hop_is(path, 0, "#")) {
		path_remove_hop(path, 0);
		path_rebase(path, fs->system_path);
	}
	else if (path_hop_is(path, 0, "~"))
		if (fs->user_path == NULL) {
			// no user directory set, ~/ is a sandbox violation.
			goto on_error;
		}
		else {
			path_remove_hop(path, 0);
			path_rebase(path, fs->user_path);
		}
	else
		path_rebase(path, fs->root_path);

	resolved_name = strdup(path_cstr(path));
	path_free(path);
	return resolved_name;

on_error:
	path_free(path);
	return NULL;
}
