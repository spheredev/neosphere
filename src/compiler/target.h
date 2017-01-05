#ifndef CELL__TARGET_H__INCLUDED
#define CELL__TARGET_H__INCLUDED

#include "fs.h"
#include "tool.h"

typedef struct target target_t;

target_t*     target_new        (const path_t* name, const path_t* path, tool_t* tool);
target_t*     target_ref        (target_t* target);
void          target_free       (target_t* target);
const path_t* target_name       (const target_t* target);
const path_t* target_path       (const target_t* target);
void          target_add_source (target_t* target, target_t* source);
bool          target_build      (target_t* target, const fs_t* fs);

#endif // CELL__TARGET_H__INCLUDED
