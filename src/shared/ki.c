/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2017, Fat Cerberus
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
 *  * Neither the name of miniSphere nor the names of its contributors may be
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

#include "posix.h"
#include "ki.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "vector.h"

struct ki_message
{
	vector_t*      dvalues;
	dmessage_tag_t tag;
};

struct ki_atom
{
	ki_tag_t tag;
	union {
		double       float_value;
		unsigned int handle;
		int          int_value;
		struct {
			void*  data;
			size_t size;
		} buffer;
	};
};

ki_message_t*
dmessage_new(dmessage_tag_t tag)
{
	ki_message_t* message;

	message = calloc(1, sizeof(ki_message_t));
	message->dvalues = vector_new(sizeof(ki_atom_t*));
	message->tag = tag;
	return message;
}

void
dmessage_free(ki_message_t* it)
{
	iter_t iter;

	if (it== NULL)
		return;

	iter = vector_enum(it->dvalues);
	while (iter_next(&iter)) {
		ki_atom_t* dvalue = *(ki_atom_t**)iter.ptr;
		dvalue_free(dvalue);
	}
	vector_free(it->dvalues);
	free(it);
}

int
dmessage_len(const ki_message_t* it)
{
	return (int)vector_len(it->dvalues);
}

dmessage_tag_t
dmessage_tag(const ki_message_t* it)
{
	return it->tag;
}

ki_tag_t
dmessage_get_atom_tag(const ki_message_t* it, int index)
{
	ki_atom_t* dvalue;

	dvalue = *(ki_atom_t**)vector_get(it->dvalues, index);
	return dvalue_tag(dvalue);
}

const ki_atom_t*
dmessage_get_dvalue(const ki_message_t* it, int index)
{
	return *(ki_atom_t**)vector_get(it->dvalues, index);
}

double
dmessage_get_float(const ki_message_t* it, int index)
{
	ki_atom_t* dvalue;

	dvalue = *(ki_atom_t**)vector_get(it->dvalues, index);
	return dvalue_as_float(dvalue);
}

unsigned int
dmessage_get_handle(const ki_message_t* it, int index)
{
	ki_atom_t* dvalue;

	dvalue = *(ki_atom_t**)vector_get(it->dvalues, index);
	return dvalue_as_handle(dvalue);
}

int
dmessage_get_int(const ki_message_t* it, int index)
{
	ki_atom_t* dvalue;

	dvalue = *(ki_atom_t**)vector_get(it->dvalues, index);
	return dvalue_as_int(dvalue);
}

const char*
dmessage_get_string(const ki_message_t* it, int index)
{
	ki_atom_t* dvalue;

	dvalue = *(ki_atom_t**)vector_get(it->dvalues, index);
	return dvalue_as_cstr(dvalue);
}

void
dmessage_add_dvalue(ki_message_t* it, const ki_atom_t* dvalue)
{
	ki_atom_t* dup;

	dup = dvalue_dup(dvalue);
	vector_push(it->dvalues, &dup);
}

void
dmessage_add_float(ki_message_t* it, double value)
{
	ki_atom_t* dvalue;

	dvalue = dvalue_new_float(value);
	vector_push(it->dvalues, &dvalue);
}

void
dmessage_add_handle(ki_message_t* it, unsigned int value)
{
	ki_atom_t* dvalue;

	dvalue = dvalue_new_handle(value);
	vector_push(it->dvalues, &dvalue);
}

void
dmessage_add_int(ki_message_t* it, int value)
{
	ki_atom_t* dvalue;

	dvalue = dvalue_new_int(value);
	vector_push(it->dvalues, &dvalue);
}

void
dmessage_add_string(ki_message_t* it, const char* value)
{
	ki_atom_t* dvalue;

	dvalue = dvalue_new_string(value);
	vector_push(it->dvalues, &dvalue);
}

