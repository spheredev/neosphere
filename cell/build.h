#ifndef CELL__BUILD_H__INCLUDED
#define CELL__BUILD_H__INCLUDED

#include <stdbool.h>
#include "assets.h"
#include "path.h"

typedef struct build  build_t;
typedef struct target target_t;

extern build_t*  new_build     (const path_t* in_path, const path_t* out_path);
extern void      free_build    (build_t* build);
extern target_t* add_target    (build_t* build, asset_t* asset, const path_t* subdir);
extern void      add_install   (build_t* build, const target_t* target, const path_t* path);
extern bool      evaluate_rule (build_t* build, const char* name);
extern bool      run_build     (build_t* build);

#endif // CELL__BUILD_H__INCLUDED
