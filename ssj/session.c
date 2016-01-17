#include "ssj.h"
#include "session.h"

#include "remote.h"

#define CL_BUFFER_SIZE 65536

struct session
{
	char      cl_buffer[CL_BUFFER_SIZE];
	remote_t* remote;
	bool      is_paused;
};

enum req_command
{
	REQ_BASIC_INFO = 0x10,
	REQ_SEND_STATUS = 0x11,
	REQ_PAUSE = 0x12,
	REQ_RESUME = 0x13,
	REQ_STEP_INTO = 0x14,
	REQ_STEP_OVER = 0x15,
	REQ_STEP_OUT = 0x16,
	REQ_LIST_BREAK = 0x17,
	REQ_ADD_BREAK = 0x18,
	REQ_DEL_BREAK = 0x19,
	REQ_GET_VAR = 0x1A,
	REQ_PUT_VAR = 0x1B,
	REQ_CALLSTACK = 0x1C,
	REQ_GET_LOCALS = 0x1D,
	REQ_EVAL = 0x1E,
	REQ_DETACH = 0x1F,
	REQ_DUMP_HEAP = 0x20,
	REQ_GET_BYTECODE = 0x21,
};

enum nfy_command
{
	NFY_STATUS = 0x01,
	NFY_PRINT = 0x02,
	NFY_ALERT = 0x03,
	NFY_LOG = 0x04,
	NFY_THROW = 0x05,
	NFY_DETACHING = 0x06,
};

static void       do_command_line (session_t* sess);
static message_t* handle_req      (session_t* sess, const message_t* msg);
static void       process_message (session_t* sess, const message_t* msg);

session_t*
new_session(const char* hostname, int port)
{
	session_t* sess;

	sess = calloc(1, sizeof(session_t));
	if (!(sess->remote = connect_remote(hostname, port)))
		goto on_error;
	return sess;

on_error:
	free(sess);
	return NULL;
}

void
run_session(session_t* sess)
{
	message_t* msg;
	
	while (true) {
		if (!(msg = msg_receive(sess->remote)))
			goto on_error;
		process_message(sess, msg);
		msg_free(msg);
		if (sess->is_paused)
			do_command_line(sess);
	}
	return;

on_error:
	msg_free(msg);
	printf("Communication error, ending session.\n");
	return;
}

static void
do_command_line(session_t* sess)
{
	char*      argument;
	char*      command;
	int        ch = '\0';
	size_t     ch_idx = 0;
	char*      parsee;
	message_t* req;

	// get a command from the user
	sess->cl_buffer[0] = '\0';
	while (sess->cl_buffer[0] == '\0') {
		printf("SSJ: ");
		ch = getchar();
		while (ch != '\n') {
			if (ch_idx >= CL_BUFFER_SIZE - 1) {
				printf("ERROR: command string is too long (> 64k)");
				sess->cl_buffer[0] = '\0';
				break;
			}
			sess->cl_buffer[ch_idx++] = ch;
			ch = getchar();
		}
		sess->cl_buffer[ch_idx] = '\0';
	}
	
	// parse the command line
	parsee = strdup(sess->cl_buffer);
	command = strtok_r(parsee, " ", &argument);
	if (strcmp(command, "x") == 0) {
		req = msg_new(MSG_CLASS_REQ);
		msg_add_int(req, REQ_DETACH);
		msg_free(handle_req(sess, req));
		msg_free(req);
	}
	else if (strcmp(command, "r") == 0) {
		req = msg_new(MSG_CLASS_REQ);
		msg_add_int(req, REQ_RESUME);
		msg_free(handle_req(sess, req));
		msg_free(req);
	}
	free(parsee);
}

static message_t*
handle_req(session_t* sess, const message_t* msg)
{
	message_t* response;

	msg_send(sess->remote, msg);
	do {
		if (!(response = msg_receive(sess->remote))) return NULL;
		if (msg_get_class(response) == MSG_CLASS_NFY) {
			process_message(sess, response);
			msg_free(response);
		}
	} while (msg_get_class(response) == MSG_CLASS_NFY);
	return response;
}

static void
process_message(session_t* sess, const message_t* msg)
{
	const char* filename;
	const char* error_text;
	const char* func_name;
	int32_t     line_no;
	int32_t     notify_id;
	int32_t     state;
	int32_t     throw_type;

	switch (msg_get_class(msg)) {
	case MSG_CLASS_NFY:
		msg_get_int(msg, 0, &notify_id);
		switch (notify_id) {
		case NFY_STATUS:
			msg_get_int(msg, 1, &state);
			msg_get_string(msg, 2, &filename);
			msg_get_string(msg, 3, &func_name);
			msg_get_int(msg, 4, &line_no);
			if (sess->is_paused = state != 0) {
				printf("Paused in function '%s'\n", func_name);
				printf("  script: '%s':%i\n\n", filename, line_no);
			}
			break;
		case NFY_THROW:
			msg_get_int(msg, 1, &throw_type);
			msg_get_string(msg, 2, &error_text);
			msg_get_string(msg, 3, &filename);
			msg_get_int(msg, 4, &line_no);
			if (throw_type != 0) {
				printf("Intercepted %s, halting execution\n", error_text);
				printf("  script: '%s':%i\n\n", filename, line_no);
			}
			break;
		case NFY_DETACHING:
			printf("Target has been detached.\n");
			break;
		}
		break;
	}
}