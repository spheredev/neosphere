#include "ssj.h"
#include "inferior.h"

#include "backtrace.h"
#include "help.h"
#include "message.h"
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
static bool handle_notify (inferior_t* o, const message_t* msg);

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
	inferior_t* o;
	message_t*  req;
	message_t*  rep;

	o = calloc(1, sizeof(inferior_t));
	printf("connecting to %s:%d... ", hostname, port);
	fflush(stdout);
	if (!(o->socket = socket_connect(hostname, port, 30.0)))
		goto on_error;
	printf("OK.\n");
	if (!do_handshake(o->socket))
		goto on_error;

	printf("querying target... ");
	req = message_new(MESSAGE_REQ);
	message_add_int(req, REQ_APP_REQUEST);
	message_add_int(req, APPREQ_GAME_INFO);
	rep = inferior_request(o, req);
	o->title = strdup(message_get_string(rep, 0));
	o->author = strdup(message_get_string(rep, 1));
	message_free(rep);
	printf("OK.\n");

	printf("    game: %s\n", o->title);
	printf("    author: %s\n", o->author);

	o->id_no = s_next_id_no++;
	return o;

on_error:
	free(o);
	return NULL;
}

void
inferior_free(inferior_t* o)
{
	int i;

	for (i = 0; i < o->num_sources; ++i) {
		source_free(o->sources[i].source);
		free(o->sources[i].filename);
	}
	socket_close(o->socket);
	free(o->title);
	free(o->author);
	free(o);
}

bool
inferior_update(inferior_t* o)
{
	bool       is_active = true;
	message_t* msg = NULL;

	if (o->is_detached)
		return false;
	
	if (!(msg = message_recv(o->socket)))
		goto detached;
	if (!handle_notify(o, msg))
		goto detached;
	message_free(msg);
	return true;

detached:
	message_free(msg);
	o->is_detached = true;
	return false;
}

bool
inferior_is_attached(const inferior_t* o)
{
	return !o->is_detached;
}

bool
inferior_is_running(const inferior_t* o)
{
	return !o->is_paused && inferior_is_attached(o);
}

const char*
inferior_author(const inferior_t* o)
{
	return o->author;
}

const char*
inferior_title(const inferior_t* o)
{
	return o->title;
}

const source_t*
inferior_get_source(inferior_t* o, const char* filename)
{
	int         cache_id;
	message_t*  msg;
	source_t*   source;
	const char* text;

	int i;

	for (i = 0; i < o->num_sources; ++i) {
		if (strcmp(filename, o->sources[i].filename) == 0)
			return o->sources[i].source;
	}

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_APP_REQUEST);
	message_add_int(msg, APPREQ_SOURCE);
	message_add_string(msg, filename);
	if (!(msg = inferior_request(o, msg)))
		goto on_error;
	if (message_tag(msg) == MESSAGE_ERR)
		goto on_error;
	text = message_get_string(msg, 0);
	source = source_new(text);

	cache_id = o->num_sources++;
	o->sources = realloc(o->sources, o->num_sources * sizeof(struct source));
	o->sources[cache_id].filename = strdup(filename);
	o->sources[cache_id].source = source;

	return source;

on_error:
	message_free(msg);
	return NULL;
}

const backtrace_t*
inferior_get_stack(inferior_t* o)
{
	const char* filename;
	int         line_no;
	message_t*  msg;
	const char* name;
	int         num_frames;

	int i;

	if (o->stack == NULL) {
		msg = message_new(MESSAGE_REQ);
		message_add_int(msg, REQ_GET_CALLSTACK);
		if (!(msg = inferior_request(o, msg)))
			return NULL;
		num_frames = message_len(msg) / 4;
		o->stack = backtrace_new();
		for (i = 0; i < num_frames; ++i) {
			name = message_get_string(msg, i * 4 + 1);
			filename = message_get_string(msg, i * 4);
			line_no = message_get_int(msg, i * 4 + 2);
			backtrace_add(o->stack, name, filename, line_no);
		}
		message_free(msg);
	}

	return o->stack;
}

void
inferior_detach(inferior_t* o)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_DETACH);
	if (!(msg = inferior_request(o, msg)))
		return;
	message_free(msg);
	o->is_paused = false;
}

dvalue_t*
inferior_eval(inferior_t* o, const char* expr, int frame, bool* out_is_error)
{
	dvalue_t*  dvalue = NULL;
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_EVAL);
	message_add_string(msg, expr);
	message_add_int(msg, -(1 + frame));
	msg = inferior_request(o, msg);
	dvalue = dvalue_dup(message_get_dvalue(msg, 1));
	*out_is_error = message_get_int(msg, 0) != 0;
	message_free(msg);
	return dvalue;
}

bool
inferior_pause(inferior_t* o)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg, REQ_PAUSE);
	if (!(msg = inferior_request(o, msg)))
		return false;
	return true;
}

message_t*
inferior_request(inferior_t* o, message_t* msg)
{
	message_t* response = NULL;

	message_send(msg, o->socket);
	do {
		message_free(response);
		if (!(response = message_recv(o->socket)))
			goto lost_connection;
		if (message_tag(response) == MESSAGE_NFY)
			handle_notify(o, response);
	} while (message_tag(response) == MESSAGE_NFY);
	message_free(msg);
	return response;

lost_connection:
	message_free(msg);
	return NULL;
}

bool
inferior_resume(inferior_t* o, resume_op_t op)
{
	message_t* msg;

	msg = message_new(MESSAGE_REQ);
	message_add_int(msg,
		op == OP_STEP_OVER ? REQ_STEP_OVER
		: op == OP_STEP_IN ? REQ_STEP_INTO
		: op == OP_STEP_OUT ? REQ_STEP_OUT
		: REQ_RESUME);
	if (!(msg = inferior_request(o, msg)))
		return false;
	o->frame_index = 0;
	o->is_paused = false;
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
handle_notify(inferior_t* o, const message_t* msg)
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
			backtrace_free(o->stack);
			o->stack = NULL;
			o->is_paused = status != 0;
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
				printf("inferior #%u disconnected normally.\n", o->id_no);
			else
				printf("inferior #%u disconnected due to an error.\n", o->id_no);
			o->is_detached = true;
			return false;
		}
		break;
	}
	return true;
}
