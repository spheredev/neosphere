#ifndef SSJ__SOURCE_H__INCLUDED
#define SSJ__SOURCE_H__INCLUDED

typedef struct source source_t;

source_t*   source_new      (const char* text);
void        source_free     (source_t* src);
int         source_cloc     (const source_t* src);
const char* source_get_line (const source_t* src, int line_index);
void        source_print    (const source_t* src, int lineno, int num_lines, int active_lineno);

#endif // SSJ__SOURCE_H__INCLUDED
