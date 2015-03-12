#include "minisphere.h"

#include "obsmap.h"

struct obsmap
{
	int    num_lines;
	int    max_lines;
	rect_t *lines;
};

obsmap_t*
new_obsmap(void)
{
	obsmap_t* obsmap = NULL;

	if ((obsmap = calloc(1, sizeof(obsmap_t))) == NULL)
		return NULL;
	obsmap->max_lines = 0;
	obsmap->num_lines = 0;
	return obsmap;
}

void
free_obsmap(obsmap_t* obsmap)
{
	if (obsmap == NULL)
		return;
	free(obsmap->lines);
	free(obsmap);
}

bool
add_obsmap_line(obsmap_t* obsmap, rect_t line)
{
	int    new_size;
	rect_t *line_list;
	
	if (obsmap->num_lines + 1 > obsmap->max_lines) {
		new_size = (obsmap->num_lines + 1) * 2;
		if ((line_list = realloc(obsmap->lines, new_size * sizeof(rect_t))) == NULL)
			return false;
		obsmap->max_lines = new_size;
		obsmap->lines = line_list;
	}
	obsmap->lines[obsmap->num_lines] = line;
	++obsmap->num_lines;
	return true;
}

bool
test_obsmap_line(const obsmap_t* obsmap, rect_t line)
{
	int i;

	for (i = 0; i < obsmap->num_lines; ++i) {
		if (do_lines_intersect(line, obsmap->lines[i]))
			return true;
	}
	return false;
}

bool
test_obsmap_rect(const obsmap_t* obsmap, rect_t rect)
{
	return test_obsmap_line(obsmap, new_rect(rect.x1, rect.y1, rect.x2, rect.y1))
		|| test_obsmap_line(obsmap, new_rect(rect.x2, rect.y1, rect.x2, rect.y2))
		|| test_obsmap_line(obsmap, new_rect(rect.x1, rect.y2, rect.x2, rect.y2))
		|| test_obsmap_line(obsmap, new_rect(rect.x1, rect.y1, rect.x1, rect.y2));
}
