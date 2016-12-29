#include "cell.h"
#include "tool.h"

struct tool
{
	unsigned int refcount;
	duk_context* js_ctx;
	void*        js_func;
};

tool_t*
tool_new(duk_context* ctx, duk_idx_t idx)
{
	tool_t* tool;

	tool = calloc(1, sizeof(tool_t));
	tool->js_ctx = ctx;
	tool->js_func = duk_ref_heapptr(ctx, idx);
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
	
	duk_unref_heapptr(tool->js_ctx, tool->js_func);
	free(tool);
}

bool
tool_exec(tool_t* tool, const path_t* out_path, vector_t* in_paths)
{
	duk_uarridx_t array_index;
	duk_context*  js_ctx;
	bool          is_outdated = false;
	time_t        last_time = 0;
	struct stat   sb;

	iter_t iter;
	path_t* *p_path;

	if (tool == NULL)
		return true;

	js_ctx = tool->js_ctx;

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

	// if the target file is out of date, invoke the tool to rebuild it.
	if (is_outdated) {
		duk_push_heapptr(js_ctx, tool->js_func);
		duk_push_string(js_ctx, path_cstr(out_path));
		duk_push_array(js_ctx);
		iter = vector_enum(in_paths);
		while (p_path = vector_next(&iter)) {
			array_index = (duk_uarridx_t)duk_get_length(js_ctx, -1);
			duk_push_string(js_ctx, path_cstr(*p_path));
			duk_put_prop_index(js_ctx, -2, array_index);
		}
		duk_call(js_ctx, 2);
		duk_pop(js_ctx);
	}
	return true;
}
