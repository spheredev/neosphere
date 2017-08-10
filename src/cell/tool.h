#ifndef CELL__TOOL_H__INCLUDED
#define CELL__TOOL_H__INCLUDED

#include "fs.h"
#include "visor.h"

typedef struct tool tool_t;

tool_t* tool_new   (duk_context* ctx, const char* verb);
tool_t* tool_ref   (tool_t* tool);
void    tool_unref (tool_t* tool);
bool    tool_run   (tool_t* tool, visor_t* visor, const fs_t* fs, const path_t* out_path, vector_t* in_paths);

#endif // CELL__TOOL_H__INCLUDED
