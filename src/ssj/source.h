#ifndef SSJ__SOURCE_H__INCLUDED
#define SSJ__SOURCE_H__INCLUDED

#include "session.h"

typedef struct source source_t;

source_t*   source_load     (session_t* session, const char* filename);
int         source_cloc     (const source_t* source);
const char* source_get_line (const source_t* source, int line_index);

#endif // SSJ__SOURCE_H__INCLUDED
