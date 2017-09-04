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
	unsigned int   id_no;
	int            num_sources;
	bool           is_detached;
	bool           is_paused;
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

static unsigned int s_next_id_no = 1;

void
inferiors_init(void)
{
	sockets_init();
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
		if (clock() > timeout)
			goto timed_out;
		sockets_update();
	}

	printf("OK.\n");
	obj->protocol = do_handshake(obj->socket);
	if (obj->protocol == 0)
		goto on_error;

	// set watermark (shown on bottom left)
	request = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(request, REQ_APPREQUEST);
	dmessage_add_int(request, APPREQ_WATERMARK);
	dmessage_add_string(request, "ssj");
	dmessage_add_int(request, 255);
	dmessage_add_int(request, 224);
	dmessage_add_int(request, 0);
	reply = inferior_request(obj, request);
	dmessage_free(reply);

	printf("querying target... ");
	request = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(request, REQ_APPREQUEST);
	dmessage_add_int(request, APPREQ_GAME_INFO);
	reply = inferior_request(obj, request);
	obj->title = strdup(dmessage_get_string(reply, 0));
	obj->author = strdup(dmessage_get_string(reply, 1));
	dmessage_free(reply);
	printf("OK.\n");

	printf("    game: %s\n", obj->title);
	printf("    author: %s\n", obj->author);

	obj->id_no = s_next_id_no++;
	obj->show_trace = show_trace;
	return obj;

timed_out:
	printf("\33[31mtimeout\33[m\n");
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

	if (!(notify = dmessage_recv(obj->socket)))
		goto detached;
	if (!handle_notify(obj, notify))
		goto detached;
	dmessage_free(notify);
	return true;

detached:
	dmessage_free(notify);
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
	return !obj->is_paused && inferior_attached(obj);
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
		request = dmessage_new(DMESSAGE_REQ);
		dmessage_add_int(request, REQ_GETCALLSTACK);
		if (!(request = inferior_request(obj, request)))
			return NULL;
		num_frames = dmessage_len(request) / 4;
		obj->calls = backtrace_new();
		for (i = 0; i < num_frames; ++i) {
			function_name = dmessage_get_string(request, i * 4 + 1);
			if (strcmp(function_name, "") == 0)
				call_name = strdup("[anon]");
			else
				call_name = strnewf("%s()", function_name);
			filename = dmessage_get_string(request, i * 4);
			line_no = dmessage_get_int(request, i * 4 + 2);
			backtrace_add(obj->calls, call_name, filename, line_no);
		}
		dmessage_free(request);
	}

	return obj->calls;
}

objview_t*
inferior_get_object(inferior_t* obj, remote_ptr_t heapptr, bool get_all)
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

	request = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(request, REQ_GETOBJPROPDESCRANGE);
	dmessage_add_heapptr(request, heapptr);
	dmessage_add_int(request, 0);
	dmessage_add_int(request, INT_MAX);
	if (!(request = inferior_request(obj, request)))
		return NULL;
	view = objview_new();
	while (index < dmessage_len(request)) {
		prop_flags = dmessage_get_int(request, index++);
		prop_key = dmessage_get_dvalue(request, index++);
		if (dvalue_tag(prop_key) == DVALUE_STRING)
			key_string = strdup(dvalue_as_cstr(prop_key));
		else
			key_string = strnewf("%d", dvalue_as_int(prop_key));
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
			getter = dmessage_get_dvalue(request, index++);
			setter = dmessage_get_dvalue(request, index++);
			objview_add_accessor(view, key_string, getter, setter, flags);
		}
		else {
			value = dmessage_get_dvalue(request, index++);
			if (dvalue_tag(value) != DVALUE_UNUSED)
				objview_add_value(view, key_string, value, flags);
		}
		free(key_string);
	}
	dmessage_free(request);
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

	request = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(request, REQ_APPREQUEST);
	dmessage_add_int(request, APPREQ_SOURCE);
	dmessage_add_string(request, filename);
	if (!(request = inferior_request(obj, request)))
		goto on_error;
	if (dmessage_tag(request) == DMESSAGE_ERR)
		goto on_error;
	text = dmessage_get_string(request, 0);
	source = source_new(text);

	cache_id = obj->num_sources++;
	obj->sources = realloc(obj->sources, obj->num_sources * sizeof(struct source));
	obj->sources[cache_id].filename = strdup(filename);
	obj->sources[cache_id].source = source;

	return source;

on_error:
	dmessage_free(request);
	return NULL;
}

objview_t*
inferior_get_vars(inferior_t* obj, int frame)
{
	ki_message_t*    msg;
	const char*      name;
	int              num_vars;
	const ki_atom_t* value;
	objview_t*       vars;

	int i;

	msg = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(msg, REQ_GETLOCALS);
	dmessage_add_int(msg, -frame - 1);
	if (!(msg = inferior_request(obj, msg)))
		return NULL;
	vars = objview_new();
	num_vars = dmessage_len(msg) / 2;
	for (i = 0; i < num_vars; ++i) {
		name = dmessage_get_string(msg, i * 2 + 0);
		value = dmessage_get_dvalue(msg, i * 2 + 1);
		objview_add_value(vars, name, value, 0x0);
	}
	return vars;
}

