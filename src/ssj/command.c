#include "ssj.h"

#include "command.h"

enum token_tag
{
	TOK_STRING,
	TOK_NUMBER,
};

struct token
{
	enum token_tag tag;
	union {
		char*   string;
		double  number;
	};
	struct token*  prev;
	struct token*  next;
};

struct command
{
	struct token* first_token;
};

static struct token*
tokenize(const char* string)
{
	struct token* first_token = NULL;
	size_t        len;
	char*         end_ptr;
	char          next_char;
	long          number_value;
	char          quote[2];
	char*         string_value;
	struct token* token;
	struct token* prev_token = NULL;
	const char*   p;
	
	p = string;
	while (*p != '\0') {
		while (*p == ' ' || *p == '\t')
			++p;
		token = calloc(1, sizeof(struct token));
		if (first_token == NULL) first_token = token;
		if (prev_token != NULL)
			prev_token->next = token;
		token->prev = prev_token;
		prev_token = token;
		if (*p >= '0' && *p <= '9') {
			len = strspn(p, "0123456789");
			number_value = strtol(p, &end_ptr, 10);
			next_char = *end_ptr;
			if (next_char != '\0' && next_char != ' ' && next_char != '\t')
				goto syntax_error;
			token->tag = TOK_NUMBER;
			token->number = number_value;
			p += len;
		}
		else if (*p == '"' || *p == '\'') {
			sprintf(quote, "%c", *p);
			len = strcspn(p + 1, quote);
			next_char = *(p + 1 + len);
			if (next_char != quote[0])
				goto syntax_error;
			string_value = malloc(len + 1);
			strncpy(string_value, p + 1, len); string_value[len] = '\0';
			token->tag = TOK_STRING;
			token->string = string_value;
			p += len + 2;
		}
		else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
			len = strspn(p, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
			next_char = *(p + len);
			if (next_char != '\0' && next_char != ' ' && next_char != '\t')
				goto syntax_error;
			string_value = malloc(len + 1);
			strncpy(string_value, p, len); string_value[len] = '\0';
			token->tag = TOK_STRING;
			token->string = string_value;
			p += len;
		}
		else
			goto syntax_error;
	}
	if (first_token->tag != TOK_STRING)
		goto syntax_error;
	return first_token;

syntax_error:
	printf("syntax error.\n");
	return NULL;
}

command_t*
command_read(void)
{
	static char buffer[4096];
	
	command_t* command;
	char       ch;
	size_t     ch_idx = 0;
	bool       is_parsed = false;
	
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
	command = calloc(1, sizeof(command_t));
	command->first_token = tokenize(buffer);

	return command;
}
