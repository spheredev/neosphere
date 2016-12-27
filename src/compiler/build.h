#ifndef CELL__BUILD_H__INCLUDED
#define CELL__BUILD_H__INCLUDED

#include "path.h"
#include "target.h"

typedef struct build build_t;

build_t*      build_new      (const path_t* in_path, const path_t* out_path);
void          build_free     (build_t* build);
const path_t* build_in_path  (const build_t* build);
const path_t* build_out_path (const build_t* build);
void          build_run      (build_t* build);

#endif // CELL__BUILD_H__INCLUDED
