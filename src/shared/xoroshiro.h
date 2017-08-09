#ifndef FATCERBERUS__XOROSHIRO_H__INCLUDED
#define FATCERBERUS__XOROSHIRO_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef struct xoro xoro_t;

xoro_t*  xoro_new        (uint64_t seed);
xoro_t*  xoro_ref        (xoro_t* xoro);
void     xoro_unref      (xoro_t* xoro);
void     xoro_get_state  (xoro_t* xoro, char* buffer);
bool     xoro_set_state  (xoro_t* xoro, const char* snapshot);
double   xoro_gen_double (xoro_t* xoro);
uint64_t xoro_gen_uint   (xoro_t* xoro);
void     xoro_jump       (xoro_t* xoro);
void     xoro_reseed     (xoro_t* xoro, uint64_t seed);

#endif // FATCERBERUS__XOROSHIRO_H__INCLUDED
