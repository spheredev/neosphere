#include "ssj.h"
#include "session.h"

#include "remote.h"

#define CL_BUFFER_SIZE 65536

struct session
{
	char       cl_buffer[CL_BUFFER_SIZE];
	lstring_t* filename;
	int32_t    line;
	remote_t*  remote;
	bool       is_breakpoint;
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
	REQ_GET_CALLSTACK = 0x1C,
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

static bool       do_command_line (session_t* sess);
static message_t* do_request      (session_t* sess, message_t* msg);
static bool       process_message (session_t* sess, const message_t* msg);

session_t*
new_session(void)
{
	session_t* sess;

	sess = calloc(1, sizeof(session_t));
	return sess;
}

bool
attach_session(session_t* sess, const char* hostname, int port)
{
	sess->remote = connect_remote(hostname, port);
	return sess->remote != NULL;
}

void
run_session(session_t* sess)
{
	bool       is_active = true;
	message_t* msg;
	
	while (is_active) {
		if (sess->remote == NULL || sess->is_breakpoint)
			is_active &= do_command_line(sess);
		else {
			if (!(msg = msg_receive(sess->remote)))
				goto on_error;
			is_active &= process_message(sess, msg);
			msg_free(msg);
		}
	}
	return;

on_error:
	msg_free(msg);
	printf("Communication error, ending session.\n");
	return;
}

static bool
do_command_line(session_t* sess)
{
	char*       argument;
	char*       command;
	int         ch = '\0';
	size_t      ch_idx = 0;
	lstring_t*  eval_code;
	const char* eval_result;
	const char* filename;
	int32_t     line_no;
	size_t      num_items;
	char*       parsee;
	message_t*  reply;
	message_t*  req;
	const char* item_name;

	size_t i;

	// get a command from the user
	sess->cl_buffer[0] = '\0';
	while (sess->cl_buffer[0] == '\0') {
		printf("\n\33[0;1m%s:%d \33[33;1mssj$\33[m ",
			lstr_cstr(sess->filename), sess->line);
		ch = getchar();
		while (ch != '\n') {
			if (ch_idx >= CL_BUFFER_SIZE - 1) {
				printf("Command string is too long, must be < 64k");
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
	if (strcmp(command, "c") == 0) {
		if (sess->remote != NULL)
			printf("Already attached.");
		else {
			if (!(sess->remote = connect_remote("127.0.0.1", 1208)))
				printf("Failed to connect to minisphere.\n");
		}
	}
	else if (strcmp(command, "quit") == 0) {
		printf("Quit, asking target to detach... ");
		fflush(stdout);
		if (sess->remote == NULL)
			return false;
		else {
			req = msg_new(MSG_CLASS_REQ);
			msg_add_int(req, REQ_DETACH);
			msg_free(do_request(sess, req));
			sess->is_breakpoint = false;
		}
		printf("OK.\n");
	}
	else if (strcmp(command, "go") == 0) {
		if (sess->remote == NULL)
			printf("minisphere not attached, use 'c' to attach.\n");
		else {
			printf("Resuming execution at %s:%d\n", lstr_cstr(sess->filename), sess->line);
			req = msg_new(MSG_CLASS_REQ);
			msg_add_int(req, REQ_RESUME);
			msg_free(do_request(sess, req));
			sess->is_breakpoint = false;
		}
	}
	else if (strcmp(command, "stack") == 0) {
		if (sess->remote == NULL)
			printf("minisphere not attached, use 'c' to attach.\n");
		else {
			req = msg_new(MSG_CLASS_REQ);
			msg_add_int(req, REQ_GET_CALLSTACK);
			reply = do_request(sess, req);
			num_items = msg_get_length(reply) / 4;
			for (i = 0; i < num_items; ++i) {
				msg_get_string(reply, i * 2, &filename);
				msg_get_string(reply, i * 2 + 1, &item_name);
				msg_get_int(reply, i * 2 + 2, &line_no);
				printf("%3zd - %s() <%s:%d>\n", i, item_name, filename, line_no);
			}
			msg_free(reply);
		}
	}
	else if (strcmp(command, "next") == 0) {
		if (sess->remote == NULL)
			printf("minisphere not attached, use 'c' to attach.\n");
		else {
			req = msg_new(MSG_CLASS_REQ);
			msg_add_int(req, REQ_STEP_OVER);
			msg_free(do_request(sess, req));
			sess->is_breakpoint = false;
		}
	}
	else if (strcmp(command, "step") == 0) {
		if (sess->remote == NULL)
			printf("minisphere not attached, use 'c' to attach.\n");
		else {
			req = msg_new(MSG_CLASS_REQ);
			msg_add_int(req, REQ_STEP_INTO);
			msg_free(do_request(sess, req));
			sess->is_breakpoint = false;
		}
	}
	else if (strcmp(command, "out") == 0) {
		if (sess->remote == NULL)
			printf("minisphere not attached, use 'c' to attach.\n");
		else {
			req = msg_new(MSG_CLASS_REQ);
			msg_add_int(req, REQ_STEP_OUT);
			msg_free(do_request(sess, req));
			sess->is_breakpoint = false;
		}
	}
	else if (strcmp(command, "eval") == 0) {
		if (sess->remote == NULL)
			printf("minisphere not attached, use 'c' to attach.\n");
		else {
			eval_code = lstr_newf(
				"(function() { try { return Duktape.enc('jx', eval(\"%s\"), null, 3); } catch (e) { return e.toString(); } }).call(this);",
				argument);
			req = msg_new(MSG_CLASS_REQ);
			msg_add_int(req, REQ_EVAL);
			msg_add_string(req, lstr_cstr(eval_code));
			reply = do_request(sess, req);
			msg_get_string(reply, 1, &eval_result);
			printf("%s\n", eval_result);
			msg_free(reply);
		}
	}
	else if (strcmp(command, "var") == 0) {
		req = msg_new(MSG_CLASS_REQ);
		msg_add_int(req, REQ_GET_LOCALS);
		reply = do_request(sess, req);
		num_items = msg_get_length(reply) / 2;
		for (i = 0; i < num_items; ++i) {
			msg_get_string(reply, i * 2, &item_name);
			printf("%s = [value]\n", item_name);
		}
		msg_free(reply);
	}
	else {
		printf("'%s' not recognized in this context\n", command);
	}
	free(parsee);
	return true;
}

static message_t*
do_request(session_t* sess, message_t* msg)
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
			sess->is_breakpoint = flag != 0;
			sess->filename = lstr_new(filename);
			sess->line = line_no;
			break;
		case NFY_PRINT:
			msg_get_string(msg, 1, &text);
			printf("\x1B[36m%s\x1B[m", text);
			break;
		case NFY_ALERT:
			msg_get_string(msg, 1, &text);
			printf("\x1B[31m%s\x1B[m", text);
			break;
		case NFY_THROW:
			msg_get_int(msg, 1, &flag);
			msg_get_string(msg, 2, &text);
			msg_get_string(msg, 3, &filename);
			msg_get_int(msg, 4, &line_no);
			if (flag != 0)
				printf("\033[0;1m'%s' thrown at %s:%d\033[m\n", text, filename, line_no);
			break;
		case NFY_DETACHING:
			msg_get_int(msg, 1, &flag);
			if (flag == 0)
				printf("minisphere detached cleanly.\n");
			else
				printf("minisphere detached due to an error.\n");
			return false;
		}
		break;
	}
	return true;
}
