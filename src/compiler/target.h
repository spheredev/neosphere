#ifndef CELL__TARGET_H__INCLUDED
#define CELL__TARGET_H__INCLUDED

#include "fs.h"
#include "tool.h"
#include "visor.h"

typedef struct target target_t;

target_t*     target_new         (const path_t* name, fs_t* fs, const path_t* path, tool_t* tool, time_t timestamp, bool tracked);
target_t*     target_ref         (target_t* target);
void          target_free        (target_t* target);
const path_t* target_name        (const target_t* target);
const path_t* target_path        (const target_t* target);
const path_t* target_source_path (const target_t* target);
void          target_add_source  (target_t* target, target_t* source);
bool          target_build       (target_t* target, visor_t* visor, bool force_build);

#endif // CELL__TARGET_H__INCLUDED
