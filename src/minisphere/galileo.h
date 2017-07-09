#ifndef MINISPHERE__GALILEO_H__INCLUDED
#define MINISPHERE__GALILEO_H__INCLUDED

typedef struct shader shader_t;
typedef struct shape  shape_t;
typedef struct model  model_t;

typedef
enum shader_type
{
	SHADER_TYPE_PIXEL,
	SHADER_TYPE_VERTEX,
	SHADER_TYPE_MAX
} shader_type_t;

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

void         galileo_init           (bool programmable);
void         galileo_uninit         (void);
bool         galileo_programmable   (void);
shader_t*    galileo_shader         (void);
model_t*     model_new              (shader_t* shader);
model_t*     model_ref              (model_t* it);
void         model_free             (model_t* it);
shader_t*    model_get_shader       (const model_t* it);
transform_t* model_get_transform    (const model_t* it);
void         model_set_shader       (model_t* it, shader_t* shader);
void         model_set_transform    (model_t* it, transform_t* transform);
bool         model_add_shape        (model_t* it, shape_t* shape);
void         model_draw             (const model_t* it, image_t* surface);
void         model_put_float        (model_t* it, const char* name, float value);
void         model_put_int          (model_t* it, const char* name, int value);
void         model_put_float_vector (model_t* it, const char* name, float values[], int size);
void         model_put_int_vector   (model_t* it, const char* name, int values[], int size);
void         model_put_matrix       (model_t* it, const char* name, const transform_t* matrix);
shader_t*    shader_new             (const char* vs_path, const char* fs_path);
shader_t*    shader_ref             (shader_t* shader);
void         shader_free            (shader_t* shader);
bool         shader_use             (shader_t* shader);
shape_t*     shape_new              (shape_type_t type, image_t* texture);
shape_t*     shape_ref              (shape_t* it);
void         shape_free             (shape_t* it);
float_rect_t shape_bounds           (const shape_t* it);
image_t*     shape_texture          (const shape_t* it);
void         shape_set_texture      (shape_t* it, image_t* texture);
bool         shape_add_vertex       (shape_t* it, vertex_t vertex);
void         shape_calculate_uv     (shape_t* it);
void         shape_draw             (shape_t* it, transform_t* matrix);
void         shape_upload           (shape_t* it);

#endif // MINISPHERE__GALILEO_H__INCLUDED
