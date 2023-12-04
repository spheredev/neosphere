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

#include "parser.h"

struct token
{
	token_tag_t  tag;
	unsigned int handle;
	int          line_no;
	double       number;
	char*        rest_string;
	char*        string;
};

struct command
{
	int          num_tokens;
	struct token *tokens;
};

command_t*
command_parse(const char* string)
{
	command_t*   command;
	int          array_len = 8;
	unsigned int handle;
	int          index = 0;
	size_t       length;
	char         next_char;
	double       number_value;
	char         quote[2];
	char*        string_value;
	const char   *p_ch;
	char         *p_tail;
	struct token *new_tokens;
	struct token *tokens;

	if (!(tokens = malloc(array_len * sizeof(struct token))))
		goto on_error;
	p_ch = string;
	while (*p_ch != '\0') {
		while (*p_ch == ' ' || *p_ch == '\t')
			++p_ch;  // skip whitespace
		if (*p_ch == '\0')
			break;  // avoid parsing the NUL terminator as a token
		if (index >= array_len) {
			array_len *= 2;
			if (!(new_tokens = realloc(tokens, array_len * sizeof(struct token))))
				goto on_error;
			tokens = new_tokens;
		}
		memset(&tokens[index], 0, sizeof(struct token));
		tokens[index].rest_string = strdup(p_ch);
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
			if (!(string_value = malloc(length + 1)))
				goto on_error;
			strncpy(string_value, p_ch + 1, length); string_value[length] = '\0';
			tokens[index].tag = TOK_STRING;
			tokens[index].string = string_value;
			p_ch += length + 2;
		}
		else if (*p_ch == '*') {
			length = strspn(++p_ch, "0123456789");
			handle = strtoul(p_ch, &p_tail, 10);
			next_char = *p_tail;
			if (next_char != '\0' && next_char != ' ' && next_char != '\t')
				goto syntax_error;
			tokens[index].tag = TOK_REF;
			tokens[index].handle = handle;
			p_ch += length;
		}
		else {
			length = strcspn(p_ch, " \t'\":");
			if (!(string_value = malloc(length + 1)))
				goto on_error;
			strncpy(string_value, p_ch, length); string_value[length] = '\0';
			tokens[index].tag = TOK_STRING;
			tokens[index].string = string_value;
			p_ch += length;
		}
		if (tokens[index].tag == TOK_STRING && *p_ch == ':') {
			// a string token followed immediately by a colon with no intervening whitespace
			// should be parsed instead as a file:line token.
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

	if (index > 0 && tokens[0].tag != TOK_STRING) {
		printf("missing instruction in command line");
		goto on_error;
	}

	if (!(command = calloc(1, sizeof(command_t))))
		goto on_error;
	command->num_tokens = index;
	command->tokens = tokens;
	return command;

syntax_error:
	tokens[index++].tag = TOK_ERROR;
	if (!(command = calloc(1, sizeof(command_t))))
		goto on_error;
	command->num_tokens = index;
	command->tokens = tokens;
	return command;

on_error:
	free(tokens);
	return NULL;
}

void
command_free(command_t* it)
{
	int i;

	if (it == NULL)
		return;

	for (i = 0; i < it->num_tokens; ++i) {
		free(it->tokens[i].rest_string);
		free(it->tokens[i].string);
	}
	free(it->tokens);
	free(it);
}

int
command_len(const command_t* it)
{
	return it->num_tokens;
}

token_tag_t
command_get_tag(const command_t* it, int index)
{
	return it->tokens[index].tag;
}

double
command_get_float(const command_t* it, int index)
{
	return it->tokens[index].tag == TOK_NUMBER
		? it->tokens[index].number : 0.0;
}

unsigned int
command_get_handle(const command_t* it, int index)
{
	return it->tokens[index].tag == TOK_REF
		? it->tokens[index].handle : 0;
}

int
command_get_int(const command_t* it, int index)
{
	return it->tokens[index].tag == TOK_NUMBER ? (int)it->tokens[index].number
		: it->tokens[index].tag == TOK_FILE_LINE ? it->tokens[index].line_no
		: 0;
}

const char*
command_get_rest(const command_t* it, int index)
{
	return it->tokens[index].rest_string;
}

const char*
command_get_string(const command_t* it, int index)
{
	return it->tokens[index].tag == TOK_STRING || it->tokens[index].tag == TOK_FILE_LINE
		? it->tokens[index].string : NULL;
}
