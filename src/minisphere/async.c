/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2017, Fat Cerberus
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
#include "async.h"

#include "script.h"
#include "vector.h"

struct job
{
	bool         finished;
	async_hint_t hint;
	double       priority;
	uint32_t     timer;
	int64_t      token;
	script_t*    script;
};

static int sort_jobs (const void* in_a, const void* in_b);

static bool      s_need_sort = false;
static int64_t   s_next_token = 1;
static vector_t* s_onetime;
static vector_t* s_recurring;

void
async_init(void)
{
	console_log(1, "initializing dispatch manager");
	s_onetime = vector_new(sizeof(struct job));
	s_recurring = vector_new(sizeof(struct job));
}

void
async_uninit(void)
{
	console_log(1, "shutting down dispatch manager");
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
	struct job* job;

	iter_t iter;

	iter = vector_enum(s_onetime);
	while (job = iter_next(&iter))
		job->finished = true;

	if (recurring) {
		iter = vector_enum(s_recurring);
		while (job = iter_next(&iter))
			job->finished = true;
		s_need_sort = true;
	}
}

void
async_cancel(int64_t token)
{
	struct job* job;

	iter_t iter;

	iter = vector_enum(s_onetime);
	while (job = iter_next(&iter)) {
		if (job->token == token)
			job->finished = true;
	}

	iter = vector_enum(s_recurring);
	while (job = iter_next(&iter)) {
		if (job->token == token)
			job->finished = true;
	}

	s_need_sort = true;
}

int64_t
async_defer(script_t* script, uint32_t timeout, async_hint_t hint)
{
	struct job job;

	if (s_onetime == NULL)
		return 0;
	job.finished = false;
	job.hint = hint;
	job.script = script;
	job.timer = timeout;
	job.token = s_next_token++;
	vector_push(s_onetime, &job);
	return job.token;
}

int64_t
async_recur(script_t* script, double priority, async_hint_t hint)
{
	struct job job;

	if (s_recurring == NULL)
		return 0;
	if (hint == ASYNC_RENDER) {
		// invert priority for render jobs.  this ensures higher priority jobs
		// get rendered later in a frame, i.e. closer to the screen.
		priority = -priority;
	}
	job.finished = false;
	job.hint = hint;
	job.priority = priority;
	job.script = script;
	job.token = s_next_token++;
	vector_push(s_recurring, &job);

	s_need_sort = true;

	return job.token;
}

void
async_run_jobs(async_hint_t hint)
{
	struct job* job;

	int i;

	if (s_need_sort)
		vector_sort(s_recurring, sort_jobs);

	// process recurring jobs
	for (i = 0; i < vector_len(s_recurring); ++i) {
		job = vector_get(s_recurring, i);
		if (job->hint == hint && !job->finished)
			script_run(job->script, true);
		if (job->finished) {
			script_unref(job->script);
			vector_remove(s_recurring, i--);
		}
	}

	// process one-time jobs.  swap in a fresh queue first to allow nested callbacks
	// to work.
	if (s_onetime != NULL) {
		for (i = 0; i < vector_len(s_onetime); ++i) {
			job = vector_get(s_onetime, i);
			if (job->hint == hint && job->timer-- == 0 && !job->finished) {
				script_run(job->script, false);
				job->finished = true;
			}
			if (job->finished) {
				script_unref(job->script);
				vector_remove(s_onetime, i--);
			}
		}
	}
}

static int
sort_jobs(const void* in_a, const void* in_b)
{
	// qsort() is not stable.  luckily job tokens are strictly sequential,
	// so we can maintain FIFO order by just using the token as part of the
	// sort key.

	double      delta = 0.0;
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
