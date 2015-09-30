#ifndef MINISPHERE__PATH_H__INCLUDED
#define MINISPHERE__PATH_H__INCLUDED

#include <stdbool.h>
#include <stddef.h>

typedef struct path path_t;

path_t*     path_new           (const char* pathname);
path_t*     path_new_dir       (const char* pathname);
path_t*     path_dup           (const path_t* path);
void        path_free          (path_t* path);
const char* path_cstr          (const path_t* path);
const char* path_filename_cstr (const path_t* path);
const char* path_hop_cstr      (const path_t* path, size_t idx);
path_t*     path_insert_hop    (path_t* path, size_t idx, const char* name);
bool        path_is_rooted     (const path_t* path);
size_t      path_num_hops      (const path_t* path);
bool        path_hop_cmp       (const path_t* path, size_t idx, const char* name);
path_t*     path_append        (path_t* path, const char* pathname);
path_t*     path_append_dir    (path_t* path, const char* pathname);
path_t*     path_cat           (path_t* path, const path_t* tail);
bool        path_cmp           (const path_t* path1, const path_t* path2);
path_t*     path_collapse      (path_t* path, bool collapse_double_dots);
bool        path_mkdir         (const path_t* path);
path_t*     path_rebase        (path_t* path, const path_t* root);
path_t*     path_remove_hop    (path_t* path, size_t idx);
path_t*     path_resolve       (path_t* path, const path_t* relative_to);
path_t*     path_set           (path_t* path, const char* pathname);
path_t*     path_set_dir       (path_t* path, const char* pathname);
path_t*     path_strip         (path_t* path);

#endif // MINISPHERE__PATH_H__INCLUDED
