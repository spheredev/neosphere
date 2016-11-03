#include "minisphere.h"
#include "async.h"

#include "script.h"
#include "vector.h"

struct job
{
	async_hint_t hint;
	double       priority;
	double       timeout;
	uint64_t     token;
	script_t*    script;
};

static int sort_jobs (const void* in_a, const void* in_b);

static uint64_t  s_next_token = 1;
static vector_t* s_onetime;
static vector_t* s_recurring;

void
async_init(void)
{
	console_log(1, "initializing async subsystem");
	s_onetime = vector_new(sizeof(job_t*));
	s_recurring = vector_new(sizeof(job_t*));
}

void
async_uninit(void)
{
	console_log(1, "shutting down async subsystem");
	vector_free(s_onetime);
	vector_free(s_recurring);
}

void
async_cancel(uint64_t token)
{
	iter_t  iter;
	job_t** p_job;

	iter = vector_enum(s_onetime);
	while (p_job = vector_next(&iter)) {
		if ((*p_job)->token == token) {
			iter_remove(&iter);
			free(*p_job);
		}
	}
	iter = vector_enum(s_recurring);
	while (p_job = vector_next(&iter)) {
		if ((*p_job)->token == token) {
			iter_remove(&iter);
			free(*p_job);
		}
	}
}

uint64_t
async_defer(script_t* script, double timeout, async_hint_t hint)
{
	job_t* job;

	if (s_onetime == NULL)
		return 0;
	job = calloc(1, sizeof(job_t));
	job->timeout = al_get_time() + timeout;
	job->token = s_next_token++;
	job->script = script;
	job->hint = hint;
	vector_push(s_onetime, &job);
	return job->token;
}

uint64_t
async_recur(script_t* script, double priority, async_hint_t hint)
{
	job_t* job;

	if (s_recurring == NULL)
		return 0;
	job = calloc(1, sizeof(job_t));
	job->token = s_next_token++;
	job->script = script;
	job->hint = hint;
	job->priority = priority;
	vector_push(s_recurring, &job);
	vector_sort(s_recurring, sort_jobs);
	return job->token;
}

void
async_run_jobs(async_hint_t hint)
{
	job_t*    job;
	vector_t* vector;

	iter_t iter;

	// process recurring jobs
	iter = vector_enum(s_recurring);
	while (vector_next(&iter)) {
		job = *(job_t**)iter.ptr;
		if (job->hint == hint)
			run_script(job->script, true);
	}

	// process one-time jobs.  swap in a fresh queue first to allow nested callbacks
	// to work.
	vector = s_onetime;
	s_onetime = vector_new(sizeof(job_t*));
	if (vector != NULL) {
		iter = vector_enum(vector);
		while (vector_next(&iter)) {
			job = *(job_t**)iter.ptr;
			if (job->hint == hint && al_get_time() >= job->timeout) {
				run_script(job->script, false);
				free_script(job->script);
			}
			else {
				vector_push(s_onetime, &job);
			}
		}
		vector_free(vector);
	}
}

static int
sort_jobs(const void* in_a, const void* in_b)
{
	double    diff;
	ptrdiff_t order;
	
	diff = (*(job_t**)in_a)->priority - (*(job_t**)in_b)->priority;
	order = (uint8_t*)in_a - (uint8_t*)in_b;
	return diff < 0.0 ? -1 : diff > 0.0 ? 1
		: order < 0 ? -1 : order > 0 ? 1
		: 0;
}
