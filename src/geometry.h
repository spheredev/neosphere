#ifndef MINISPHERE__GEOMETRY_H__INCLUDED
#define MINISPHERE__GEOMETRY_H__INCLUDED

typedef struct point3     point3_t;
typedef struct rect       rect_t;
typedef struct float_rect float_rect_t;

extern rect_t       new_rect             (int x1, int y1, int x2, int y2);
extern float_rect_t new_float_rect       (float x1, float y1, float x2, float y2);
extern bool         do_lines_intersect   (rect_t a, rect_t b);
extern bool         do_rects_intersect   (rect_t a, rect_t b);
extern bool         is_point_in_rect     (int x, int y, rect_t bounds);
extern float_rect_t scale_float_rect     (float_rect_t rect, float x_scale, float y_scale);
extern rect_t       translate_rect       (rect_t rect, int x_offset, int y_offset);
extern float_rect_t translate_float_rect (float_rect_t rect, float x_offset, float y_offset);
extern rect_t       zoom_rect            (rect_t rect, double scale_x, double scale_y);

extern bool fread_rect_16 (FILE* file, rect_t* out_rect);
extern bool fread_rect_32 (FILE* file, rect_t* out_rect);

struct point3
{
	int x;
	int y;
	int z;
};

struct rect
{
	int x1;
	int y1;
	int x2;
	int y2;
};

struct float_rect
{
	float x1;
	float y1;
	float x2;
	float y2;
};

#endif // MINISPHERE__GEOMETRY_H__INCLUDED
