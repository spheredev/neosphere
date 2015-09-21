#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "path.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define PATH_MAX_HOPS 256

static void promote_filename (path_t* path);
static void update_pathname  (path_t* path);

struct path
{
	char*  filename;
	char** hops;
	size_t num_hops;
	char*  pathname;
};

path_t*
path_new(const char* pathname, bool force_dir_path)
{
	path_t* path;
	
	path = calloc(1, sizeof(path_t));
	
	path->hops = malloc(PATH_MAX_HOPS * sizeof(char*));
	path_append(path, pathname, force_dir_path);
	return path;
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

path_t*
path_dup(const path_t* path)
{
	return path_new(path_cstr(path), false);
}

const char*
path_cstr(const path_t* path)
{
	return path->pathname;
}

const char*
path_filename_cstr(const path_t* path)
{
	return path->filename;
}

const char*
path_hop_cstr(const path_t* path, size_t idx)
{
	return path->hops[idx];
}

bool
path_is_rooted(const path_t* path)
{
	const char* first_part = path->num_hops >= 1 ? path->hops[0] : "Unforgivable!";
	return strlen(first_part) >= 2 && first_part[1] == ':'
		|| strcmp(first_part, "") == 0;
}

size_t
path_num_hops(const path_t* path)
{
	return path->num_hops;
}

path_t*
path_append(path_t* path, const char* pathname, bool force_dir_path)
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
	do {
		if (*p == '\\') *p = '/';
	} while (*++p != '\0');

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

	// this isn't an exact science, strtok() collapses consecutive delimiters for
	// example. then again the Windows command prompt and Bash both exhibit similar
	// behavior, so I think we're okay.
	token = strtok(parse, "/");
	while (token != NULL) {
		path->hops[path->num_hops++] = strdup(token);
		token = strtok(NULL, "/");
	}
	free(parse);
	
	if (force_dir_path)
		promote_filename(path);
	path_collapse(path, false);
	update_pathname(path);
	return path;

on_error:
	return NULL;
}

path_t*
path_collapse(path_t* path, bool collapse_double_dots)
{
	int         depth = 0;
	const char* hop;
	bool        is_rooted;
	
	size_t i;

	is_rooted = path_is_rooted(path);
	for (i = 0; i < path_num_hops(path); ++i) {
		++depth;
		hop = path_hop_cstr(path, i);
		if (strcmp(hop, ".") == 0) {
			--depth;
			path_remove_hop(path, i--);
		}
		else if (strcmp(hop, "..") == 0) {
			depth -= 2;
			if (!collapse_double_dots)
				continue;
			path_remove_hop(path, i--);
			if (i > 0 || !is_rooted)  // don't strip the root directory
				path_remove_hop(path, i--);
		}
	}
	update_pathname(path);
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
	update_pathname(path);
	return path;
}

path_t*
path_strip(path_t* path)
{
	free(path->filename);
	path->filename = NULL;
	update_pathname(path);
	return path;
}

static void
promote_filename(path_t* path)
{
	if (path->filename == NULL)
		return;
	path->hops[path->num_hops++] = path->filename;
	path->filename = NULL;
}

static void
update_pathname(path_t* path)
{
	// notes: * the canonical path separator is the forward slash (/).
	//        * directory paths are always reported with a trailing slash.
	//        * the pathname generated here, when used to construct a new path object,
	//          must result in the same type of path (file, directory, etc.).

	char  buffer[1024];  // FIXME: security risk

	size_t i;

	strcpy(buffer, !path_is_rooted(path) ? "./" : "");
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
