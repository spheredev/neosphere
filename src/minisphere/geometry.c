#include "minisphere.h"
#include "geometry.h"

point2_t
point2(int x, int y)
{
	point2_t point;

	point.x = x;
	point.y = y;
	return point;
}

point3_t
point3(int x, int y, int z)
{
	point3_t point;

	point.x = x;
	point.y = y;
	point.z = z;
	return point;
}

size2_t
size2(int width, int height)
{
	size2_t size;

	size.width = width;
	size.height = height;
	return size;
}

rect_t
new_rect(int x1, int y1, int x2, int y2)
{
	rect_t rectangle;

	rectangle.x1 = x1;
	rectangle.y1 = y1;
	rectangle.x2 = x2;
	rectangle.y2 = y2;
	return rectangle;
}

float_rect_t
new_float_rect(float x1, float y1, float x2, float y2)
{
	float_rect_t rectangle;

	rectangle.x1 = x1;
	rectangle.y1 = y1;
	rectangle.x2 = x2;
	rectangle.y2 = y2;
	return rectangle;
}

bool
do_lines_intersect(rect_t a, rect_t b)
{
	float d, q, r, s;

	q = (a.y1 - b.y1) * (b.x2 - b.x1) - (a.x1 - b.x1) * (b.y2 - b.y1);
	d = (a.x2 - a.x1) * (b.y2 - b.y1) - (a.y2 - a.y1) * (b.x2 - b.x1);
	if (d == 0)
		return false;
	r = q / d;
	q = (a.y1 - b.y1) * (a.x2 - a.x1) - (a.x1 - b.x1) * (a.y2 - a.y1);
	s = q / d;
	return !(r < 0 || r > 1 || s < 0 || s > 1);
}

bool
do_rects_intersect(rect_t a, rect_t b)
{
	return !(a.x1 > b.x2 || a.x2 < b.x1 || a.y1 > b.y2 || a.y2 < b.y1);
}

bool
is_point_in_rect(int x, int y, rect_t bounds)
{
	return x >= bounds.x1 && x < bounds.x2
		&& y >= bounds.y1 && y < bounds.y2;
}

void
normalize_rect(rect_t* inout_rect)
{
	int tmp;
	
	if (inout_rect->x1 > inout_rect->x2) {
		tmp = inout_rect->x1;
		inout_rect->x1 = inout_rect->x2;
		inout_rect->x2 = tmp;
	}
	if (inout_rect->y1 > inout_rect->y2) {
		tmp = inout_rect->y1;
		inout_rect->y1 = inout_rect->y2;
		inout_rect->y2 = tmp;
	}
}

float_rect_t
scale_float_rect(float_rect_t rect, float x_scale, float y_scale)
{
	return new_float_rect(
		rect.x1 * x_scale, rect.y1 * y_scale,
		rect.x2 * x_scale, rect.y2 * y_scale);
}

rect_t
translate_rect(rect_t rect, int x_offset, int y_offset)
{
	return new_rect(
		rect.x1 + x_offset, rect.y1 + y_offset,
		rect.x2 + x_offset, rect.y2 + y_offset);
}

float_rect_t
translate_float_rect(float_rect_t rect, float x_offset, float y_offset)
{
	return new_float_rect(
		rect.x1 + x_offset, rect.y1 + y_offset,
		rect.x2 + x_offset, rect.y2 + y_offset);
}

rect_t
zoom_rect(rect_t rect, double scale_x, double scale_y)
{
	return new_rect(
		rect.x1 * scale_x, rect.y1 * scale_y,
		rect.x2 * scale_x, rect.y2 * scale_y);
}

bool
fread_rect_16(sfs_file_t* file, rect_t* out_rect)
{
	int16_t x1, y1, x2, y2;

	if (sfs_fread(&x1, 2, 1, file) != 1) return false;
	if (sfs_fread(&y1, 2, 1, file) != 1) return false;
	if (sfs_fread(&x2, 2, 1, file) != 1) return false;
	if (sfs_fread(&y2, 2, 1, file) != 1) return false;
	out_rect->x1 = x1; out_rect->y1 = y1;
	out_rect->x2 = x2; out_rect->y2 = y2;
	return true;
}

bool
fread_rect_32(sfs_file_t* file, rect_t* out_rect)
{
	int32_t x1, y1, x2, y2;

	if (sfs_fread(&x1, 4, 1, file) != 1) return false;
	if (sfs_fread(&y1, 4, 1, file) != 1) return false;
	if (sfs_fread(&x2, 4, 1, file) != 1) return false;
	if (sfs_fread(&y2, 4, 1, file) != 1) return false;
	out_rect->x1 = x1; out_rect->y1 = y1;
	out_rect->x2 = x2; out_rect->y2 = y2;
	return true;
}
