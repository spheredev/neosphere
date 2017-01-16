#ifndef CELL__TOOL_H__INCLUDED
#define CELL__TOOL_H__INCLUDED

#include "fs.h"

typedef struct tool tool_t;

tool_t* tool_new  (duk_context* ctx, duk_idx_t idx, const char* verb);
tool_t* tool_ref  (tool_t* tool);
void    tool_free (tool_t* tool);
bool    tool_exec (tool_t* tool, const fs_t* fs, const path_t* out_path, vector_t* in_paths, bool forced);

#endif // CELL__TOOL_H__INCLUDED
