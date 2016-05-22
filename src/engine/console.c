// this needs cleanup at some point.  there is a ton of duplicate code here,
// it would be better to refactor it so that all the console methods piggybacked on the
// console.log() implementation.

#include "minisphere.h"
#include "console.h"

#include "api.h"
#include "debugger.h"

static duk_ret_t js_console_assert (duk_context* ctx);
static duk_ret_t js_console_debug  (duk_context* ctx);
static duk_ret_t js_console_error  (duk_context* ctx);
static duk_ret_t js_console_info   (duk_context* ctx);
static duk_ret_t js_console_log    (duk_context* ctx);
static duk_ret_t js_console_trace  (duk_context* ctx);
static duk_ret_t js_console_warn   (duk_context* ctx);

static int s_verbosity = 1;

void
initialize_console(int verbosity)
{
	s_verbosity = verbosity;
}

int
get_log_verbosity(void)
{
	return s_verbosity;
}

void
console_log(int level, const char* fmt, ...)
{
	va_list ap;

	if (level > s_verbosity)
		return;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	fputc('\n', stdout);
	va_end(ap);
}

void
init_console_api(void)
{
	api_register_function(g_duk, "console", "assert", js_console_assert);
	api_register_function(g_duk, "console", "debug", js_console_debug);
	api_register_function(g_duk, "console", "error", js_console_error);
	api_register_function(g_duk, "console", "info", js_console_info);
	api_register_function(g_duk, "console", "log", js_console_log);
	api_register_function(g_duk, "console", "trace", js_console_trace);
	api_register_function(g_duk, "console", "warn", js_console_warn);

	// `console` is a Proxy so that unimplemented methods do not throw
	duk_eval_string_noresult(g_duk,
		"global.console = new Proxy(global.console, {\n"
		"    get: function(t, name) {\n"
		"        return name in t ? t[name] : function() {};\n"
		"    }\n"
		"});"
	);
}

static duk_ret_t
js_console_assert(duk_context* ctx)
{
	const char* message;
	bool        result;

	result = duk_to_boolean(ctx, 0);
	message = duk_safe_to_string(ctx, 1);

	if (!result)
		debug_print(message, PRINT_ASSERT);
	return 0;
}

static duk_ret_t
js_console_debug(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_DEBUG);
	return 0;
}

static duk_ret_t
js_console_error(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_ERROR);
	return 0;
}

static duk_ret_t
js_console_info(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_INFO);
	return 0;
}

static duk_ret_t
js_console_log(duk_context* ctx)
{
	// note: console.log() does not currently support format specifiers.
	//       this may change in a future implementation.

	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_NORMAL);
	return 0;
}

static duk_ret_t
js_console_trace(duk_context* ctx)
{
	// note: console.log() does not currently support format specifiers.
	//       this may change in a future implementation.

	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_TRACE);
	return 0;
}

static duk_ret_t
js_console_warn(duk_context* ctx)
{
	int num_items;

	// join the passed-in arguments separated with spaces
	num_items = duk_get_top(ctx);
	duk_push_string(ctx, " ");
	duk_insert(ctx, 0);
	duk_join(ctx, num_items);

	debug_print(duk_get_string(ctx, -1), PRINT_WARN);
	return 0;
}
