/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#ifndef SPHERE__GALILEO_H__INCLUDED
#define SPHERE__GALILEO_H__INCLUDED

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

void                   galileo_init            (void);
void                   galileo_uninit          (void);
shader_t*              galileo_shader          (void);
void                   galileo_reset           (void);
ibo_t*                 ibo_new                 (void);
ibo_t*                 ibo_ref                 (ibo_t* it);
void                   ibo_unref               (ibo_t* it);
ALLEGRO_INDEX_BUFFER*  ibo_buffer              (const ibo_t* it);
int                    ibo_len                 (const ibo_t* it);
void                   ibo_add_index           (ibo_t* it, uint16_t index);
bool                   ibo_upload              (ibo_t* it);
model_t*               model_new               (shader_t* shader);
model_t*               model_ref               (model_t* it);
void                   model_unref             (model_t* it);
shader_t*              model_get_shader        (const model_t* it);
transform_t*           model_get_transform     (const model_t* it);
void                   model_set_shader        (model_t* it, shader_t* shader);
void                   model_set_transform     (model_t* it, transform_t* transform);
bool                   model_add_shape         (model_t* it, shape_t* shape);
void                   model_draw              (const model_t* it, image_t* surface);
shader_t*              shader_new              (const char* vert_filename, const char* frag_filename);
shader_t*              shader_dup              (const shader_t* it);
shader_t*              shader_ref              (shader_t* shader);
void                   shader_unref            (shader_t* shader);
ALLEGRO_SHADER*        shader_program          (const shader_t* it);
void                   shader_put_bool         (shader_t* it, const char* name, bool value);
void                   shader_put_float        (shader_t* it, const char* name, float value);
void                   shader_put_float_array  (shader_t* it, const char* name, float values[], int size);
void                   shader_put_float_vector (shader_t* it, const char* name, float values[], int size);
void                   shader_put_int          (shader_t* it, const char* name, int value);
void                   shader_put_int_array    (shader_t* it, const char* name, int values[], int size);
void                   shader_put_int_vector   (shader_t* it, const char* name, int values[], int size);
void                   shader_put_matrix       (shader_t* it, const char* name, const transform_t* transform);
void                   shader_put_sampler      (shader_t* it, const char* name, image_t* texture, int texture_unit);
bool                   shader_use              (shader_t* shader, bool force_set);
shape_t*               shape_new               (vbo_t* vertices, ibo_t* indices, shape_type_t type, image_t* texture);
shape_t*               shape_ref               (shape_t* it);
void                   shape_unref             (shape_t* it);
ibo_t*                 shape_get_ibo           (const shape_t* it);
image_t*               shape_get_texture       (const shape_t* it);
vbo_t*                 shape_get_vbo           (const shape_t* it);
void                   shape_set_ibo           (shape_t* it, ibo_t* ibo);
void                   shape_set_texture       (shape_t* it, image_t* texture);
void                   shape_set_vbo           (shape_t* it, vbo_t* vbo);
void                   shape_draw              (shape_t* it, image_t* surface, transform_t* transform);
vbo_t*                 vbo_new                 (void);
vbo_t*                 vbo_ref                 (vbo_t* it);
void                   vbo_unref               (vbo_t* it);
ALLEGRO_VERTEX_BUFFER* vbo_buffer              (const vbo_t* it);
int                    vbo_len                 (const vbo_t* it);
void                   vbo_add_vertex          (vbo_t* it, vertex_t vertex);
bool                   vbo_upload              (vbo_t* it);

#endif // SPHERE__GALILEO_H__INCLUDED
