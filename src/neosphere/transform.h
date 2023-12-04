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

#ifndef SPHERE__TRANSFORM_H__INCLUDED
#define SPHERE__TRANSFORM_H__INCLUDED

typedef struct transform transform_t;

transform_t*             transform_new          (void);
transform_t*             transform_clone        (const transform_t* it);
transform_t*             transform_ref          (transform_t* it);
void                     transform_unref        (transform_t* it);
bool                     transform_dirty        (const transform_t* it);
const ALLEGRO_TRANSFORM* transform_matrix       (const transform_t* it);
float*                   transform_values       (transform_t* it);
void                     transform_compose      (transform_t* it, const transform_t* other);
void                     transform_identity     (transform_t* it);
void                     transform_make_clean   (transform_t* it);
void                     transform_orthographic (transform_t* it, float left, float top, float right, float bottom, float z_near, float z_far);
void                     transform_perspective  (transform_t* it, float left, float top, float right, float bottom, float z_near, float z_far);
void                     transform_rotate       (transform_t* it, float theta, float vx, float vy, float vz);
void                     transform_scale        (transform_t* it, float sx, float sy, float sz);
void                     transform_translate    (transform_t* it, float dx, float dy, float dz);

#endif // SPHERE__TRANSFORM_H__INCLUDED
