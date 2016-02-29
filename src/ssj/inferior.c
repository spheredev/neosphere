#include "ssj.h"
#include "inferior.h"

#include "backtrace.h"
#include "help.h"
#include "message.h"
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
	uint8_t        ptr_size;
	socket_t*      socket;
	struct source* sources;
};

static void clear_pause_cache (inferior_t* obj);
static bool do_handshake      (socket_t* socket);
static bool handle_notify     (inferior_t* obj, const message_t* msg);

static unsigned int s_next_id_no = 1;

void
inferiors_init(void)
{
	sockets_init();
}

void
inferiors_deinit(void)
{
	sockets_deinit();
}

inferior_t*
inferior_new(const char* hostname, int port)
{
	inferior_t* obj;
	message_t*  req;
	message_t*  rep;

	obj = calloc(1, sizeof(inferior_t));
	printf("connecting to %s:%d... ", hostname, port);
	fflush(stdout);
	if (!(obj->socket = socket_connect(hostname, port, 30.0)))
		goto on_error;
	printf("OK.\n");
	if (!do_handshake(obj->socket))
		goto on_error;

	printf("querying target... ");
	req = message_new(MESSAGE_REQ);
	message_add_int(req, REQ_APP_REQUEST);
	message_add_int(req, APPREQ_GAME_INFO);
	rep = inferior_request(obj, req);
	obj->title = strdup(message_get_string(rep, 0));
	obj->author = strdup(message_get_string(rep, 1));
	message_free(rep);
	printf("OK.\n");

	printf("    game: %s\n", obj->title);
	printf("    author: %s\n", obj->author);

	obj->id_no = s_next_id_no++;
	return obj;

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
	bool       is_active = true;
	message_t* msg = NULL;

	if (obj->is_detached)
		return false;
	
	if (!(msg = message_recv(obj->socket)))
		goto detached;
	if (!handle_notify(obj, msg))
		goto detached;
	message_free(msg);
	return true;

detached:
	message_free(msg);
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
	char*       call_name;
	const char* filename;
	const char* function_name;
	int         line_no;
	message_t*  msg;
	int         num_frames;

	int i;

	if (obj->calls == NULL) {
		msg = message_new(MESSAGE_REQ);
		message_add_int(msg, REQ_GET_CALLSTACK);
		if (!(msg = inferior_request(obj, msg)))
			return NULL;
		num_frames = message_len(msg) / 4;
		obj->calls = backtrace_new();
		for (i = 0; i < num_frames; ++i) {
			function_name = message_get_string(msg, i * 4 + 1);
			if (strcmp(function_name, "") == 0)
				call_name = strdup("[anon]");
			else
				call_name = strnewf("%s()", function_name);
			filename = message_get_string(msg, i * 4);
			line_no = message_get_int(msg, i * 4 + 2);
			backtrace_add(obj->calls, call_name, filename, line_no);
		}
		message_free(msg);
	}

	return obj->calls;
}

objview_t*
inferior_get_object(inferior_t* obj, remote_ptr_t heapptr, bool get_all)
{
	unsigned int    flags;
	const dvalue_t* getter;
	bool            is_accessor;
	int             index = 0;
	char*           key_string;
	int             prop_flags;
	const dvalue_t* prop_key;
	message_t*      msg;
	const dvalue_t* setter;
	const dvalue_t* value;
	objview_t*      view;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_INSPECT_PROPS);
	message_add_heapptr(msg, heapptr);
	message_add_int(msg, 0);
	message_add_int(msg, INT_MAX);
	if (!(msg = inferior_request(obj, msg)))
		return NULL;
	view = objview_new();
	while (index < message_len(msg)) {
		prop_flags = message_get_int(msg, index++);
		prop_key = message_get_dvalue(msg, index++);
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
			getter = message_get_dvalue(msg, index++);
			setter = message_get_dvalue(msg, index++);
			objview_add_accessor(view, key_string, getter, setter, flags);
		}
		else {
			value = message_get_dvalue(msg, index++);
			if (dvalue_tag(value) != DVALUE_UNUSED)
				objview_add_value(view, key_string, value, flags);
		}
		free(key_string);
	}
	message_free(msg);
	return view;
}

const source_t*
inferior_get_source(inferior_t* obj, const char* filename)
{
	int         cache_id;
	message_t*  msg;
	source_t*   source;
	const char* text;

	int i;

	for (i = 0; i < obj->num_sources; ++i) {
		if (strcmp(filename, obj->sources[i].filename) == 0)
			return obj->sources[i].source;
	}

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_APP_REQUEST);
	message_add_int(msg, APPREQ_SOURCE);
	message_add_string(msg, filename);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (message_tag(msg) == MESSAGE_ERR)
		goto on_error;
	text = message_get_string(msg, 0);
	source = source_new(text);

	cache_id = obj->num_sources++;
	obj->sources = realloc(obj->sources, obj->num_sources * sizeof(struct source));
	obj->sources[cache_id].filename = strdup(filename);
	obj->sources[cache_id].source = source;

	return source;

on_error:
	message_free(msg);
	return NULL;
}

