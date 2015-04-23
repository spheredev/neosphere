#ifndef MINISPHERE__SAPPHIRE_H__INCLUDED
#define MINISPHERE__SAPPHIRE_H__INCLUDED

typedef struct vertex vertex_t;
typedef struct shape  shape_t;
typedef struct group  group_t;

extern shape_t* new_shape  (image_t* texture);
extern void     free_shape (shape_t* shape);

extern void init_sapphire_api (void);

#endif // MINISPHERE__SAPPHIRE_H__INCLUDED
