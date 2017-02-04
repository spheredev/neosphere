#include "ssj.h"
#include "inferior.h"

#include "backtrace.h"
#include "dmessage.h"
#include "help.h"
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
static bool handle_notify     (inferior_t* obj, const dmessage_t* msg);

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
inferior_new(const char* hostname, int port, bool show_trace)
{
	inferior_t* obj;
	dmessage_t*  req;
	dmessage_t*  rep;

	obj = calloc(1, sizeof(inferior_t));
	printf("connecting to %s:%d... ", hostname, port);
	fflush(stdout);
	if (!(obj->socket = socket_connect(hostname, port, 30.0)))
		goto on_error;
	printf("OK.\n");
	obj->protocol = do_handshake(obj->socket);
	if (obj->protocol == 0)
		goto on_error;

	// set watermark (shown on bottom left)
	req = dmessage_new(MESSAGE_REQ);
	dmessage_add_int(req, REQ_APPREQUEST);
	dmessage_add_int(req, APPREQ_WATERMARK);
	dmessage_add_string(req, "ssj");
	dmessage_add_int(req, 255);
	dmessage_add_int(req, 224);
	dmessage_add_int(req, 0);
	rep = inferior_request(obj, req);
	dmessage_free(rep);

	printf("querying target... ");
	req = dmessage_new(MESSAGE_REQ);
	dmessage_add_int(req, REQ_APPREQUEST);
	dmessage_add_int(req, APPREQ_GAME_INFO);
	rep = inferior_request(obj, req);
	obj->title = strdup(dmessage_get_string(rep, 0));
	obj->author = strdup(dmessage_get_string(rep, 1));
	dmessage_free(rep);
	printf("OK.\n");

	printf("    game: %s\n", obj->title);
	printf("    author: %s\n", obj->author);

	obj->id_no = s_next_id_no++;
	obj->show_trace = show_trace;
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
	dmessage_t* msg = NULL;

	if (obj->is_detached)
		return false;
	
	if (!(msg = dmessage_recv(obj->socket)))
		goto detached;
	if (!handle_notify(obj, msg))
		goto detached;
	dmessage_free(msg);
	return true;

detached:
	dmessage_free(msg);
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
	dmessage_t*  msg;
	int         num_frames;

	int i;

	if (obj->calls == NULL) {
		msg = dmessage_new(MESSAGE_REQ);
		dmessage_add_int(msg, REQ_GETCALLSTACK);
		if (!(msg = inferior_request(obj, msg)))
			return NULL;
		num_frames = dmessage_len(msg) / 4;
		obj->calls = backtrace_new();
		for (i = 0; i < num_frames; ++i) {
			function_name = dmessage_get_string(msg, i * 4 + 1);
			if (strcmp(function_name, "") == 0)
				call_name = strdup("[anon]");
			else
				call_name = strnewf("%s()", function_name);
			filename = dmessage_get_string(msg, i * 4);
			line_no = dmessage_get_int(msg, i * 4 + 2);
			backtrace_add(obj->calls, call_name, filename, line_no);
		}
		dmessage_free(msg);
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
	dmessage_t*      msg;
	const dvalue_t* setter;
	const dvalue_t* value;
	objview_t*      view;

	msg = dmessage_new(MESSAGE_REQ);
	dmessage_add_int(msg, REQ_GETOBJPROPDESCRANGE);
	dmessage_add_heapptr(msg, heapptr);
	dmessage_add_int(msg, 0);
	dmessage_add_int(msg, INT_MAX);
	if (!(msg = inferior_request(obj, msg)))
		return NULL;
	view = objview_new();
	while (index < dmessage_len(msg)) {
		prop_flags = dmessage_get_int(msg, index++);
		prop_key = dmessage_get_dvalue(msg, index++);
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
			getter = dmessage_get_dvalue(msg, index++);
			setter = dmessage_get_dvalue(msg, index++);
			objview_add_accessor(view, key_string, getter, setter, flags);
		}
		else {
			value = dmessage_get_dvalue(msg, index++);
			if (dvalue_tag(value) != DVALUE_UNUSED)
				objview_add_value(view, key_string, value, flags);
		}
		free(key_string);
	}
	dmessage_free(msg);
	return view;
}

