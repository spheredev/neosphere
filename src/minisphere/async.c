#include "minisphere.h"
#include "api.h"
#include "script.h"
#include "vector.h"

#include "async.h"

static duk_ret_t js_Async (duk_context* ctx);

static vector_t* s_scripts;

bool
initialize_async(void)
{
	console_log(1, "initializing Async");
	s_scripts = vector_new(sizeof(script_t*));
	return s_scripts != NULL;
}

void
shutdown_async(void)
{
	console_log(1, "shutting down Async");
	vector_free(s_scripts);
}

void
update_async(void)
{
	iter_t     iter;
	script_t** pscript;
	vector_t*  vector;
	
	vector = s_scripts;
	s_scripts = vector_new(sizeof(script_t*));
	if (vector != NULL) {
		iter = vector_enum(vector);
		while (pscript = vector_next(&iter)) {
			run_script(*pscript, false);
			free_script(*pscript);
		}
		vector_free(vector);
	}
}

bool
queue_async_script(script_t* script)
{
	if (s_scripts != NULL)
		return vector_push(s_scripts, &script);
	else
		return false;
}

void
init_async_api(void)
{
	register_api_method(g_duk, NULL, "Async", js_Async);
}

static duk_ret_t
js_Async(duk_context* ctx)
{
	script_t* script = duk_require_sphere_script(ctx, 0, "[async script]");

	if (!queue_async_script(script))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Async(): unable to queue async script");
	return 0;
}
