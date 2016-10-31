#include "minisphere.h"
#include "async.h"

#include "script.h"
#include "vector.h"

struct job
{
	async_hint_t hint;
	script_t*    script;
};

static vector_t* s_onetime;
static vector_t* s_recurring;

void
async_init(void)
{
	console_log(1, "initializing async subsystem");
	s_onetime = vector_new(sizeof(struct job));
	s_recurring = vector_new(sizeof(struct job));
}

void
async_uninit(void)
{
	console_log(1, "shutting down async subsystem");
	vector_free(s_onetime);
	vector_free(s_recurring);
}

bool
async_dispatch(script_t* script, async_hint_t hint)
{
	struct job job;

	if (s_onetime == NULL)
		return false;
	job.script = script;
	job.hint = hint;
	return vector_push(s_onetime, &job);
}

bool
async_recur(script_t* script, async_hint_t hint)
{
	struct job job;

	if (s_recurring == NULL)
		return false;
	job.script = script;
	job.hint = hint;
	return vector_push(s_recurring, &job);
}

void
async_run(async_hint_t hint)
{
	struct job* job;
	vector_t*   vector;

	iter_t iter;

	// process recurring jobs
	iter = vector_enum(s_recurring);
	while (job = vector_next(&iter)) {
		if (job->hint == hint)
			run_script(job->script, true);
	}

	// process one-time jobs.  swap in a fresh queue first to allow nested callbacks
	// to work.
	vector = s_onetime;
	s_onetime = vector_new(sizeof(struct job));
	if (vector != NULL) {
		iter = vector_enum(vector);
		while (job = vector_next(&iter)) {
			if (job->hint == hint) {
				run_script(job->script, false);
				free_script(job->script);
			}
			else {
				vector_push(s_onetime, job);
			}
		}
		vector_free(vector);
	}
}
