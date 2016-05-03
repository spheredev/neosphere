#ifndef SSJ__PARSER_H__INCLUDED
#define SSJ__PARSER_H__INCLUDED

typedef struct command command_t;

typedef
enum token_tag
{
	TOK_STRING,
	TOK_NUMBER,
	TOK_FILE_LINE,
} token_tag_t;

command_t*  command_parse      (const char* string);
void        command_free       (command_t* obj);
int         command_len        (const command_t* obj);
token_tag_t command_get_tag    (const command_t* obj, int index);
int         command_get_int    (const command_t* obj, int index);
double      command_get_float  (const command_t* obj, int index);
const char* command_get_string (const command_t* obj, int index);

#endif // SSJ__PARSER_H__INCLUDED
