#ifndef MINISPHERE__SAPPHIRE_H__INCLUDED
#define MINISPHERE__SAPPHIRE_H__INCLUDED

typedef struct vertex vertex_t;
typedef struct shape  shape_t;
typedef struct group  group_t;

extern shape_t* new_shape         (image_t* texture);
extern shape_t* ref_shape         (shape_t* shape);
extern void     free_shape        (shape_t* shape);
extern image_t* get_shape_texture (shape_t* shape);
extern void     set_shape_texture (shape_t* shape, image_t* texture);
extern bool     add_vertex        (shape_t* shape, vertex_t vertex);
extern void     draw_shape        (shape_t* shape, float x, float y);

extern group_t* new_group  (void);
extern group_t* ref_group  (group_t* group);
extern void     free_group (group_t* group);
extern bool     add_shape  (group_t* group, shape_t* shape);
extern void     draw_group (group_t* group);

extern void init_sapphire_api (void);

#endif // MINISPHERE__SAPPHIRE_H__INCLUDED
