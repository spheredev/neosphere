#include "ssj.h"
#include "session.h"
#include "source.h"

#include "message.h"
#include "sockets.h"

#define CL_BUFFER_SIZE 65536

struct frame
{
	char* function_name;
	char* filename;
	int   line_no;
};

struct session
{
	bool          is_attached;
	struct frame* backtrace;
	int           n_frames;
	char          cl_buffer[CL_BUFFER_SIZE];
	socket_t*     socket;
	int           frame_index;
	char*         function_name;
	char*         filename;
	bool          has_pc_changed;
	bool          have_debug_info;
	int           line_no;
	uint8_t       ptr_size;
	path_t*       source_path;
	bool          is_stopped;
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
	REQ_APPREQUEST = 0x23,
};

enum nfy_command
{
	NFY_STATUS = 0x01,
	NFY_PRINT = 0x02,
	NFY_ALERT = 0x03,
	NFY_LOG = 0x04,
	NFY_THROW = 0x05,
	NFY_DETACHING = 0x06,
	NFY_APPNOTIFY = 0x07,
};

enum appnotify
{
	APPNFY_TRACE,
};

enum apprequest
{
	APPREQ_NOP,
	APPREQ_SRC_PATH,
};

enum err_command
{
	ERR_UNKNOWN = 0x00,
	ERR_UNSUPPORTED = 0x01,
	ERR_TOO_MANY = 0x02,
	ERR_NOT_FOUND = 0x03,
};

static void       clear_cli_cache     (session_t* sess);
static message_t* converse            (session_t* sess, message_t* msg);
static bool       do_command_line     (session_t* sess);
static bool       parse_file_and_line (session_t* sess, const char* string, char* *out_filename, int *out_line_no);
static void       print_help          (session_t* sess);
static void       print_msg_atom      (session_t* sess, const message_t* message, size_t index, int obj_verbosity);
static bool       process_message     (session_t* sess, const message_t* msg);
static void       refresh_backtrace   (session_t* sess);

static bool
parse_handshake(socket_t* socket)
{
	static char handshake[128];

	char* next_token;
	char* token;
	char  *p_ch;

	printf("verifying... ");
	memset(handshake, 0, sizeof handshake);
	p_ch = handshake;
	do {
		socket_recv(socket, p_ch, 1);
	} while (*p_ch++ != '\n');
	*(p_ch - 1) = '\0';

	// parse handshake line
	if (!(token = strtok_r(handshake, " ", &next_token)))
		goto on_error;
	if (atoi(token) != 1) goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	printf("OK.\n");
	printf("   inferior is \33[36m%s\33[m\n", next_token);
	printf("   duktape \33[36m%s\33[m\n", token);

	return true;

on_error:
	printf("\33[31;1merror!\33[m\n");
	return false;
}

void
sessions_init(void)
{
	sockets_init();
}

void
sessions_deinit(void)
{
	sockets_deinit();
}

session_t*
new_session(const char* hostname, int port)
{
	path_t*     origin = NULL;
	message_t*  req;
	message_t*  rep;
	session_t*  session;

	session = calloc(1, sizeof(session_t));
	printf("connecting to \33[36m%s:%d\33[m... ", hostname, port);
	fflush(stdout);
	if (!(session->socket = socket_connect(hostname, port, 30.0)))
		goto on_error;
	printf("OK.\n");
	if (!parse_handshake(session->socket))
		goto on_error;
	session->is_attached = true;

	// find out where the source tree is by querying the target
	printf("locating sources... ");
	req = msg_new(MSG_TYPE_REQ);
	msg_add_int(req, REQ_APPREQUEST);
	msg_add_int(req, APPREQ_SRC_PATH);
	rep = converse(session, req);
	if (msg_get_string(rep, 0) != NULL)
		origin = path_resolve(path_new(msg_get_string(rep, 0)), NULL);
	if (origin == NULL)
		printf("\33[31;1mnone found.\33[m\n");
	else {
		printf("OK.\n");
		printf("   sources in \33[33m%s\33[m\n", path_cstr(origin));
	}
	msg_free(rep);
	session->source_path = origin;

	return session;

on_error:
	free(session);
	return NULL;
}

