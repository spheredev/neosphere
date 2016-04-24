#ifndef MINISPHERE__MATRIX_H__INCLUDED
#define MINISPHERE__MATRIX_H__INCLUDED

typedef struct matrix matrix_t;

matrix_t* matrix_new       (void);
matrix_t* matrix_ref       (matrix_t* matrix);
void      matrix_free      (matrix_t* matrix);
void      matrix_identity  (matrix_t* matrix);
void      matrix_scale     (matrix_t* matrix, float sx, float sy);
void      matrix_rotate    (matrix_t* matrix, float theta);
void      matrix_translate (matrix_t* matrix, float tx, float ty);

#endif // MINISPHERE__MATRIX_H__INCLUDED
