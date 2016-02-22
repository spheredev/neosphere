#include "ssj.h"
#include "commands.h"

#include "parser.h"

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
	"stepin",     "si", "",
	"stepout",    "so", "",
	"stepover",   "s",  "",
	"stepin",     "si", "",
	"list",       "l",  "~nf",
	"list",       "l",  "~nf",
	"up",         "u",  "~n",
	"vars",       "v",  "",
	"where",      "w",  "",
	"quit",       "q",  "",
	"help",       "h",  "~s",
};

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
	if (command_size(this) - 1 < want_num_args) {
		printf("at least %d arguments needed for '%s'\n", want_num_args, verb_name);
		return false;
	}
	p_type = pattern;
	while (index < command_size(this) - 1) {
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
	printf("expected %s for argument %d of '%s'.\n", want_type, index + 1, verb_name);
	return false;
}

const char*
find_ssj_command(command_t* command)
{
	const char* full_name;
	int         num_commands;
	const char* pattern;
	const char* short_name;
	const char* verb;
	
	int i;

	if (command_size(command) < 1)
		return NULL;
	
	num_commands = sizeof(command_db) / sizeof(command_db[0]) / 3;
	verb = command_get_string(command, 0);
	for (i = 0; i < num_commands; ++i) {
		full_name = command_db[0 + i * 3];
		short_name = command_db[1 + i * 3];
		pattern = command_db[2 + i * 3];
		if (strcmp(verb, short_name) == 0 || strstr(full_name, verb) == full_name)
			return validate_args(command, full_name, pattern) ? full_name : NULL;
	}
	
	printf("'%s': unrecognized command name\n", verb);
	return NULL;
}
