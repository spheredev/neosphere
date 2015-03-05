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
