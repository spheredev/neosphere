#include "ssj.h"

#include "file_line.h"

struct fileline
{
	char* filename;
	int   line_no;
};

fileline_t*
new_fileline(const char* filename, int line_no)
{
	fileline_t* obj;

	obj = calloc(1, sizeof(fileline_t));
	obj->filename = strdup(filename);
	obj->line_no = line_no;
	return obj;
}

void
free_fileline(fileline_t* obj)
{
	free(obj->filename);
	free(obj);
}

const char*
fileline_filename(fileline_t* obj)
{
	return obj->filename;
}

int
fileline_line_no(fileline_t* obj)
{
	return obj->line_no;
}
