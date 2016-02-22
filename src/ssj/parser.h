#ifndef SSJ__PARSER_H__INCLUDED
#define SSJ__PARSER_H__INCLUDED

typedef struct command command_t; typedef

enum token_tag
{
	TOK_STRING,
	TOK_NUMBER,
	TOK_FILE_LINE,
} token_tag_t;

command_t*  command_parse      (const char* string);
void        command_free       (command_t* this);
int         command_size       (const command_t* this);
token_tag_t command_get_tag    (const command_t* this, int index);
int         command_get_int    (const command_t* this, int index);
double      command_get_float  (const command_t* this, int index);
const char* command_get_string (const command_t* this, int index);

#endif // SSJ__PARSER_H__INCLUDED
