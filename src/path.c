#include "path.h"

#include "vector.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static void update_pathname (path_t* path);

struct path
{
	vector_t* parts;
	char*     pathname;
};

path_t*
new_path(const char* pathname)
{
	char*   parse;
	path_t* path;
	char*   path_part;
	char*   token;

	path = calloc(1, sizeof(path_t));
	path->parts = new_vector(sizeof(char*));
	token = strtok(parse = strdup(pathname), "/\\");
	do {
		path_part = strdup(token);
		push_back_vector(path->parts, &path_part);
	} while (token = strtok(NULL, "/\\"));
	free(parse);
	update_pathname(path);
	return path;
}

void
free_path(path_t* path)
{
	char* *p_part;
	
	iter_t iter;
	
	if (path == NULL)
		return;
	iter = iterate_vector(path->parts);
	while (p_part = next_vector_item(&iter))
		free(*p_part);
	free_vector(path->parts);
	free(path->pathname);
	free(path);
}

const char*
path_cstr(const path_t* path)
{
	return path->pathname;
}

static void
update_pathname(path_t* path)
{
	char*       buffer;
	size_t      bufsize = 4;
	char*       *p_part;
	
	iter_t iter;
	
	iter = iterate_vector(path->parts);
	while (p_part = next_vector_item(&iter)) {
		if (iter.index > 0) bufsize += 1;
		bufsize += strlen(*p_part);
	}
	buffer = malloc(bufsize);
	buffer[0] = '\0';
	iter = iterate_vector(path->parts);
	while (p_part = next_vector_item(&iter)) {
		if (iter.index > 0) strcat(buffer, "/");
		strcat(buffer, *p_part);
	}
	free(path->pathname);
	path->pathname = buffer;
}
