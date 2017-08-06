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

rect_t       new_rect             (int x1, int y1, int x2, int y2);
float_rect_t new_float_rect       (float x1, float y1, float x2, float y2);
bool         do_lines_intersect   (rect_t a, rect_t b);
bool         do_rects_intersect   (rect_t a, rect_t b);
bool         is_point_in_rect     (int x, int y, rect_t bounds);
void         normalize_rect       (rect_t* inout_rect);
float_rect_t scale_float_rect     (float_rect_t rect, float x_scale, float y_scale);
rect_t       translate_rect       (rect_t rect, int x_offset, int y_offset);
float_rect_t translate_float_rect (float_rect_t rect, float x_offset, float y_offset);
rect_t       zoom_rect            (rect_t rect, double scale_x, double scale_y);

bool fread_rect_16 (sfs_file_t* file, rect_t* out_rect);
bool fread_rect_32 (sfs_file_t* file, rect_t* out_rect);

#endif // MINISPHERE__GEOMETRY_H__INCLUDED
