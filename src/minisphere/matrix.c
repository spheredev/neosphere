#include "minisphere.h"
#include "matrix.h"

struct matrix
{
	unsigned int      refcount;
	ALLEGRO_TRANSFORM transform;
};

matrix_t*
matrix_new(void)
{
	matrix_t* matrix;

	matrix = calloc(1, sizeof(matrix_t));
	al_identity_transform(&matrix->transform);
	return matrix_ref(matrix);
}

matrix_t*
matrix_ref(matrix_t* matrix)
{
	++matrix->refcount;
	return matrix;
}

void
matrix_free(matrix_t* matrix)
{
	if (--matrix->refcount > 0)
		return;
	free(matrix);
}

const float*
matrix_items(const matrix_t* matrix)
{
	return &matrix->transform.m[0][0];
}

const ALLEGRO_TRANSFORM*
matrix_transform(const matrix_t* matrix)
{
	return &matrix->transform;
}

void
matrix_identity(matrix_t* matrix)
{
	al_identity_transform(&matrix->transform);
}

void
matrix_compose(matrix_t* matrix, const matrix_t* other)
{
	al_compose_transform(&matrix->transform, &other->transform);
}

void
matrix_rotate(matrix_t* matrix, float theta, float vx, float vy, float vz)
{
	al_rotate_transform_3d(&matrix->transform, vx, vy, vz, theta);
}

void
matrix_scale(matrix_t* matrix, float sx, float sy, float sz)
{
	al_scale_transform_3d(&matrix->transform, sx, sy, sz);
}

void
matrix_translate(matrix_t* matrix, float dx, float dy, float dz)
{
	al_translate_transform_3d(&matrix->transform, dx, dy, dz);
}