int
inferior_add_breakpoint(inferior_t* obj, const char* filename, int linenum)
{
	int        handle;
	ki_message_t* msg;

	msg = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(msg, REQ_ADDBREAK);
	dmessage_add_string(msg, filename);
	dmessage_add_int(msg, linenum);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (dmessage_tag(msg) == DMESSAGE_ERR)
		goto on_error;
	handle = dmessage_get_int(msg, 0);
	dmessage_free(msg);
	return handle;

on_error:
	dmessage_free(msg);
	return -1;
}

bool
inferior_clear_breakpoint(inferior_t* obj, int handle)
{
	ki_message_t* msg;

	msg = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(msg, REQ_DELBREAK);
	dmessage_add_int(msg, handle);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (dmessage_tag(msg) == DMESSAGE_ERR)
		goto on_error;
	dmessage_free(msg);
	return true;

on_error:
	dmessage_free(msg);
	return false;
}

void
inferior_detach(inferior_t* obj)
{
	ki_message_t* msg;

	msg = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(msg, REQ_DETACH);
	if (!(msg = inferior_request(obj, msg)))
		return;
	dmessage_free(msg);
	while (inferior_attached(obj))
		inferior_update(obj);
}

ki_atom_t*
inferior_eval(inferior_t* obj, const char* expr, int frame, bool* out_is_error)
{
	ki_atom_t*  dvalue = NULL;
	ki_message_t* msg;

	msg = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(msg, REQ_EVAL);
	if (obj->protocol == 2) {
		dmessage_add_int(msg, -(1 + frame));
		dmessage_add_string(msg, expr);
	}
	else {
		dmessage_add_string(msg, expr);
		dmessage_add_int(msg, -(1 + frame));
	}
	msg = inferior_request(obj, msg);
	dvalue = dvalue_dup(dmessage_get_dvalue(msg, 1));
	*out_is_error = dmessage_get_int(msg, 0) != 0;
	dmessage_free(msg);
	return dvalue;
}

bool
inferior_pause(inferior_t* obj)
{
	ki_message_t* msg;

	msg = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(msg, REQ_PAUSE);
	if (!(msg = inferior_request(obj, msg)))
		return false;
	return true;
}

ki_message_t*
inferior_request(inferior_t* obj, ki_message_t* msg)
{
	ki_message_t* response = NULL;

	if (!(dmessage_send(msg, obj->socket)))
		goto lost_connection;
	do {
		dmessage_free(response);
		if (!(response = dmessage_recv(obj->socket)))
			goto lost_connection;
		if (dmessage_tag(response) == DMESSAGE_NFY)
			handle_notify(obj, response);
	} while (dmessage_tag(response) == DMESSAGE_NFY);
	dmessage_free(msg);
	return response;

lost_connection:
	printf("debugger lost connection with the target.\n");
	dmessage_free(msg);
	obj->is_detached = true;
	return NULL;
}

bool
inferior_resume(inferior_t* obj, resume_op_t op)
{
	ki_message_t* msg;

	msg = dmessage_new(DMESSAGE_REQ);
	dmessage_add_int(msg,
		op == OP_STEP_OVER ? REQ_STEPOVER
		: op == OP_STEP_IN ? REQ_STEPINTO
		: op == OP_STEP_OUT ? REQ_STEPOUT
		: REQ_RESUME);
	if (!(msg = inferior_request(obj, msg)))
		return false;
	obj->frame_index = 0;
	obj->is_paused = false;
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
handle_notify(inferior_t* obj, const ki_message_t* msg)
{
	const char*   heading;
	enum print_op print_op;
	int           status_type;

	switch (dmessage_tag(msg)) {
	case DMESSAGE_NFY:
		switch (dmessage_get_int(msg, 0)) {
		case NFY_APPNOTIFY:
			switch (dmessage_get_int(msg, 1)) {
			case APPNFY_DEBUG_PRINT:
				print_op = (enum print_op)dmessage_get_int(msg, 2);
				heading = print_op == PRINT_ASSERT ? "ASSERT"
					: print_op == PRINT_DEBUG ? "debug"
					: print_op == PRINT_ERROR ? "ERROR"
					: print_op == PRINT_INFO ? "info"
					: print_op == PRINT_TRACE ? "trace"
					: print_op == PRINT_WARN ? "warn"
					: "log";
				if (print_op == PRINT_TRACE && !obj->show_trace)
					break;
				if (print_op == PRINT_ASSERT || print_op == PRINT_ERROR)
					printf("\33[31;1m");
				else if (print_op == PRINT_WARN)
					printf("\33[33;1m");
				else
					printf("\33[36m");
				printf("%s: %s\n", heading, dmessage_get_string(msg, 3));
				printf("\33[m");
				break;
			}
			break;
		case NFY_STATUS:
			status_type = dmessage_get_int(msg, 1);
			obj->is_paused = status_type != 0;
			if (!obj->is_paused)
				clear_pause_cache(obj);
			break;
		case NFY_THROW:
			if ((status_type = dmessage_get_int(msg, 1)) == 0)
				break;
			printf("\33[31;1m");
			printf("%s\n", dmessage_get_string(msg, 2));
			printf("    at %s:%d\n",
				dmessage_get_string(msg, 3),
				dmessage_get_int(msg, 4));
			printf("\33[m");
			break;
		case NFY_DETACHING:
			status_type = dmessage_get_int(msg, 1);
			if (status_type == 0)
				printf("\33[36mdebugger disconnected normally.\n");
			else
				printf("\33[31mdebugger disconnected due to an error.\n");
			printf("\33[m");
			obj->is_detached = true;
			return false;
		}
		break;
	}
	return true;
}
