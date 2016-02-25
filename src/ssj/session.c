#include "ssj.h"
#include "session.h"

#include "commands.h"
#include "help.h"
#include "message.h"
#include "parser.h"
#include "sockets.h"
#include "source.h"

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
	char*         game_title;
	char*         game_author;
	int           num_frames;
	socket_t*     socket;
	int           frame_index;
	char*         function_name;
	char*         filename;
	bool          has_pc_changed;
	bool          have_debug_info;
	int           line_no;
	uint8_t       ptr_size;
	bool          is_stopped;
};

enum err_command
{
	ERR_UNKNOWN = 0x00,
	ERR_UNSUPPORTED = 0x01,
	ERR_TOO_MANY = 0x02,
	ERR_NOT_FOUND = 0x03,
};

static void       clear_cli_cache     (session_t* sess);
static bool       do_command_line     (session_t* sess);
static void       print_msg_atom      (session_t* sess, const message_t* message, size_t index, int obj_verbosity);
static bool       process_message     (session_t* sess, const message_t* msg);
static void       refresh_backtrace   (session_t* sess);

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

message_t*
do_request(session_t* sess, message_t* msg)
{
	message_t* reply = NULL;

	msg_send(msg, sess->socket);
	do {
		msg_free(reply);
		if (!(reply = msg_recv(sess->socket))) return NULL;
		if (msg_type(reply) == MSG_TYPE_NFY)
			process_message(sess, reply);
	} while (msg_type(reply) == MSG_TYPE_NFY);
	msg_free(msg);
	return reply;
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
	message_t*  req;
	message_t*  rep;
	session_t*  session;

	session = calloc(1, sizeof(session_t));
	printf("connecting to %s:%d... ", hostname, port);
	fflush(stdout);
	if (!(session->socket = socket_connect(hostname, port, 30.0)))
		goto on_error;
	printf("OK.\n");
	if (!do_handshake(session->socket))
		goto on_error;
	session->is_attached = true;

	printf("querying target... ");
	req = msg_new(MSG_TYPE_REQ);
	msg_add_int(req, REQ_APP_REQUEST);
	msg_add_int(req, APPREQ_GAME_INFO);
	rep = do_request(session, req);
	session->game_title = strdup(msg_get_string(rep, 0));
	session->game_author = strdup(msg_get_string(rep, 1));
	msg_free(rep);
	printf("OK.\n");

	printf("    game title: %s\n", session->game_title);
	printf("    author: %s\n", session->game_author);

	return session;

on_error:
	free(session);
	return NULL;
}

void
end_session(session_t* sess)
{
	clear_cli_cache(sess);
	socket_close(sess->socket);
	free(sess->game_title);
	free(sess->game_author);

	free(sess);
}

void
clear_breakpoint(session_t* sess, int index)
{
	bool        is_ok = false;
	message_t*  msg;
	message_t*  msg2;
	int         num_breaks;

	msg = msg_new(MSG_TYPE_REQ);
	msg_add_int(msg, REQ_LIST_BREAK);
	msg = do_request(sess, msg);
	num_breaks = (int)msg_len(msg) / 2;
	msg_free(msg);
	if (index >= num_breaks)
		printf("#%2d: breakpoint index out of range.\n", index);
	else {
		msg2 = msg_new(MSG_TYPE_REQ);
		msg_add_int(msg2, REQ_DEL_BREAK);
		msg_add_int(msg2, index);
		msg_free(do_request(sess, msg2));
		printf("breakpoint #%2d cleared.\n", index);
	}
}

