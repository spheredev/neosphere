#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#define strcasecmp stricmp
#define strtok_r strtok_s
#endif

#if defined(_WIN32)
#include <windows.h>
#include <direct.h>
#define PATH_MAX _MAX_PATH
#define mkdir(path, m) _mkdir(path)
#define realpath(name, r) (_access(name, 00) == 0 ? _fullpath((r), (name), PATH_MAX) : NULL)
#endif

#include "path.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#endif

#define PATH_MAX_HOPS   256
#define PATH_MAX_LENGTH 4096

static path_t* construct_path       (path_t* path, const char* pathname, bool force_dir);
static void    convert_to_directory (path_t* path);
static void    refresh_pathname     (path_t* path);

struct path
{
	char*  filename;
	char** hops;
	size_t num_hops;
	char*  pathname;
};

path_t*
path_new(const char* pathname)
{
	return construct_path(
		calloc(1, sizeof(path_t)),
		pathname, false);
}

path_t*
path_new_dir(const char* pathname)
{
	return construct_path(
		calloc(1, sizeof(path_t)),
		pathname, true);
}

path_t*
path_new_self(void)
{
#if defined(_WIN32)
	char    pathname[MAX_PATH];

	GetModuleFileNameA(NULL, pathname, MAX_PATH);
	return path_new(pathname);
#else
	char        pathname[PATH_MAX];
	ssize_t     pathname_len;

	memset(pathname, 0, sizeof pathname);
	pathname_len = readlink("/proc/self/exe", pathname, PATH_MAX);
	if (pathname_len == -1 || pathname_len == PATH_MAX)
		return NULL;
	return path_new(pathname);
#endif
}

path_t*
path_dup(const path_t* path)
{
	return path_new(path_cstr(path));
}

void
path_free(path_t* path)
{
	size_t i;
	
	if (path == NULL)
		return;
	
	for (i = 0; i < path->num_hops; ++i)
		free(path->hops[i]);
	free(path->hops);
	free(path->filename);
	free(path->pathname);
	free(path);
}

const char*
path_cstr(const path_t* path)
{
	// important note:
	// path_cstr() must be implemented such that path_new(path_cstr(path))
	// results in a perfect duplicate. in practice this means directory paths
	// will always be printed with a trailing slash.

	return path->pathname;
}

const char*
path_extension(const path_t* path)
{
	char* p;
	
	if (path->filename == NULL)
		return NULL;
	else {
		if (p = strrchr(path->filename, '.'))
			return p;
		else
			return "";
	}
}

const char*
path_filename(const path_t* path)
{
	return path->filename;
}

const char*
path_hop(const path_t* path, size_t idx)
{
	return path->hops[idx];
}

bool
path_has_extension(const path_t* path, const char* extension)
{
	const char* my_extension;

	if (!path_is_file(path))
		return false;
	else {
		my_extension = strrchr(path->filename, '.');
		if (my_extension == NULL)
			return extension == NULL || extension[0] == '\0';
		else {
			return strcasecmp(my_extension, extension) == 0;
		}
	}
}

bool
path_is_file(const path_t* path)
{
	return path->filename != NULL;
}

bool
path_is_rooted(const path_t* path)
{
	const char* first_hop = path->num_hops >= 1 ? path->hops[0] : "-";
	return strlen(first_hop) >= 2 && first_hop[1] == ':'
		|| strcmp(first_hop, "") == 0;
}

size_t
path_num_hops(const path_t* path)
{
	return path->num_hops;
}

bool
path_filename_cmp(const path_t* path, const char* name)
{
	if (path->filename == NULL)
		return false;
	return strcmp(path->filename, name) == 0;
}

bool
path_hop_cmp(const path_t* path, size_t idx, const char* name)
{
	return strcmp(path->hops[idx], name) == 0;
}