ki_message_t*
dmessage_recv(socket_t* socket)
{
	ki_atom_t*    atom;
	ki_message_t* message;

	iter_t iter;

	message = calloc(1, sizeof(ki_message_t));
	message->dvalues = vector_new(sizeof(ki_atom_t*));
	if (!(atom = dvalue_recv(socket)))
		goto lost_dvalue;
	message->tag = dvalue_tag(atom) == DVALUE_REQ ? DMESSAGE_REQ
		: dvalue_tag(atom) == DVALUE_REP ? DMESSAGE_REP
		: dvalue_tag(atom) == DVALUE_ERR ? DMESSAGE_ERR
		: dvalue_tag(atom) == DVALUE_NFY ? DMESSAGE_NFY
		: DMESSAGE_UNKNOWN;
	dvalue_free(atom);
	if (!(atom = dvalue_recv(socket)))
		goto lost_dvalue;
	while (dvalue_tag(atom) != DVALUE_EOM) {
		vector_push(message->dvalues, &atom);
		if (!(atom = dvalue_recv(socket)))
			goto lost_dvalue;
	}
	dvalue_free(atom);
	return message;

lost_dvalue:
	if (message != NULL) {
		iter = vector_enum(message->dvalues);
		while (atom = iter_next(&iter))
			dvalue_free(atom);
		vector_free(message->dvalues);
		free(message);
	}
	return NULL;
}

bool
dmessage_send(const ki_message_t* it, socket_t* socket)
{
	ki_atom_t* atom;
	ki_tag_t   lead_tag;

	iter_t iter;
	ki_atom_t* *p_dvalue;

	lead_tag = it->tag == DMESSAGE_REQ ? DVALUE_REQ
		: it->tag == DMESSAGE_REP ? DVALUE_REP
		: it->tag == DMESSAGE_ERR ? DVALUE_ERR
		: it->tag == DMESSAGE_NFY ? DVALUE_NFY
		: DVALUE_EOM;
	atom = dvalue_new(lead_tag);
	dvalue_send(atom, socket);
	dvalue_free(atom);
	iter = vector_enum(it->dvalues);
	while (p_dvalue = iter_next(&iter))
		dvalue_send(*p_dvalue, socket);
	atom = dvalue_new(DVALUE_EOM);
	dvalue_send(atom, socket);
	dvalue_free(atom);
	return socket_connected(socket);
}

ki_atom_t*
dvalue_new(ki_tag_t tag)
{
	ki_atom_t* atom;

	atom = calloc(1, sizeof(ki_atom_t));
	atom->tag = tag;
	return atom;
}

ki_atom_t*
dvalue_new_float(double value)
{
	ki_atom_t* atom;

	atom = calloc(1, sizeof(ki_atom_t));
	atom->tag = DVALUE_FLOAT;
	atom->float_value = value;
	return atom;
}

ki_atom_t*
dvalue_new_handle(unsigned int value)
{
	ki_atom_t* atom;

	atom = calloc(1, sizeof(ki_atom_t));
	atom->tag = DVALUE_HANDLE;
	atom->handle = value;
	return atom;
}

ki_atom_t*
dvalue_new_int(int value)
{
	ki_atom_t* atom;

	atom = calloc(1, sizeof(ki_atom_t));
	atom->tag = DVALUE_INT;
	atom->int_value = value;
	return atom;
}

ki_atom_t*
dvalue_new_string(const char* value)
{
	ki_atom_t* atom;

	atom = calloc(1, sizeof(ki_atom_t));
	atom->tag = DVALUE_STRING;
	atom->buffer.data = strdup(value);
	atom->buffer.size = strlen(value);
	return atom;
}

ki_atom_t*
dvalue_dup(const ki_atom_t* it)
{
	ki_atom_t* atom;

	atom = calloc(1, sizeof(ki_atom_t));
	memcpy(atom, it, sizeof(ki_atom_t));
	if (atom->tag == DVALUE_STRING || atom->tag == DVALUE_BUFFER) {
		atom->buffer.data = malloc(it->buffer.size + 1);
		memcpy(atom->buffer.data, it->buffer.data, it->buffer.size + 1);
	}
	return atom;
}

