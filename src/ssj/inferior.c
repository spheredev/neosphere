/**
 *  SSj, the Sphere JavaScript debugger
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

#include "ssj.h"
#include "inferior.h"

#include "backtrace.h"
#include "help.h"
#include "ki.h"
#include "objview.h"
#include "parser.h"
#include "session.h"
#include "sockets.h"
#include "source.h"

struct source
{
	char*      filename;
	source_t*  source;
};

struct inferior
{
	unsigned int   id;
	int            num_sources;
	bool           is_detached;
	bool           paused;
	char*          title;
	char*          author;
	backtrace_t*   calls;
	int            frame_index;
	bool           have_debug_info;
	int            line_no;
	int            protocol;
	uint8_t        ptr_size;
	bool           show_trace;
	socket_t*      socket;
	struct source* sources;
};

static void clear_pause_cache (inferior_t* obj);
static int  do_handshake      (socket_t* socket);
static bool handle_notify     (inferior_t* obj, const ki_message_t* msg);

static unsigned int s_next_inferior_id = 1;

void
inferiors_init(void)
{
	sockets_init(NULL);
}

void
inferiors_uninit(void)
{
	sockets_uninit();
}

inferior_t*
inferior_new(const char* hostname, int port, bool show_trace)
{
	inferior_t*   obj;
	char*         platform_name;
	ki_message_t* reply;
	ki_message_t* request;
	clock_t       timeout;

	obj = calloc(1, sizeof(inferior_t));
	printf("connecting to %s:%d... ", hostname, port);
	fflush(stdout);
	obj->socket = socket_new(1024, true);
	if (!socket_connect(obj->socket, hostname, port))
		goto on_error;
	timeout = clock() + 60 * CLOCKS_PER_SEC;
	while (!socket_connected(obj->socket)) {
		if (socket_closed(obj->socket))
			socket_connect(obj->socket, hostname, port);
		if (clock() > timeout)
			goto timed_out;
		sockets_update();
	}

	printf("OK.\n");
	obj->protocol = do_handshake(obj->socket);
	if (obj->protocol == 0)
		goto on_error;

	// set watermark (shown on bottom left)
	request = ki_message_new(KI_REQ);
	ki_message_add_int(request, KI_REQ_SET_WATERMARK);
	ki_message_add_string(request, "ssj");
	ki_message_add_int(request, 255);
	ki_message_add_int(request, 224);
	ki_message_add_int(request, 0);
	reply = inferior_request(obj, request);
	ki_message_free(reply);

	printf("downloading game information... ");
	request = ki_message_new(KI_REQ);
	ki_message_add_int(request, KI_REQ_GET_GAME_INFO);
	reply = inferior_request(obj, request);
	platform_name = strdup(ki_message_string(reply, 0));
	obj->title = strdup(ki_message_string(reply, 1));
	obj->author = strdup(ki_message_string(reply, 2));
	ki_message_free(reply);
	printf("OK.\n");

	printf("    engine: \33[37;1m%s\33[m\n", platform_name);
	printf("    title:  \33[37;1m%s\33[m\n", obj->title);
	printf("    author: \33[37;1m%s\33[m\n", obj->author);

	obj->id = s_next_inferior_id++;
	obj->show_trace = show_trace;
	return obj;

timed_out:
	printf("\33[31merror!\33[m\n");
	return NULL;

on_error:
	free(obj);
	return NULL;
}

void
inferior_free(inferior_t* obj)
{
	int i;

	clear_pause_cache(obj);
	for (i = 0; i < obj->num_sources; ++i) {
		source_free(obj->sources[i].source);
		free(obj->sources[i].filename);
	}
	socket_close(obj->socket);
	free(obj->title);
	free(obj->author);
	free(obj);
}

bool
inferior_update(inferior_t* obj)
{
	bool          is_active = true;
	ki_message_t* notify = NULL;

	if (obj->is_detached)
		return false;

	if (!(notify = ki_message_recv(obj->socket)))
		goto detached;
	if (!handle_notify(obj, notify))
		goto detached;
	ki_message_free(notify);
	return true;

detached:
	ki_message_free(notify);
	obj->is_detached = true;
	return false;
}

bool
inferior_attached(const inferior_t* obj)
{
	return !obj->is_detached;
}

bool
inferior_running(const inferior_t* obj)
{
	return !obj->paused && inferior_attached(obj);
}

const char*
inferior_author(const inferior_t* obj)
{
	return obj->author;
}

const char*
inferior_title(const inferior_t* obj)
{
	return obj->title;
}

const backtrace_t*
inferior_get_calls(inferior_t* obj)
{
	char*         call_name;
	const char*   filename;
	const char*   function_name;
	int           line_no;
	int           num_frames;
	ki_message_t* request;

	int i;

	if (obj->calls == NULL) {
		request = ki_message_new(KI_REQ);
		ki_message_add_int(request, KI_REQ_INSPECT_STACK);
		if (!(request = inferior_request(obj, request)))
			return NULL;
		num_frames = ki_message_len(request) / 4;
		obj->calls = backtrace_new();
		for (i = 0; i < num_frames; ++i) {
			function_name = ki_message_string(request, i * 4 + 1);
			if (strcmp(function_name, "Anonymous function") == 0)
				call_name = strdup("[anonymous function]");
			else if (strcmp(function_name, "Global code") == 0)
				call_name = strdup("[global code]");
			else
				call_name = strnewf("%s()", function_name);
			filename = ki_message_string(request, i * 4);
			line_no = ki_message_int(request, i * 4 + 2);
			backtrace_add(obj->calls, call_name, filename, line_no);
		}
		ki_message_free(request);
	}

	return obj->calls;
}

objview_t*
inferior_get_object(inferior_t* obj, unsigned int handle, bool get_all)
{
	unsigned int     flags;
	const ki_atom_t* getter;
	bool             is_accessor;
	int              index = 0;
	char*            key_string;
	int              prop_flags;
	const ki_atom_t* prop_key;
	ki_message_t*    request;
	const ki_atom_t* setter;
	const ki_atom_t* value;
	objview_t*       view;

	request = ki_message_new(KI_REQ);
	ki_message_add_int(request, KI_REQ_INSPECT_OBJ);
	ki_message_add_handle(request, handle);
	ki_message_add_int(request, 0);
	ki_message_add_int(request, INT_MAX);
	if (!(request = inferior_request(obj, request)))
		return NULL;
	view = objview_new();
	while (index < ki_message_len(request)) {
		prop_flags = ki_message_int(request, index++);
		prop_key = ki_message_atom(request, index++);
		if (ki_atom_tag(prop_key) == KI_STRING)
			key_string = strdup(ki_atom_cstr(prop_key));
		else
			key_string = strnewf("%d", ki_atom_int(prop_key));
		is_accessor = (prop_flags & 0x008) != 0;
		if (prop_flags & 0x100) {
			index += is_accessor ? 2 : 1;
			continue;
		}
		flags = 0x0;
		if (prop_flags & 0x01) flags |= PROP_WRITABLE;
		if (prop_flags & 0x02) flags |= PROP_ENUMERABLE;
		if (prop_flags & 0x04) flags |= PROP_CONFIGURABLE;
		if (is_accessor) {
			getter = ki_message_atom(request, index++);
			setter = ki_message_atom(request, index++);
			objview_add_accessor(view, key_string, getter, setter, flags);
		}
		else {
			value = ki_message_atom(request, index++);
			if (ki_atom_tag(value) != KI_UNUSED)
				objview_add_value(view, key_string, "property", value, flags);
		}
		free(key_string);
	}
	ki_message_free(request);
	return view;
}

const source_t*
inferior_get_source(inferior_t* obj, const char* filename)
{
	int           cache_id;
	ki_message_t* request;
	source_t*     source;
	const char*   text;

	int i;

	for (i = 0; i < obj->num_sources; ++i) {
		if (strcmp(filename, obj->sources[i].filename) == 0)
			return obj->sources[i].source;
	}

	request = ki_message_new(KI_REQ);
	ki_message_add_int(request, KI_REQ_GET_SOURCE);
	ki_message_add_string(request, filename);
	if (!(request = inferior_request(obj, request)))
		goto on_error;
	if (ki_message_tag(request) == KI_ERR)
		goto on_error;
	text = ki_message_string(request, 0);
	source = source_new(text);

	cache_id = obj->num_sources++;
	obj->sources = realloc(obj->sources, obj->num_sources * sizeof(struct source));
	obj->sources[cache_id].filename = strdup(filename);
	obj->sources[cache_id].source = source;

	return source;

on_error:
	ki_message_free(request);
	return NULL;
}

objview_t*
inferior_get_vars(inferior_t* obj, int frame)
{
	const char*      class_name;
	ki_message_t*    msg;
	const char*      name;
	int              num_vars;
	const ki_atom_t* value;
	objview_t*       vars;

	int i;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_INSPECT_LOCALS);
	ki_message_add_int(msg, -frame - 1);
	if (!(msg = inferior_request(obj, msg)))
		return NULL;
	vars = objview_new();
	num_vars = ki_message_len(msg) / 3;
	for (i = 0; i < num_vars; ++i) {
		name = ki_message_string(msg, i * 3 + 0);
		class_name = ki_message_string(msg, i * 3 + 1);
		value = ki_message_atom(msg, i * 3 + 2);
		objview_add_value(vars, name, class_name, value, 0x0);
	}
	return vars;
}

int
inferior_add_breakpoint(inferior_t* obj, const char* filename, int linenum)
{
	int        handle;
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_ADD_BREAK);
	ki_message_add_string(msg, filename);
	ki_message_add_int(msg, linenum);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (ki_message_tag(msg) == KI_ERR)
		goto on_error;
	handle = ki_message_int(msg, 0);
	ki_message_free(msg);
	return handle;

on_error:
	ki_message_free(msg);
	return -1;
}

bool
inferior_clear_breakpoint(inferior_t* obj, int handle)
{
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_DEL_BREAK);
	ki_message_add_int(msg, handle);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (ki_message_tag(msg) == KI_ERR)
		goto on_error;
	ki_message_free(msg);
	return true;

on_error:
	ki_message_free(msg);
	return false;
}

void
inferior_detach(inferior_t* obj)
{
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_DETACH);
	if (!(msg = inferior_request(obj, msg)))
		return;
	ki_message_free(msg);
	while (inferior_attached(obj))
		inferior_update(obj);
}

ki_atom_t*
inferior_eval(inferior_t* obj, const char* expr, int frame, bool* out_is_error)
{
	ki_atom_t*  dvalue = NULL;
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_EVAL);
	if (obj->protocol == 2) {
		ki_message_add_int(msg, -(1 + frame));
		ki_message_add_string(msg, expr);
	}
	else {
		ki_message_add_string(msg, expr);
		ki_message_add_int(msg, -(1 + frame));
	}
	msg = inferior_request(obj, msg);
	dvalue = ki_atom_dup(ki_message_atom(msg, 1));
	*out_is_error = ki_message_int(msg, 0) != 0;
	ki_message_free(msg);
	return dvalue;
}

bool
inferior_pause(inferior_t* obj)
{
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_PAUSE);
	if (!(msg = inferior_request(obj, msg)))
		return false;
	return true;
}

ki_message_t*
inferior_request(inferior_t* obj, ki_message_t* msg)
{
	ki_message_t* response = NULL;

	if (!(ki_message_send(msg, obj->socket)))
		goto lost_connection;
	do {
		ki_message_free(response);
		if (!(response = ki_message_recv(obj->socket)))
			goto lost_connection;
		if (ki_message_tag(response) == KI_NFY)
			handle_notify(obj, response);
	} while (ki_message_tag(response) == KI_NFY);
	ki_message_free(msg);
	return response;

lost_connection:
	printf("\33[31;1mSSj lost communication with the target.\33[m\n");
	ki_message_free(msg);
	obj->is_detached = true;
	return NULL;
}

bool
inferior_resume(inferior_t* obj, resume_op_t op)
{
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg,
		op == OP_STEP_OVER ? KI_REQ_STEP_OVER
		: op == OP_STEP_IN ? KI_REQ_STEP_IN
		: op == OP_STEP_OUT ? KI_REQ_STEP_OUT
		: KI_REQ_RESUME);
	if (!(msg = inferior_request(obj, msg)))
		return false;
	obj->frame_index = 0;
	obj->paused = false;
	while (inferior_running(obj))
		inferior_update(obj);
	return true;
}

static void
clear_pause_cache(inferior_t* obj)
{
	backtrace_free(obj->calls);
	obj->calls = NULL;
}

static int
do_handshake(socket_t* socket)
{
	static char handshake[128];

	ptrdiff_t    idx;
	char*        next_token;
	int          protocol;
	char*        token;

	printf("establishing communication... ");
	idx = 0;
	do {
		if (idx >= 127)  // probably not a Duktape handshake
			goto on_error;
		socket_read(socket, &handshake[idx], 1);
	} while (handshake[idx++] != '\n');
	handshake[idx - 1] = '\0';  // remove the newline

	// parse handshake line
	if (!(token = strtok_r(handshake, " ", &next_token)))
		goto on_error;
	protocol = atoi(token);
	if (protocol < 1 || protocol > 2)
		goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	printf("OK.\n");

	return protocol;

on_error:
	printf("\33[31merror!\33[m\n");
	return 0;
}

static bool
handle_notify(inferior_t* inferior, const ki_message_t* msg)
{
	const char*   heading;
	enum print_op print_op;
	int           status_type;

	switch (ki_message_tag(msg)) {
	case KI_NFY:
		switch (ki_message_int(msg, 0)) {
		case KI_NFY_DETACHING:
			status_type = ki_message_int(msg, 1);
			if (status_type == 0)
				printf("\33[36;1mthe engine disconnected from SSj normally.\n");
			else
				printf("\33[31;1ma communication error occurred while debugging.\n");
			printf("\33[m");
			inferior->is_detached = true;
			return false;
		case KI_NFY_LOG:
			print_op = (enum print_op)ki_message_int(msg, 1);
			heading = print_op == PRINT_ASSERT ? "ASSERT"
				: print_op == PRINT_DEBUG ? "debug"
				: print_op == PRINT_ERROR ? "ERROR"
				: print_op == PRINT_INFO ? "info"
				: print_op == PRINT_TRACE ? "trace"
				: print_op == PRINT_WARN ? "warn"
				: "log";
			if (print_op == PRINT_TRACE && !inferior->show_trace)
				break;
			if (print_op == PRINT_ASSERT || print_op == PRINT_ERROR)
				printf("\33[31;1m");
			else if (print_op == PRINT_WARN)
				printf("\33[33;1m");
			else
				printf("\33[36m");
			printf("%s: %s\n", heading, ki_message_string(msg, 2));
			printf("\33[m");
			break;
		case KI_NFY_PAUSE:
			inferior->paused = true;
			break;
		case KI_NFY_RESUME:
			inferior->paused = false;
			clear_pause_cache(inferior);
			break;
		case KI_NFY_THROW:
			if ((status_type = ki_message_int(msg, 1)) == 0)
				break;
			printf("\33[31;1m");
			printf("uncaught: ");
			printf("\33[m");
			printf("%s\n", ki_message_string(msg, 2));
			break;
		}
		break;
	}
	return true;
}
