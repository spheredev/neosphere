#ifndef CELL__BUILD_H__INCLUDED
#define CELL__BUILD_H__INCLUDED

#include <stdbool.h>

extern void initialize_engine (void);
extern void shutdown_engine   (void);
extern bool run_build         (void);

extern void initialize_js_api (void);

#endif // CELL__BUILD_H__INCLUDED
