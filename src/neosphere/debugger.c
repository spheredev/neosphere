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
#include "debugger.h"

#include "audio.h"
#include "jsal.h"
#include "ki.h"
#include "sockets.h"
#include "source_map.h"

static int const TCP_DEBUG_PORT = 1208;

static js_step_t  on_breakpoint_hit  (void);
static void       on_throw_exception (void);
static ki_atom_t* atom_from_value    (int stack_index);
static bool       do_attach_debugger (void);
static void       do_detach_debugger (bool is_shutdown);
static bool       process_message    (js_step_t* out_step);

static ssj_mode_t s_attach_mode;
static js_step_t  s_auto_step_op;
static color_t    s_banner_color;
static char*      s_banner_text;
static js_ref_t*  s_cell_data = NULL;
static char*      s_compiler = NULL;
static bool       s_is_attached = false;
static bool       s_needs_attachment;
static server_t*  s_server;
static socket_t*  s_socket = NULL;

void
debugger_init(ssj_mode_t attach_mode, bool allow_remote)
{
	const char* hostname;
	size_t      json_size;
	char*       json_text;
	js_ref_t*   ref;
	const char* source_name;
	const char* source_text;
	const char* url;

	s_attach_mode = attach_mode;

	if (attach_mode != SSJ_OFF) {
		jsal_debug_on_throw(on_throw_exception);
		jsal_debug_init(on_breakpoint_hit);
	}

	s_banner_text = strdup("debugger attached");
	s_banner_color = mk_color(192, 192, 192, 255);

	if ((json_text = game_read_file(g_game, "@/artifacts.json", &json_size))) {
		jsal_push_lstring(json_text, json_size);
		if (jsal_try_parse(-1) && jsal_is_object(-1))
			s_cell_data = jsal_ref(-1);
		jsal_pop(1);
	}

	source_map_init();

	// load the source map, if one is available
	if (s_cell_data != NULL) {
		jsal_push_ref_weak(s_cell_data);
		if (jsal_get_prop_string(-1, "compiler"))
			s_compiler = strdup(jsal_get_string(-1));
		if (jsal_get_prop_string(-2, "fileMap") && jsal_is_object(-1)) {
			jsal_push_new_iterator(-1);
			while (jsal_next(-1)) {
				url = jsal_get_string(-1);
				jsal_get_prop_string(-3, url);
				source_name = jsal_require_string(-1);
				source_map_add_alias(url, source_name);
				jsal_pop(2);
			}
			jsal_pop(1);
		}
		if (jsal_get_prop_string(-3, "sources") && jsal_is_object(-1)) {
			jsal_push_new_iterator(-1);
			while (jsal_next(-1)) {
				url = jsal_get_string(-1);
				jsal_get_prop_string(-3, url);
				source_text = jsal_require_string(-1);
				source_map_add_source(url, source_text);
				jsal_pop(2);
			}
			jsal_pop(1);
		}
		if (jsal_get_prop_string(-4, "sourceMaps") && jsal_is_object(-1)) {
			jsal_push_new_iterator(-1);
			while (jsal_next(-1)) {
				url = jsal_get_string(-1);
				jsal_get_prop_string(-3, url);
				ref = jsal_ref(-1);
				source_map_add_map(url, ref);
				jsal_pop(2);
			}
			jsal_pop(1);
		}
		jsal_pop(5);
	}

	// listen for SSj connection on TCP port 1208. the listening socket will remain active
	// for the duration of the session, allowing a debugger to be attached at any time.
	if (attach_mode != SSJ_OFF) {
		console_log(1, "listening for SSj on TCP port %d", TCP_DEBUG_PORT);
		hostname = allow_remote ? NULL : "127.0.0.1";
		s_server = server_new(hostname, TCP_DEBUG_PORT, 1024, 1, true);
		if (s_server != NULL) {
			server_set_no_delay(s_server, true);
		}
		else {
			debugger_log("unable to bind SSj port, debugger disabled", KI_LOG_WARN, true);
			s_attach_mode = SSJ_OFF;
		}
	}
	else {
		s_server = NULL;
	}

	// if the engine was started in debug mode, wait for a debugger to connect before
	// beginning execution.
	s_needs_attachment = s_attach_mode == SSJ_ACTIVE;
	if (s_needs_attachment && !do_attach_debugger())
		sphere_abort(NULL);  // no attachment, exit
	
	s_auto_step_op = JS_STEP_IN;
}

