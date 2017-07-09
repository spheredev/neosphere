#ifndef MINISPHERE__TRANSFORM_H__INCLUDED
#define MINISPHERE__TRANSFORM_H__INCLUDED

typedef struct transform transform_t;

transform_t*             transform_new          (void);
transform_t*             transform_clone        (const transform_t* it);
transform_t*             transform_ref          (transform_t* it);
void                     transform_free         (transform_t* it);
const ALLEGRO_TRANSFORM* transform_matrix       (const transform_t* it);
float*                   transform_values       (transform_t* it);
void                     transform_compose      (transform_t* it, const transform_t* other);
void                     transform_identity     (transform_t* it);
void                     transform_orthographic (transform_t* it, float left, float top, float right, float bottom, float z_near, float z_far);
void                     transform_perspective  (transform_t* it, float left, float top, float right, float bottom, float z_near, float z_far);
void                     transform_rotate       (transform_t* it, float theta, float vx, float vy, float vz);
void                     transform_scale        (transform_t* it, float sx, float sy, float sz);
void                     transform_translate    (transform_t* it, float dx, float dy, float dz);

#endif // MINISPHERE__TRANSFORM_H__INCLUDED
