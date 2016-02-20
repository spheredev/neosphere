#include "ssj.h"

#include "command.h"

struct argument
{
	cli_type_t type;
	union {
		char*  string;
		double number;
		struct {
			char* filename;
			int   line_no;
		};
	};
};

struct command
{
	char*     verb;
	vector_t* arguments;
};

command_t*
get_command()
{
	static char buffer[4096];
	
	command_t* command;
	char       ch;
	size_t     ch_idx = 0;
	bool       is_parsed = false;
	char*      next_token;
	char*      verb;
	
	// get a command from the user
	buffer[0] = '\0';
	ch = getchar();
	while (ch != '\n') {
		if (ch_idx >= 4095) {
			printf("string entered is too long to parse.");
			buffer[0] = '\0';
			return NULL;
		}
		buffer[ch_idx++] = ch;
		ch = getchar();
	}
	buffer[ch_idx] = '\0';

	// parse the command line
	if (buffer[0] == '\0') return NULL;
	command = calloc(1, sizeof(command_t));
	verb = strtok_r(buffer, " ", &next_token);
	ch_idx = 0;
	while (!is_parsed) {

	}

	return command;
}
