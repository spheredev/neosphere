#ifndef MINISPHERE__GEOMETRY_H__INCLUDED
#define MINISPHERE__GEOMETRY_H__INCLUDED

typedef
struct point2
{
	int x;
	int y;
	int z;
} point2_t;

typedef
struct point3
{
	int x;
	int y;
	int z;
} point3_t;

typedef
struct rect
{
	int x1;
	int y1;
	int x2;
	int y2;
} rect_t;

typedef struct size2
{
	int width;
	int height;
} size2_t;

typedef
struct float_rect
{
	float x1;
	float y1;
	float x2;
	float y2;
} float_rect_t;

point2_t     point2               (int x, int y);
point3_t     point3               (int x, int y, int z);
rect_t       rect                 (int x1, int y1, int x2, int y2);
float_rect_t rectf                (float x1, float y1, float x2, float y2);
size2_t      size2                (int width, int height);
bool         do_lines_intersect   (rect_t a, rect_t b);
bool         do_rects_intersect   (rect_t a, rect_t b);
bool         is_point_in_rect     (int x, int y, rect_t bounds);
void         normalize_rect       (rect_t* inout_rect);
float_rect_t scale_float_rect     (float_rect_t rect, float x_scale, float y_scale);
rect_t       translate_rect       (rect_t rect, int x_offset, int y_offset);
float_rect_t translate_float_rect (float_rect_t rect, float x_offset, float y_offset);
rect_t       zoom_rect            (rect_t rect, double scale_x, double scale_y);

#endif // MINISPHERE__GEOMETRY_H__INCLUDED
