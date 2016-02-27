#include "ssj.h"
#include "session.h"

#include "help.h"
#include "inferior.h"
#include "parser.h"

struct session
{
	int         frame;
	inferior_t* inferior;
};

const char* const
command_db[] =
{
	"backtrace",  "bt", "",
	"breakpoint", "bp", "~f",
	"clearbp",    "cb", "n",
	"continue",   "c",  "",
	"down",       "d",  "~n",
	"eval",       "e",  "s",
	"examine",    "x",  "s",
	"frame",      "f",  "~n",
	"list",       "l",  "~nf",
	"stepover",   "s",  "",
	"stepin",     "si", "",
	"stepout",    "so", "",
	"up",         "u",  "~n",
	"vars",       "v",  "",
	"where",      "w",  "",
	"quit",       "q",  "",
	"help",       "h",  "~s",
};

static const char* find_verb        (command_t* cmd);
static void        handle_backtrace (session_t* o, command_t* cmd);
static void        handle_eval      (session_t* o, command_t* cmd, bool is_verbose);
static void        handle_frame     (session_t* o, command_t* cmd);
static void        handle_list      (session_t* o, command_t* cmd);
static void        handle_up_down   (session_t* o, command_t* cmd, int direction);
static void        handle_where     (session_t* o, command_t* cmd);
static void        handle_quit      (session_t* o, command_t* cmd);
static void        preview_frame    (session_t* o, int frame);
static bool        validate_args    (const command_t* this, const char* verb_name, const char* pattern);

session_t*
session_new(inferior_t* inferior)
{
	session_t* o;

	o = calloc(1, sizeof(session_t));
	o->inferior = inferior;
	return o;
}

void
session_free(session_t* o)
{
	free(o);
}

void
session_run(session_t* o)
{
	char               buffer[4096];
	char               ch;
	command_t*         command;
	const char*        filename;
	const char*        function_name;
	int                idx;
	int                line_no;
	int                num_args;
	const backtrace_t* stack;
	const char*        verb;
	
	while (inferior_is_attached(o->inferior)) {
		while (inferior_is_running(o->inferior))
			inferior_update(o->inferior);
		if (!inferior_is_attached(o->inferior))
			break;
		stack = inferior_get_stack(o->inferior);
		
		function_name = backtrace_get_name(stack, o->frame);
		filename = backtrace_get_filename(stack, o->frame);
		line_no = backtrace_get_lineno(stack, o->frame);
		if (line_no != 0)
			printf("\n\33[36;1m%s:%d %s\33[m\n\33[33;1m(ssj)\33[m ", filename, line_no, function_name);
		else
			printf("\n\33[36;1msyscall %s\33[m\n\33[33;1m(ssj)\33[m ", function_name);
		idx = 0;
		ch = getchar();
		while (ch != '\n') {
			if (idx >= 4095) {
				printf("string is too long to parse.\n");
				buffer[0] = '\0';
				break;
			}
			buffer[idx++] = ch;
			ch = getchar();
		}
		buffer[idx] = '\0';

		command = command_parse(buffer);
		verb = find_verb(command);
		num_args = command_len(command) - 1;
		if (strcmp(verb, "quit") == 0)
			handle_quit(o, command);
		else if (strcmp(verb, "help") == 0)
			help_print(num_args > 0 ? command_get_string(command, 1) : NULL);
		else if (strcmp(verb, "backtrace") == 0)
			handle_backtrace(o, command);
		else if (strcmp(verb, "up") == 0)
			handle_up_down(o, command, +1);
		else if (strcmp(verb, "down") == 0)
			handle_up_down(o, command, -1);
		else if (strcmp(verb, "continue") == 0)
			inferior_resume(o->inferior, OP_RESUME);
		else if (strcmp(verb, "eval") == 0)
			handle_eval(o, command, false);
		else if (strcmp(verb, "examine") == 0)
			handle_eval(o, command, true);
		else if (strcmp(verb, "frame") == 0)
			handle_frame(o, command);
		else if (strcmp(verb, "list") == 0)
			handle_list(o, command);
		else if (strcmp(verb, "stepover") == 0)
			inferior_resume(o->inferior, OP_STEP_OVER);
		else if (strcmp(verb, "stepin") == 0)
			inferior_resume(o->inferior, OP_STEP_IN);
		else if (strcmp(verb, "stepout") == 0)
			inferior_resume(o->inferior, OP_STEP_OUT);
		else if (strcmp(verb, "where") == 0)
			handle_where(o, command);
		else
			printf("'%s': not implemented.\n", verb);
		command_free(command);
	}
	printf("SSJ session terminated.\n");
}

