#include "minisphere.h"
#include "transform.h"

struct transform
{
	unsigned int      refcount;
	ALLEGRO_TRANSFORM matrix;
};

transform_t*
transform_new(void)
{
	transform_t* transform;

	transform = calloc(1, sizeof(transform_t));
	al_identity_transform(&transform->matrix);
	return transform_ref(transform);
}

transform_t*
transform_clone(const transform_t* it)
{
	transform_t* dolly;

	dolly = calloc(1, sizeof(transform_t));
	al_copy_transform(&dolly->matrix, &it->matrix);
	return transform_ref(dolly);
}

transform_t*
transform_ref(transform_t* it)
{
	++it->refcount;
	return it;
}

void
transform_free(transform_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	free(it);
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
}

void
transform_identity(transform_t* it)
{
	al_identity_transform(&it->matrix);
}

void
transform_orthographic(transform_t* it, float left, float top, float right, float bottom, float z_near, float z_far)
{
	al_orthographic_transform(&it->matrix, left, top, z_near, right, bottom, z_far);
}

void
transform_perspective(transform_t* it, float left, float top, float right, float bottom, float z_near, float z_far)
{
	al_perspective_transform(&it->matrix, left, top, z_near, right, bottom, z_far);
}

void
transform_rotate(transform_t* it, float theta, float vx, float vy, float vz)
{
#if defined(MINISPHERE_USE_3D_TRANSFORM)
	al_rotate_transform_3d(&it->matrix, vx, vy, vz, theta);
#else
	al_rotate_transform(&it->matrix, theta);
#endif
}

void
transform_scale(transform_t* it, float sx, float sy, float sz)
{
#if defined(MINISPHERE_USE_3D_TRANSFORM)
	al_scale_transform_3d(&it->matrix, sx, sy, sz);
#else
	al_scale_transform(&it->matrix, sx, sy);
#endif
}

void
transform_translate(transform_t* it, float dx, float dy, float dz)
{
#if defined(MINISPHERE_USE_3D_TRANSFORM)
	al_translate_transform_3d(&it->matrix, dx, dy, dz);
#else
	al_translate_transform(&it->matrix, dx, dy);
#endif
}
