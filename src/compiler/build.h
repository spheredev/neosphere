#ifndef CELL__BUILD_H__INCLUDED
#define CELL__BUILD_H__INCLUDED

#include "assets.h"

typedef struct build  build_t;
typedef struct target target_t;

build_t*  build_new        (const path_t* in_path, const path_t* out_path, bool make_source_map);
void      build_free       (build_t* build);
bool      build_is_ok      (const build_t* build, int *out_num_errors, int *out_num_warnings);
time_t    build_timestamp  (const build_t* build);
target_t* build_add_asset  (build_t* build, asset_t* asset, const path_t* subdir);
vector_t* build_add_files  (build_t* build, const path_t* pattern, bool recursive);
void      build_emit_error (build_t* build, const char* fmt, ...);
void      build_emit_warn  (build_t* build, const char* fmt, ...);
void      build_install    (build_t* build, const target_t* target, const path_t* path);
bool      build_prime      (build_t* build, const char* rule_name);
bool      build_run        (build_t* build);

#endif // CELL__BUILD_H__INCLUDED
