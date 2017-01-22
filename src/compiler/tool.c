#include "cell.h"
#include "tool.h"

#include "fs.h"
#include "visor.h"

struct tool
{
	unsigned int refcount;
	duk_context* js_ctx;
	void*        callback_ptr;
	char*        verb;
};

tool_t*
tool_new(duk_context* ctx, const char* verb)
{
	void*   callback_ptr;
	tool_t* tool;

	callback_ptr = duk_ref_heapptr(ctx, -1);
	duk_pop(ctx);
	
	tool = calloc(1, sizeof(tool_t));
	tool->verb = strdup(verb);
	tool->js_ctx = ctx;
	tool->callback_ptr = callback_ptr;
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
	if (tool == NULL || --tool->refcount > 0)
		return;
	
	duk_unref_heapptr(tool->js_ctx, tool->callback_ptr);
	free(tool->verb);
	free(tool);
}

bool
tool_run(tool_t* tool, visor_t* visor, const fs_t* fs, const path_t* out_path, vector_t* in_paths)
{
	duk_uarridx_t array_index;
	path_t*       dir_path;
	const char*   filename;
	bool          is_outdated = false;
	duk_context*  js_ctx;
	time_t        last_time = 0;
	int           line_number;
	bool          result = true;

	iter_t iter;
	path_t* *p_path;

	if (tool == NULL)
		return true;

	js_ctx = tool->js_ctx;

	visor_begin_op(visor, "%s %s", tool->verb, path_cstr(out_path));

	// ensure the target directory exists
	dir_path = path_strip(path_dup(out_path));
	fs_mkdir(fs, path_cstr(dir_path));
	path_free(dir_path);

	duk_push_heapptr(js_ctx, tool->callback_ptr);
	duk_push_string(js_ctx, path_cstr(out_path));
	duk_push_array(js_ctx);
	iter = vector_enum(in_paths);
	while (p_path = vector_next(&iter)) {
		array_index = (duk_uarridx_t)duk_get_length(js_ctx, -1);
		duk_push_string(js_ctx, path_cstr(*p_path));
		duk_put_prop_index(js_ctx, -2, array_index);
	}
	if (duk_pcall(js_ctx, 2) != DUK_EXEC_SUCCESS) {
		duk_get_prop_string(js_ctx, -1, "fileName");
		filename = duk_safe_to_string(js_ctx, -1);
		duk_get_prop_string(js_ctx, -2, "lineNumber");
		line_number = duk_get_int(js_ctx, -1);
		duk_dup(js_ctx, -3);
		duk_to_string(js_ctx, -1);
		visor_error(visor, "%s", duk_get_string(js_ctx, -1));
		visor_info(visor, "@ [%s:%d]", filename, line_number);
		duk_pop_3(js_ctx);
		result = false;
	}
	duk_pop(js_ctx);

	visor_end_op(visor);
	return result;
}
