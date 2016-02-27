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
	char*     filename;
	source_t* source;
};

struct inferior
{
	unsigned int   id_no;
	int            num_sources;
	bool           is_detached;
	bool           is_paused;
	char*          title;
	char*          author;
	backtrace_t*   stack;
	int            frame_index;
	bool           have_debug_info;
	int            line_no;
	uint8_t        ptr_size;
	socket_t*      socket;
	struct source* sources;
};

static bool do_handshake  (socket_t* socket);
static bool handle_notify (inferior_t* dis, const message_t* msg);

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
	inferior_t* dis;
	message_t*  req;
	message_t*  rep;

	dis = calloc(1, sizeof(inferior_t));
	printf("connecting to %s:%d... ", hostname, port);
	fflush(stdout);
	if (!(dis->socket = socket_connect(hostname, port, 30.0)))
		goto on_error;
	printf("OK.\n");
	if (!do_handshake(dis->socket))
		goto on_error;

	printf("querying target... ");
	req = message_new(MESSAGE_REQ);
	message_add_int(req, REQ_APP_REQUEST);
	message_add_int(req, APPREQ_GAME_INFO);
	rep = inferior_request(dis, req);
	dis->title = strdup(message_get_string(rep, 0));
	dis->author = strdup(message_get_string(rep, 1));
	message_free(rep);
	printf("OK.\n");

	printf("    game: %s\n", dis->title);
	printf("    author: %s\n", dis->author);

	dis->id_no = s_next_id_no++;
	return dis;

on_error:
	free(dis);
	return NULL;
}

void
inferior_free(inferior_t* dis)
{
	int i;

	for (i = 0; i < dis->num_sources; ++i) {
		source_free(dis->sources[i].source);
		free(dis->sources[i].filename);
	}
	socket_close(dis->socket);
	free(dis->title);
	free(dis->author);
	free(dis);
}

bool
inferior_update(inferior_t* dis)
{
	bool       is_active = true;
	message_t* msg = NULL;

	if (dis->is_detached)
		return false;
	
	if (!(msg = message_recv(dis->socket)))
		goto detached;
	if (!handle_notify(dis, msg))
		goto detached;
	message_free(msg);
	return true;

detached:
	message_free(msg);
	dis->is_detached = true;
	return false;
}

bool
inferior_is_attached(const inferior_t* dis)
{
	return !dis->is_detached;
}

bool
inferior_is_running(const inferior_t* dis)
{
	return !dis->is_paused && inferior_is_attached(dis);
}

const char*
inferior_author(const inferior_t* dis)
{
	return dis->author;
}

const char*
inferior_title(const inferior_t* dis)
{
	return dis->title;
}

const source_t*
inferior_get_source(inferior_t* dis, const char* filename)
{
	int         cache_id;
	message_t*  msg;
	source_t*   source;
	const char* text;

	int i;

	for (i = 0; i < dis->num_sources; ++i) {
		if (strcmp(filename, dis->sources[i].filename) == 0)
			return dis->sources[i].source;
	}

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_APP_REQUEST);
	message_add_int(msg, APPREQ_SOURCE);
	message_add_string(msg, filename);
	if (!(msg = inferior_request(dis, msg)))
		goto on_error;
	if (message_tag(msg) == MESSAGE_ERR)
		goto on_error;
	text = message_get_string(msg, 0);
	source = source_new(text);

	cache_id = dis->num_sources++;
	dis->sources = realloc(dis->sources, dis->num_sources * sizeof(struct source));
	dis->sources[cache_id].filename = strdup(filename);
	dis->sources[cache_id].source = source;

	return source;

on_error:
	message_free(msg);
	return NULL;
}

const backtrace_t*
inferior_get_stack(inferior_t* dis)
{
	const char* filename;
	const char* function_name;
	int         line_no;
	message_t*  msg;
	int         num_frames;
	char*       view_name;

	int i;

	if (dis->stack == NULL) {
		msg = message_new(MESSAGE_REQ);
		message_add_int(msg, REQ_GET_CALLSTACK);
		if (!(msg = inferior_request(dis, msg)))
			return NULL;
		num_frames = message_len(msg) / 4;
		dis->stack = backtrace_new();
		for (i = 0; i < num_frames; ++i) {
			function_name = message_get_string(msg, i * 4 + 1);
			if (strcmp(function_name, "") == 0)
				view_name = strdup("[anon]");
			else
				view_name = strnewf("%s()", function_name);
			filename = message_get_string(msg, i * 4);
			line_no = message_get_int(msg, i * 4 + 2);
			backtrace_add(dis->stack, view_name, filename, line_no);
		}
		message_free(msg);
	}

	return dis->stack;
}

