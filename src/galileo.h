#ifndef MINISPHERE__GALILEO_H__INCLUDED
#define MINISPHERE__GALILEO_H__INCLUDED

typedef struct vertex vertex_t;
typedef struct shape  shape_t;
typedef struct group  group_t;

struct vertex
{
	float   x, y;
	float   u, v;
	color_t color;
};

extern vertex_t vertex (float x, float y, float u, float v, color_t color);

extern shape_t* new_shape         (image_t* texture);
extern shape_t* ref_shape         (shape_t* shape);
extern void     free_shape        (shape_t* shape);
extern image_t* get_shape_texture (shape_t* shape);
extern void     set_shape_texture (shape_t* shape, image_t* texture);
extern bool     add_vertex        (shape_t* shape, vertex_t vertex);
extern void     draw_shape        (shape_t* shape, float x, float y);
extern void     remove_vertex     (shape_t* shape, int index);

extern group_t* new_group    (void);
extern group_t* ref_group    (group_t* group);
extern void     free_group   (group_t* group);
extern bool     add_shape    (group_t* group, shape_t* shape);
extern void     clear_group  (group_t* group);
extern void     draw_group   (group_t* group);
extern void     remove_shape (group_t* group, int index);

extern void init_galileo_api (void);

#endif // MINISPHERE__GALILEO_H__INCLUDED
