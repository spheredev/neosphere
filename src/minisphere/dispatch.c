/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2019, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "minisphere.h"
#include "dispatch.h"

#include "script.h"
#include "vector.h"

struct then
{
	js_ref_t* callback;
	js_ref_t* resolver;
	js_ref_t* rejector;
};

struct job
{
	bool       background;
	bool       critical;
	bool       finished;
	job_type_t hint;
	double     priority;
	bool       paused;
	int        timer;
	int64_t    token;
	script_t*  script;
	vector_t*  then_scripts;
};

static bool        do_then_callback  (int num_args, bool is_ctor, intptr_t magic);
static struct job* job_from_token    (int64_t token);
static void        recheck_busy_flag (void);
static void        run_thens         (struct job* job);
static int         sort_jobs         (const void* in_a, const void* in_b);

static vector_t* s_exit_jobs;
static bool      s_is_busy = false;
static bool      s_need_sort = false;
static int64_t   s_next_token = 1;
static vector_t* s_onetime_jobs;
static vector_t* s_recurring_jobs;

void
dispatch_init(void)
{
	console_log(1, "initializing dispatch manager");
	s_onetime_jobs = vector_new(sizeof(struct job));
	s_recurring_jobs = vector_new(sizeof(struct job));
	s_exit_jobs = vector_new(sizeof(struct job));

	// reserve extra slots for one-time jobs.  realloc() is fairly expensive
	// and the one-time queue gets very heavy traffic.
	vector_reserve(s_onetime_jobs, 32);
}

void
dispatch_uninit(void)
{
	console_log(1, "shutting down dispatch manager");
	vector_free(s_onetime_jobs);
	vector_free(s_recurring_jobs);
	vector_free(s_exit_jobs);
}

bool
dispatch_busy(void)
{
	return s_is_busy || vector_len(s_onetime_jobs) > 0;
}

bool
dispatch_can_exit(void)
{
	return !dispatch_busy() && vector_len(s_exit_jobs) == 0;
}

void
dispatch_cancel(int64_t token)
{
	struct job* job;

	if (!(job = job_from_token(token)))
		return;
	job->finished = true;

	recheck_busy_flag();
	s_need_sort = true;
}

void
dispatch_cancel_all(bool recurring, bool also_critical)
{
	// NOTE: don't pass `true` for `also_critical` unless you're absolutely positive
	//       you know what you're doing!  doing this will cancel promise continuations,
	//       which is probably not what you want to do.

	struct job* job;

	iter_t iter;

	iter = vector_enum(s_onetime_jobs);
	while ((job = iter_next(&iter))) {
		if (!job->critical || also_critical)
			job->finished = true;
	}

	iter = vector_enum(s_exit_jobs);
	while ((job = iter_next(&iter))) {
		if (!job->critical || also_critical)
			job->finished = true;
	}

	if (recurring) {
		iter = vector_enum(s_recurring_jobs);
		while ((job = iter_next(&iter)))
			job->finished = true;
		s_need_sort = true;
		s_is_busy = false;
	}
}

int64_t
dispatch_defer(script_t* script, int timeout, job_type_t hint, bool critical)
{
	struct job job;
	vector_t*  queue;

	if (s_onetime_jobs == NULL)
		return 0;
	job.critical = critical;
	job.finished = false;
	job.hint = hint;
	job.script = script;
	job.paused = false;
	job.then_scripts = NULL;
	job.timer = timeout;
	job.token = s_next_token++;
	queue = hint == JOB_ON_EXIT ? s_exit_jobs
		: s_onetime_jobs;
	vector_push(queue, &job);
	return job.token;
}

void
dispatch_pause(int64_t token, bool paused)
{
	struct job* job;

	if (!(job = job_from_token(token)))
		return;
	job->paused = paused;
}

int64_t
dispatch_recur(script_t* script, double priority, bool background, job_type_t hint)
{
	struct job job;

	if (s_recurring_jobs == NULL)
		return 0;
	if (hint == JOB_ON_RENDER) {
		// invert priority for render jobs.  this ensures higher priority jobs
		// get rendered later in a frame, i.e. closer to the screen.
		priority = -priority;
	}
	job.background = background;
	job.finished = false;
	job.hint = hint;
	job.priority = priority;
	job.script = script;
	job.paused = false;
	job.then_scripts = NULL;
	job.token = s_next_token++;
	vector_push(s_recurring_jobs, &job);

	recheck_busy_flag();
	s_need_sort = true;

	return job.token;
}

