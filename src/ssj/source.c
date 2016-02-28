#include "ssj.h"
#include "source.h"

struct source
{
	vector_t* lines;
};

static char*
read_line(const char** p_string)
{
	char*  buffer;
	size_t buf_size;
	char   ch;
	bool   have_line = false;
	size_t length;

	buffer = malloc(buf_size = 256);
	length = 0;
	while (!have_line) {
		if (length + 1 >= buf_size)
			buffer = realloc(buffer, buf_size *= 2);
		if ((ch = *(*p_string)) == '\0')
			goto hit_eof;
		++(*p_string);
		switch (ch) {
		case '\n': have_line = true; break;
		case '\r':
			have_line = true;
			if (*(*p_string) == '\n')  // CR LF?
				++(*p_string);
			break;
		default:
			buffer[length++] = ch;
		}
	}

hit_eof:
	buffer[length] = '\0';
	if (*(*p_string) == '\0' && length == 0) {
		free(buffer);
		return NULL;
	}
	else
		return buffer;
}

source_t*
source_new(const char* text)
{
	char*       line_text;
	vector_t*   lines;
	source_t*   source = NULL;
	const char* p_source;

	p_source = text;
	lines = vector_new(sizeof(char*));
	while (line_text = read_line(&p_source))
		vector_push(lines, &line_text);

	source = calloc(1, sizeof(source_t));
	source->lines = lines;
	return source;
}

void
source_free(source_t* source)
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

void
source_print(const source_t* source, int lineno, int num_lines, int active_lineno)
{
	const char* arrow;
	int         line_count;
	int         median;
	int         start, end;
	const char* text;

	int i;

	line_count = source_cloc(source);
	median = num_lines / 2;
	start = lineno > median ? lineno - (median + 1) : 0;
	end = start + num_lines < line_count ? start + num_lines : line_count;
	for (i = start; i < end; ++i) {
		text = source_get_line(source, i);
		arrow = i + 1 == active_lineno ? "=>" : "  ";
		if (num_lines > 1)
			printf("%s %4d %s\n", arrow, i + 1, text);
		else
			printf("%d %s\n", i + 1, text);
	}
}
