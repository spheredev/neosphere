#ifndef CELL__BUILD_H__INCLUDED
#define CELL__BUILD_H__INCLUDED

#include "fs.h"
#include "path.h"
#include "target.h"

typedef struct build build_t;

build_t* build_new        (const path_t* in_path, const path_t* out_path);
void     build_free       (build_t* build);
fs_t*    build_fs         (const build_t* build);
void     build_add_target (build_t* build, target_t* target);
void     build_run        (build_t* build);

#endif // CELL__BUILD_H__INCLUDED