static const char*
find_verb(command_t* cmd)
{
	const char* full_name;
	const char* matches[100];
	int         num_commands;
	int         num_matches = 0;
	const char* pattern;
	const char* short_name;
	const char* verb;

	int i;

	if (command_len(cmd) < 1)
		return NULL;

	num_commands = sizeof(command_db) / sizeof(command_db[0]) / 3;
	verb = command_get_string(cmd, 0);
	for (i = 0; i < num_commands; ++i) {
		full_name = command_db[0 + i * 3];
		short_name = command_db[1 + i * 3];
		if (strcmp(verb, short_name) == 0) {
			matches[0] = full_name;
			pattern = command_db[2 + i * 3];
			num_matches = 1;  // canonical short name is never ambiguous
			break;
		}
		if (strstr(full_name, verb) == full_name) {
			matches[num_matches] = full_name;
			if (num_matches == 0)
				pattern = command_db[2 + i * 3];
			++num_matches;
		}
	}

	if (num_matches == 1)
		return validate_args(cmd, matches[0], pattern) ? matches[0] : NULL;
	else if (num_matches > 1) {
		printf("'%s': abbreviated name is ambiguous between:\n", verb);
		for (i = 0; i < num_matches; ++i)
			printf("    * %s\n", matches[i]);
		return NULL;
	}
	else {
		printf("'%s': unrecognized command name.\n", verb);
		return NULL;
	}
}

static void
handle_backtrace(session_t* o, command_t* cmd)
{
	const backtrace_t* stack;
	
	if (!(stack = inferior_get_stack(o->inferior)))
		return;
	backtrace_print(stack, o->frame, true);
}

static void
handle_eval(session_t* o, command_t* cmd, bool is_verbose)
{
	const char*     expr;
	const dvalue_t* getter;
	dukptr_t        heapptr;
	bool            is_accessor;
	bool            is_error;
	message_t*      msg;
	int             prop_flags;
	const dvalue_t* prop_key;
	const dvalue_t* setter;
	dvalue_t*       result;
	
	int idx = 0;

	expr = command_get_string(cmd, 1);
	result = inferior_eval(o->inferior, expr, o->frame, &is_error);
	printf(is_error ? "error: " : "= ");
	if (dvalue_tag(result) != DVALUE_OBJ)
		dvalue_print(result, is_verbose);
	else {
		heapptr = dvalue_as_ptr(result);
		msg = message_new(MESSAGE_REQ);
		message_add_int(msg, REQ_INSPECT_PROPS);
		message_add_heapptr(msg, heapptr);
		message_add_int(msg, 0);
		message_add_int(msg, INT_MAX);
		if (!(msg = inferior_request(o->inferior, msg)))
			return;
		printf("{\n");
		while (idx < message_len(msg)) {
			prop_flags = message_get_int(msg, idx++);
			prop_key = message_get_dvalue(msg, idx++);
			is_accessor = (prop_flags & 0x008) != 0;
			if (prop_flags & 0x100) {
				idx += is_accessor ? 4 : 3;
				continue;
			}
			printf("    prop ");
			dvalue_print(prop_key, false);
			printf(" = ");
			if (!is_accessor)
				dvalue_print(message_get_dvalue(msg, idx++), is_verbose);
			else {
				getter = message_get_dvalue(msg, idx++);
				setter = message_get_dvalue(msg, idx++);
				printf("{ get: ");
				dvalue_print(getter, is_verbose);
				printf(", set: ");
				dvalue_print(setter, is_verbose);
				printf(" }");
			}
			printf("\n");
		}
		printf("}");
	}
	printf("\n");
	dvalue_free(result);
}

static void
handle_frame(session_t* o, command_t* cmd)
{
	int                frame;
	const backtrace_t* stack;

	if (!(stack = inferior_get_stack(o->inferior)))
		return;
	frame = o->frame;
	if (command_len(cmd) >= 1)
		frame = command_get_int(cmd, 1);
	if (frame < 0 || frame >= backtrace_len(stack))
		printf("stack frame #%2d doesn't exist.\n", frame);
	else {
		o->frame = frame;
		preview_frame(o, o->frame);
	}
}

