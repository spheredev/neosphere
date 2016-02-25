#include "ssj.h"
#include "source.h"

#include "session.h"

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
source_load(session_t* session, const char* filename)
{
	int         cache_id;
	char*       line_text;
	vector_t*   lines;
	message_t*  msg;
	source_t*   source = NULL;
	const char* text;
	const char* p_source;

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

	msg = msg_new(MSG_TYPE_REQ);
	msg_add_int(msg, REQ_APP_REQUEST);
	msg_add_int(msg, APPREQ_SOURCE);
	msg_add_string(msg, filename);
	msg = do_request(session, msg);
	if (msg_type(msg) == MSG_TYPE_ERR)
		goto on_error;
	p_source = text = msg_get_string(msg, 0);
	lines = vector_new(sizeof(char*));
	while (line_text = read_line(&p_source))
		vector_push(lines, &line_text);
	msg_free(msg);

	source = calloc(1, sizeof(source_t));
	source->lines = lines;
	s_cache[cache_id].filename = strdup(filename);
	s_cache[cache_id].source = source;
	return source;

on_error:
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
