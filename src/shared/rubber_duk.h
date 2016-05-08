#ifndef FATCERBERUS__RUBBER_DUK_H__INCLUDED
#define FATCERBERUS__RUBBER_DUK_H__INCLUDED

#include "duktape.h"

typedef duk_ret_t (* dukrub_safe_call_function) (duk_context* ctx);

void      dukrub_debugger_attach (duk_context* ctx, duk_debug_read_function read_cb, duk_debug_write_function write_cb, duk_debug_peek_function peek_cb, duk_debug_read_flush_function read_flush_cb, duk_debug_write_flush_function write_flush_cb, duk_debug_request_function request_cb, duk_debug_detached_function detached_cb, void* udata);
duk_int_t dukrub_safe_call       (duk_context* ctx, dukrub_safe_call_function func, duk_int_t nargs, duk_int_t nrets);

#endif // FATCERBERUS__RUBBER_DUK_H__INCLUDED
