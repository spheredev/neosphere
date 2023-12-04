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

#include "ssj.h"
#include "inferior.h"

#include "backtrace.h"
#include "help.h"
#include "ki.h"
#include "listing.h"
#include "objview.h"
#include "parser.h"
#include "session.h"
#include "sockets.h"

struct source
{
	char*      filename;
	listing_t* listing;
};

struct inferior
{
	unsigned int   id;
	int            num_sources;
	bool           is_detached;
	bool           paused;
	char*          title;
	int            api_version;
	int            api_level;
	char*          author;
	backtrace_t*   calls;
	char*          compiler;
	int            frame_index;
	bool           have_api_info;
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
	char*         engine_name;
	inferior_t*   inferior;
	ki_message_t* reply;
	ki_message_t* request;
	clock_t       timeout;

	if (!(inferior = calloc(1, sizeof(inferior_t))))
		goto on_error;
	printf("connecting to %s:%d... ", hostname, port);
	fflush(stdout);
	inferior->socket = socket_new(1024, true);
	socket_set_no_delay(inferior->socket, true);
	if (!socket_connect(inferior->socket, hostname, port))
		goto on_error;
	timeout = clock() + 5 * CLOCKS_PER_SEC;
	while (!socket_connected(inferior->socket)) {
		if (socket_closed(inferior->socket))
			socket_connect(inferior->socket, hostname, port);
		if (clock() > timeout)
			goto timed_out;
		sockets_update();
	}

	printf("OK.\n");
	inferior->protocol = do_handshake(inferior->socket);
	if (inferior->protocol == 0)
		goto on_error;

	// set watermark (shown on bottom left)
	request = ki_message_new(KI_REQ);
	ki_message_add_int(request, KI_REQ_WATERMARK);
	ki_message_add_string(request, "SSj CLI");
	ki_message_add_int(request, 255);
	ki_message_add_int(request, 224);
	ki_message_add_int(request, 0);
	if (!(reply = inferior_request(inferior, request)))
		goto on_error;
	ki_message_free(reply);

	printf("requesting game information... ");
	request = ki_message_new(KI_REQ);
	ki_message_add_int(request, KI_REQ_GAME_INFO);
	if (!(reply = inferior_request(inferior, request)))
		goto on_error;
	engine_name = strdup(ki_message_string(reply, 0));
	inferior->title = strdup(ki_message_string(reply, 1));
	inferior->author = strdup(ki_message_string(reply, 2));
	if (ki_message_len(reply) >= 8) {
		inferior->api_version = ki_message_int(reply, 6);
		inferior->api_level = ki_message_int(reply, 7);
		inferior->have_api_info = true;
	}
	if (ki_message_len(reply) >= 9) {
		inferior->compiler = strdup(ki_message_string(reply, 8));
	}
	ki_message_free(reply);
	printf("OK.\n");

	printf("   engine:   \33[37;1m%s\33[m\n", engine_name);
	printf("   title:    \33[37;1m%s\33[m\n", inferior->title);
	if (inferior->have_api_info)
		printf("   API:      \33[37;1mSphere v%d level %d\33[m\n", inferior->api_version, inferior->api_level);
	if (inferior->compiler != NULL)
		printf("   compiler: \33[37;1m%s\33[m\n", inferior->compiler);

	free(engine_name);

	inferior->id = s_next_inferior_id++;
	inferior->show_trace = show_trace;
	return inferior;

timed_out:
	printf("\33[31mtimed out!\33[m\n");
	free(inferior);
	return NULL;

on_error:
	free(inferior);
	return NULL;
}

void
inferior_free(inferior_t* it)
{
	int i;

	clear_pause_cache(it);
	for (i = 0; i < it->num_sources; ++i) {
		listing_free(it->sources[i].listing);
		free(it->sources[i].filename);
	}
	socket_close(it->socket);
	free(it->compiler);
	free(it->title);
	free(it->author);
	free(it);
}

bool
inferior_update(inferior_t* it)
{
	ki_message_t* notify = NULL;

	if (it->is_detached)
		return false;

	if (!(notify = ki_message_recv(it->socket)))
		goto detached;
	if (!handle_notify(it, notify))
		goto detached;
	ki_message_free(notify);
	return true;

detached:
	ki_message_free(notify);
	it->is_detached = true;
	return false;
}

bool
inferior_attached(const inferior_t* it)
{
	return !it->is_detached;
}

bool
inferior_running(const inferior_t* it)
{
	return !it->paused && inferior_attached(it);
}

