#include "ssj.h"
#include "session.h"
#include "source.h"

#include "remote.h"

#define CL_BUFFER_SIZE 65536

struct session
{
	char       cl_buffer[CL_BUFFER_SIZE];
	size_t     current_frame;
	lstring_t* function;
	lstring_t* filename;
	bool       have_debug_info;
	bool       is_stopped;
	int32_t    line;
	remote_t*  remote;
	source_t*  source;
	path_t*    source_path;
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
	REQ_INSPECT_OBJ = 0x22,
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

static message_t* converse        (session_t* sess, message_t* msg);
static bool       do_command_line (session_t* sess);
static bool       process_message (session_t* sess, const message_t* msg);

session_t*
new_session(const char* hostname, int port)
{
	path_t*     origin = NULL;
	message_t*  req;
	message_t*  rep;
	session_t*  session;

	session = calloc(1, sizeof(session_t));
	if (!(session->remote = connect_remote(hostname, port)))
		goto on_error;

	// get the original source tree path from the target.
	req = msg_new(MSG_CLASS_REQ);
	msg_add_int(req, REQ_EVAL);
	msg_add_string(req, "global.SourceMap.origin");
	rep = converse(session, req);
	if (msg_atom_int(rep, 0) == 0)
		origin = path_new(msg_atom_string(rep, 1));
	msg_free(rep);
	session->source_path = origin;
	
	return session;

on_error:
	free(session);
	return NULL;
}

void
print_commands(session_t* sess)
{
	printf(
		"\33[0;1m"
		"SSJ Debugger Commands                                                          \n"
		"\33[m"
		"Abbreviated names are listed first, followed by the full, verbose name of each \n"
		"command. Unlike GDB, truncated names are not allowed.                          \n\n"

		" bt, backtrace  Show a list of all function calls currently on the stack       \n"
		" bp, break      Set a breakpoint at file:line (e.g. scripts/main.js:812)       \n"
		" cl, clear      Clear a breakpoint set a file:line (see 'break')               \n"
		" c,  continue   Run either until a breakpoint is hit or an error is thrown     \n"
		" e,  eval       Evaluate a JavaScript expression                               \n"
		" f,  frame      Change the stack frame used for commands like 'eval' and 'var' \n"
		" l,  list       Show source text around the line of code being debugged        \n"
		" s,  step       Run the next line of code                                      \n"
		" si, stepin     Run the next line of code, stepping into functions             \n"
		" so, stepout    Run until the current function call returns                    \n"
		" v,  var        List local variables and their values in the active frame      \n"
		" w,  where      Show the filename and line number of the next line of code     \n"
		" h,  help       Show this list of commands                                     \n"
		" q,  quit       Detach and terminate your SSJ debugging session                \n\n"

		"Type 'help <command>' for usage of individual commands.                        \n"
	);
}

void
eval_expression(session_t* sess, const char* expr, size_t frame)
{
	lstring_t*  eval_code;
	int32_t     flag;
	message_t*  request;
	message_t*  response;
	const char* result;

	eval_code = lstr_newf(
		"(function() { return Duktape.enc('jx', eval(\"%s\"), null, 3); }).call(this);",
		expr);
	request = msg_new(MSG_CLASS_REQ);
	msg_add_int(request, REQ_EVAL);
	msg_add_string(request, lstr_cstr(eval_code));
	msg_add_int(request, -(int32_t)(1 + frame));
	response = converse(sess, request);
	flag = msg_atom_int(response, 0);
	result = msg_atom_string(response, 1);
	if (flag != 0) printf("\33[31;1m");
		else printf("= \33[0;1m");
	printf("%s\33[m\n", result);
	msg_free(response);
}

void
print_callstack(session_t* sess, size_t frame, bool show_all)
{
	lstring_t*       display_name;
	lstring_t*       filename;
	const char*      function_name;
	const lstring_t* line;
	int32_t          line_num;
	size_t           n_items;
	message_t*       request;
	message_t*       response;
	source_t*        source;

	size_t i;

	request = msg_new(MSG_CLASS_REQ);
	msg_add_int(request, REQ_GET_CALLSTACK);
	response = converse(sess, request);
	n_items = msg_len(response) / 4;
	if (frame < 0 || frame >= n_items)
		printf("no frame at index %zd, use `backtrace` for list\n", frame);
	else {
		sess->current_frame = frame;
		for (i = 0; i < n_items; ++i) {
			filename = lstr_new(msg_atom_string(response, i * 4));
			function_name = msg_atom_string(response, i * 4 + 1);
			line_num = msg_atom_int(response, i * 4 + 2);
			if (i == frame || show_all) {
				display_name = function_name[0] != '\0' ? lstr_newf("%s()", function_name)
					: lstr_new("anon");
				printf("%3zd: \33[36;1m%s\33[m at \33[36;1m%s:%d\33[m \33[33;1m%s\33[m\n",
					i, lstr_cstr(display_name), lstr_cstr(filename), line_num,
					i == frame ? "<<<" : "");
				lstr_free(display_name);
				if (!show_all) {
					if (source = load_source(filename, sess->source_path)) {
						line = get_source_line(source, line_num - 1);
						printf("\33[30;1m%4d\33[m %s\n", line_num, lstr_cstr(line));
						free_source(source);
					}
				}
			}
			lstr_free(filename);
		}
	}
	msg_free(response);
}

void
print_status(session_t* sess)
{
	const lstring_t* line;
	
	printf("PC is at \33[36;1m%s:%d\33[m in function \33[36;1m%s\33[m\n",
		lstr_cstr(sess->filename), sess->line, lstr_cstr(sess->function));
	if (sess->source != NULL) {
		line = get_source_line(sess->source, sess->line - 1);
		printf("\33[30;1m%4d\33[m %s\n", sess->line, lstr_cstr(line));
	}
}

void
print_variables(session_t* sess, size_t frame)
{
	size_t      n_items;
	message_t*  request;
	message_t*  response;

	size_t i;

	request = msg_new(MSG_CLASS_REQ);
	msg_add_int(request, REQ_GET_LOCALS);
	msg_add_int(request, -(int32_t)(1 + frame));
	response = converse(sess, request);
	n_items = msg_len(response) / 2;
	for (i = 0; i < n_items; ++i) {
		printf("var \33[36;1m%s\33[m = \33[0;1m", msg_atom_string(response, i * 2));
		switch (msg_atom_type(response, i * 2 + 1)) {
		case ATOM_UNDEFINED: printf("undefined"); break;
		case ATOM_OBJECT: printf("{ ... }"); break;
		case ATOM_FLOAT:
			printf("%g", msg_atom_float(response, i * 2 + 1));
			break;
		case ATOM_INT:
			printf("%d", msg_atom_int(response, i * 2 + 1));
			break;
		case ATOM_STRING:
			printf("\"%s\"", msg_atom_string(response, i * 2 + 1));
			break;
		default:
			printf("<unknown>");
		}
		printf("\33[m\n");
	}
	msg_free(response);
}

void
set_breakpoint(session_t* sess, const char* filename, int line)
{
	message_t* request;
	message_t* response;

	request = msg_new(MSG_CLASS_REQ);
	msg_add_int(request, REQ_ADD_BREAK);
	msg_add_string(request, filename);
	msg_add_int(request, (int32_t)line);
	response = converse(sess, request);
	printf("breakpoint \33[33;1m%d\33[m set at \33[36;1m%s:%d\33[m\n",
		msg_atom_int(response, 0), filename, line);
	msg_free(response);
}

void
execute_next(session_t* sess, exec_op_t op)
{
	message_t* request;

	request = msg_new(MSG_CLASS_REQ);
	msg_add_int(request,
		op == EXEC_STEP_OVER ? REQ_STEP_OVER
		: op == EXEC_STEP_IN ? REQ_STEP_INTO
		: op == EXEC_STEP_OUT ? REQ_STEP_OUT
		: REQ_RESUME);
	msg_free(converse(sess, request));
	sess->current_frame = 0;
	sess->is_stopped = false;
}

void
run_session(session_t* sess)
{
	bool       is_active = true;
	message_t* msg;

	printf("\n");
	while (is_active) {
		if (sess->remote == NULL || sess->is_stopped)
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

static message_t*
converse(session_t* sess, message_t* msg)
{
	message_t* response = NULL;

	msg_send(sess->remote, msg);
	do {
		msg_free(response);
		if (!(response = msg_receive(sess->remote))) return NULL;
		if (msg_get_class(response) == MSG_CLASS_NFY)
			process_message(sess, response);
	} while (msg_get_class(response) == MSG_CLASS_NFY);
	return response;
}

static bool
do_command_line(session_t* sess)
{
	char*       argument;
	char*       command;
	int         ch = '\0';
	size_t      ch_idx = 0;
	char*       parsee;
	message_t*  req;

	// get a command from the user
	sess->cl_buffer[0] = '\0';
	while (sess->cl_buffer[0] == '\0') {
		printf("\n\33[36;1m%s \33[33;1mssj$\33[m ", lstr_cstr(sess->function));
		ch = getchar();
		while (ch != '\n') {
			if (ch_idx >= CL_BUFFER_SIZE - 1) {
				printf("String entered is too long to parse.");
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
	if (strcmp(command, "quit") == 0 || strcmp(command, "q") == 0) {
		req = msg_new(MSG_CLASS_REQ);
		msg_add_int(req, REQ_DETACH);
		msg_free(converse(sess, req));
		sess->is_stopped = false;
	}
	else if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0)
		print_commands(sess);
	else if (strcmp(command, "backtrace") == 0 || strcmp(command, "bt") == 0)
		print_callstack(sess, sess->current_frame, true);
	else if (strcmp(command, "break") == 0 || strcmp(command, "bp") == 0)
		set_breakpoint(sess, "scripts/main.js", 30);
	else if (strcmp(command, "continue") == 0 || strcmp(command, "c") == 0)
		execute_next(sess, EXEC_RESUME);
	else if (strcmp(command, "eval") == 0 || strcmp(command, "e") == 0)
		eval_expression(sess, argument, sess->current_frame);
	else if (strcmp(command, "frame") == 0 || strcmp(command, "f") == 0)
		print_callstack(sess, atoi(argument), false);
	else if (strcmp(command, "step") == 0 || strcmp(command, "s") == 0)
		execute_next(sess, EXEC_STEP_OVER);
	else if (strcmp(command, "stepin") == 0 || strcmp(command, "si") == 0)
		execute_next(sess, EXEC_STEP_IN);
	else if (strcmp(command, "stepout") == 0 || strcmp(command, "so") == 0)
		execute_next(sess, EXEC_STEP_OUT);
	else if (strcmp(command, "var") == 0 || strcmp(command, "v") == 0)
		print_variables(sess, sess->current_frame);
	else if (strcmp(command, "where") == 0 || strcmp(command, "w") == 0)
		print_status(sess);
	else
		printf("'%s' - command not recognized\n", command);
	free(parsee);
	return true;
}

static void
load_debug_info(session_t* sess)
{
}

static bool
process_message(session_t* sess, const message_t* msg)
{
	const char* filename;
	int32_t     flag;
	const char* function_name;
	int32_t     line_no;
	const char* text;
	bool        was_running;

	switch (msg_get_class(msg)) {
	case MSG_CLASS_NFY:
		switch (msg_atom_int(msg, 0)) {
		case NFY_STATUS:
			was_running = !sess->is_stopped;
			free_source(sess->source);
			lstr_free(sess->filename);
			lstr_free(sess->function);
			flag = msg_atom_int(msg, 1);
			function_name = msg_atom_string(msg, 3);
			sess->filename = lstr_new(msg_atom_string(msg, 2));
			sess->function = function_name[0] != '\0'
				? lstr_newf("%s()", msg_atom_string(msg, 3))
				: lstr_new("anon");
			sess->line = msg_atom_int(msg, 4);
			sess->source = load_source(sess->filename, sess->source_path);
			sess->is_stopped = flag != 0;
			if (sess->is_stopped && was_running)
				print_status(sess);
			break;
		case NFY_PRINT:
			printf("\33[36m%s\x1B[m", msg_atom_string(msg, 1));
			break;
		case NFY_ALERT:
			printf("\33[31m%s\x1B[m", msg_atom_string(msg, 1));
			break;
		case NFY_THROW:
			flag = msg_atom_int(msg, 1);
			text = msg_atom_string(msg, 2);
			filename = msg_atom_string(msg, 3);
			line_no = msg_atom_int(msg, 4);
			printf("\33[31;1m%s `%s` at %s:%d\33[m\n", flag != 0 ? "uncaught" : "threw",
				text, filename, line_no);
			break;
		case NFY_DETACHING:
			flag = msg_atom_int(msg, 1);
			if (flag == 0)
				printf("\33[0;1mminisphere detached normally.");
			else
				printf("\33[31;1mUnrecoverable error, target detached");
			printf("\33[m\n");
			return false;
		}
		break;
	}
	return true;
}