void
debugger_uninit()
{
	if (s_attach_mode != SSJ_OFF) {
		do_detach_debugger(true);
		jsal_debug_uninit();
		server_unref(s_server);
	}

	jsal_unref(s_cell_data);
	free(s_compiler);
	free(s_banner_text);

	source_map_uninit();
}

void
debugger_update(void)
{
	socket_t*     client;
	char*         handshake;
	js_step_t     step_op;

	if (s_attach_mode == SSJ_OFF)
		return;

	if (s_is_attached) {
		if (s_socket != NULL && socket_closed(s_socket)) {
			socket_unref(s_socket);
			s_socket = NULL;
		}
		if (s_socket == NULL)
			do_detach_debugger(false);
	}

	// watch for incoming SSj client and attach debugger
	if ((client = server_accept(s_server))) {
		if (s_socket != NULL) {
			console_log(2, "rejected connection from %s, debugger attached",
				socket_hostname(client));
			socket_unref(client);
		}
		else {
			screen_set_fullscreen(g_screen, false);
			jsal_debug_breakpoint_inject();
			console_log(0, "connected to debugger at %s", socket_hostname(client));
			handshake = strnewf("SSj/Ki v%d %s %s\n", KI_VERSION, SPHERE_ENGINE_NAME, SPHERE_VERSION);
			socket_write(client, handshake, (int)strlen(handshake));
			free(handshake);
			s_socket = client;
			s_is_attached = true;
		}
	}

	// process any incoming SSj requests
	if (s_socket == NULL || socket_bytes_avail(s_socket) == 0)
		return;
	process_message(&step_op);
}

bool
debugger_attached(void)
{
	return s_is_attached;
}

color_t
debugger_color(void)
{
	return s_banner_color;
}

const char*
debugger_compiler(void)
{
	return s_compiler;
}

const char*
debugger_name(void)
{
	return s_banner_text;
}

void
debugger_log(const char* text, ki_log_op_t op, bool use_console)
{
	const char*   heading;
	ki_message_t* notify;

	if (use_console) {
		heading = op == KI_LOG_TRACE ? "\33[30;1mtrace\33[m: "
			: op == KI_LOG_WARN ? "\33[31;1mwarn\33[m: "
			: op == KI_LOG_ERROR ? "\33[33;1merror\33[m: "
			: "";
		printf("%s%s\33[m\n", heading, text);
	}
	if (s_socket != NULL) {
		notify = ki_message_new(KI_NFY);
		ki_message_add_int(notify, KI_NFY_LOG);
		ki_message_add_int(notify, op);
		ki_message_add_string(notify, text);
		ki_message_send(notify, s_socket);
		ki_message_free(notify);
	}
}

static js_step_t
on_breakpoint_hit(void)
{
	int           column;
	const char*   filename;
	int           line;
	mapping_t     mapping;
	ki_message_t* message;
	js_step_t     step_op;

	if (s_socket == NULL)
		return JS_STEP_CONTINUE;

	filename = jsal_get_string(0);
	line = jsal_get_int(1);
	column = jsal_get_int(2);
	mapping = source_map_lookup(filename, line, column);
	
	if (strncmp(mapping.filename, "#/", 2) == 0)
		return s_auto_step_op;

	audio_suspend();

	message = ki_message_new(KI_NFY);
	ki_message_add_int(message, KI_NFY_PAUSE);
	ki_message_add_string(message, mapping.filename);
	ki_message_add_string(message, "");
	ki_message_add_int(message, mapping.line + 1);
	ki_message_add_int(message, mapping.column + 1);
	ki_message_send(message, s_socket);
	ki_message_free(message);

	while (!process_message(&step_op));

	if (s_socket != NULL) {
		message = ki_message_new(KI_NFY);
		ki_message_add_int(message, KI_NFY_RESUME);
		ki_message_send(message, s_socket);
		ki_message_free(message);
	}

	audio_resume();

	s_auto_step_op = step_op == JS_STEP_CONTINUE ? JS_STEP_IN : step_op;
	return step_op;
}

static void
on_throw_exception(void)
{
	int           column;
	const char*   error_text;
	const char*   filename;
	int           line;
	mapping_t     mapping;
	ki_message_t* message;

	if (s_socket == NULL)
		return;

	error_text = jsal_get_string(0);
	filename = jsal_get_string(1);
	line = jsal_get_int(2);
	column = jsal_get_int(3);
	mapping = source_map_lookup(filename, line, column);

	message = ki_message_new(KI_NFY);
	ki_message_add_int(message, KI_NFY_THROW);
	ki_message_add_int(message, 1);
	ki_message_add_string(message, error_text);
	ki_message_add_string(message, mapping.filename);
	ki_message_add_int(message, mapping.line + 1);
	ki_message_add_int(message, mapping.column + 1);
	ki_message_send(message, s_socket);
	ki_message_free(message);
}

