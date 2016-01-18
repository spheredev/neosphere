#include "ssj.h"
#include "session.h"

#include "remote.h"

#define CL_BUFFER_SIZE 65536

struct session
{
	char      cl_buffer[CL_BUFFER_SIZE];
	remote_t* remote;
	bool      is_breakpoint;
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
static bool       process_message (session_t* sess, const message_t* msg);

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
	bool       is_active = true;
	message_t* msg;
	
	while (is_active) {
		if (sess->is_breakpoint)
			do_command_line(sess);
		else {
			if (!(msg = msg_receive(sess->remote)))
				goto on_error;
			is_active = process_message(sess, msg);
			msg_free(msg);
		}
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
	char*       argument;
	char*       command;
	int         ch = '\0';
	size_t      ch_idx = 0;
	lstring_t*  eval_code;
	const char* eval_result;
	size_t      num_vars;
	char*       parsee;
	message_t*  reply;
	message_t*  req;
	const char* var_name;

	size_t i;

	// get a command from the user
	sess->cl_buffer[0] = '\0';
	while (sess->cl_buffer[0] == '\0') {
		printf("ssj$ ");
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
	if (strcmp(command, "q") == 0) {
		printf("Shutdown requested, sending detach request.\n");
		req = msg_new(MSG_CLASS_REQ);
		msg_add_int(req, REQ_DETACH);
		msg_free(handle_req(sess, req));
		msg_free(req);
		sess->is_breakpoint = false;
	}
	else if (strcmp(command, "r") == 0) {
		req = msg_new(MSG_CLASS_REQ);
		msg_add_int(req, REQ_RESUME);
		msg_free(handle_req(sess, req));
		msg_free(req);
		sess->is_breakpoint = false;
	}
	else if (strcmp(command, "e") == 0) {
		eval_code = lstr_newf(
			"(function() { try { return Duktape.enc('jx', eval(\"%s\"), null, 3); } catch (e) { return e.toString(); } }).call(this);",
			argument);
		req = msg_new(MSG_CLASS_REQ);
		msg_add_int(req, REQ_EVAL);
		msg_add_string(req, lstr_cstr(eval_code));
		reply = handle_req(sess, req);
		msg_free(req);
		msg_get_string(reply, 1, &eval_result);
		printf("%s\n", eval_result);
		msg_free(reply);
	}
	else if (strcmp(command, "lv") == 0) {
		req = msg_new(MSG_CLASS_REQ);
		msg_add_int(req, REQ_GET_LOCALS);
		reply = handle_req(sess, req);
		msg_free(req);
		num_vars = msg_get_length(reply) / 2;
		for (i = 0; i < num_vars; ++i) {
			msg_get_string(reply, i * 2, &var_name);
			printf("%s = [value]\n", var_name);
		}
		msg_free(reply);
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

static bool
process_message(session_t* sess, const message_t* msg)
{
	const char* filename;
	int32_t     flag;
	const char* func_name;
	int32_t     line_no;
	int32_t     notify_id;
	const char* text;

	switch (msg_get_class(msg)) {
	case MSG_CLASS_NFY:
		msg_get_int(msg, 0, &notify_id);
		switch (notify_id) {
		case NFY_STATUS:
			msg_get_int(msg, 1, &flag);
			msg_get_string(msg, 2, &filename);
			msg_get_string(msg, 3, &func_name);
			msg_get_int(msg, 4, &line_no);
			if (flag != 0 && !sess->is_breakpoint)
				printf("[SSJ @ %s:%i] BREAK: in function '%s'\n", filename, line_no, func_name);
			sess->is_breakpoint = flag != 0;
			break;
		case NFY_PRINT:
			msg_get_string(msg, 1, &text);
			printf("%s", text);
			break;
		case NFY_THROW:
			msg_get_int(msg, 1, &flag);
			msg_get_string(msg, 2, &text);
			msg_get_string(msg, 3, &filename);
			msg_get_int(msg, 4, &line_no);
			if (flag != 0)
				printf("[SSJ @ %s:%i] FATAL: %s\n", filename, line_no, text);
			break;
		case NFY_DETACHING:
			msg_get_int(msg, 1, &flag);
			if (flag == 0)
				printf("Target detached cleanly.\n");
			else
				printf("Target detached due to stream error.\n");
			return false;
		}
		break;
	}
	return true;
}