const char*
inferior_author(const inferior_t* it)
{
	return it->author;
}

const char*
inferior_title(const inferior_t* it)
{
	return it->title;
}

const backtrace_t*
inferior_get_calls(inferior_t* it)
{
	char*         call_name;
	const char*   filename;
	const char*   function_name;
	int           line_no;
	int           num_frames;
	ki_message_t* request;

	int i;

	if (it->calls == NULL) {
		request = ki_message_new(KI_REQ);
		ki_message_add_int(request, KI_REQ_INSPECT_STACK);
		if (!(request = inferior_request(it, request)))
			return NULL;
		num_frames = ki_message_len(request) / 4;
		it->calls = backtrace_new();
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
			backtrace_add(it->calls, call_name, filename, line_no);
			free(call_name);
		}
		ki_message_free(request);
	}

	return it->calls;
}

const listing_t*
inferior_get_listing(inferior_t* it, const char* filename)
{
	int            cache_id;
	listing_t*     listing;
	ki_message_t*  request;
	struct source* sources;
	const char*    text;

	int i;

	for (i = 0; i < it->num_sources; ++i) {
		if (strcmp(filename, it->sources[i].filename) == 0)
			return it->sources[i].listing;
	}

	request = ki_message_new(KI_REQ);
	ki_message_add_int(request, KI_REQ_DOWNLOAD);
	ki_message_add_string(request, filename);
	if (!(request = inferior_request(it, request)))
		goto on_error;
	if (ki_message_tag(request) == KI_ERR)
		goto on_error;
	text = ki_message_string(request, 0);
	listing = listing_new(text);

	if (!(sources = realloc(it->sources, (it->num_sources + 1) * sizeof(struct source))))
		goto on_error;
	cache_id = it->num_sources++;
	sources[cache_id].filename = strdup(filename);
	sources[cache_id].listing = listing;
	it->sources = sources;

	return listing;

on_error:
	ki_message_free(request);
	return NULL;
}

objview_t*
inferior_get_object(inferior_t* it, unsigned int handle, bool get_all)
{
	int              attributes;
	unsigned int     flags;
	const ki_atom_t* getter;
	int              index = 0;
	bool             is_accessor;
	const ki_atom_t* key_atom;
	char*            key_string;
	ki_message_t*    request;
	const ki_atom_t* setter;
	const ki_atom_t* value;
	objview_t*       view;

	request = ki_message_new(KI_REQ);
	ki_message_add_int(request, KI_REQ_INSPECT_OBJ);
	ki_message_add_ref(request, handle);
	ki_message_add_int(request, 0);
	ki_message_add_int(request, INT_MAX);
	if (!(request = inferior_request(it, request)))
		return NULL;
	view = objview_new();
	while (index < ki_message_len(request)) {
		key_atom = ki_message_atom(request, index++);
		attributes = ki_message_int(request, index++);
		is_accessor = (attributes & 0x008) != 0;
		if (attributes & 0x100) {
			index += is_accessor ? 2 : 1;
			continue;
		}
		if (ki_atom_type(key_atom) == KI_STRING)
			key_string = strdup(ki_atom_string(key_atom));
		else
			key_string = strnewf("%d", ki_atom_int(key_atom));
		flags = 0x0;
		if (attributes & 0x01)
			flags |= PROP_WRITABLE;
		if (attributes & 0x02)
			flags |= PROP_ENUMERABLE;
		if (attributes & 0x04)
			flags |= PROP_CONFIGURABLE;
		if (is_accessor) {
			getter = ki_message_atom(request, index++);
			setter = ki_message_atom(request, index++);
			objview_add_accessor(view, key_string, getter, setter, flags);
		}
		else {
			value = ki_message_atom(request, index++);
			objview_add_value(view, key_string, "property", value, flags);
		}
		free(key_string);
	}
	ki_message_free(request);
	return view;
}

objview_t*
inferior_get_vars(inferior_t* it, int frame)
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
	ki_message_add_int(msg, frame);
	if (!(msg = inferior_request(it, msg)))
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
inferior_add_breakpoint(inferior_t* it, const char* filename, int linenum)
{
	int        handle;
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_ADD_BREAK);
	ki_message_add_string(msg, filename);
	ki_message_add_int(msg, linenum);
	if (!(msg = inferior_request(it, msg)))
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
inferior_clear_breakpoint(inferior_t* it, int handle)
{
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_DEL_BREAK);
	ki_message_add_int(msg, handle);
	if (!(msg = inferior_request(it, msg)))
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
inferior_detach(inferior_t* it)
{
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_DETACH);
	if (!(msg = inferior_request(it, msg)))
		return;
	ki_message_free(msg);
	while (inferior_attached(it))
		inferior_update(it);
}

