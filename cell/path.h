#ifndef CELL__PATH_H__INCLUDED
#define CELL__PATH_H__INCLUDED

#include <stdbool.h>

typedef struct path path_t;

path_t*     path_new           (const char* pathname, bool force_dir_path);
void        path_free          (path_t* path);
path_t*     path_dup           (const path_t* path);
const char* path_cstr          (const path_t* path);
bool        path_is_rooted     (const path_t* path);
bool        path_append        (path_t* path, const char* pathname, bool force_dir_path);
path_t*     path_collapse      (path_t* path, bool collapse_double_dots);
path_t*     path_strip         (path_t* path);

#endif // CELL__PATH_H__INCLUDED