path_t*
path_append(path_t* path, const char* pathname)
{
	char* parse;
	char* token;
	char  *p_filename;

	char   *p;
	size_t i;

	if (path->filename != NULL)
		goto on_error;

	// normalize path to forward slash delimiters
	parse = strdup(pathname);
	p = parse;
	while (*p != '\0') {
		if (*p == '\\') *p = '/';
		++p;
	}

	// parse the filename out of the path, if any
	if (!(p_filename = strrchr(parse, '/')))
		p_filename = parse;
	else
		++p_filename;
	path->filename = *p_filename != '\0'
		? strdup(p_filename) : NULL;

	*p_filename = '\0';  // strip filename
	if (parse[0] == '/') {
		// pathname is absolute, replace wholesale instead of appending
		for (i = 0; i < path->num_hops; ++i)
			free(path->hops[i]);
		path->num_hops = 0;
		path->hops[path->num_hops++] = strdup("");
	}

	// strtok_r() collapses consecutive separator characters, but this is okay: POSIX
	// requires multiple slashes to be treated as a single separator anyway.
	token = strtok_r(parse, "/", &p);
	while (token != NULL) {
		path->hops[path->num_hops++] = strdup(token);
		token = strtok_r(NULL, "/", &p);
	}
	free(parse);
	
	path_collapse(path, false);
	refresh_pathname(path);
	return path;

on_error:
	return NULL;
}

path_t*
path_append_dir(path_t* path, const char* pathname)
{
	path_append(path, pathname);
	convert_to_directory(path);
	return path;
}

path_t*
path_cat(path_t* path, const path_t* tail)
{
	return path_append(path, path_cstr(tail));
}

path_t*
path_change_name(path_t* path, const char* filename)
{
	free(path->filename);
	path->filename = strdup(filename);
	refresh_pathname(path);
	return path;
}

bool
path_cmp(const path_t* path1, const path_t* path2)
{
	bool    retval;
	path_t* cmp1;
	path_t* cmp2;

	cmp1 = path_collapse(path_dup(path1), false);
	cmp2 = path_collapse(path_dup(path2), false);
	retval = strcmp(path_cstr(cmp1), path_cstr(cmp2)) == 0;
	path_free(cmp1);
	path_free(cmp2);
	return retval;
}

path_t*
path_collapse(path_t* path, bool collapse_uplevel)
{
	const char* hop;
	bool        is_rooted;
	
	size_t i;

	is_rooted = path_is_rooted(path);
	for (i = 0; i < path_num_hops(path); ++i) {
		hop = path_hop(path, i);
		if (strcmp(hop, ".") == 0)
			path_remove_hop(path, i--);
		else if (strcmp(hop, "..") == 0 && i > 0 && collapse_uplevel) {
			path_remove_hop(path, i--);
			if (i > 0 || !is_rooted)  // don't strip the root directory
				path_remove_hop(path, i--);
		}
	}
	refresh_pathname(path);
	return path;
}

path_t*
path_insert_hop(path_t* path, size_t idx, const char* name)
{
	size_t i;
	
	for (i = path->num_hops; i > idx; --i)
		path->hops[i] = path->hops[i - 1];
	++path->num_hops;
	path->hops[idx] = strdup(name);
	refresh_pathname(path);
	return path;
}

bool
path_mkdir(const path_t* path)
{
	bool    is_ok = true;
	path_t* parent_path;
	
	size_t i;

	// appending an empty string to a path is a no-op, so we have to
	// explicitly check for an empty string in the first hop and root the
	// path manually.
	parent_path = path_num_hops(path) > 0 && strcmp(path_hop(path, 0), "") == 0
		? path_new("/") : path_new("./");
	
	// create directories recursively, starting from the rootmost
	// ancestor and working our way down.
	for (i = 0; i < path->num_hops; ++i) {
		path_append_dir(parent_path, path_hop(path, i));
		is_ok = mkdir(path_cstr(parent_path), 0777) == 0;
	}
	path_free(parent_path);
	return is_ok;
}

path_t*
path_rebase(path_t* path, const path_t* root)
{
	char** new_hops;
	size_t num_root_hops;

	size_t i;
	
	if (path_filename(root) != NULL)
		return NULL;
	if (path_is_rooted(path))
		return path;
	new_hops = malloc(PATH_MAX_HOPS * sizeof(char*));
	num_root_hops = path_num_hops(root);
	for (i = 0; i < path_num_hops(root); ++i)
		new_hops[i] = strdup(root->hops[i]);
	for (i = 0; i < path_num_hops(path); ++i)
		new_hops[i + num_root_hops] = path->hops[i];
	free(path->hops);
	path->hops = new_hops;
	path->num_hops += num_root_hops;
	
	path_collapse(path, false);
	return path;
}