void
end_session(session_t* sess)
{
	clear_cli_cache(sess);
	path_free(sess->source_path);
	socket_close(sess->socket);

	free(sess);
}

void
clear_breakpoint(session_t* sess, const char* filename, int line_no)
{
	bool        is_ok = false;
	message_t*  msg;
	message_t*  msg2;
	size_t      num_breaks;

	size_t idx;

	msg = msg_new(MSG_TYPE_REQ);
	msg_add_int(msg, REQ_LIST_BREAK);
	msg = converse(sess, msg);
	num_breaks = msg_len(msg) / 2;
	for (idx = 0; idx < num_breaks; ++idx) {
		if (strcmp(filename, msg_get_string(msg, idx * 2)) == 0
		    && line_no == msg_get_int(msg, idx * 2 + 1))
		{
			msg2 = msg_new(MSG_TYPE_REQ);
			msg_add_int(msg2, REQ_DEL_BREAK);
			msg_add_int(msg2, (int)idx);
			msg_free(converse(sess, msg2));
			printf("breakpoint #%2zd cleared at \33[36;1m%s:%d\33[m.\n", idx, filename, line_no);
			is_ok = true;
		}
	}
	if (!is_ok)
		printf("no breakpoints at \33[36;1m%s:%d\33[m.\n", filename, line_no);
}

void
print_backtrace(session_t* sess, int frame_index, bool show_all)
{
	const char*   filename;
	struct frame* framedata;
	const char*   function_name;
	int           line_no;

	int i;

	refresh_backtrace(sess);
	
	if (frame_index < 0 || frame_index >= sess->n_frames)
		printf("no active frame at index %d.\n", frame_index);
	else {
		sess->frame_index = frame_index;
		for (i = 0; i < sess->n_frames; ++i) {
			filename = sess->backtrace[i].filename;
			function_name = sess->backtrace[i].function_name;
			line_no = sess->backtrace[i].line_no;
			if (i == frame_index || show_all) {
				printf("\33[33m%s\33[m #%2d: \33[36m%s\33[m at \33[36m%s:%d\33[m\n",
					i == frame_index ? ">>" : "  ", i, function_name, filename, line_no);
				if (!show_all)
					print_source(sess, filename, line_no, 1);
			}
		}
		framedata = &sess->backtrace[frame_index];
		free(sess->function_name);
		free(sess->filename);
		sess->function_name = strdup(framedata->function_name);
		sess->filename = strdup(framedata->filename);
		sess->line_no = framedata->line_no;
	}
}

void
print_breakpoints(session_t* sess)
{
	const char* filename;
	int         line_no;
	message_t*  msg;
	size_t      num_breaks;

	size_t idx;

	msg = msg_new(MSG_TYPE_REQ);
	msg_add_int(msg, REQ_LIST_BREAK);
	msg = converse(sess, msg);
	if ((num_breaks = msg_len(msg) / 2) == 0)
		printf("no active breakpoints.\n");
	for (idx = 0; idx < num_breaks; ++idx) {
		filename = msg_get_string(msg, idx * 2);
		line_no = msg_get_int(msg, idx * 2 + 1);
		printf("#%2zd: breakpoint at \33[36;1m%s:%d\33[m\n",
			idx, filename, line_no);
	}
}

void
print_eval(session_t* sess, const char* expr, int frame, bool show_metadata)
{
	message_t* req;

	req = msg_new(MSG_TYPE_REQ);
	msg_add_int(req, REQ_EVAL);
	msg_add_string(req, expr);
	msg_add_int(req, -(1 + frame));
	req = converse(sess, req);
	if (msg_get_int(req, 0) == 0)
		printf("= ");
	else
		printf("\33[31;1merror: \33[m");
	print_msg_atom(sess, req, 1, show_metadata ? 2 : 1);
	printf("\33[m\n");
	msg_free(req);
}

