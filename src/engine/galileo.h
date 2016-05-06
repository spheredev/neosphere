#ifndef MINISPHERE__GALILEO_H__INCLUDED
#define MINISPHERE__GALILEO_H__INCLUDED

#include "shader.h"

typedef struct shape shape_t;
typedef struct group group_t;

typedef
enum shape_type
{
	SHAPE_AUTO,
	SHAPE_POINTS,
	SHAPE_LINES,
	SHAPE_LINE_LOOP,
	SHAPE_LINE_STRIP,
	SHAPE_TRIANGLES,
	SHAPE_TRI_FAN,
	SHAPE_TRI_STRIP,
	SHAPE_MAX
} shape_type_t;

typedef
struct vertex
{
	float   x, y, z;
	float   u, v;
	color_t color;
} vertex_t;

void      initialize_galileo (void);
void      shutdown_galileo   (void);
shader_t* get_default_shader (void);

vertex_t vertex (float x, float y, float z, float u, float v, color_t color);

group_t*     group_new           (shader_t* shader);
group_t*     group_ref           (group_t* group);
void         group_free          (group_t* group);
shader_t*    group_get_shader    (const group_t* group);
matrix_t*    group_get_transform (const group_t* group);
void         group_set_shader    (group_t* group, shader_t* shader);
void         group_set_transform (group_t* group, matrix_t* transform);
bool         group_add_shape     (group_t* group, shape_t* shape);
void         group_draw          (const group_t* group, image_t* surface);
void         group_put_float     (group_t* group, const char* name, float value);
void         group_put_int       (group_t* group, const char* name, int value);
void         group_put_matrix    (group_t* group, const char* name, const matrix_t* matrix);
shape_t*     shape_new           (shape_type_t type, image_t* texture);
shape_t*     shape_ref           (shape_t* shape);
void         shape_free          (shape_t* shape);
float_rect_t shape_bounds        (const shape_t* shape);
image_t*     shape_texture       (const shape_t* shape);
void         shape_set_texture   (shape_t* shape, image_t* texture);
bool         shape_add_vertex    (shape_t* shape, vertex_t vertex);
void         shape_draw          (shape_t* shape, matrix_t* matrix, image_t* surface);
void         shape_upload        (shape_t* shape);

void init_galileo_api (void);

#endif // MINISPHERE__GALILEO_H__INCLUDED
