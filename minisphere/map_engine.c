#include "minisphere.h"
#include "api.h"
#include "map_engine.h"

static duk_ret_t _js_IsMapEngineRunning(duk_context* ctx);

void
init_map_engine_api(duk_context* ctx)
{
	register_api_func(ctx, NULL, "IsMapEngineRunning", &_js_IsMapEngineRunning);
}

static duk_ret_t
_js_IsMapEngineRunning(duk_context* ctx)
{
	duk_push_false(ctx);
	return 1;
}
