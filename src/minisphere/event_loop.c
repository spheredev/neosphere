/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2018, Fat Cerberus
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
#include "event_loop.h"

#include "dispatch.h"

static bool run_main_event_loop (int num_args, bool is_ctor, intptr_t magic);

static bool s_exiting = false;
static int  s_frame_rate = 60;

bool
events_exiting(void)
{
	return s_exiting;
}

int
events_get_frame_rate(void)
{
	return s_frame_rate;
}

void
events_set_frame_rate(int frame_rate)
{
	s_frame_rate = frame_rate;
}

bool
events_run_main_loop(void)
{
	if (jsal_try(run_main_event_loop, 0)) {
		jsal_pop(1);  // don't need return value
		return true;
	}
	else {
		// leave the error for the caller, don't pop it off
		return false;
	}
}

static bool
run_main_event_loop(int num_args, bool is_ctor, intptr_t magic)
{
	// SPHERE v2 UNIFIED EVENT LOOP
	// once started, the event loop should continue to cycle until none of the
	// following remain:
	//    - Dispatch API jobs (either one-time or recurring)
	//    - promise continuations (e.g. await completions or .then())
	//    - JS module loader jobs
	//    - unhandled promise rejections

	// Sphere v1 exit paths disable the JavaScript VM to force the engine to
	// bail, so we need to re-enable it here.
	jsal_enable_vm(true);

	while (dispatch_busy() || jsal_busy())
		sphere_tick(2, true, s_frame_rate);

	// deal with Dispatch.onExit() jobs
	// note: the JavaScript VM might have been disabled due to a Sphere v1
	//       bailout; we'll need to re-enable it if so.
	jsal_enable_vm(true);
	s_exiting = true;
	while (!dispatch_can_exit() || jsal_busy()) {
		sphere_heartbeat(true, 2);
		dispatch_run(JOB_ON_TICK);
		dispatch_run(JOB_ON_EXIT);
	}
	s_exiting = false;

	return false;
}
