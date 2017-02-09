#include "minisphere.h"
#include "obsmap.h"

struct obsmap
{
	unsigned int id;
	int          num_lines;
	int          max_lines;
	rect_t       *lines;
};

static unsigned int s_next_obsmap_id = 0;

obsmap_t*
obsmap_new(void)
{
	obsmap_t* obsmap = NULL;

	console_log(4, "creating new obstruction map #%u", s_next_obsmap_id);
	
	obsmap = calloc(1, sizeof(obsmap_t));
	obsmap->max_lines = 0;
	obsmap->num_lines = 0;

	obsmap->id = s_next_obsmap_id++;
	return obsmap;
}

void
obsmap_free(obsmap_t* obsmap)
{
	if (obsmap == NULL)
		return;
	console_log(4, "disposing obstruction map #%u no longer in use", obsmap->id);
	free(obsmap->lines);
	free(obsmap);
}

bool
obsmap_add_line(obsmap_t* obsmap, rect_t line)
{
	int    new_size;
	rect_t *line_list;
	
	console_log(4, "adding line segment (%i,%i)-(%i,%i) to obstruction map #%u",
		line.x1, line.y1, line.x2, line.y2, obsmap->id);
	
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
obsmap_test_line(const obsmap_t* obsmap, rect_t line)
{
	int i;

	for (i = 0; i < obsmap->num_lines; ++i) {
		if (do_lines_intersect(line, obsmap->lines[i]))
			return true;
	}
	return false;
}

bool
obsmap_test_rect(const obsmap_t* obsmap, rect_t rect)
{
	return obsmap_test_line(obsmap, new_rect(rect.x1, rect.y1, rect.x2, rect.y1))
		|| obsmap_test_line(obsmap, new_rect(rect.x2, rect.y1, rect.x2, rect.y2))
		|| obsmap_test_line(obsmap, new_rect(rect.x1, rect.y2, rect.x2, rect.y2))
		|| obsmap_test_line(obsmap, new_rect(rect.x1, rect.y1, rect.x1, rect.y2));
}
