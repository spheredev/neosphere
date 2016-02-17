#include "ssj.h"
#include "source.h"

#define CACHE_SIZE 10

struct source
{
	vector_t* lines;
};

struct cache_entry
{
	char*     filename;
	source_t* source;
};

static struct cache_entry s_cache[CACHE_SIZE];
static int                s_next_cache_id;

static void
free_source(source_t* source)
{
	iter_t it;
	char*  *p_line;

	if (source == NULL) return;
	it = vector_enum(source->lines);
	while (p_line = vector_next(&it))
		free(*p_line);
	vector_free(source->lines);
	free(source);
}

static char*
read_line(FILE* h_file)
{
	char*  buffer;
	size_t buf_size;
	char   ch;
	bool   have_line = false;
	size_t index;

	buffer = malloc(buf_size = 256);
	index = 0;
	while (!have_line) {
		if (index + 1 >= buf_size)
			buffer = realloc(buffer, buf_size *= 2);
		if (fread(&ch, 1, 1, h_file) != 1) {
			if (feof(h_file))
				goto hit_eof;
			else
				goto on_error;
		}
		switch (ch) {
		case '\n': have_line = true; break;
		case '\r':
			if (fread(&ch, 1, 1, h_file) != 1 && !feof(h_file))
				goto on_error;
			if (!feof(h_file) && ch != '\n')
				ungetc(ch, h_file);
			have_line = true;
			break;
		default:
			buffer[index++] = ch;
		}
	}
	buffer[index] = '\0';
	return buffer;

hit_eof:
	buffer[index] = '\0';
	if (buffer[0] != '\0')
		return buffer;
	else {
		free(buffer);
		return NULL;
	}

on_error:
	free(buffer);
	return NULL;
}

source_t*
source_load(const char* filename, const path_t* source_path)
{
	int       cache_id;
	path_t*   full_path;
	FILE*     h_file = NULL;
	vector_t* lines;
	source_t* source = NULL;
	char*     text;

	int i;

	for (i = 0; i < CACHE_SIZE; ++i) {
		if (s_cache[i].filename != NULL && strcmp(filename, s_cache[i].filename) == 0)
			return s_cache[i].source;
	}
	cache_id = s_next_cache_id++;
	if (s_next_cache_id >= CACHE_SIZE)
		s_next_cache_id = 0;
	free(s_cache[cache_id].filename);
	free_source(s_cache[cache_id].source);

	if (source_path == NULL)
		return NULL;
	full_path = path_rebase(path_new(filename), source_path);
	if (!(h_file = fopen(path_cstr(full_path), "rb")))
		goto on_error;
	path_free(full_path);
	lines = vector_new(sizeof(char*));
	while (text = read_line(h_file))
		vector_push(lines, &text);
	fclose(h_file);

	source = calloc(1, sizeof(source_t));
	source->lines = lines;
	s_cache[cache_id].filename = strdup(filename);
	s_cache[cache_id].source = source;
	return source;

on_error:
	if (h_file != NULL)
        fclose(h_file);
	free(source);
	return NULL;
}

int
source_cloc(const source_t* source)
{
	return (int)vector_len(source->lines);
}

const char*
source_get_line(const source_t* source, int line_index)
{
	if (line_index < 0 || line_index >= source_cloc(source))
		return NULL;
	return *(char**)vector_get(source->lines, line_index);
}
