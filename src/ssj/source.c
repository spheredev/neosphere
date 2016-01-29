#include "ssj.h"
#include "source.h"

struct source
{
	vector_t* lines;
};

source_t*
load_source(const path_t* path, const path_t* source_path)
{
	path_t*   full_path;
	FILE*     h_file;
	vector_t* lines;
	source_t* source;

	full_path = path_rebase(path_dup(path), source_path);
	h_file = fopen(path_cstr(full_path), "rt");
	lines = vector_new(sizeof(lstring_t*));

	source = calloc(1, sizeof(source_t));
	return source;

on_error:
	free(source);
	return NULL;
}

void
free_source(source_t* source)
{
	vector_free(source->lines);
	free(source);
}
