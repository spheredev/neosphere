#ifndef MINISPHERE__XOROSHIRO_H__INCLUDED
#define MINISPHERE__XOROSHIRO_H__INCLUDED

#include <stdint.h>

typedef struct xoro xoro_t;

xoro_t*  xoro_new        (uint64_t seed);
xoro_t*  xoro_ref        (xoro_t* xoro);
void     xoro_free       (xoro_t* xoro);
void     xoro_jump       (xoro_t* xoro);
double   xoro_gen_double (xoro_t* xoro);
uint64_t xoro_gen_int    (xoro_t* xoro);
void     xoro_reseed     (xoro_t* xoro, uint64_t seed);

#endif // MINISPHERE__XOROSHIRO_H__INCLUDED