void
dvalue_free(ki_atom_t* it)
{
	if (it == NULL)
		return;

	if (it->tag == DVALUE_STRING || it->tag == DVALUE_BUFFER)
		free(it->buffer.data);
	free(it);
}

ki_tag_t
dvalue_tag(const ki_atom_t* it)
{
	return it->tag;
}

const char*
dvalue_as_cstr(const ki_atom_t* it)
{
	return it->tag == DVALUE_STRING ? it->buffer.data : NULL;
}

double
dvalue_as_float(const ki_atom_t* it)
{
	return it->tag == DVALUE_FLOAT ? it->float_value
		: it->tag == DVALUE_INT ? (double)it->int_value
		: 0.0;
}

unsigned int
dvalue_as_handle(const ki_atom_t* it)
{
	return it->tag == DVALUE_HANDLE ? it->handle : 0;
}

int
dvalue_as_int(const ki_atom_t* it)
{
	return it->tag == DVALUE_INT ? it->int_value
		: it->tag == DVALUE_FLOAT ? (int)it->float_value
		: 0;
}

void
dvalue_print(const ki_atom_t* it, bool is_verbose)
{
	switch (dvalue_tag(it)) {
	case DVALUE_UNDEF: printf("undefined"); break;
	case DVALUE_UNUSED: printf("unused"); break;
	case DVALUE_NULL: printf("null"); break;
	case DVALUE_TRUE: printf("true"); break;
	case DVALUE_FALSE: printf("false"); break;
	case DVALUE_FLOAT: printf("%g", it->float_value); break;
	case DVALUE_INT: printf("%d", it->int_value); break;
	case DVALUE_STRING: printf("\"%s\"", (char*)it->buffer.data); break;
	case DVALUE_BUFFER: printf("{buf:\"%zd bytes\"}", it->buffer.size); break;
	case DVALUE_HANDLE:
		if (!is_verbose)
			printf("{...}");
		else {
			printf("{ obj:%08x }", it->handle);
		}
		break;
	default:
		printf("*munch*");
	}
}

ki_atom_t*
dvalue_recv(socket_t* socket)
{
	ki_atom_t* atom;
	uint8_t    data[32];
	uint8_t    ib;
	int        read_size;

	atom = calloc(1, sizeof(ki_atom_t));
	if (socket_read(socket, &ib, 1) == 0)
		goto lost_connection;
	atom->tag = (ki_tag_t)ib;
	switch (ib) {
	case DVALUE_INT:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		atom->tag = DVALUE_INT;
		atom->int_value = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		break;
	case DVALUE_STRING:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		atom->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		atom->buffer.data = calloc(1, atom->buffer.size + 1);
		read_size = (int)atom->buffer.size;
		if (socket_read(socket, atom->buffer.data, read_size) != read_size)
			goto lost_connection;
		atom->tag = DVALUE_STRING;
		break;
	case DVALUE_STRING16:
		if (socket_read(socket, data, 2) == 0)
			goto lost_connection;
		atom->buffer.size = (data[0] << 8) + data[1];
		atom->buffer.data = calloc(1, atom->buffer.size + 1);
		read_size = (int)atom->buffer.size;
		if (socket_read(socket, atom->buffer.data, read_size) != read_size)
			goto lost_connection;
		atom->tag = DVALUE_STRING;
		break;
	case DVALUE_BUFFER:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		atom->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		atom->buffer.data = calloc(1, atom->buffer.size + 1);
		read_size = (int)atom->buffer.size;
		if (socket_read(socket, atom->buffer.data, read_size) != read_size)
			goto lost_connection;
		atom->tag = DVALUE_BUFFER;
		break;
	case DVALUE_BUF16:
		if (socket_read(socket, data, 2) == 0)
			goto lost_connection;
		atom->buffer.size = (data[0] << 8) + data[1];
		atom->buffer.data = calloc(1, atom->buffer.size + 1);
		read_size = (int)atom->buffer.size;
		if (socket_read(socket, atom->buffer.data, read_size) != read_size)
			goto lost_connection;
		atom->tag = DVALUE_BUFFER;
		break;
	case DVALUE_FLOAT:
		if (socket_read(socket, data, 8) == 0)
			goto lost_connection;
		((uint8_t*)&atom->float_value)[0] = data[7];
		((uint8_t*)&atom->float_value)[1] = data[6];
		((uint8_t*)&atom->float_value)[2] = data[5];
		((uint8_t*)&atom->float_value)[3] = data[4];
		((uint8_t*)&atom->float_value)[4] = data[3];
		((uint8_t*)&atom->float_value)[5] = data[2];
		((uint8_t*)&atom->float_value)[6] = data[1];
		((uint8_t*)&atom->float_value)[7] = data[0];
		atom->tag = DVALUE_FLOAT;
		break;
	case DVALUE_HANDLE:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		atom->tag = DVALUE_HANDLE;
		atom->handle = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		break;
	default:
		if (ib >= 0x60 && ib <= 0x7F) {
			atom->tag = DVALUE_STRING;
			atom->buffer.size = ib - 0x60;
			atom->buffer.data = calloc(1, atom->buffer.size + 1);
			read_size = (int)atom->buffer.size;
			if (socket_read(socket, atom->buffer.data, read_size) != read_size)
				goto lost_connection;
		}
		else if (ib >= 0x80 && ib <= 0xBF) {
			atom->tag = DVALUE_INT;
			atom->int_value = ib - 0x80;
		}
		else if (ib >= 0xC0) {
			if (socket_read(socket, data, 1) == 0)
				goto lost_connection;
			atom->tag = DVALUE_INT;
			atom->int_value = ((ib - 0xC0) << 8) + data[0];
		}
	}
	return atom;

lost_connection:
	free(atom);
	return NULL;
}

