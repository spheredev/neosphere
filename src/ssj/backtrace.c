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
	backtrace_t* bt;

	bt = calloc(1, sizeof(backtrace_t));
	return bt;
}

void
backtrace_free(backtrace_t* bt)
{
	int i;

	if (bt == NULL)
		return;
	
	for (i = 0; i < bt->num_frames; ++i) {
		free(bt->frames[i].name);
		free(bt->frames[i].filename);
	}
	free(bt->frames);
	free(bt);
}

int
backtrace_len(const backtrace_t* bt)
{
	return bt->num_frames;
}

const char*
backtrace_get_name(const backtrace_t* bt, int index)
{
	return bt->frames[index].name;
}

const char*
backtrace_get_filename(const backtrace_t* bt, int index)
{
	return bt->frames[index].filename;
}

int
backtrace_get_lineno(const backtrace_t* bt, int index)
{
	return bt->frames[index].lineno;
}

void
backtrace_add(backtrace_t* bt, const char* name, const char* filename, int line_no)
{
	int index;
	
	index = bt->num_frames++;
	bt->frames = realloc(bt->frames, bt->num_frames * sizeof(struct frame));
	bt->frames[index].name = strdup(name);
	bt->frames[index].filename = strdup(filename);
	bt->frames[index].lineno = line_no;
}

void
backtrace_print(const backtrace_t* bt, int active_frame, bool show_all)
{
	const char* arrow;
	const char* filename;
	int         line_no;
	const char* name;

	int i;

	for (i = 0; i < backtrace_len(bt); ++i) {
		name = backtrace_get_name(bt, i);
		filename = backtrace_get_filename(bt, i);
		line_no = backtrace_get_lineno(bt, i);
		if (i == active_frame || show_all) {
			arrow = i == active_frame ? "->" : "  ";
			if (line_no > 0)
				printf("%s #%2d: %s at %s:%d\n", arrow, i, name, filename, line_no);
			else
				printf("%s #%2d: %s <system call>\n", arrow, i, name);
		}
	}
}
