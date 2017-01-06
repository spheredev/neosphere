#include "cell.h"
#include "tool.h"

#include "fs.h"

struct tool
{
	unsigned int refcount;
	duk_context* js_ctx;
	void*        js_func;
	char*        verb;
};

tool_t*
tool_new(duk_context* ctx, duk_idx_t idx, const char* verb)
{
	tool_t* tool;

	tool = calloc(1, sizeof(tool_t));
	tool->verb = strdup(verb);
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
	if (tool == NULL || --tool->refcount > 0)
		return;
	
	duk_unref_heapptr(tool->js_ctx, tool->js_func);
	free(tool->verb);
	free(tool);
}

bool
tool_exec(tool_t* tool, const fs_t* fs, const path_t* out_path, vector_t* in_paths)
{
	duk_uarridx_t array_index;
	path_t*       dir_path;
	const char*   filename;
	bool          is_outdated = false;
	duk_context*  js_ctx;
	time_t        last_time = 0;
	int           line_number;
	struct stat   sb;

	iter_t iter;
	path_t* *p_path;

	if (tool == NULL)
		return true;

	js_ctx = tool->js_ctx;

	// check whether the output file is out of date with respect to its sources.
	// this is a simple timestamp comparison for now; it might be good to eventually
	// switch to a hash-based solution like in SCons.
	if (fs_stat(fs, path_cstr(out_path), &sb) == 0)
		last_time = sb.st_mtime;
	iter = vector_enum(in_paths);
	while (p_path = vector_next(&iter)) {
		fs_stat(fs, path_cstr(*p_path), &sb);
		if (is_outdated = sb.st_mtime > last_time)
			break;  // short circuit
	}

	// if the target file is out of date, invoke the tool to rebuild it.
	if (is_outdated) {
		printf("    %s %s\n", tool->verb, path_cstr(out_path));

		// ensure the target directory exists
		dir_path = path_strip(path_dup(out_path));
		fs_mkdir(fs, path_cstr(dir_path));
		path_free(dir_path);

		duk_push_heapptr(js_ctx, tool->js_func);
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
			printf("    %s\n", duk_get_string(js_ctx, -1));
			printf("    @ [%s:%d]\n", filename, line_number);
			duk_pop_3(js_ctx);
		}
		duk_pop(js_ctx);
	}
	return true;
}
