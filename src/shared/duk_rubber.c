#include "duk_rubber.h"

#include "duktape.h"

struct safe_call_args
{
	dukrub_safe_call_function func;
};

static void      shim_v1_debug_detached (duk_context* ctx, void* udata);
static duk_ret_t shim_v1_safe_call      (duk_context* ctx, void* udata);

static dukrub_debug_detached_function s_detached_cb;

void
dukrub_debugger_attach(duk_context* ctx,
	duk_debug_read_function read_cb, duk_debug_write_function write_cb, duk_debug_peek_function peek_cb,
	duk_debug_read_flush_function read_flush_cb, duk_debug_write_flush_function write_flush_cb,
	duk_debug_request_function request_cb, dukrub_debug_detached_function detached_cb,
	void* udata)
{
#if DUK_VERSION >= 19999
	s_detached_cb = detached_cb;
	duk_debugger_attach(ctx, read_cb, write_cb, peek_cb, read_flush_cb, write_flush_cb, request_cb, shim_v1_debug_detached, udata);
#else
	duk_debugger_attach_custom(ctx, read_cb, write_cb, peek_cb, read_flush_cb, write_flush_cb, request_cb, detached_cb, udata);
#endif
}

duk_int_t
dukrub_safe_call(duk_context* ctx, dukrub_safe_call_function func, duk_int_t nargs, duk_int_t nrets)
{
#if DUK_VERSION >= 19999
	struct safe_call_args safe_args;
	
	safe_args.func = func;
	return duk_safe_call(ctx, shim_v1_safe_call, &safe_args, nargs, nrets);
#else
	return duk_safe_call(ctx, func, nargs, nrets);
#endif
}

static duk_ret_t
shim_v1_safe_call(duk_context* ctx, void* udata)
{
	struct safe_call_args* safe_args;

	safe_args = (struct safe_call_args*)udata;
	return safe_args->func(ctx);
}

static void
shim_v1_debug_detached(duk_context* ctx, void* udata)
{
	s_detached_cb(udata);
}
