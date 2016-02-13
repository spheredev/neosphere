#ifndef SSJ__COMMANDLINE_H__INCLUDED
#define SSJ__COMMANDLINE_H__INCLUDED

typedef struct command command_t;

typedef
enum cli_type
{
	CLI_STRING,
	CLI_NUMBER,
	CLI_FILE_LINE,
} cli_type_t;

#endif // SSJ__COMMANDLINE_H__INCLUDED
