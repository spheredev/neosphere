#ifndef CELL__SCRIPT_H__INCLUDED
#define CELL__SCRIPT_H__INCLUDED

typedef struct script script_t;

script_t* script_open  (const path_t* path);
void      script_close (script_t* script);

#endif // CELL__SCRIPT_H__INCLUDED
