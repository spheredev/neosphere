#include "ssj.h"
#include "source.h"

struct source
{
	vector_t* lines;
};

static char* freadline (FILE* h_file);
static char* fslurp    (const char* filename);

source_t*
load_source(const char* filename, const path_t* source_path)
{
	path_t*   full_path;
	FILE*     h_file = NULL;
	vector_t* lines;
	source_t* source = NULL;
	char*     text;

	if (source_path == NULL)
		return NULL;
	full_path = path_rebase(path_new(filename), source_path);
	if (!(h_file = fopen(path_cstr(full_path), "rb")))
        goto on_error;
	lines = vector_new(sizeof(char*));
    while (text = freadline(h_file))
        vector_push(lines, &text);

	source = calloc(1, sizeof(source_t));
	source->lines = lines;
	return source;

on_error:
	if (h_file != NULL)
        fclose(h_file);
	free(source);
	return NULL;
}

void
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

const char*
get_source_line(const source_t* source, int line_index)
{
	if (line_index < 0 || line_index >= get_source_size(source))
		return NULL;
	return *(char**)vector_get(source->lines, line_index);
}

int
get_source_size(const source_t* source)
{
	return (int)vector_len(source->lines);
}

static char*
freadline(FILE* h_file)
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

static char*
fslurp(const char* filename)
{
	char*  buffer = NULL;
	FILE*  h_file;
	size_t length;

	if (!(h_file = fopen(filename, "rb")))
        goto on_error;
    fseek(h_file, 0, SEEK_END);
    length = ftell(h_file);
    fseek(h_file, 0, SEEK_SET);
    if (fread(buffer = malloc(length), 1, length, h_file) != length)
        goto on_error;
	fclose(h_file);
	return buffer;

on_error:
	if (h_file != NULL) fclose(h_file);
    free(buffer);
    return NULL;
}
