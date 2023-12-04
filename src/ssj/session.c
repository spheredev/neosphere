/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "ssj.h"
#include "session.h"

#include "help.h"
#include "inferior.h"
#include "parser.h"

enum auto_action
{
	AUTO_NONE,
	AUTO_LIST,
	AUTO_CONTINUE,
	AUTO_STEP_IN,
	AUTO_STEP_OUT,
	AUTO_STEP_OVER,
	AUTO_UP_DOWN,
};

struct breakpoint
{
	int   handle;
	char* filename;
	int   linenum;
};

struct session
{
	enum auto_action   auto_action;
	struct breakpoint* breaks;
	int                frame;
	inferior_t*        inferior;
	int                list_num_lines;
	char*              list_filename;
	int                list_linenum;
	int                num_breaks;
	int                up_down_direction;
};

const char* const
command_db[] =
{
	"backtrace",  "bt", "",
	"breakpoint", "bp", "~f",
	"clear",      "cb", "n",
	"continue",   "c",  "",
	"down",       "d",  "~n",
	"eval",       "e",  "*",
	"examine",    "x",  "*",
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

static void        autoselect_frame  (session_t* session);
static void        clear_auto_action (session_t* session);
static void        do_command_line   (session_t* session);
static const char* find_verb         (const char* abbrev, const char* *o_pattern);
static const char* resolve_command   (command_t* cmd);
static void        handle_backtrace  (session_t* session, command_t* cmd);
static void        handle_breakpoint (session_t* session, command_t* cmd);
static void        handle_clear      (session_t* session, command_t* cmd);
static void        handle_eval       (session_t* session, command_t* cmd, bool is_verbose);
static void        handle_frame      (session_t* session, command_t* cmd);
static void        handle_help       (session_t* session, command_t* cmd);
static void        handle_list       (session_t* session, command_t* cmd);
static void        handle_resume     (session_t* session, command_t* cmd, resume_op_t op);
static void        handle_up_down    (session_t* session, command_t* cmd, int direction);
static void        handle_vars       (session_t* session, command_t* cmd);
static void        handle_where      (session_t* session, command_t* cmd);
static void        handle_quit       (session_t* session, command_t* cmd);
static void        preview_frame     (session_t* session, int frame);
static bool        validate_args     (const command_t* command, const char* verb_name, const char* pattern);

session_t*
session_new(inferior_t* inferior)
{
	session_t* session;

	if (!(session = calloc(1, sizeof(session_t))))
		return NULL;
	session->inferior = inferior;
	return session;
}

void
session_free(session_t* it)
{
	free(it);
}

void
session_run(session_t* it, bool run_now)
{
	if (run_now)
		inferior_resume(it->inferior, OP_RESUME);
	if (inferior_attached(it->inferior)) {
		autoselect_frame(it);
		preview_frame(it, it->frame);
	}

	while (inferior_attached(it->inferior))
		do_command_line(it);
	printf("the SSj debugger has been detached.\n");
}

static const char*
find_verb(const char* abbrev, const char* *out_pattern)
{
	const char* full_name;
	const char* matches[100];
	int         num_commands;
	int         num_matches = 0;
	const char* short_name;

	int i;

	num_commands = sizeof(command_db) / sizeof(command_db[0]) / 3;
	for (i = 0; i < num_commands; ++i) {
		full_name = command_db[0 + i * 3];
		short_name = command_db[1 + i * 3];
		if (strcmp(abbrev, short_name) == 0) {
			matches[0] = full_name;
			num_matches = 1;
			if (out_pattern != NULL)
				*out_pattern = command_db[2 + i * 3];
			break;  // canonical short name is never ambiguous
		}
		if (strstr(full_name, abbrev) == full_name) {
			matches[num_matches] = full_name;
			if (num_matches == 0 && out_pattern != NULL)
				*out_pattern = command_db[2 + i * 3];
			++num_matches;
		}
	}

	if (num_matches == 1)
		return matches[0];
	else if (num_matches > 1) {
		printf("command '%s' is ambiguous.  did you want:\n", abbrev);
		for (i = 0; i < num_matches; ++i)
			printf("  - %s\n", matches[i]);
		return NULL;
	}
	else {
		printf("'%s': unrecognized command name.\n", abbrev);
		return NULL;
	}
}

static void
autoselect_frame(session_t* session)
{
	const backtrace_t* calls;

	calls = inferior_get_calls(session->inferior);
	session->frame = 0;
	while (backtrace_get_linenum(calls, session->frame) == 0)
		++session->frame;
}

static void
clear_auto_action(session_t* session)
{
	switch (session->auto_action) {
	case AUTO_LIST:
		free(session->list_filename);
		break;
	}
	session->auto_action = AUTO_NONE;
}

static void
do_command_line(session_t* session)
{
	char               buffer[4096];
	const backtrace_t* calls;
	char               ch;
	command_t*         command;
	const char*        filename;
	const char*        function_name;
	int                line_no;
	char*              synth;
	const char*        verb;

	int idx;

	calls = inferior_get_calls(session->inferior);
	function_name = backtrace_get_call_name(calls, session->frame);
	filename = backtrace_get_filename(calls, session->frame);
	line_no = backtrace_get_linenum(calls, session->frame);
	if (line_no != 0)
		printf("\n\33[36;1m%s:%d %s\33[m\n\33[33;1mSSj:\33[m ", filename, line_no, function_name);
	else
		printf("\n\33[36;1msyscall %s\33[m\n\33[33;1mSSj:\33[m ", function_name);
	idx = 0;
	while ((ch = getchar()) != '\n') {
		if (idx >= sizeof buffer - 1) {
			printf("string is too long to parse.\n");
			buffer[0] = '\0';
			while (getchar() != '\n');
			break;
		}
		buffer[idx++] = ch;
	}
	buffer[idx] = '\0';
	if (!(command = command_parse(buffer)))
		goto finished;

	// if the command line is empty, this is a cue from the user that we should
	// repeat the last command.  the implementation of this is very hacky and would benefit
	// from some refactoring in the future.
	if (command_len(command) == 0) {
		command_free(command);
		switch (session->auto_action) {
		case AUTO_CONTINUE:
			command = command_parse("continue");
			break;
		case AUTO_LIST:
			synth = strnewf("list %d \"%s\":%d", session->list_num_lines, session->list_filename, session->list_linenum);
			command = command_parse(synth);
			free(synth);
			break;
		case AUTO_STEP_IN:
			command = command_parse("stepin");
			break;
		case AUTO_STEP_OUT:
			command = command_parse("stepout");
			break;
		case AUTO_STEP_OVER:
			command = command_parse("stepover");
			break;
		case AUTO_UP_DOWN:
			if (session->up_down_direction > 0)
				command = command_parse("up");
			else if (session->up_down_direction < 0)
				command = command_parse("down");
			else  // crash prevention
				command = command_parse("frame");
			break;
		default:
			printf("nothing to repeat, please enter a valid SSj command.\n");
			printf("type 'help' to see a list of usable commands.\n");
			command = NULL;
			goto finished;
		}
	}

	if (!(verb = resolve_command(command)))
		goto finished;

	// figure out which handler to run based on the command name. this could
	// probably be refactored to get rid of the massive if/elseif tower, but for
	// now it serves its purpose.
	clear_auto_action(session);
	if (strcmp(verb, "quit") == 0)
		handle_quit(session, command);
	else if (strcmp(verb, "help") == 0)
		handle_help(session, command);
	else if (strcmp(verb, "backtrace") == 0)
		handle_backtrace(session, command);
	else if (strcmp(verb, "breakpoint") == 0)
		handle_breakpoint(session, command);
	else if (strcmp(verb, "up") == 0)
		handle_up_down(session, command, +1);
	else if (strcmp(verb, "down") == 0)
		handle_up_down(session, command, -1);
	else if (strcmp(verb, "clear") == 0)
		handle_clear(session, command);
	else if (strcmp(verb, "continue") == 0)
		handle_resume(session, command, OP_RESUME);
	else if (strcmp(verb, "eval") == 0)
		handle_eval(session, command, false);
	else if (strcmp(verb, "examine") == 0)
		handle_eval(session, command, true);
	else if (strcmp(verb, "frame") == 0)
		handle_frame(session, command);
	else if (strcmp(verb, "list") == 0)
		handle_list(session, command);
	else if (strcmp(verb, "stepover") == 0)
		handle_resume(session, command, OP_STEP_OVER);
	else if (strcmp(verb, "stepin") == 0)
		handle_resume(session, command, OP_STEP_IN);
	else if (strcmp(verb, "stepout") == 0)
		handle_resume(session, command, OP_STEP_OUT);
	else if (strcmp(verb, "vars") == 0)
		handle_vars(session, command);
	else if (strcmp(verb, "where") == 0)
		handle_where(session, command);
	else
		printf("'%s': not implemented.\n", verb);

finished:
	command_free(command);
}

static const char*
resolve_command(command_t* command)
{
	const char* pattern;
	const char* verb;

	if (command_len(command) < 1)
		return NULL;

	if (!(verb = find_verb(command_get_string(command, 0), &pattern)))
		return NULL;
	return validate_args(command, verb, pattern)
		? verb : NULL;
}

static void
handle_backtrace(session_t* session, command_t* cmd)
{
	const backtrace_t* calls;

	if (!(calls = inferior_get_calls(session->inferior)))
		return;
	backtrace_print(calls, session->frame, true);
}

static void
handle_breakpoint(session_t* session, command_t* cmd)
{
	const char*        filename;
	int                handle;
	int                linenum;
	const listing_t*   listing;
	struct breakpoint* new_breaks;

	int i;

	if (command_len(cmd) < 2) {
		if (session->num_breaks <= 0) {
			printf("no breakpoints are currently set.\n");
		}
		else {
			for (i = 0; i < session->num_breaks; ++i) {
				printf("#%2d: breakpoint at %s:%d\n", i,
					session->breaks[i].filename,
					session->breaks[i].linenum);
			}
		}
	}
	else {
		filename = command_get_string(cmd, 1);
		linenum = command_get_int(cmd, 1);
		if ((handle = inferior_add_breakpoint(session->inferior, filename, linenum)) < 0)
			goto on_error;
		if (!(new_breaks = realloc(session->breaks, (session->num_breaks + 1) * sizeof(struct breakpoint))))
			goto on_error;
		i = session->num_breaks++;
		new_breaks[i].handle = handle;
		new_breaks[i].filename = strdup(command_get_string(cmd, 1));
		new_breaks[i].linenum = command_get_int(cmd, 1);
		printf("breakpoint #%2d set at %s:%d.\n", i,
			new_breaks[i].filename,
			new_breaks[i].linenum);
		session->breaks = new_breaks;
		if ((listing = inferior_get_listing(session->inferior, filename)))
			listing_print(listing, linenum, 1, 0);
	}
	return;

on_error:
	printf("SSj was unable to set the breakpoint.");
}

static void
handle_clear(session_t* session, command_t* cmd)
{
	const char*      filename;
	int              handle;
	int              index;
	int              linenum;
	const listing_t* listing;

	index = command_get_int(cmd, 1);
	if (session->num_breaks <= 0) {
		printf("there are no breakpoints to clear.\n");
	}
	else if (index < 0 || index >= session->num_breaks) {
		printf("invalid breakpoint index, valid range is [0,%d].", session->num_breaks - 1);
	}
	else {
		handle = session->breaks[index].handle;
		filename = session->breaks[index].filename;
		linenum = session->breaks[index].linenum;
		if (!inferior_clear_breakpoint(session->inferior, handle))
			return;
		free(session->breaks[index].filename);
		--session->num_breaks;
		memmove(session->breaks + index, session->breaks + index + 1, sizeof(struct breakpoint) * (session->num_breaks - index));
		printf("cleared breakpoint #%2d at %s:%d.\n", index,
			session->breaks[index].filename, session->breaks[index].linenum);
		if ((listing = inferior_get_listing(session->inferior, filename)))
			listing_print(listing, linenum, 1, 0);
	}
}

static void
handle_resume(session_t* session, command_t* cmd, resume_op_t op)
{
	switch (op) {
	case OP_RESUME:
		session->auto_action = AUTO_CONTINUE;
		break;
	case OP_STEP_IN:
		session->auto_action = AUTO_STEP_IN;
		break;
	case OP_STEP_OUT:
		session->auto_action = AUTO_STEP_OUT;
		break;
	case OP_STEP_OVER:
		session->auto_action = AUTO_STEP_OVER;
		break;
	}

	inferior_resume(session->inferior, op);
	if (inferior_attached(session->inferior)) {
		autoselect_frame(session);
		preview_frame(session, session->frame);
	}
}

static void
handle_eval(session_t* session, command_t* cmd, bool verbose)
{
	const char*      expr = NULL;
	char             flag_string[4] = "---";
	const ki_atom_t* getter;
	unsigned int     handle;
	bool             is_accessor;
	bool             is_error = false;
	int              max_len = 0;
	objview_t*       object;
	unsigned int     prop_flags;
	const char*      prop_key;
	ki_atom_t*       result;
	const ki_atom_t* setter;

	int i = 0;

	if (command_get_tag(cmd, 1) == TOK_REF) {
		handle = command_get_handle(cmd, 1);
	}
	else {
		expr = command_get_rest(cmd, 1);
		result = inferior_eval(session->inferior, expr, session->frame, &is_error);
		handle = ki_atom_handle(result);
		if (ki_atom_type(result) != KI_REF) {  // primitive value?
			if (!is_error) {
				printf("%s = \33[37;1m", expr);
				ki_atom_print(result, verbose);
				printf("\33[m\n");
			} else {
				printf("\33[31;1merror: \33[37;1m%s\33[m\n", ki_atom_string(result));
			}
			ki_atom_free(result);
			return;
		}
	}

	if (!(object = inferior_get_object(session->inferior, handle, verbose)))
		return;
	if (is_error)
		printf("\33[31;1m");
	if (objview_len(object) == 0) {
		printf("object has no properties or doesn't exist.\n");
		return;
	}
	if (!verbose)
		printf("= {\n");
	for (i = 0; i < objview_len(object); ++i) {
		prop_key = objview_get_key(object, i);
		if ((int)strlen(prop_key) > max_len)
			max_len = (int)strlen(prop_key);
	}
	for (i = 0; i < objview_len(object); ++i) {
		is_accessor = objview_get_tag(object, i) == KI_ATTR_ACCESSOR;
		prop_key = objview_get_key(object, i);
		prop_flags = objview_get_flags(object, i);
		if (verbose) {
			if (prop_flags & PROP_WRITABLE)
				flag_string[0] = 'w';
			if (prop_flags & PROP_ENUMERABLE)
				flag_string[1] = 'e';
			if (prop_flags & PROP_CONFIGURABLE)
				flag_string[2] = 'c';
			printf("\33[30;1mprop\33[m  %s  %-*s  ", flag_string, max_len, prop_key);
		}
		else {
			printf("    \"%s\": ", prop_key);
		}
		if (!is_accessor) {
			ki_atom_print(objview_get_value(object, i), verbose);
		}
		else {
			getter = objview_get_getter(object, i);
			setter = objview_get_setter(object, i);
			printf("get ");
			ki_atom_print(getter, verbose);
			printf(", set ");
			ki_atom_print(setter, verbose);
		}
		printf("\n");
	}
	objview_free(object);
	if (!verbose)
		printf("}\n");
	if (is_error)
		printf("\33[m");
}

static void
handle_frame(session_t* session, command_t* cmd)
{
	const backtrace_t* calls;
	int                frame;

	if (!(calls = inferior_get_calls(session->inferior)))
		return;
	frame = session->frame;
	if (command_len(cmd) >= 2)
		frame = command_get_int(cmd, 1);
	if (frame < 0 || frame >= backtrace_len(calls))
		printf("stack frame #%2d doesn't exist.\n", frame);
	else {
		session->frame = frame;
		preview_frame(session, session->frame);
	}
}

static void
handle_help(session_t* session, command_t* cmd)
{
	const char* verb;

	if (command_len(cmd) > 1) {
		if (!(verb = find_verb(command_get_string(cmd, 1), NULL)))
			return;
		help_print(verb);
	}
	else
		help_print(NULL);
}

static void
handle_list(session_t* session, command_t* cmd)
{
	const char*        active_filename;
	int                active_lineno = 0;
	const backtrace_t* calls;
	const char*        filename;
	int                line_number;
	const listing_t*   listing;
	int                num_lines = 10;

	calls = inferior_get_calls(session->inferior);
	active_filename = backtrace_get_filename(calls, session->frame);
	active_lineno = backtrace_get_linenum(calls, session->frame);
	filename = active_filename;
	line_number = active_lineno;
	if (command_len(cmd) >= 2)
		num_lines = command_get_int(cmd, 1);
	if (command_len(cmd) >= 3) {
		filename = command_get_string(cmd, 2);
		line_number = command_get_int(cmd, 2);
	}
	if (!(listing = inferior_get_listing(session->inferior, filename))) {
		printf("no source code is available for '%s'.\n", filename);
	}
	else {
		if (strcmp(filename, active_filename) != 0)
			active_lineno = 0;
		line_number -= num_lines / 2;
		if (line_number < 1)
			line_number = 1;
		listing_print(listing, line_number, num_lines, active_lineno);
		session->list_num_lines = num_lines;
		session->list_filename = strdup(filename);
		session->list_linenum = (line_number + num_lines / 2) + num_lines;
		session->auto_action = AUTO_LIST;
	}
}

static void
handle_up_down(session_t* session, command_t* cmd, int direction)
{
	const backtrace_t* calls;
	int                new_frame;
	int                num_steps;

	if (!(calls = inferior_get_calls(session->inferior)))
		return;
	new_frame = session->frame + direction;
	if (new_frame >= backtrace_len(calls))
		printf("innermost frame: can't go up any further.\n");
	else if (new_frame < 0)
		printf("outermost frame: can't go down any further.\n");
	else {
		num_steps = command_len(cmd) >= 2 ? command_get_int(cmd, 1) : 1;
		new_frame = session->frame + num_steps * direction;
		session->frame = new_frame < 0 ? 0
			: new_frame >= backtrace_len(calls) ? backtrace_len(calls) - 1
			: new_frame;
		preview_frame(session, session->frame);
	}
	session->auto_action = AUTO_UP_DOWN;
	session->up_down_direction = direction;
}

static void
handle_vars(session_t* session, command_t* cmd)
{
	const backtrace_t* calls;
	const char*        call_name;
	int                class_len;
	const char*        class_name;
	int                max_class_len = 0;
	int                max_name_len = 0;
	int                name_len;
	const ki_atom_t*   value;
	const objview_t*   vars;
	const char*        var_name;

	int i;

	if (!(calls = inferior_get_calls(session->inferior)))
		return;
	if (!(vars = inferior_get_vars(session->inferior, session->frame)))
		return;
	for (i = 0; i < objview_len(vars); ++i) {
		var_name = objview_get_key(vars, i);
		class_name = objview_get_class(vars, i);
		name_len = (int)strlen(var_name);
		class_len = (int)strlen(class_name);
		if (name_len > max_name_len)
			max_name_len = name_len;
		if (class_len > max_class_len)
			max_class_len = class_len;
	}
	if (objview_len(vars) == 0) {
		call_name = backtrace_get_call_name(calls, session->frame);
		printf("no local variables in scope for %s\n", call_name);
	}
	for (i = 0; i < objview_len(vars); ++i) {
		var_name = objview_get_key(vars, i);
		class_name = objview_get_class(vars, i);
		value = objview_get_value(vars, i);
		printf("\33[30;1m%-*s\33[m  %-*s  \33[37;1m",
			max_class_len, class_name,
			max_name_len, var_name);
		ki_atom_print(value, true);
		printf("\33[m\n");
	}
}

static void
handle_where(session_t* session, command_t* cmd)
{
	int                frame = 0;
	const backtrace_t* calls;

	if (!(calls = inferior_get_calls(session->inferior)))
		return;
	while (backtrace_get_linenum(calls, frame) <= 0)
		++frame;
	preview_frame(session, frame);
}

static void
handle_quit(session_t* session, command_t* cmd)
{
	inferior_detach(session->inferior);
}

static void
preview_frame(session_t* session, int frame)
{
	const backtrace_t* calls;
	const char*        filename;
	int                lineno;
	const listing_t*   listing;

	if (!(calls = inferior_get_calls(session->inferior)))
		return;
	backtrace_print(calls, frame, false);
	filename = backtrace_get_filename(calls, frame);
	lineno = backtrace_get_linenum(calls, frame);
	if (lineno == 0)
		printf("system call, no source provided.\n");
	else {
		if (!(listing = inferior_get_listing(session->inferior, filename)))
			printf("source unavailable for %s.\n", filename);
		else
			listing_print(listing, lineno, 1, lineno);
	}
}

static bool
validate_args(const command_t* command, const char* verb_name, const char* pattern)
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
	if (command_len(command) - 1 < want_num_args) {
		printf("'%s': expected at least %d arguments.\n", verb_name, want_num_args);
		return false;
	}
	p_type = pattern;
	while (index < command_len(command) - 1) {
		if (*p_type == '~')
			++p_type;
		if (*p_type == '\0')
			break;
		want_tag = TOK_ANY;
		switch (*p_type) {
		case 'f':
			want_tag = TOK_FILE_LINE;
			want_type = "file:line";
			break;
		case 'n':
			want_tag = TOK_NUMBER;
			want_type = "number";
			break;
		case 'r':
			want_tag = TOK_REF;
			want_type = "reference";
			break;
		case 's':
			want_tag = TOK_STRING;
			want_type = "string";
			break;
		}
		if (want_tag != TOK_ANY && command_get_tag(command, index + 1) != want_tag)
			goto wrong_type;
		++p_type;
		++index;
	}
	return true;

wrong_type:
	printf("'%s': expected a %s for argument %d.\n", verb_name, want_type, index + 1);
	return false;
}
