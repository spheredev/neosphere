#include "minisphere.h"
#include "async.h"

#include "api.h"
#include "script.h"
#include "vector.h"

static duk_ret_t js_DispatchScript (duk_context* ctx);

static unsigned int s_next_script_id = 1;
static vector_t*    s_scripts;

bool
initialize_async(void)
{
	console_log(1, "initializing async manager");
	if (!(s_scripts = vector_new(sizeof(script_t*))))
		return false;
	return true;
}

void
shutdown_async(void)
{
	console_log(1, "shutting down async manager");
	vector_free(s_scripts);
}

void
update_async(void)
{
	iter_t     iter;
	script_t** p_script;
	vector_t*  vector;
	
	vector = s_scripts;
	s_scripts = vector_new(sizeof(script_t*));
	if (vector != NULL) {
		iter = vector_enum(vector);
		while (p_script = vector_next(&iter)) {
			run_script(*p_script, false);
			free_script(*p_script);
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
	register_api_method(g_duk, NULL, "DispatchScript", js_DispatchScript);
}

static duk_ret_t
js_DispatchScript(duk_context* ctx)
{
	script_t* script;
	char*     script_name;

	script_name = strnewf("synth:async~%u.js", s_next_script_id++);
	script = duk_require_sphere_script(ctx, 0, script_name);
	free(script_name);

	if (!queue_async_script(script))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "unable to dispatch async script");
	return 0;
}