static ki_atom_t*
atom_from_value(int stack_index)
{
	if (jsal_is_boolean(stack_index))
		return ki_atom_new_bool(jsal_get_boolean(stack_index));
	else if (jsal_is_number(stack_index))
		return ki_atom_new_number(jsal_get_number(stack_index));
	else if (jsal_is_null(stack_index))
		return ki_atom_new(KI_NULL);
	else if (jsal_is_undefined(stack_index))
		return ki_atom_new(KI_UNDEFINED);
	else
		return ki_atom_new_string(jsal_to_string(stack_index));
}

static bool
do_attach_debugger(void)
{
	double timeout;

	console_log(0, "waiting for SSj debugger to connect...");
	fflush(stdout);
	timeout = al_get_time() + 30.0;
	while (s_socket == NULL && al_get_time() < timeout) {
		debugger_update();
		sphere_sleep(0.05);
	}
	if (s_socket == NULL)  // did we time out?
		console_warn(0, "timed out waiting for SSj");
	return s_socket != NULL;
}

static void
do_detach_debugger(bool is_shutdown)
{
	ki_message_t* notify;

	if (!s_is_attached)
		return;

	// detach the debugger
	console_log(1, "detaching SSj debug session");
	s_is_attached = false;
	if (s_socket != NULL) {
		notify = ki_message_new(KI_NFY);
		ki_message_add_int(notify, KI_NFY_DETACH);
		ki_message_add_int(notify, 0);
		ki_message_send(notify, s_socket);
		ki_message_free(notify);
		socket_close(s_socket);
		while (socket_connected(s_socket))
			sphere_sleep(0.05);
	}
	socket_unref(s_socket);
	s_socket = NULL;
	if (s_needs_attachment && !is_shutdown)
		sphere_abort(NULL);  // clean detach, exit
}

