#ifndef SSJ__SOURCE_H__INCLUDED
#define SSJ__SOURCE_H__INCLUDED

typedef struct source source_t;

source_t*   source_load     (const char* filename, const path_t* source_path);
int         source_cloc     (const source_t* source);
const char* source_get_line (const source_t* source, int line_index);

#endif // SSJ__SOURCE_H__INCLUDED
