#ifndef SSJ__SOURCE_H__INCLUDED
#define SSJ__SOURCE_H__INCLUDED

typedef struct source source_t;

source_t*        load_source     (const char* filename, const path_t* source_path);
void             free_source     (source_t* source);
const lstring_t* get_source_line (const source_t* source, int index);
int              get_source_size (const source_t* source);

#endif // SSJ__SOURCE_H__INCLUDED