bool
dvalue_send(const ki_atom_t* it, socket_t* socket)
{
	uint8_t  data[32];
	uint32_t str_length;

	data[0] = (uint8_t)it->tag;
	socket_write(socket, data, 1);
	switch (it->tag) {
	case DVALUE_FLOAT:
		data[0] = ((uint8_t*)&it->float_value)[7];
		data[1] = ((uint8_t*)&it->float_value)[6];
		data[2] = ((uint8_t*)&it->float_value)[5];
		data[3] = ((uint8_t*)&it->float_value)[4];
		data[4] = ((uint8_t*)&it->float_value)[3];
		data[5] = ((uint8_t*)&it->float_value)[2];
		data[6] = ((uint8_t*)&it->float_value)[1];
		data[7] = ((uint8_t*)&it->float_value)[0];
		socket_write(socket, data, 8);
		break;
	case DVALUE_HANDLE:
		data[0] = (uint8_t)(it->handle >> 24 & 0xFF);
		data[1] = (uint8_t)(it->handle >> 16 & 0xFF);
		data[2] = (uint8_t)(it->handle >> 8 & 0xFF);
		data[3] = (uint8_t)(it->handle & 0xFF);
		socket_write(socket, data, 4);
		break;
	case DVALUE_INT:
		data[0] = (uint8_t)(it->int_value >> 24 & 0xFF);
		data[1] = (uint8_t)(it->int_value >> 16 & 0xFF);
		data[2] = (uint8_t)(it->int_value >> 8 & 0xFF);
		data[3] = (uint8_t)(it->int_value & 0xFF);
		socket_write(socket, data, 4);
		break;
	case DVALUE_STRING:
		str_length = (uint32_t)strlen(it->buffer.data);
		data[0] = (uint8_t)(str_length >> 24 & 0xFF);
		data[1] = (uint8_t)(str_length >> 16 & 0xFF);
		data[2] = (uint8_t)(str_length >> 8 & 0xFF);
		data[3] = (uint8_t)(str_length & 0xFF);
		socket_write(socket, data, 4);
		socket_write(socket, it->buffer.data, (int)str_length);
		break;
	}
	return socket_connected(socket);
}
