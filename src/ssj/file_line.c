#include "ssj.h"

#include "file_line.h"

struct file_line
{
	char* filename;
	int   lineno;
};

file_line_t*
new_fileline(const char* filename, int line_no)
{
	file_line_t* obj;

	obj = calloc(1, sizeof(file_line_t));
	obj->filename = strdup(filename);
	obj->lineno = line_no;
	return obj;
}

void
free_fileline(file_line_t* obj)
{
	free(obj->filename);
	free(obj);
}

const char*
file_line_filename(file_line_t* obj)
{
	return obj->filename;
}

int
file_line_lineno(file_line_t* obj)
{
	return obj->lineno;
}