static void
handle_list(session_t* o, command_t* cmd)
{
	const char*        active_filename;
	int                active_lineno = 0;
	const char*        filename;
	int                lineno;
	int                num_lines = 10;
	const source_t*    source;
	const backtrace_t* stack;

	stack = inferior_get_stack(o->inferior);
	active_filename = backtrace_get_filename(stack, o->frame);
	active_lineno = backtrace_get_lineno(stack, o->frame);
	filename = active_filename;
	lineno = active_lineno;
	if (command_len(cmd) >= 2)
		num_lines = command_get_int(cmd, 1);
	if (command_len(cmd) >= 3) {
		filename = command_get_string(cmd, 2);
		lineno = command_get_int(cmd, 2);
	}
	if (!(source = inferior_get_source(o->inferior, filename)))
		printf("source code unavailable for %s", filename);
	else {
		if (strcmp(filename, active_filename) != 0)
			active_lineno = 0;
		source_print(source, lineno, num_lines, active_lineno);
	}
}

static void
handle_up_down(session_t* o, command_t* cmd, int direction)
{
	int                new_frame;
	int                num_steps;
	const backtrace_t* stack;

	if (!(stack = inferior_get_stack(o->inferior)))
		return;
	new_frame = o->frame + direction;
	if (new_frame >= backtrace_len(stack))
		printf("can't go up any further.\n");
	else if (new_frame < 0)
		printf("can't go down any further.\n");
	else {
		num_steps = command_len(cmd) >= 1 ? command_get_int(cmd, 1) : 1;
		new_frame = o->frame + num_steps * direction;
		o->frame = new_frame < 0 ? 0
			: new_frame >= backtrace_len(stack) ? backtrace_len(stack) - 1
			: new_frame;
		preview_frame(o, o->frame);
	}
}

static void
handle_where(session_t* o, command_t* cmd)
{
	int                frame = 0;
	const backtrace_t* stack;

	if (!(stack = inferior_get_stack(o->inferior)))
		return;
	while (backtrace_get_lineno(stack, frame) <= 0)
		++frame;
	preview_frame(o, o->frame);
}

static void
handle_quit(session_t* o, command_t* cmd)
{
	inferior_detach(o->inferior);
}

static void
preview_frame(session_t* o, int frame)
{
	const char*        filename;
	int                lineno;
	const source_t*    source;
	const backtrace_t* stack;

	if (!(stack = inferior_get_stack(o->inferior)))
		return;
	backtrace_print(stack, frame, false);
	filename = backtrace_get_filename(stack, frame);
	lineno = backtrace_get_lineno(stack, frame);
	if (lineno == 0)
		printf("no source provided for system call\n");
	else {
		if (!(source = inferior_get_source(o->inferior, filename)))
			printf("source is unavailable for %s\n", filename);
		else
			source_print(source, lineno, 1, lineno);
	}
}

static bool
validate_args(const command_t* this, const char* verb_name, const char* pattern)
{
	int         index = 0;
	int         want_num_args;
	token_tag_t want_tag;
	const char* want_type;
	const char  *p_type;

	if (strchr(pattern, '~'))
		want_num_args = (int)(strchr(pattern, '~') - pattern);
	else
		want_num_args = (int)strlen(pattern);
	if (command_len(this) - 1 < want_num_args) {
		printf("'%s': expected at least %d arguments.\n", verb_name, want_num_args);
		return false;
	}
	p_type = pattern;
	while (index < command_len(this) - 1) {
		if (*p_type == '~') ++p_type;
		if (*p_type == '\0') break;
		switch (*p_type) {
		case 's':
			want_tag = TOK_STRING;
			want_type = "string";
			break;
		case 'n':
			want_tag = TOK_NUMBER;
			want_type = "number";
			break;
		case 'f':
			want_tag = TOK_FILE_LINE;
			want_type = "file:line";
			break;
		}
		if (command_get_tag(this, index + 1) != want_tag)
			goto wrong_type;
		++p_type;
		++index;
	}
	return true;

wrong_type:
	printf("'%s': expected a %s for argument %d.\n", verb_name, want_type, index + 1);
	return false;
}
