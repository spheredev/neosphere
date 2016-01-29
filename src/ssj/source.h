#ifndef SSJ__SOURCE_H__INCLUDED
#define SSJ__SOURCE_H__INCLUDED

typedef struct source source_t;

source_t* load_source (const path_t* path, const path_t* source_path);
void      free_source (source_t* source);

#endif SSJ__SOURCE_H__INCLUDED