void
inferior_detach(inferior_t* dis)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_DETACH);
	if (!(msg = inferior_request(dis, msg)))
		return;
	message_free(msg);
	dis->is_paused = false;
}

dvalue_t*
inferior_eval(inferior_t* dis, const char* expr, int frame, bool* out_is_error)
{
	dvalue_t*  dvalue = NULL;
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_EVAL);
	message_add_string(msg, expr);
	message_add_int(msg, -(1 + frame));
	msg = inferior_request(dis, msg);
	dvalue = dvalue_dup(message_get_dvalue(msg, 1));
	*out_is_error = message_get_int(msg, 0) != 0;
	message_free(msg);
	return dvalue;
}

objview_t*
inferior_get_locals(inferior_t* dis, int frame)
{
	const char*     name;
	message_t*      msg;
	int             num_vars;
	const dvalue_t* value;
	objview_t*      view;

	int i;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_GET_LOCALS);
	message_add_int(msg, -frame - 1);
	if (!(msg = inferior_request(dis, msg)))
		return NULL;
	view = objview_new();
	num_vars = message_len(msg) / 2;
	for (i = 0; i < num_vars; ++i) {
		name = message_get_string(msg, i * 2 + 0);
		value = message_get_dvalue(msg, i * 2 + 1);
		objview_add_value(view, name, value);
	}
	return view;
}

objview_t*
inferior_inspect_obj(inferior_t* dis, dukptr_t heapptr)
{
	const dvalue_t* getter;
	bool            is_accessor;
	int             index = 0;
	int             prop_flags;
	const char*     prop_key;
	message_t*      msg;
	objview_t*   obj_view;
	const dvalue_t* setter;
	const dvalue_t* value;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_INSPECT_PROPS);
	message_add_heapptr(msg, heapptr);
	message_add_int(msg, 0);
	message_add_int(msg, INT_MAX);
	if (!(msg = inferior_request(dis, msg)))
		return NULL;
	obj_view = objview_new();
	while (index < message_len(msg)) {
		prop_flags = message_get_int(msg, index++);
		prop_key = message_get_string(msg, index++);
		is_accessor = (prop_flags & 0x008) != 0;
		if (prop_key[0] == '\xFF') {
			index += is_accessor ? 2 : 1;
			continue;
		}
		if (is_accessor) {
			getter = message_get_dvalue(msg, index++);
			setter = message_get_dvalue(msg, index++);
			objview_add_accessor(obj_view, prop_key, getter, setter);
		}
		else {
			value = message_get_dvalue(msg, index++);
			objview_add_value(obj_view, prop_key, value);
		}
	}
	message_free(msg);
	return obj_view;
}

bool
inferior_pause(inferior_t* dis)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_PAUSE);
	if (!(msg = inferior_request(dis, msg)))
		return false;
	return true;
}

message_t*
inferior_request(inferior_t* dis, message_t* msg)
{
	message_t* response = NULL;

	message_send(msg, dis->socket);
	do {
		message_free(response);
		if (!(response = message_recv(dis->socket)))
			goto lost_connection;
		if (message_tag(response) == MESSAGE_NFY)
			handle_notify(dis, response);
	} while (message_tag(response) == MESSAGE_NFY);
	message_free(msg);
	return response;

lost_connection:
	message_free(msg);
	return NULL;
}

bool
inferior_resume(inferior_t* dis, resume_op_t op)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg,
		op == OP_STEP_OVER ? REQ_STEP_OVER
		: op == OP_STEP_IN ? REQ_STEP_INTO
		: op == OP_STEP_OUT ? REQ_STEP_OUT
		: REQ_RESUME);
	if (!(msg = inferior_request(dis, msg)))
		return false;
	dis->frame_index = 0;
	dis->is_paused = false;
	return true;
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
handle_notify(inferior_t* dis, const message_t* msg)
{
	int status;

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
			status = message_get_int(msg, 1);
			backtrace_free(dis->stack);
			dis->stack = NULL;
			dis->is_paused = status != 0;
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
			if ((status = message_get_int(msg, 1)) == 0)
				break;
			printf("FATAL UNCAUGHT - %s\n", message_get_string(msg, 2));
			printf("    at: %s:%d\n", message_get_string(msg, 3), message_get_int(msg, 4));
			break;
		case NFY_DETACHING:
			status = message_get_int(msg, 1);
			if (status == 0)
				printf("inferior #%u disconnected normally.\n", dis->id_no);
			else
				printf("inferior #%u disconnected due to an error.\n", dis->id_no);
			dis->is_detached = true;
			return false;
		}
		break;
	}
	return true;
}
