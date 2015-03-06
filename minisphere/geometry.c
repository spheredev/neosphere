#include "minisphere.h"

#include "geometry.h"

bool
is_point_in_rect(int x, int y, rect_t bounds)
{
	return x >= bounds.x1 && x < bounds.x2
		&& y >= bounds.y1 && y < bounds.y2;
}

bool
collide_rects(rect_t a, rect_t b)
{
	return !(a.x1 >= b.x2 || a.x2 <= b.x1 || a.y1 >= b.y2 || a.y2 <= b.y1);
}

bool
do_lines_intersect(rect_t a, rect_t b)
{
	return false;
}

bool
al_fread_rect(ALLEGRO_FILE* file, rect_t* out_rect)
{
	int16_t x1, y1, x2, y2;

	if (al_fread(file, &x1, 2) != 2) return false;
	if (al_fread(file, &y1, 2) != 2) return false;
	if (al_fread(file, &x2, 2) != 2) return false;
	if (al_fread(file, &y2, 2) != 2) return false;
	out_rect->x1 = x1; out_rect->y1 = y1;
	out_rect->x2 = x2; out_rect->y2 = y2;
	return true;
}
