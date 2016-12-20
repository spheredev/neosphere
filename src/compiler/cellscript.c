#include "cell.h"
#include "cellscript.h"

#include "api.h"

static duk_ret_t js_system_name    (duk_context* ctx);
static duk_ret_t js_system_version (duk_context* ctx);

void
cell_api_init(duk_context* ctx)
{
	api_init(ctx);

	api_define_function(ctx, "system", "name", js_system_name);
	api_define_function(ctx, "system", "version", js_system_version);
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
