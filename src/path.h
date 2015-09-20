#ifndef CELL__PATH_H__INCLUDED
#define CELL__PATH_H__INCLUDED

typedef struct path path_t;

path_t*     new_path  (const char* pathname);
void        free_path (path_t* path);
const char* path_cstr (const path_t* path);

#endif // CELL__PATH_H__INCLUDED
