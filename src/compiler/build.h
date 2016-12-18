#ifndef CELL__BUILD_H__INCLUDED
#define CELL__BUILD_H__INCLUDED

#include "target.h"

typedef struct build build_t;

build_t* build_new     (void);
void     build_free    (build_t* build);
void     build_exec    (build_t* build, const path_t* in_path, const path_t* out_path);
void     build_install (build_t* build, target_t* target, const path_t* path);

#endif // CELL__BUILD_H__INCLUDED
