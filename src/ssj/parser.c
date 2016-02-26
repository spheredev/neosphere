#include "ssj.h"

#include "parser.h"

struct token
{
	token_tag_t tag;
	char*       string;
	int         line_no;
	double      number;
};

struct command
{
	int          num_tokens;
	struct token *tokens;
};

command_t*
command_parse(const char* string)
{
	command_t*   this;
	int          array_len = 8;
	int          index = 0;
	size_t       length;
	char         next_char;
	double       number_value;
	char         quote[2];
	char*        string_value;
	const char   *p_ch;
	char         *p_tail;
	struct token *tokens;
	
	tokens = malloc(array_len * sizeof(struct token));
	p_ch = string;
	while (*p_ch != '\0') {
		while (*p_ch == ' ' || *p_ch == '\t')
			++p_ch;  // skip whitespace
		if (*p_ch == '\0')
			break;  // avoid parsing the NUL terminator as a token
		if (index >= array_len) {
			array_len *= 2;
			tokens = realloc(tokens, array_len * sizeof(struct token));
		}
		memset(&tokens[index], 0, sizeof(struct token));
		if (*p_ch >= '0' && *p_ch <= '9') {
			length = strspn(p_ch, "0123456789.");
			number_value = strtod(p_ch, &p_tail);
			next_char = *p_tail;
			if (next_char != '\0' && next_char != ' ' && next_char != '\t')
				goto syntax_error;
			tokens[index].tag = TOK_NUMBER;
			tokens[index].number = number_value;
			p_ch += length;
		}
		else if (*p_ch == '"' || *p_ch == '\'') {
			sprintf(quote, "%c", *p_ch);
			length = strcspn(p_ch + 1, quote);
			next_char = *(p_ch + 1 + length);
			if (next_char != quote[0])
				goto syntax_error;
			string_value = malloc(length + 1);
			strncpy(string_value, p_ch + 1, length); string_value[length] = '\0';
			tokens[index].tag = TOK_STRING;
			tokens[index].string = string_value;
			p_ch += length + 2;
		}
		else {
			length = strcspn(p_ch, " \t'\":");
			next_char = *(p_ch + length);
			string_value = malloc(length + 1);
			strncpy(string_value, p_ch, length); string_value[length] = '\0';
			tokens[index].tag = TOK_STRING;
			tokens[index].string = string_value;
			p_ch += length;
		}
		if (tokens[index].tag == TOK_STRING && *p_ch == ':') {
			// a string token followed immediately by a colon with no intervening whitespace
			// should be parsed instead as a filename/linenumber pair.
			length = strspn(p_ch + 1, "0123456789");
			number_value = strtod(p_ch + 1, &p_tail);
			next_char = *p_tail;
			if (next_char != '\0' && next_char != ' ' && next_char != '\t')
				goto syntax_error;
			tokens[index].tag = TOK_FILE_LINE;
			tokens[index].line_no = (int)number_value;
			p_ch += length + 1;
		}
		++index;
	}
	
	if (index > 0 && tokens[0].tag != TOK_STRING)
		goto syntax_error;
	
	this = calloc(1, sizeof(command_t));
	this->num_tokens = index;
	this->tokens = tokens;
	return this;

syntax_error:
	printf("syntax error in command line.\n");
	return NULL;
}

void
command_free(command_t* this)
{
	int i;

	for (i = 0; i < this->num_tokens; ++i)
		free(this->tokens[i].string);
	free(this->tokens);
	free(this);
}

int
command_len(const command_t* this)
{
	return this->num_tokens;
}

token_tag_t
command_get_tag(const command_t* this, int index)
{
	return this->tokens[index].tag;
}

double
command_get_float(const command_t* this, int index)
{
	return this->tokens[index].tag == TOK_NUMBER
		? this->tokens[index].number : 0.0;
}

int
command_get_int(const command_t* this, int index)
{
	return this->tokens[index].tag == TOK_NUMBER ? (int)this->tokens[index].number
		: this->tokens[index].tag == TOK_FILE_LINE ? this->tokens[index].line_no
		: 0;
}

const char*
command_get_string(const command_t* this, int index)
{
	return this->tokens[index].tag == TOK_STRING || this->tokens[index].tag == TOK_FILE_LINE
		? this->tokens[index].string : NULL;
}
