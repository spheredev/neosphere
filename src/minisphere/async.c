#include "minisphere.h"
#include "async.h"

#include "script.h"
#include "vector.h"

struct job
{
	async_hint_t hint;
	double       priority;
	uint32_t     timer;
	int64_t      token;
	script_t*    script;
};

static int sort_jobs (const void* in_a, const void* in_b);

static int64_t   s_next_token = 1;
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

bool
async_busy(void)
{
	return vector_len(s_recurring) > 0
		|| vector_len(s_onetime) > 0;
}

void
async_cancel_all(bool recurring)
{
	vector_clear(s_onetime);
	if (recurring)
		vector_clear(s_recurring);
}

void
async_cancel(int64_t token)
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

int64_t
async_defer(script_t* script, uint32_t timeout, async_hint_t hint)
{
	job_t* job;

	if (s_onetime == NULL)
		return 0;
	job = calloc(1, sizeof(job_t));
	job->timer = timeout;
	job->token = s_next_token++;
	job->script = script;
	job->hint = hint;
	vector_push(s_onetime, &job);
	return job->token;
}

int64_t
async_recur(script_t* script, double priority, async_hint_t hint)
{
	job_t* job;

	if (s_recurring == NULL)
		return 0;
	if (hint == ASYNC_RENDER) {
		// invert priority for render jobs.  this ensures higher priority jobs
		// get rendered later in a frame, i.e. closer to the screen.
		priority = -priority;
	}
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
			script_run(job->script, true);
	}

	// process one-time jobs.  swap in a fresh queue first to allow nested callbacks
	// to work.
	vector = s_onetime;
	s_onetime = vector_new(sizeof(job_t*));
	if (vector != NULL) {
		iter = vector_enum(vector);
		while (vector_next(&iter)) {
			job = *(job_t**)iter.ptr;
			if (job->hint == hint && job->timer-- == 0) {
				script_run(job->script, false);
				script_free(job->script);
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
	// qsort() is not stable.  luckily job tokens are strictly sequential,
	// so we can maintain FIFO order by just using the token as part of the
	// sort key.

	job_t*  job_a;
	job_t*  job_b;
	double  delta = 0.0;
	int64_t fifo_delta;

	job_a = *(job_t**)in_a;
	job_b = *(job_t**)in_b;
	delta = job_b->priority - job_a->priority;
	fifo_delta = job_a->token - job_b->token;
	return delta < 0.0 ? -1 : delta > 0.0 ? 1
		: fifo_delta < 0 ? -1 : fifo_delta > 0 ? 1
		: 0;
}