ki_atom_t*
inferior_eval(inferior_t* it, const char* expr, int frame, bool* out_is_error)
{
	ki_atom_t*  dvalue = NULL;
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_EVAL);
	ki_message_add_int(msg, frame);
	ki_message_add_string(msg, expr);
	msg = inferior_request(it, msg);
	dvalue = ki_atom_dup(ki_message_atom(msg, 1));
	*out_is_error = !ki_message_bool(msg, 0);
	ki_message_free(msg);
	return dvalue;
}

bool
inferior_pause(inferior_t* it)
{
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg, KI_REQ_PAUSE);
	if (!(msg = inferior_request(it, msg)))
		return false;
	return true;
}

ki_message_t*
inferior_request(inferior_t* it, ki_message_t* msg)
{
	ki_message_t* response = NULL;

	if (!(ki_message_send(msg, it->socket)))
		goto lost_connection;
	do {
		ki_message_free(response);
		if (!(response = ki_message_recv(it->socket)))
			goto lost_connection;
		if (ki_message_tag(response) == KI_NFY)
			handle_notify(it, response);
	} while (ki_message_tag(response) == KI_NFY);
	ki_message_free(msg);
	return response;

lost_connection:
	printf("\33[31;1mSSj lost communication with the target.\33[m\n");
	ki_message_free(msg);
	it->is_detached = true;
	return NULL;
}

bool
inferior_resume(inferior_t* it, resume_op_t op)
{
	ki_message_t* msg;

	msg = ki_message_new(KI_REQ);
	ki_message_add_int(msg,
		op == OP_STEP_OVER ? KI_REQ_STEP_OVER
		: op == OP_STEP_IN ? KI_REQ_STEP_IN
		: op == OP_STEP_OUT ? KI_REQ_STEP_OUT
		: KI_REQ_RESUME);
	if (!(msg = inferior_request(it, msg)))
		return false;
	it->frame_index = 0;
	it->paused = false;
	while (inferior_running(it))
		inferior_update(it);
	return true;
}

static void
clear_pause_cache(inferior_t* inferior)
{
	backtrace_free(inferior->calls);
	inferior->calls = NULL;
}

static int
do_handshake(socket_t* socket)
{
	static char handshake[128];

	ptrdiff_t idx;
	char*     next_token = NULL;
	char*     token;
	int       version;

	printf("establishing communication... ");
	idx = 0;
	do {
		if (idx >= 127)  // probably not an SSj/Ki handshake
			goto on_error;
		socket_read(socket, &handshake[idx], 1);
	} while (handshake[idx++] != '\n');
	handshake[idx - 1] = '\0';  // remove the newline

	// parse handshake line
	if (!(token = strtok_r(handshake, " ", &next_token)))
		goto on_error;
	if (strcmp(token, "SSj/Ki") != 0)
		goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	sscanf(token, "v%d", &version);
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	printf("OK.\n");

	return version;

on_error:
	printf("\33[31merror!\33[m\n");
	return 0;
}

static bool
handle_notify(inferior_t* inferior, const ki_message_t* msg)
{
	const char*    heading;
	enum ki_log_op log_op;
	int            status_type;

	switch (ki_message_tag(msg)) {
	case KI_NFY:
		switch (ki_message_int(msg, 0)) {
		case KI_NFY_DETACH:
			status_type = ki_message_int(msg, 1);
			if (status_type == 0)
				printf("\33[37;1mSSj/Ki debug session disconnected normally.\n");
			else
				printf("\33[31;1ma communication error occurred while debugging.\n");
			printf("\33[m");
			inferior->is_detached = true;
			return false;
		case KI_NFY_LOG:
			log_op = (enum ki_log_op)ki_message_int(msg, 1);
			if (log_op == KI_LOG_TRACE && !inferior->show_trace)
				break;
			heading = log_op == KI_LOG_TRACE ? "\33[37;1mtrace\33[m"
				: log_op == KI_LOG_WARN ? "\33[31;1mwarn\33[m"
				: log_op == KI_LOG_ERROR ? "\33[33;1merror\33[m"
				: "\33[36;1mlog\33[m";
			printf("%s: %s\n", heading, ki_message_string(msg, 2));
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
			printf("UNCAUGHT EXCEPTION: ");
			printf("\33[m");
			printf("%s\n", ki_message_string(msg, 2));
			break;
		}
		break;
	}
	return true;
}
