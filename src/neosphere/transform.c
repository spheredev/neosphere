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

#include "neosphere.h"
#include "transform.h"

struct transform
{
	unsigned int      refcount;
	bool              dirty;
	ALLEGRO_TRANSFORM matrix;
};

transform_t*
transform_new(void)
{
	transform_t* transform;

	if (!(transform = calloc(1, sizeof(transform_t))))
		return NULL;
	transform->dirty = true;
	al_identity_transform(&transform->matrix);
	return transform_ref(transform);
}

transform_t*
transform_clone(const transform_t* it)
{
	transform_t* dolly;

	if (!(dolly = calloc(1, sizeof(transform_t))))
		return NULL;
	al_copy_transform(&dolly->matrix, &it->matrix);
	return transform_ref(dolly);
}

transform_t*
transform_ref(transform_t* it)
{
	if (it == NULL)
		return NULL;
	++it->refcount;
	return it;
}

void
transform_unref(transform_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	free(it);
}

bool
transform_dirty(const transform_t* it)
{
	return it != NULL ? it->dirty : false;
}

const ALLEGRO_TRANSFORM*
transform_matrix(const transform_t* it)
{
	return &it->matrix;
}

float*
transform_values(transform_t* it)
{
	return &it->matrix.m[0][0];
}

void
transform_compose(transform_t* it, const transform_t* other)
{
	al_compose_transform(&it->matrix, &other->matrix);
	it->dirty = true;
}

void
transform_identity(transform_t* it)
{
	al_identity_transform(&it->matrix);
	it->dirty = true;
}

void
transform_make_clean(transform_t* it)
{
	if (it != NULL)
		it->dirty = false;
}

void
transform_orthographic(transform_t* it, float left, float top, float right, float bottom, float z_near, float z_far)
{
	al_orthographic_transform(&it->matrix, left, top, z_near, right, bottom, z_far);
	it->dirty = true;
}

void
transform_perspective(transform_t* it, float left, float top, float right, float bottom, float z_near, float z_far)
{
	al_perspective_transform(&it->matrix, left, top, z_near, right, bottom, z_far);
	it->dirty = true;
}

void
transform_rotate(transform_t* it, float theta, float vx, float vy, float vz)
{
	al_rotate_transform_3d(&it->matrix, vx, vy, vz, theta);
	it->dirty = true;
}

void
transform_scale(transform_t* it, float sx, float sy, float sz)
{
	al_scale_transform_3d(&it->matrix, sx, sy, sz);
	it->dirty = true;
}

void
transform_translate(transform_t* it, float dx, float dy, float dz)
{
	al_translate_transform_3d(&it->matrix, dx, dy, dz);
	it->dirty = true;
}
