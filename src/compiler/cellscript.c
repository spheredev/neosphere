#include "cell.h"
#include "cellscript.h"

#include "api.h"
#include "target.h"
#include "tool.h"

static duk_ret_t js_new_Target      (duk_context* ctx);
static duk_ret_t js_Target_finalize (duk_context* ctx);
static duk_ret_t js_new_Tool        (duk_context* ctx);
static duk_ret_t js_Tool_finalize   (duk_context* ctx);
static duk_ret_t js_system_name     (duk_context* ctx);
static duk_ret_t js_system_version  (duk_context* ctx);

void
cell_api_init(duk_context* ctx)
{
	api_init(ctx);

	api_define_function(ctx, "system", "name", js_system_name);
	api_define_function(ctx, "system", "version", js_system_version);

	api_define_class(ctx, "Target", js_new_Target, js_Target_finalize);

	api_define_class(ctx, "Tool", js_new_Tool, js_Tool_finalize);
}

static duk_ret_t
js_system_name(duk_context* ctx)
{
	duk_push_string(ctx, COMPILER_NAME);
	return 1;
}

static duk_ret_t
js_system_version(duk_context* ctx)
{
	duk_push_string(ctx, VERSION_NAME);
	return 1;
}

static duk_ret_t
js_new_Target(duk_context* ctx)
{
	path_t*   name;
	path_t*   out_path;
	target_t* target;
	tool_t*   tool;

	if (!duk_is_constructor_call(ctx))
		duk_error_blamed(ctx, -1, DUK_ERR_TYPE_ERROR, "constructor requires 'new'");

	tool = duk_require_class_obj(ctx, 0, "Tool");
	out_path = path_new(duk_require_string(ctx, 1));

	name = path_new(path_filename(out_path));
	target = target_new(name, out_path, tool);
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Target", target);
	return 0;
}

static duk_ret_t
js_Target_finalize(duk_context* ctx)
{
	target_t* target;

	target = duk_require_class_obj(ctx, 0, "Target");

	target_free(target);
	return 0;
}

static duk_ret_t
js_new_Tool(duk_context* ctx)
{
	tool_t* tool;
	
	duk_push_this(ctx);
	duk_to_class_obj(ctx, -1, "Tool", tool);
	return 0;
}

static duk_ret_t
js_Tool_finalize(duk_context* ctx)
{
	tool_t* tool;

	tool = duk_require_class_obj(ctx, 0, "Tool");

	tool_free(tool);
	return 0;
}