const source_t*
inferior_get_source(inferior_t* obj, const char* filename)
{
	int         cache_id;
	dmessage_t*  msg;
	source_t*   source;
	const char* text;

	int i;

	for (i = 0; i < obj->num_sources; ++i) {
		if (strcmp(filename, obj->sources[i].filename) == 0)
			return obj->sources[i].source;
	}

	msg = dmessage_new(MESSAGE_REQ);
	dmessage_add_int(msg, REQ_APPREQUEST);
	dmessage_add_int(msg, APPREQ_SOURCE);
	dmessage_add_string(msg, filename);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (dmessage_tag(msg) == MESSAGE_ERR)
		goto on_error;
	text = dmessage_get_string(msg, 0);
	source = source_new(text);

	cache_id = obj->num_sources++;
	obj->sources = realloc(obj->sources, obj->num_sources * sizeof(struct source));
	obj->sources[cache_id].filename = strdup(filename);
	obj->sources[cache_id].source = source;

	return source;

on_error:
	dmessage_free(msg);
	return NULL;
}

objview_t*
inferior_get_vars(inferior_t* obj, int frame)
{
	const char*     name;
	dmessage_t*      msg;
	int             num_vars;
	const dvalue_t* value;
	objview_t*      vars;

	int i;

	msg = dmessage_new(MESSAGE_REQ);
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
	dmessage_t* msg;
	
	msg = dmessage_new(MESSAGE_REQ);
	dmessage_add_int(msg, REQ_ADDBREAK);
	dmessage_add_string(msg, filename);
	dmessage_add_int(msg, linenum);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (dmessage_tag(msg) == MESSAGE_ERR)
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
	dmessage_t* msg;

	msg = dmessage_new(MESSAGE_REQ);
	dmessage_add_int(msg, REQ_DELBREAK);
	dmessage_add_int(msg, handle);
	if (!(msg = inferior_request(obj, msg)))
		goto on_error;
	if (dmessage_tag(msg) == MESSAGE_ERR)
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
	dmessage_t* msg;

	msg = dmessage_new(MESSAGE_REQ);
	dmessage_add_int(msg, REQ_DETACH);
	if (!(msg = inferior_request(obj, msg)))
		return;
	dmessage_free(msg);
	while (inferior_attached(obj))
		inferior_update(obj);
}

dvalue_t*
inferior_eval(inferior_t* obj, const char* expr, int frame, bool* out_is_error)
{
	dvalue_t*  dvalue = NULL;
	dmessage_t* msg;

	msg = dmessage_new(MESSAGE_REQ);
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
	dmessage_t* msg;

	msg = dmessage_new(MESSAGE_REQ);
	dmessage_add_int(msg, REQ_PAUSE);
	if (!(msg = inferior_request(obj, msg)))
		return false;
	return true;
}

dmessage_t*
inferior_request(inferior_t* obj, dmessage_t* msg)
{
	dmessage_t* response = NULL;

	if (!(dmessage_send(msg, obj->socket)))
		goto lost_connection;
	do {
		dmessage_free(response);
		if (!(response = dmessage_recv(obj->socket)))
			goto lost_connection;
		if (dmessage_tag(response) == MESSAGE_NFY)
			handle_notify(obj, response);
	} while (dmessage_tag(response) == MESSAGE_NFY);
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
	dmessage_t* msg;

	msg = dmessage_new(MESSAGE_REQ);
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
	printf("\33[31;1merror!\33[m\n");
	return 0;
}

static bool
handle_notify(inferior_t* obj, const dmessage_t* msg)
{
	const char*   heading;
	enum print_op print_op;
	int           status_type;

	switch (dmessage_tag(msg)) {
	case MESSAGE_NFY:
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
			printf("**************************************************\n");
			printf("uncaught exception! - at %s:%d\n",
				dmessage_get_string(msg, 3),
				dmessage_get_int(msg, 4));
			dvalue_print(dmessage_get_dvalue(msg, 2), true);
			printf("\n**************************************************\n\n");
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
