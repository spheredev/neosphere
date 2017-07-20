#ifndef MINISPHERE__GALILEO_H__INCLUDED
#define MINISPHERE__GALILEO_H__INCLUDED

typedef struct ibo    ibo_t;
typedef struct model  model_t;
typedef struct shader shader_t;
typedef struct shape  shape_t;
typedef struct vbo    vbo_t;

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

void         galileo_init           (void);
void         galileo_uninit         (void);
shader_t*    galileo_shader         (void);
void         galileo_reset          (void);
ibo_t*       ibo_new                (void);
ibo_t*       ibo_ref                (ibo_t* it);
void         ibo_free               (ibo_t* it);
ALLEGRO_INDEX_BUFFER* ibo_buffer    (const ibo_t* it);
int          ibo_len                (const ibo_t* it);
void         ibo_add_index          (ibo_t* it, uint16_t index);
bool         ibo_upload             (ibo_t* it);
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
void         model_put_float_array  (model_t* it, const char* name, float values[], int size);
void         model_put_float_vector (model_t* it, const char* name, float values[], int size);
void         model_put_int          (model_t* it, const char* name, int value);
void         model_put_int_array    (model_t* it, const char* name, int values[], int size);
void         model_put_int_vector   (model_t* it, const char* name, int values[], int size);
void         model_put_matrix       (model_t* it, const char* name, const transform_t* transform);
shader_t*    shader_new             (const char* vs_path, const char* fs_path);
shader_t*    shader_ref             (shader_t* shader);
void         shader_free            (shader_t* shader);
bool         shader_use             (shader_t* shader, bool force_set);
shape_t*     shape_new              (vbo_t* vertices, ibo_t* indices, shape_type_t type, image_t* texture);
shape_t*     shape_ref              (shape_t* it);
void         shape_free             (shape_t* it);
ibo_t*       shape_get_ibo          (const shape_t* it);
image_t*     shape_get_texture      (const shape_t* it);
vbo_t*       shape_get_vbo          (const shape_t* it);
void         shape_set_ibo          (shape_t* it, ibo_t* ibo);
void         shape_set_texture      (shape_t* it, image_t* texture);
void         shape_set_vbo          (shape_t* it, vbo_t* vbo);
void         shape_draw             (shape_t* it, image_t* surface, transform_t* transform);
vbo_t*       vbo_new                (void);
vbo_t*       vbo_ref                (vbo_t* it);
void         vbo_free               (vbo_t* it);
ALLEGRO_VERTEX_BUFFER* vbo_buffer   (const vbo_t* it);
int          vbo_len                (const vbo_t* it);
void         vbo_add_vertex         (vbo_t* it, vertex_t vertex);
bool         vbo_upload             (vbo_t* it);

#endif // MINISPHERE__GALILEO_H__INCLUDED