objview_t*
inferior_get_vars(inferior_t* obj, int frame)
{
	const char*     name;
	message_t*      msg;
	int             num_vars;
	const dvalue_t* value;
	objview_t*      vars;

	int i;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_GET_LOCALS);
	message_add_int(msg, -frame - 1);
	if (!(msg = inferior_request(obj, msg)))
		return NULL;
	vars = objview_new();
	num_vars = message_len(msg) / 2;
	for (i = 0; i < num_vars; ++i) {
		name = message_get_string(msg, i * 2 + 0);
		value = message_get_dvalue(msg, i * 2 + 1);
		objview_add_value(vars, name, value, 0x0);
	}
	return vars;
}

int
inferior_add_breakpoint(inferior_t* obj, const char* filename, int linenum)
{
	int        handle;
	message_t* msg;
	
	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_ADD_BREAK);
	message_add_string(msg, filename);
	message_add_int(msg, linenum);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (message_tag(msg) == MESSAGE_ERR)
		goto on_error;
	handle = message_get_int(msg, 0);
	message_free(msg);
	return handle;

on_error:
	message_free(msg);
	return -1;
}

bool
inferior_clear_breakpoint(inferior_t* obj, int handle)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_DEL_BREAK);
	message_add_int(msg, handle);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (message_tag(msg) == MESSAGE_ERR)
		goto on_error;
	message_free(msg);
	return true;

on_error:
	message_free(msg);
	return false;
}

void
inferior_detach(inferior_t* obj)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_DETACH);
	if (!(msg = inferior_request(obj, msg)))
		return;
	message_free(msg);
	while (inferior_attached(obj))
		inferior_update(obj);
}

dvalue_t*
inferior_eval(inferior_t* obj, const char* expr, int frame, bool* out_is_error)
{
	dvalue_t*  dvalue = NULL;
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_EVAL);
	message_add_string(msg, expr);
	message_add_int(msg, -(1 + frame));
	msg = inferior_request(obj, msg);
	dvalue = dvalue_dup(message_get_dvalue(msg, 1));
	*out_is_error = message_get_int(msg, 0) != 0;
	message_free(msg);
	return dvalue;
}

bool
inferior_pause(inferior_t* obj)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_PAUSE);
	if (!(msg = inferior_request(obj, msg)))
		return false;
	return true;
}

message_t*
inferior_request(inferior_t* obj, message_t* msg)
{
	message_t* response = NULL;

	if (!(message_send(msg, obj->socket)))
		goto lost_connection;
	do {
		message_free(response);
		if (!(response = message_recv(obj->socket)))
			goto lost_connection;
		if (message_tag(response) == MESSAGE_NFY)
			handle_notify(obj, response);
	} while (message_tag(response) == MESSAGE_NFY);
	message_free(msg);
	return response;

lost_connection:
	printf("inferior lost connection with the target.\n");
	message_free(msg);
	obj->is_detached = true;
	return NULL;
}

bool
inferior_resume(inferior_t* obj, resume_op_t op)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg,
		op == OP_STEP_OVER ? REQ_STEP_OVER
		: op == OP_STEP_IN ? REQ_STEP_INTO
		: op == OP_STEP_OUT ? REQ_STEP_OUT
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

static bool
do_handshake(socket_t* socket)
{
	static char handshake[128];

	ptrdiff_t    idx;
	char*        next_token;
	char*        token;

	printf("verifying... ");
	idx = 0;
	do {
		if (idx >= 127)  // probably not a Duktape handshake
			goto on_error;
		socket_recv(socket, &handshake[idx], 1);
	} while (handshake[idx++] != '\n');
	handshake[idx - 1] = '\0';  // remove the newline

	// parse handshake line
	if (!(token = strtok_r(handshake, " ", &next_token)))
		goto on_error;
	if (atoi(token) != 1) goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	printf("OK.\n");

	return true;

on_error:
	printf("\33[31;1merror!\33[m\n");
	return false;
}

static bool
handle_notify(inferior_t* obj, const message_t* msg)
{
	int status_type;

	switch (message_tag(msg)) {
	case MESSAGE_NFY:
		switch (message_get_int(msg, 0)) {
		case NFY_APP_NOTIFY:
			switch (message_get_int(msg, 1)) {
			case APPNFY_DEBUG_PRINT:
				printf("t: %s", message_get_string(msg, 2));
				break;
			}
			break;
		case NFY_STATUS:
			status_type = message_get_int(msg, 1);
			obj->is_paused = status_type != 0;
			if (!obj->is_paused)
				clear_pause_cache(obj);
			break;
		case NFY_PRINT:
			printf("p: %s", message_get_string(msg, 1));
			break;
		case NFY_ALERT:
			printf("a: %s", message_get_string(msg, 1));
			break;
		case NFY_LOG:
			printf("l: %s", message_get_string(msg, 1));
			break;
		case NFY_THROW:
			if ((status_type = message_get_int(msg, 1)) == 0)
				break;
			printf("uncaught error! - %s\n", message_get_string(msg, 2));
			printf("    at: %s:%d\n",
				message_get_string(msg, 3),
				message_get_int(msg, 4));
			break;
		case NFY_DETACHING:
			status_type = message_get_int(msg, 1);
			if (status_type == 0)
				printf("inferior disconnected normally.\n");
			else
				printf("inferior disconnected due to an error.\n");
			obj->is_detached = true;
			return false;
		}
		break;
	}
	return true;
}
