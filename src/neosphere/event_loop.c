/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

#include "neosphere.h"
#include "event_loop.h"

#include "api.h"
#include "dispatch.h"
#include "pegasus.h"
#include "sockets.h"

enum task_type
{
	TASK_ACCEPT_CLIENT,
	TASK_CLOSE_SOCKET,
	TASK_CONNECT,
	TASK_READ_SOCKET,
	TASK_WRITE_SOCKET,
};

struct task
{
	enum task_type type;
	js_ref_t*      resolver;
	js_ref_t*      rejector;
	server_t*      server;
	socket_t*      socket;
	js_ref_t*      buffer_ref;
	int            bytes_left;
	uint8_t*       ptr;
};

static void         free_task             (struct task* task);
static struct task* push_new_task_promise (enum task_type type);
static bool         run_main_event_loop   (int num_args, bool is_ctor, intptr_t magic);

static bool      s_exiting = false;
static int       s_frame_rate = 60;
static vector_t* s_tasks;

void
events_init(void)
{
	s_tasks = vector_new(sizeof(struct task));
}

void
events_uninit(void)
{
	struct task* task;

	int i, len;

	for (i = 0, len = vector_len(s_tasks); i < len; ++i) {
		task = vector_get(s_tasks, i);
		free_task(task);
	}
	vector_free(s_tasks);
}

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

void
events_accept_client(server_t* server)
{
	struct task* task;

	task = push_new_task_promise(TASK_ACCEPT_CLIENT);
	task->server = server_ref(server);
}

void
events_close_socket(socket_t* socket)
{
	struct task* task;

	socket_close(socket);

	task = push_new_task_promise(TASK_CLOSE_SOCKET);
	task->socket = socket_ref(socket);
}

void
events_connect_to(socket_t* socket, const char* hostname, int port)
{
	struct task* task;

	if (socket == NULL)
		socket = socket_new(1024, false);
	else
		socket_ref(socket);
	socket_connect(socket, hostname, port);

	task = push_new_task_promise(TASK_CONNECT);
	task->socket = socket_ref(socket);
}

void
events_read_socket(socket_t* socket, int num_bytes)
{
	js_ref_t*    buffer_ref;
	void*        data_ptr;
	struct task* task;

	jsal_push_new_buffer(JS_ARRAYBUFFER, num_bytes, &data_ptr);
	buffer_ref = jsal_pop_ref();

	task = push_new_task_promise(TASK_READ_SOCKET);
	task->socket = socket_ref(socket);
	task->buffer_ref = buffer_ref;
	task->bytes_left = num_bytes;
	task->ptr = data_ptr;
}

void
events_write_socket(socket_t* socket, const void* data, int num_bytes)
{
	struct task* task;

	socket_write(socket, data, num_bytes);

	task = push_new_task_promise(TASK_WRITE_SOCKET);
	task->socket = socket_ref(socket);
	task->bytes_left = socket_bytes_out(socket) + num_bytes;
}

bool
events_run_main_loop(void)
{
	if (jsal_try(run_main_event_loop, 0)) {
		jsal_pop(1);  // don't need the return value
		return true;
	}
	else {
		// leave the error for the caller, don't pop it off
		return false;
	}
}

void
events_tick(int api_version, bool clear_screen, int framerate)
{
	int          bytes_read;
	socket_t*    client;
	bool         task_errored;
	bool         task_finished;
	struct task* task;

	int i;

	sphere_heartbeat(true, api_version);

	if (!screen_skipping_frame(g_screen)) {
		if (!dispatch_run(JOB_ON_RENDER))
			return;
	}

	// flip the backbuffer.  if this is a Sphere v2 frame, also reset clipping.
	screen_flip(g_screen, framerate, clear_screen);
	if (api_version >= 2)
		image_set_scissor(screen_backbuffer(g_screen), screen_bounds(g_screen));

	if (!dispatch_run(JOB_ON_UPDATE))
		return;

	if (!dispatch_run(JOB_ON_TICK))
		return;

	// handle ongoing asynchronous tasks
	for (i = 0; i < vector_len(s_tasks); ++i) {
		task = vector_get(s_tasks, i);
		task_finished = false;
		task_errored = false;
		switch (task->type) {
		case TASK_ACCEPT_CLIENT:
			if (client = server_accept(task->server)) {
				jsal_push_class_obj(PEGASUS_SOCKET, client, false);
				task_finished = true;
			}
			break;
		case TASK_CONNECT:
			if (socket_connected(task->socket)) {
				jsal_push_class_obj(PEGASUS_SOCKET, task->socket, false);
				task_finished = true;
			}
			else if (socket_closed(task->socket)) {
				jsal_push_new_error(JS_ERROR, "Unable to establish TCP connection");
				task_errored = true;
			}
			break;
		case TASK_CLOSE_SOCKET:
			if (socket_closed(task->socket)) {
				jsal_push_undefined();
				task_finished = true;
			}
			break;
		case TASK_READ_SOCKET:
			bytes_read = socket_read(task->socket, task->ptr, task->bytes_left);
			task->bytes_left -= bytes_read;
			task->ptr += bytes_read;
			if (task->bytes_left == 0) {
				jsal_push_ref_weak(task->buffer_ref);
				task_finished = true;
			}
			else if (!socket_connected(task->socket)) {
				jsal_push_new_error(JS_ERROR, "Connection was lost before completion of read");
				task_errored = true;
			}
			break;
		case TASK_WRITE_SOCKET:
			if (socket_bytes_out(task->socket) >= task->bytes_left) {
				jsal_push_undefined();
				task_finished = true;
			}
			else if (!socket_connected(task->socket)) {
				jsal_push_new_error(JS_ERROR, "Connection was lost before completion of write");
				task_errored = true;
			}
			break;
		}
		if (task_finished || task_errored) {
			jsal_push_ref_weak(task_errored ? task->rejector : task->resolver);
			jsal_pull(-2);
			jsal_call(1);
			jsal_pop(1);
			free_task(task);
			vector_remove(s_tasks, i--);
		}
	}

	// run the microtask queue one more time to finalize any promises that
	// were settled above.
	if (!dispatch_run(JOB_ON_TICK))
		return;

	++g_tick_count;
}

static struct task*
push_new_task_promise(enum task_type type)
{
	js_ref_t*   rejector;
	js_ref_t*   resolver;
	struct task task;

	// note: leave promise on top of the JSAL stack for convenience
	jsal_push_new_promise(&resolver, &rejector);

	memset(&task, 0, sizeof(struct task));
	task.type = type;
	task.resolver = resolver;
	task.rejector = rejector;
	vector_push(s_tasks, &task);
	return vector_get(s_tasks, vector_len(s_tasks) - 1);
}

static void
free_task(struct task* task)
{
	jsal_unref(task->buffer_ref);
	jsal_unref(task->rejector);
	jsal_unref(task->resolver);
	socket_unref(task->socket);
	server_unref(task->server);
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

	while (dispatch_busy() || jsal_busy() || vector_len(s_tasks) > 0)
		events_tick(2, true, s_frame_rate);

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