bool
dispatch_run(job_type_t hint)
{
	static unsigned int last_call_id = 0;

	unsigned int call_id;
	struct job*  job;
	vector_t*    queue;

	int i;

	// each call to `dispatch_run` gets a unique call ID.  this is used to detect
	// reentrancy: if at any time `call_id` differs from `last_call_id`, that means another
	// call to `dispatch_run` happened before this one returned.
	call_id = ++last_call_id;

	if (s_need_sort) {
		vector_sort(s_recurring_jobs, sort_jobs);
		s_need_sort = false;
	}

	// process recurring jobs
	for (i = 0; i < vector_len(s_recurring_jobs); ++i) {
		job = (struct job*)vector_get(s_recurring_jobs, i);
		if (job->hint != hint)
			continue;
		if (!job->paused && !job->finished)
			script_run(job->script, false);  // invalidates job pointer
		if (last_call_id == call_id) {
			job = (struct job*)vector_get(s_recurring_jobs, i);
			if (job->finished) {
				script_unref(job->script);
				run_thens(job);
				vector_remove(s_recurring_jobs, i--);
			}
		}
		else {
			// reentrancy detected; bail out since it's unsafe to continue
			return false;
		}
	}

	// process one-time jobs
	queue = hint == JOB_ON_EXIT ? s_exit_jobs
		: s_onetime_jobs;
	for (i = 0; i < vector_len(queue); ++i) {
		job = (struct job*)vector_get(queue, i);
		if (job->hint != hint)
			continue;
		if (!job->paused && job->timer-- <= 0 && !job->finished) {
			job->finished = true;
			script_run(job->script, false);  // invalidates job pointer
		}
		if (last_call_id == call_id) {
			job = (struct job*)vector_get(queue, i);
			if (job->finished) {
				script_unref(job->script);
				run_thens(job);
				vector_remove(queue, i--);
			}
		}
		else {
			// reentrancy detected; bail out since it's unsafe to continue
			return false;
		}
	}

	return true;
}

js_ref_t*
dispatch_then(int64_t token, js_ref_t* callback)
{
	// IMPORTANT: `callback` reference is stolen; the caller must not use its own reference
	//            afterwards, not even to call jsal_unref()!
	
	struct job*  job;
	js_ref_t*    promise;
	js_ref_t*    rejector;
	js_ref_t*    resolver;
	script_t*    script;
	struct then* then;

	then = malloc(sizeof(struct then));
	jsal_push_new_promise(&resolver, &rejector);
	promise = jsal_ref(-1);
	then->callback = callback;
	then->resolver = resolver;
	then->rejector = rejector;
	jsal_push_new_function(do_then_callback, "", 0, false, (intptr_t)then);
	script = script_new_function(-1);
	jsal_pop(2);

	if ((job = job_from_token(token))) {
		if (job->then_scripts == NULL)
			job->then_scripts = vector_new(sizeof(script_t*));
		vector_push(job->then_scripts, &script);
	}
	else {
		// job already finished, dispatch callback immediately
		dispatch_defer(script, 0, JOB_ON_TICK, true);
	}

	return promise;
}

static bool
do_then_callback(int num_args, bool is_ctor, intptr_t magic)
{
	js_ref_t*    handler;
	struct then* then;

	then = (struct then*)magic;
	jsal_push_ref_weak(then->callback);
	jsal_push_undefined();
	handler = jsal_try_call(1)
		? then->resolver   // success, resolve promise with return value
		: then->rejector;  // callback threw, reject promise
	jsal_push_ref_weak(handler);
	jsal_pull(-2);
	jsal_call(1);
	jsal_unref(then->callback);
	jsal_unref(then->resolver);
	jsal_unref(then->rejector);
	free(then);
	return false;
}

static struct job*
job_from_token(int64_t token)
{
	struct job* job;

	iter_t iter;

	iter = vector_enum(s_recurring_jobs);
	while ((job = iter_next(&iter))) {
		if (token == job->token)
			return job;
	}
	iter = vector_enum(s_onetime_jobs);
	while ((job = iter_next(&iter))) {
		if (token == job->token)
			return job;
	}
	iter = vector_enum(s_exit_jobs);
	while ((job = iter_next(&iter))) {
		if (token == job->token)
			return job;
	}
	return NULL;
}

static void
recheck_busy_flag(void)
{
	struct job* job;

	iter_t iter;

	// check for recurring jobs that should keep the event loop alive
	s_is_busy = false;
	iter = vector_enum(s_recurring_jobs);
	while (job = iter_next(&iter))
		s_is_busy |= !job->finished && !job->background;
}

static void
run_thens(struct job* job)
{
	script_t* script;

	iter_t iter;

	if (job->then_scripts == NULL)
		return;
	
	iter = vector_enum(job->then_scripts);
	while (iter_next(&iter)) {
		script = *(script_t**)iter.ptr;
		dispatch_defer(script, 0, JOB_ON_TICK, true);
	}
	vector_free(job->then_scripts);
}

static int
sort_jobs(const void* in_a, const void* in_b)
{
	// qsort() is not stable.  luckily job tokens are strictly sequential,
	// so we can maintain FIFO order by just using the token as part of the
	// sort key.

	double      delta;
	int64_t     fifo_delta;
	struct job* job_a;
	struct job* job_b;

	job_a = (struct job*)in_a;
	job_b = (struct job*)in_b;
	delta = job_b->priority - job_a->priority;
	fifo_delta = job_a->token - job_b->token;
	return delta < 0.0 ? -1 : delta > 0.0 ? 1
		: fifo_delta < 0 ? -1 : fifo_delta > 0 ? 1
		: 0;
}
