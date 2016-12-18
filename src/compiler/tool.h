#ifndef CELL__TOOL_H__INCLUDED
#define CELL__TOOL_H__INCLUDED

typedef struct tool tool_t;

tool_t* tool_new  (void);
tool_t* tool_ref  (tool_t* tool);
void    tool_free (tool_t* tool);
bool    tool_exec (tool_t* tool, const path_t* out_path, vector_t* in_paths);

#endif // CELL__TOOL_H__INCLUDED