void
print_locals(session_t* sess, int frame)
{
	size_t      num_vars;
	message_t*  request;
	message_t*  response;

	size_t i;

	request = msg_new(MSG_TYPE_REQ);
	msg_add_int(request, REQ_GET_LOCALS);
	msg_add_int(request, -(1 + frame));
	response = converse(sess, request);
	if ((num_vars = msg_len(response) / 2) == 0)
		printf("no locals in function \33[36m%s\33[m.\n", sess->function_name);
	for (i = 0; i < num_vars; ++i) {
		printf("var \33[36m%s\33[m = ", msg_get_string(response, i * 2));
		dvalue_print(msg_get_dvalue(response, i * 2 + 1), false);
		printf("\n");
	}
	msg_free(response);
}

void
print_source(session_t* sess, const char* filename, int line_no, int window)
{
	bool        is_next_line;
	int         line_count;
	int         median;
	const char* prefix;
	source_t*   source;
	int         start, end;
	const char* text;

	int i;

	if (!(source = source_load(filename, sess->source_path)))
		printf("no source code available.\n");
	else {
		line_count = source_cloc(source);
		median = window / 2;
		start = line_no > median ? line_no - (median + 1) : 0;
		end = start + window < line_count ? start + window : line_count;
		for (i = start; i < end; ++i) {
			text = source_get_line(source, i);
			is_next_line = i == sess->line_no - 1 && strcmp(filename, sess->filename) == 0;
			prefix = is_next_line ? ">>" : "  ";
			if (window > 1)
				printf("\33[36m%s \33[30;1m%4d\33[m %s\n", prefix, i + 1, text);
			else
				printf("\33[30;1m%d:\33[m %s\n", i + 1, text);
		}
	}
}

int
set_breakpoint(session_t* sess, const char* filename, int line_no)
{
	int        index;
	message_t* msg;

	msg = msg_new(MSG_TYPE_REQ);
	msg_add_int(msg, REQ_ADD_BREAK);
	msg_add_string(msg, filename);
	msg_add_int(msg, (int32_t)line_no);
	msg = converse(sess, msg);
	index = msg_get_int(msg, 0);
	msg_free(msg);
	return index;
}

void
execute_next(session_t* sess, exec_op_t op)
{
	message_t* request;

	clear_cli_cache(sess);
	request = msg_new(MSG_TYPE_REQ);
	msg_add_int(request,
		op == EXEC_STEP_OVER ? REQ_STEP_OVER
		: op == EXEC_STEP_IN ? REQ_STEP_INTO
		: op == EXEC_STEP_OUT ? REQ_STEP_OUT
		: REQ_RESUME);
	msg_free(converse(sess, request));
	sess->frame_index = 0;
	sess->is_stopped = false;
}

void
run_session(session_t* sess)
{
	bool       is_active = true;
	message_t* msg;

	printf("\n");
	while (is_active) {
		if (sess->socket == NULL || sess->is_stopped)
			is_active &= do_command_line(sess);
		else {
			if (!(msg = msg_recv(sess->socket)))
				goto on_error;
			is_active &= process_message(sess, msg);
			msg_free(msg);
		}
	}
	return;

on_error:
	msg_free(msg);
	printf("communication error. session detached.\n");
	return;
}

static void
clear_cli_cache(session_t* sess)
{
	int i;

	free(sess->function_name);
	free(sess->filename);
	sess->function_name = NULL;
	sess->filename = NULL;
	if (sess->backtrace != NULL) {
		for (i = 0; i < sess->n_frames; ++i) {
			free(sess->backtrace[i].filename);
			free(sess->backtrace[i].function_name);
		}
		free(sess->backtrace);
		sess->backtrace = NULL;
	}
}

static message_t*
converse(session_t* sess, message_t* msg)
{
	message_t* reply = NULL;

	msg_send(msg, sess->socket);
	do {
		msg_free(reply);
		if (!(reply = msg_recv(sess->socket))) return NULL;
		if (msg_type(reply) == MSG_TYPE_NFY)
			process_message(sess, reply);
	} while (msg_type(reply) == MSG_TYPE_NFY);
	return reply;
}

