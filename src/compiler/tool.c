#include "cell.h"
#include "tool.h"

struct tool
{
	unsigned int refcount;
};

tool_t*
tool_new(void)
{
	tool_t* tool;

	tool = calloc(1, sizeof(tool_t));
	return tool_ref(tool);
}

tool_t*
tool_ref(tool_t* tool)
{
	if (tool == NULL)
		return NULL;
	++tool->refcount;
	return tool;
}

void
tool_free(tool_t* tool)
{
	if (--tool->refcount > 0)
		return;
	free(tool);
}

bool
tool_exec(tool_t* tool, const path_t* out_path, vector_t* in_paths)
{
	bool        is_outdated = false;
	time_t      last_time = 0;
	struct stat sb;

	iter_t iter;
	path_t* *p_path;
	
	if (tool == NULL)
		return true;
	
	// check whether the output file is out of date with respect to its sources.
	// this is a simple timestamp comparison for now; it might be good to eventually
	// switch to a hash-based solution like in SCons.
	if (stat(path_cstr(out_path), &sb) == 0)
		last_time = sb.st_mtime;
	iter = vector_enum(in_paths);
	while (p_path = vector_next(&iter)) {
		stat(path_cstr(*p_path), &sb);
		if (is_outdated = sb.st_mtime > last_time)
			break;  // short circuit
	}
	
	// if the target file is out of date, rebuild it.
	if (is_outdated) {
		// TODO: build the target file
	}
	return true;
}
