#include "minisphere.h"
#include "async.h"

#include "script.h"
#include "vector.h"

static vector_t* s_scripts;

bool
async_init(void)
{
	console_log(1, "initializing async subsystem");
	if (!(s_scripts = vector_new(sizeof(script_t*))))
		return false;
	return true;
}

void
async_uninit(void)
{
	console_log(1, "shutting down async subsystem");
	vector_free(s_scripts);
}

void
async_update(void)
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
async_dispatch(script_t* script)
{
	if (s_scripts != NULL)
		return vector_push(s_scripts, &script);
	else
		return false;
}