static bool
do_command_line(session_t* sess)
{
	char*       argument;
	char*       command;
	int         ch = '\0';
	size_t      ch_idx = 0;
	char*       filename;
	int         frame_index;
	int         index;
	int         line_no;
	int         num_lines;
	char*       parsee;
	message_t*  req;

	if (sess->has_pc_changed) {
		sess->has_pc_changed = false;
		print_backtrace(sess, 0, false);
	}
	
	// get a command from the user
	sess->cl_buffer[0] = '\0';
	while (sess->cl_buffer[0] == '\0') {
		printf("\n\33[36;1m%s:%d %s\33[m\n\33[33;1mssj:\33[m ", sess->filename, sess->line_no,
			sess->function_name);
		ch = getchar();
		while (ch != '\n') {
			if (ch_idx >= CL_BUFFER_SIZE - 1) {
				printf("string entered is too long to parse.");
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
	command = strtok_r(parsee, " \t", &argument);
	if (strcmp(command, "quit") == 0 || strcmp(command, "q") == 0) {
		req = msg_new(MSG_TYPE_REQ);
		msg_add_int(req, REQ_DETACH);
		msg_free(converse(sess, req));
		sess->is_stopped = false;
	}
	else if (strcmp(command, "help") == 0 || strcmp(command, "h") == 0)
		print_help(sess);
	else if (strcmp(command, "backtrace") == 0 || strcmp(command, "bt") == 0)
		print_backtrace(sess, sess->frame_index, true);
	else if (strcmp(command, "up") == 0 || strcmp(command, "u") == 0) {
		if (argument[0] != '\0') frame_index = sess->frame_index + atoi(argument);
			else frame_index = sess->frame_index + 1;
		frame_index = (frame_index < sess->n_frames) ? frame_index
			: sess->n_frames - 1;
		print_backtrace(sess, frame_index, false);
	}
	else if (strcmp(command, "down") == 0 || strcmp(command, "d") == 0) {
		if (argument[0] != '\0') frame_index = sess->frame_index - atoi(argument);
			else frame_index = sess->frame_index - 1;
		frame_index = (frame_index >= 0) ? frame_index : 0;
		print_backtrace(sess, frame_index, false);
	}
	else if (strcmp(command, "breakpoint") == 0 || strcmp(command, "bp") == 0) {
		if (argument[0] == '\0')
			print_breakpoints(sess);
		else if (parse_file_and_line(sess, argument, &filename, &line_no)) {
			index = set_breakpoint(sess, filename, line_no);
			printf("breakpoint #%2d set at \33[36;1m%s:%d\33[m.\n",
				index, filename, line_no);
			print_source(sess, filename, line_no, 1);
			free(filename);
		}
	}
	else if (strcmp(command, "clearbreak") == 0 || strcmp(command, "cb") == 0) {
		if (argument[0] == '\0')
			printf("please specify location of breakpoint to clear.\n");
		else if (parse_file_and_line(sess, argument, &filename, &line_no)) {
			clear_breakpoint(sess, filename, line_no);
			free(filename);
		}
	}
	else if (strcmp(command, "continue") == 0 || strcmp(command, "c") == 0)
		execute_next(sess, EXEC_RESUME);
	else if (strcmp(command, "eval") == 0 || strcmp(command, "e") == 0)
		print_eval(sess, argument, sess->frame_index, false);
	else if (strcmp(command, "examine") == 0 || strcmp(command, "x") == 0)
		print_eval(sess, argument, sess->frame_index, true);
	else if (strcmp(command, "frame") == 0 || strcmp(command, "f") == 0)
		print_backtrace(sess, atoi(argument), false);
	else if (strcmp(command, "list") == 0 || strcmp(command, "l") == 0) {
		if (argument[0] != 0)
			num_lines = atoi(argument);
		else
			num_lines = 10;
		print_source(sess, sess->filename, sess->line_no, num_lines);
	}
	else if (strcmp(command, "step") == 0 || strcmp(command, "s") == 0)
		execute_next(sess, EXEC_STEP_OVER);
	else if (strcmp(command, "stepin") == 0 || strcmp(command, "si") == 0)
		execute_next(sess, EXEC_STEP_IN);
	else if (strcmp(command, "stepout") == 0 || strcmp(command, "so") == 0)
		execute_next(sess, EXEC_STEP_OUT);
	else if (strcmp(command, "vars") == 0 || strcmp(command, "v") == 0)
		print_locals(sess, sess->frame_index);
	else if (strcmp(command, "where") == 0 || strcmp(command, "w") == 0)
		print_backtrace(sess, 0, false);
	else
		printf("'%s': command not recognized.\n", command);
	free(parsee);
	return true;
}

static bool
parse_file_and_line(session_t* sess, const char* string, char* *out_filename, int *out_line_no)
{
	char*   next;
	char*   parsee = NULL;
	path_t* path = NULL;
	char*   token;
	
	if (strchr(string, ':') == NULL)
		goto on_error;
	parsee = strdup(string);
	token = strtok_r(parsee, ":", &next);
	path = path_rebase(path_new(token), sess->source_path);
	if (!path_resolve(path, NULL)) {
		printf("'%s': source file not found.\n", token);
		goto on_error;
	}
	*out_filename = strdup(token);
	*out_line_no = atoi(next);
	free(parsee);
	return *out_line_no > 0;

on_error:
	free(parsee);
	path_free(path);
	return false;
}

static void
print_help(session_t* sess)
{
	printf(
		"\33[0;1m"
		"SSJ Debugger Commands                                                          \n"
		"\33[m"
		"Abbreviated names are listed first, followed by the full, verbose name of each \n"
		"command. Unlike GDB, truncated names are not allowed.                          \n\n"

		" bt, backtrace    Show a list of all function calls currently on the stack     \n"
		" u,  up           Move up the call stack (outwards) from the selected frame    \n"
		" d,  down         Move down the call stack (inwards) from the selected frame   \n"
		" bp, breakpoint   Set a breakpoint at file:line (e.g. scripts/eaty-pig.js:812) \n"
		" cb, clearbreak   Clear a breakpoint set at file:line (see 'breakpoint')       \n"
		" c,  continue     Run either until a breakpoint is hit or an error is thrown   \n"
		" e,  eval         Evaluate a JavaScript expression                             \n"
		" f,  frame        Select the stack frame used for, e.g. 'eval' and 'var'       \n"
		" l,  list         Show source text around the line of code being debugged      \n"
		" s,  step         Run the next line of code                                    \n"
		" si, stepin       Run the next line of code, stepping into functions           \n"
		" so, stepout      Run until the current function call returns                  \n"
		" v,  vars         List local variables and their values in the active frame    \n"
		" w,  where        Show the filename and line number of the next line of code   \n"
		" x,  examine      Like 'eval' but shows low-level runtime metadata for objects \n"
		" h,  help         Show this list of commands                                   \n"
		" q,  quit         Detach and terminate your SSJ debugging session              \n\n"

		"Type 'help <command>' for usage of individual commands.                        \n"
		);
}

static void
print_msg_atom(session_t* sess, const message_t* message, size_t index, int obj_verbosity)
{
	unsigned int bitmask;
	int32_t      flags;
	bool         have_get;
	bool         have_set;
	dukptr_t    heapptr;
	size_t       idx;
	bool         is_accessor;
	bool         is_metadata;
	message_t*   req;
	
	if (msg_get_atom_tag(message, index) != DVALUE_OBJ || obj_verbosity <= 0)
		dvalue_print(msg_get_dvalue(message, index), obj_verbosity >= 2);
	else {
		heapptr = dvalue_as_ptr(msg_get_dvalue(message, index));
		req = msg_new(MSG_TYPE_REQ);
		msg_add_int(req, REQ_INSPECT_OBJ);
		msg_add_heapptr(req, heapptr);
		msg_add_int(req, 0x0);
		req = converse(sess, req);
		idx = 0;
		printf("\33[0;1m{\33[m\n");
		while (idx < msg_len(req)) {
			flags = msg_get_int(req, idx);
			bitmask = obj_verbosity >= 2 ? 0x0100 : 0x0300;
			if (!(flags & bitmask)) {
				is_accessor = (flags & 0x0008) != 0x0;
				is_metadata = (flags & 0x0200) != 0x0;
				printf("   %s \33[36m", is_metadata ? "meta" : "prop");
				dvalue_print(msg_get_dvalue(req, idx + 1), false);
				printf("\33[m = ");
				if (!is_accessor)
					dvalue_print(msg_get_dvalue(req, idx + 2), obj_verbosity >= 2);
				else {
					if (obj_verbosity < 2) {
						have_get = msg_get_atom_tag(req, idx + 2) != DVALUE_NULL;
						have_set = msg_get_atom_tag(req, idx + 3) != DVALUE_NULL;
						if (have_set && have_get) printf("{ get, set }");
							else if (have_set) printf("{ set }");
							else if (have_get) printf("{ get }");
					}
					else {
						printf("{ get: ");
						dvalue_print(msg_get_dvalue(req, idx + 2), obj_verbosity >= 2);
						printf(", set: ");
						dvalue_print(msg_get_dvalue(req, idx + 3), obj_verbosity >= 2);
						printf(" }");
					}
				}
				printf("\33[m\n");
			}
			idx += (flags & 0x0008) ? 4 : 3;
		}
		printf("\33[0;1m}\33[m");
		msg_free(req);
	}
}

static bool
process_message(session_t* sess, const message_t* msg)
{
	int32_t     flag;
	const char* function_name;
	bool        was_running;

	switch (msg_type(msg)) {
	case MSG_TYPE_NFY:
		switch (msg_get_int(msg, 0)) {
		case NFY_STATUS:
			was_running = !sess->is_stopped;
			free(sess->filename);
			free(sess->function_name);
			flag = msg_get_int(msg, 1);
			function_name = msg_get_string(msg, 3);
			sess->filename = strdup(msg_get_string(msg, 2));
			sess->function_name = function_name[0] != '\0'
				? strnewf("%s()", msg_get_string(msg, 3))
				: strdup("anon");
			sess->line_no = msg_get_int(msg, 4);
			sess->is_stopped = flag != 0;
			if (!sess->is_stopped) clear_cli_cache(sess);
			if (sess->is_stopped && was_running)
				sess->has_pc_changed = true;
			break;
		case NFY_PRINT:
			printf("\33[35mprint:\33[m %s", msg_get_string(msg, 1));
			break;
		case NFY_ALERT:
			printf("\33[33malert:\33[m %s", msg_get_string(msg, 1));
			break;
		case NFY_LOG:
			printf("\33[32mlog:\33[m %s", msg_get_string(msg, 1));
			break;
		case NFY_THROW:
			if ((flag = msg_get_int(msg, 1)) == 0)
				break;
			printf("\33[31;1mFATAL:\33[0;1m %s\33[m ", msg_get_string(msg, 2));
			printf("[at \33[36;1m%s:%d\33[m]\n", msg_get_string(msg, 3), msg_get_int(msg, 4));
			break;
		case NFY_APPNOTIFY:
			switch (msg_get_int(msg, 1)) {
			case APPNFY_TRACE:
				printf("\33[36mtrace:\33[m %s", msg_get_string(msg, 2));
				break;
			}
			break;
		case NFY_DETACHING:
			sess->is_attached = false;
			flag = msg_get_int(msg, 1);
			if (flag == 0)
				printf("\33[0;1mSSJ session has been detached.");
			else
				printf("\33[31;1mSSJ detached due to an error in inferior.");
			printf("\33[m\n");
			return false;
		}
		break;
	}
	return true;
}

static void
refresh_backtrace(session_t* sess)
{
	struct frame* frame;
	const char*   function_name;
	message_t*    msg;

	int i;

	if (sess->backtrace != NULL)
		return;

	msg = msg_new(MSG_TYPE_REQ);
	msg_add_int(msg, REQ_GET_CALLSTACK);
	msg = converse(sess, msg);
	sess->n_frames = (int)(msg_len(msg) / 4);
	sess->backtrace = calloc(sess->n_frames, sizeof(struct frame));
	for (i = 0; i < sess->n_frames; ++i) {
		frame = sess->backtrace + i;
		function_name = msg_get_string(msg, i * 4 + 1);
		frame->filename = strdup(msg_get_string(msg, i * 4));
		frame->function_name = function_name[0] != '\0' ? strnewf("%s()", function_name)
			: strdup("anon");
		frame->line_no = msg_get_int(msg, i * 4 + 2);
	}
}