void
print_backtrace(session_t* sess, int frame_index, bool select_frame, bool show_all)
{
	const char*   filename;
	struct frame* framedata;
	const char*   function_name;
	int           line_no;

	int i;

	refresh_backtrace(sess);
	
	if (frame_index < 0 || frame_index >= sess->num_frames)
		printf("no active frame at index %d.\n", frame_index);
	else {
		for (i = 0; i < sess->num_frames; ++i) {
			filename = sess->backtrace[i].filename;
			function_name = sess->backtrace[i].function_name;
			line_no = sess->backtrace[i].line_no;
			if (i == frame_index || show_all) {
				if (line_no > 0)
					printf("%s #%2d: %s at %s:%d\n", i == sess->frame_index ? "=>" : "  ", i, function_name, filename, line_no);
				else
					printf("%s #%2d: %s <system call>\n", i == sess->frame_index ? "=>" : "  ", i, function_name);
				if (!show_all)
					print_source(sess, filename, line_no, 1);
			}
		}
		if (select_frame) {
			framedata = &sess->backtrace[frame_index];
			free(sess->function_name);
			free(sess->filename);
			sess->frame_index = frame_index;
			sess->function_name = strdup(framedata->function_name);
			sess->filename = strdup(framedata->filename);
			sess->line_no = framedata->line_no;
		}
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
	msg = do_request(sess, msg);
	if ((num_breaks = msg_len(msg) / 2) == 0)
		printf("no active breakpoints.\n");
	for (idx = 0; idx < num_breaks; ++idx) {
		filename = msg_get_string(msg, idx * 2);
		line_no = msg_get_int(msg, idx * 2 + 1);
		printf("#%2zd: breakpoint at %s:%d\n",
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
	req = do_request(sess, req);
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
	response = do_request(sess, request);
	if ((num_vars = msg_len(response) / 2) == 0)
		printf("no locals in function %s.\n", sess->function_name);
	for (i = 0; i < num_vars; ++i) {
		printf("var %s = ", msg_get_string(response, i * 2));
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

	if (line_no <= 0)
		printf("no source code for system call.\n");
	else if (!(source = source_load(sess, filename)))
		printf("source code unavailable for %s.\n", filename);
	else {
		line_count = source_cloc(source);
		median = window / 2;
		start = line_no > median ? line_no - (median + 1) : 0;
		end = start + window < line_count ? start + window : line_count;
		for (i = start; i < end; ++i) {
			text = source_get_line(source, i);
			is_next_line = i == sess->line_no - 1 && strcmp(filename, sess->filename) == 0;
			prefix = is_next_line ? "=>" : "  ";
			if (window > 1)
				printf("%s %4d: %s\n", prefix, i + 1, text);
			else
				printf("%d: %s\n", i + 1, text);
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
	msg = do_request(sess, msg);
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
	msg_free(do_request(sess, request));
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
		for (i = 0; i < sess->num_frames; ++i) {
			free(sess->backtrace[i].filename);
			free(sess->backtrace[i].function_name);
		}
		free(sess->backtrace);
		sess->backtrace = NULL;
	}
}

static bool
do_command_line(session_t* sess)
{
	static char buffer[CL_BUFFER_SIZE];
	int         ch = '\0';
	size_t      ch_idx = 0;
	command_t*  command;
	const char* filename;
	int         frame_index;
	int         index;
	int         line_no;
	int         num_args;
	int         num_lines;
	message_t*  req;
	const char* verb = NULL;

	if (sess->has_pc_changed) {
		sess->has_pc_changed = false;
		refresh_backtrace(sess);
		frame_index = 0;
		while (sess->backtrace[frame_index].line_no == 0)
			++frame_index;
		print_backtrace(sess, frame_index, true, false);
	}

	// get a command from the user
	while (verb == NULL) {
		if (sess->line_no != 0)
			printf("\n\33[36;1m%s:%d %s\33[m\n\33[33;1m(ssj)\33[m ", sess->filename, sess->line_no, sess->function_name);
		else
			printf("\n\33[36;1msyscall %s\33[m\n\33[33;1m(ssj)\33[m ", sess->function_name);
		ch_idx = 0;
		ch = getchar();
		while (ch != '\n') {
			if (ch_idx >= CL_BUFFER_SIZE - 1) {
				printf("string is too long to parse.");
				buffer[0] = '\0';
				break;
			}
			buffer[ch_idx++] = ch;
			ch = getchar();
		}
		buffer[ch_idx] = '\0';
		if ((command = command_parse(buffer)))
			verb = find_ssj_command(command);
	}
	
	num_args = command_size(command) - 1;
	if (strcmp(verb, "quit") == 0) {
		req = msg_new(MSG_TYPE_REQ);
		msg_add_int(req, REQ_DETACH);
		msg_free(do_request(sess, req));
		sess->is_stopped = false;
	}
	else if (strcmp(verb, "help") == 0)
		print_help(num_args > 0 ? command_get_string(command, 1) : NULL);
	else if (strcmp(verb, "backtrace") == 0)
		print_backtrace(sess, sess->frame_index, false, true);
	else if (strcmp(verb, "up") == 0) {
		if (sess->frame_index >= sess->num_frames - 1)
			printf("can't go up any further.\n");
		else {
			if (num_args >= 1)
				frame_index = sess->frame_index + command_get_int(command, 1);
			else
				frame_index = sess->frame_index + 1;
			frame_index = (frame_index < sess->num_frames) ? frame_index
				: sess->num_frames - 1;
			print_backtrace(sess, frame_index, true, false);
		}
	}
	else if (strcmp(verb, "down") == 0) {
		if (sess->frame_index <= 0)
			printf("can't go down any further.\n");
		else {
			if (num_args >= 1)
				frame_index = sess->frame_index - command_get_int(command, 1);
			else
				frame_index = sess->frame_index - 1;
			frame_index = (frame_index >= 0) ? frame_index : 0;
			print_backtrace(sess, frame_index, true, false);
		}
	}
	else if (strcmp(verb, "breakpoint") == 0 || strcmp(verb, "bp") == 0) {
		if (num_args == 0)
			print_breakpoints(sess);
		else {
			filename = command_get_string(command, 1);
			line_no = command_get_int(command, 1);
			index = set_breakpoint(sess, filename, line_no);
			printf("breakpoint #%2d set at %s:%d.\n",
				index, filename, line_no);
			print_source(sess, filename, line_no, 1);
		}
	}
	else if (strcmp(verb, "clearbp") == 0)
		clear_breakpoint(sess, command_get_int(command, 1));
	else if (strcmp(verb, "continue") == 0)
		execute_next(sess, EXEC_RESUME);
	else if (strcmp(verb, "eval") == 0)
		print_eval(sess, num_args > 0 ? command_get_string(command, 1) : "",
			sess->frame_index, false);
	else if (strcmp(verb, "examine") == 0)
		print_eval(sess, num_args > 0 ? command_get_string(command, 1) : "",
			sess->frame_index, true);
	else if (strcmp(verb, "frame") == 0) {
		if (num_args >= 1)
			print_backtrace(sess, command_get_int(command, 1), true, false);
		else
			print_backtrace(sess, sess->frame_index, false, false);
	}
	else if (strcmp(verb, "list") == 0) {
		num_lines = 10;
		filename = sess->filename;
		line_no = sess->line_no;
		if (num_args >= 1)
			num_lines = command_get_int(command, 1);
		if (num_args >= 2) {
			filename = command_get_string(command, 2);
			line_no = command_get_int(command, 2);
		}
		print_source(sess, filename, line_no, num_lines);
	}
	else if (strcmp(verb, "stepover") == 0)
		execute_next(sess, EXEC_STEP_OVER);
	else if (strcmp(verb, "stepin") == 0)
		execute_next(sess, EXEC_STEP_IN);
	else if (strcmp(verb, "stepout") == 0)
		execute_next(sess, EXEC_STEP_OUT);
	else if (strcmp(verb, "vars") == 0)
		print_locals(sess, sess->frame_index);
	else if (strcmp(verb, "where") == 0 || strcmp(verb, "w") == 0) {
		frame_index = 0;
		while (sess->backtrace[frame_index].line_no == 0)
			++frame_index;
		print_backtrace(sess, frame_index, false, false);
	}
	else
		printf("'%s': internal error.\n", verb);
	command_free(command);
	return true;
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
		msg_add_int(req, 0x03);
		req = do_request(sess, req);
		idx = 0;
		printf("{\n");
		while (idx < msg_len(req)) {
			flags = msg_get_int(req, idx);
			bitmask = obj_verbosity >= 2 ? 0x0100 : 0x0300;
			if (!(flags & bitmask)) {
				is_accessor = (flags & 0x0008) != 0x0;
				is_metadata = (flags & 0x0200) != 0x0;
				printf("    %s ", is_metadata ? "rtdata" : "prop");
				dvalue_print(msg_get_dvalue(req, idx + 1), false);
				printf(" = ");
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
				printf("\n");
			}
			idx += (flags & 0x0008) ? 4 : 3;
		}
		printf("}");
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
				: strdup("[anon]");
			sess->line_no = msg_get_int(msg, 4);
			sess->is_stopped = flag != 0;
			if (!sess->is_stopped) clear_cli_cache(sess);
			if (sess->is_stopped && was_running)
				sess->has_pc_changed = true;
			break;
		case NFY_PRINT:
			printf("p: %s", msg_get_string(msg, 1));
			break;
		case NFY_ALERT:
			printf("a: %s", msg_get_string(msg, 1));
			break;
		case NFY_LOG:
			printf("l: %s", msg_get_string(msg, 1));
			break;
		case NFY_THROW:
			if ((flag = msg_get_int(msg, 1)) == 0)
				break;
			printf("FATAL UNCAUGHT - %s\n", msg_get_string(msg, 2));
			refresh_backtrace(sess);
			printf("    at: %s:%d\n", msg_get_string(msg, 3), msg_get_int(msg, 4));
			break;
		case NFY_APP_NOTIFY:
			switch (msg_get_int(msg, 1)) {
			case APPNFY_DEBUGPRINT:
				printf("t: %s", msg_get_string(msg, 2));
				break;
			}
			break;
		case NFY_DETACHING:
			sess->is_attached = false;
			flag = msg_get_int(msg, 1);
			if (flag != 0)
				printf("unexpected error in inferior!\n");
			printf("SSJ session closed by target.\n");
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
	msg = do_request(sess, msg);
	sess->num_frames = (int)(msg_len(msg) / 4);
	sess->backtrace = calloc(sess->num_frames, sizeof(struct frame));
	for (i = 0; i < sess->num_frames; ++i) {
		frame = sess->backtrace + i;
		function_name = msg_get_string(msg, i * 4 + 1);
		frame->filename = strdup(msg_get_string(msg, i * 4));
		frame->function_name = function_name[0] != '\0' ? strnewf("%s()", function_name)
			: strdup("[anon]");
		frame->line_no = msg_get_int(msg, i * 4 + 2);
	}
	msg_free(msg);
}
