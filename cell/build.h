#ifndef CELL__BUILD_H__INCLUDED
#define CELL__BUILD_H__INCLUDED

#include <stdbool.h>
#include "assets.h"
#include "path.h"
#include "vector.h"

typedef struct build  build_t;
typedef struct target target_t;

build_t*  new_build     (const path_t* in_path, const path_t* out_path);
void      free_build    (build_t* build);
vector_t* add_files     (build_t* build, const path_t* pattern, bool recursive);
void      add_install   (build_t* build, const target_t* target, const path_t* path);
target_t* add_target    (build_t* build, asset_t* asset, const path_t* subdir);
bool      evaluate_rule (build_t* build, const char* name);
bool      run_build     (build_t* build);

#endif // CELL__BUILD_H__INCLUDED
