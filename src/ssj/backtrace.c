#include "ssj.h"
#include "backtrace.h"

struct frame
{
	char* name;
	char* filename;
	int   lineno;
};

struct backtrace
{
	int           num_frames;
	struct frame* frames;
};

backtrace_t*
backtrace_new(void)
{
	backtrace_t* obj;

	obj = calloc(1, sizeof(backtrace_t));
	return obj;
}

void
backtrace_free(backtrace_t* obj)
{
	int i;

	if (obj == NULL)
		return;
	
	for (i = 0; i < obj->num_frames; ++i) {
		free(obj->frames[i].name);
		free(obj->frames[i].filename);
	}
	free(obj->frames);
	free(obj);
}

int
backtrace_len(const backtrace_t* obj)
{
	return obj->num_frames;
}

const char*
backtrace_get_name(const backtrace_t* obj, int index)
{
	return obj->frames[index].name;
}

const char*
backtrace_get_filename(const backtrace_t* obj, int index)
{
	return obj->frames[index].filename;
}

int
backtrace_get_lineno(const backtrace_t* obj, int index)
{
	return obj->frames[index].lineno;
}

void
backtrace_add(backtrace_t* obj, const char* name, const char* filename, int line_no)
{
	int index;
	
	index = obj->num_frames++;
	obj->frames = realloc(obj->frames, obj->num_frames * sizeof(struct frame));
	obj->frames[index].name = strdup(name);
	obj->frames[index].filename = strdup(filename);
	obj->frames[index].lineno = line_no;
}

void
backtrace_print(const backtrace_t* obj, int active_frame, bool show_all)
{
	const char* arrow;
	const char* filename;
	int         line_no;
	const char* name;

	int i;

	for (i = 0; i < backtrace_len(obj); ++i) {
		name = backtrace_get_name(obj, i);
		filename = backtrace_get_filename(obj, i);
		line_no = backtrace_get_lineno(obj, i);
		if (i == active_frame || show_all) {
			arrow = i == active_frame ? "=>" : "  ";
			if (line_no > 0)
				printf("%s #%2d: %s at %s:%d\n", arrow, i, name, filename, line_no);
			else
				printf("%s #%2d: %s <system call>\n", arrow, i, name);
		}
	}
}