path_t*
path_relativize(path_t* path, const path_t* origin)
{
	size_t  num_backhops;
	path_t* origin_path;

	size_t i;

	origin_path = path_collapse(path_strip(path_dup(origin)), false);
	num_backhops = path_num_hops(origin_path);
	while (path->num_hops > 0 && origin_path->num_hops > 0) {
		if (strcmp(origin_path->hops[0], path->hops[0]) != 0)
			break;
		--num_backhops;
		path_remove_hop(origin_path, 0);
		path_remove_hop(path, 0);
	}
	for (i = 0; i < num_backhops; ++i)
		path_insert_hop(path, 0, "..");
	path_free(origin_path);
	return path;
}

path_t*
path_remove_hop(path_t* path, size_t idx)
{
	size_t i;

	free(path->hops[idx]);
	--path->num_hops;
	for (i = idx; i < path->num_hops; ++i)
		path->hops[i] = path->hops[i + 1];
	refresh_pathname(path);
	return path;
}

path_t*
path_resolve(path_t* path, const path_t* relative_to)
{
	path_t*     new_path;
	path_t*     origin;
	char*       pathname;
	struct stat stat_buf;

	if (!(pathname = realpath(path_cstr(path), NULL)))
		return NULL;
	if (stat(path_cstr(path), &stat_buf) != 0) return NULL;
	new_path = stat_buf.st_mode & S_IFDIR
		? path_new_dir(pathname)
		: path_new(pathname);
	free(pathname);
	if (relative_to != NULL) {
		if (!(origin = path_resolve(path_dup(relative_to), NULL)))
			return NULL;
		path_relativize(new_path, origin);
		path_free(origin);
	}
	construct_path(path, path_cstr(new_path), false);
	path_free(new_path);
	return path;
}

path_t*
path_set(path_t* path, const char* pathname)
{
	return construct_path(path, pathname, false);
}

path_t*
path_set_dir(path_t* path, const char* pathname)
{
	return construct_path(path, pathname, true);
}

path_t*
path_strip(path_t* path)
{
	free(path->filename);
	path->filename = NULL;
	refresh_pathname(path);
	return path;
}

static path_t*
construct_path(path_t* path, const char* pathname, bool force_dir)
{
	size_t i;

	// construct_path() may be used to replace a path in-place. in that
	// case we have to free the existing components.
	free(path->filename);
	for (i = 0; i < path->num_hops; ++i)
		free(path->hops[i]);
	free(path->hops);

	// construct the new path
	path->filename = NULL;
	path->hops = malloc(PATH_MAX_HOPS * sizeof(char*));
	path->num_hops = 0;
	if (force_dir) path_append_dir(path, pathname);
		else path_append(path, pathname);
	return path;
}

static void
convert_to_directory(path_t* path)
{
	if (path->filename == NULL)
		return;
	path->hops[path->num_hops++] = path->filename;
	path->filename = NULL;
	refresh_pathname(path);
}

static void
refresh_pathname(path_t* path)
{
	// notes: * the pathname generated here, when used to construct a new path object,
	//          must result in the same type of path (file, directory, etc.). see note
	//          on path_cstr() above.
	//        * the canonical path separator is the forward slash (/).
	//        * directory paths are always reported with a trailing slash.

	char  buffer[PATH_MAX_LENGTH];  // FIXME: security risk

	size_t i;

	strcpy(buffer, path->num_hops == 0 && path->filename == NULL
		? "./" : "");
	for (i = 0; i < path->num_hops; ++i) {
		if (i > 0) strcat(buffer, "/");
		strcat(buffer, path->hops[i]);
	}
	if (path->num_hops > 0) strcat(buffer, "/");
	if (path->filename != NULL)
		strcat(buffer, path->filename);
	free(path->pathname);
	path->pathname = strdup(buffer);
}