static bool
process_message(js_step_t* out_step)
{
	unsigned int  breakpoint_id;
	int           call_index;
	int           column;
	const char*   compiler;
	char*         engine_name;
	const char*   eval_code;
	bool          eval_errored;
	char*         file_data;
	size_t        file_size;
	const char*   filename;
	unsigned int  handle;
	unsigned int  line;
	mapping_t     mapping;
	ki_message_t* reply;
	ki_message_t* request = NULL;
	size2_t       resolution;
	bool          resuming = false;
	const char*   source_text;

	int i;

	if (!s_is_attached) {
		*out_step = JS_STEP_CONTINUE;
		return false;  // TODO: should this be true?
	}

	if (!(request = ki_message_recv(s_socket)))
		goto on_error;
	if (ki_message_tag(request) != KI_REQ)
		goto on_error;
	reply = ki_message_new(KI_REP);
	switch (ki_message_int(request, 0)) {
	case KI_REQ_ADD_BREAK:
		filename = ki_message_string(request, 1);
		line = ki_message_int(request, 2) - 1;
		mapping = source_map_reverse(filename, line, 0);
		breakpoint_id = jsal_debug_breakpoint_add(mapping.filename, mapping.line, mapping.column);
		ki_message_add_int(reply, breakpoint_id);
		break;
	case KI_REQ_DEL_BREAK:
		breakpoint_id = ki_message_int(request, 1);
		jsal_debug_breakpoint_remove(breakpoint_id);
		break;
	case KI_REQ_DETACH:
		ki_message_send(reply, s_socket);
		ki_message_free(reply);
		ki_message_free(request);
		do_detach_debugger(false);
		*out_step = JS_STEP_CONTINUE;
		return true;
	case KI_REQ_DOWNLOAD:
		filename = ki_message_string(request, 1);

		// first check if the requested filename exists in the source map
		if ((source_text = source_map_source_text(filename))) {
			ki_message_add_string(reply, source_text);
			goto finished;
		}

		// not present in source map, try loading from the file system
		filename = source_map_url_of(filename);
		if ((file_data = game_read_file(g_game, filename, &file_size))) {
			ki_message_add_string(reply, file_data);
			free(file_data);
		}
		else {
			ki_message_free(reply);
			reply = ki_message_new(KI_ERR);
			ki_message_add_int(reply, 4);
			ki_message_add_string(reply, "no source code available");
		}
		break;
	case KI_REQ_EVAL:
		call_index = ki_message_int(request, 1);
		eval_code = ki_message_string(request, 2);
		jsal_debug_inspect_eval(call_index, eval_code, &eval_errored);
		ki_message_add_bool(reply, !eval_errored);
		if (jsal_get_uint(-1) != 0)
			ki_message_add_ref(reply, jsal_get_uint(-1));
		else
			ki_message_add_atom(reply, atom_from_value(-2));
		ki_message_add_string(reply, jsal_get_string(-3));
		jsal_pop(3);
		break;
	case KI_REQ_GAME_INFO:
		engine_name = strnewf("%s %s", SPHERE_ENGINE_NAME, SPHERE_VERSION);
		resolution = game_resolution(g_game);
		compiler = debugger_compiler();
		ki_message_add_string(reply, engine_name);
		ki_message_add_string(reply, game_name(g_game));
		ki_message_add_string(reply, game_author(g_game));
		ki_message_add_string(reply, game_summary(g_game));
		ki_message_add_int(reply, resolution.width);
		ki_message_add_int(reply, resolution.height);
		ki_message_add_int(reply, game_version(g_game));
		ki_message_add_int(reply, game_api_level(g_game));
		if (compiler != NULL)
			ki_message_add_string(reply, compiler);
		free(engine_name);
		break;
	case KI_REQ_INSPECT_BREAKS:
		i = 0;
		while (jsal_debug_inspect_breakpoint(i++)) {
			filename = jsal_get_string(-3);
			line = jsal_get_int(-2);
			column = jsal_get_int(-1);
			mapping = source_map_lookup(filename, line, column);
			ki_message_add_string(reply, mapping.filename);
			ki_message_add_int(reply, mapping.line + 1);
			ki_message_add_int(reply, mapping.column + 1);
			jsal_pop(3);
		}
		break;
	case KI_REQ_INSPECT_LOCALS:
		call_index = ki_message_int(request, 1);
		i = 0;
		while (jsal_debug_inspect_var(call_index, i++)) {
			ki_message_add_string(reply, jsal_get_string(-4));
			ki_message_add_string(reply, jsal_get_string(-3));
			if (jsal_get_uint(-1) != 0)
				ki_message_add_ref(reply, jsal_get_uint(-1));
			else
				ki_message_add_atom(reply, atom_from_value(-2));
			jsal_pop(3);
		}
		break;
	case KI_REQ_INSPECT_OBJ:
		handle = ki_message_handle(request, 1);
		i = 0;
		while (jsal_debug_inspect_object(handle, i++)) {
			ki_message_add_string(reply, jsal_get_string(-3));
			ki_message_add_int(reply, KI_ATTR_NONE);
			if (jsal_get_uint(-1) != 0)
				ki_message_add_ref(reply, jsal_get_uint(-1));
			else
				ki_message_add_atom(reply, atom_from_value(-2));
			jsal_pop(3);
		}
		break;
	case KI_REQ_INSPECT_STACK:
		i = 0;
		while (jsal_debug_inspect_call(i++)) {
			filename = jsal_get_string(-4);
			line = jsal_get_int(-2);
			column = jsal_get_int(-1);
			mapping = source_map_lookup(filename, line, column);
			ki_message_add_string(reply, mapping.filename);
			ki_message_add_string(reply, jsal_get_string(-3));
			ki_message_add_int(reply, mapping.line + 1);
			ki_message_add_int(reply, mapping.column + 1);
			jsal_pop(4);
		}
		break;
	case KI_REQ_PAUSE:
		jsal_debug_breakpoint_inject();
		break;
	case KI_REQ_RESUME:
		*out_step = JS_STEP_CONTINUE;
		resuming = true;
		break;
	case KI_REQ_STEP_IN:
		*out_step = JS_STEP_IN;
		resuming = true;
		break;
	case KI_REQ_STEP_OUT:
		*out_step = JS_STEP_OUT;
		resuming = true;
		break;
	case KI_REQ_STEP_OVER:
		*out_step = JS_STEP_OVER;
		resuming = true;
		break;
	case KI_REQ_WATERMARK:
		s_banner_text = strdup(ki_message_string(request, 1));
		s_banner_color = mk_color(
			ki_message_int(request, 2),
			ki_message_int(request, 3),
			ki_message_int(request, 4),
			255);
		break;
	}

finished:
	ki_message_send(reply, s_socket);
	ki_message_free(reply);
	ki_message_free(request);
	return resuming;

on_error:
	ki_message_free(request);
	socket_unref(s_socket);
	s_socket = NULL;
	*out_step = JS_STEP_CONTINUE;
	return true;
}
