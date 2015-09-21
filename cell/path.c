#ifdef _MSC_VER
#define _CRT_NONSTDC_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "path.h"

#include "vector.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static void promote_filename (path_t* path);
static void update_pathname  (path_t* path);

struct path
{
	char*     filename;
	vector_t* hops;
	char*     pathname;
};

path_t*
path_new(const char* pathname, bool force_dir_path)
{
	path_t* path;
	
	path = calloc(1, sizeof(path_t));
	
	path->hops = new_vector(sizeof(char*));
	path_append(path, pathname, force_dir_path);
	return path;
}

void
path_free(path_t* path)
{
	char* *p_hop;
	
	iter_t iter;
	
	if (path == NULL)
		return;
	iter = iterate_vector(path->hops);
	while (p_hop = next_vector_item(&iter))
		free(*p_hop);
	free_vector(path->hops);
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

bool
path_is_rooted(const path_t* path)
{
	const char* first_part = get_vector_size(path->hops) >= 1
		? *(const char**)get_vector_item(path->hops, 0)
		: "Unforgivable!";
	return strlen(first_part) >= 2 && first_part[1] == ':'
		|| strcmp(first_part, "") == 0;
}

bool
path_append(path_t* path, const char* pathname, bool force_dir_path)
{
	char*   hop;
	char*   parse;
	char*   token;
	char    *p_filename;

	char* p;

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
		clear_vector(path->hops);
		hop = strdup("");
		push_back_vector(path->hops, &hop);
	}

	// this isn't an exact science, strtok() collapses consecutive delimiters for
	// example. then again the Windows command prompt and Bash both exhibit similar
	// behavior, so I think we're okay.
	token = strtok(parse, "/");
	while (token != NULL) {
		hop = strdup(token);
		push_back_vector(path->hops, &hop);
		token = strtok(NULL, "/");
	}
	free(parse);
	
	if (force_dir_path)
		promote_filename(path);
	path_collapse(path, false);
	update_pathname(path);
	return true;

on_error:
	return false;
}

path_t*
path_collapse(path_t* path, bool collapse_double_dots)
{
	int   depth = 0;
	char* hop;
	bool  is_rooted;
	
	size_t i;

	is_rooted = path_is_rooted(path);
	for (i = 0; i < get_vector_size(path->hops); ++i) {
		++depth;
		hop = *(char**)get_vector_item(path->hops, i);
		if (strcmp(hop, ".") == 0) {
			--depth;
			remove_vector_item(path->hops, i--);
			free(hop);
		}
		else if (strcmp(hop, "..") == 0) {
			depth -= 2;
			if (!collapse_double_dots)
				continue;
			if (is_rooted) {
				remove_vector_item(path->hops, i--);
				if (i > 0)  // don't strip the root directory
					remove_vector_item(path->hops, i--);
			}
			else if (depth >= 0) {
				remove_vector_item(path->hops, i--);
				remove_vector_item(path->hops, i--);
			}
		}
	}
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
	if (path->filename != NULL)
		push_back_vector(path->hops, &path->filename);
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
	char* *p_hop;

	iter_t iter;

	strcpy(buffer, !path_is_rooted(path) ? "./" : "");
	iter = iterate_vector(path->hops);
	while (p_hop = next_vector_item(&iter)) {
		if (iter.index > 0) strcat(buffer, "/");
		strcat(buffer, *p_hop);
	}
	if (get_vector_size(path->hops) > 0)
		strcat(buffer, "/");
	if (path->filename != NULL)
		strcat(buffer, path->filename);
	free(path->pathname);
	path->pathname = strdup(buffer);
}
