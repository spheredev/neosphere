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

#include "posix.h"
#include "ki.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include "vector.h"

struct ki_atom
{
	ki_type_t type;
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

struct ki_message
{
	vector_t* atoms;
	ki_type_t command;
};

ki_atom_t*
ki_atom_new(ki_type_t type)
{
	ki_atom_t* atom;

	if (!(atom = calloc(1, sizeof(ki_atom_t))))
		return NULL;
	atom->type = type;
	return atom;
}

ki_atom_t*
ki_atom_new_bool(bool value)
{
	ki_atom_t* atom;

	if (!(atom = calloc(1, sizeof(ki_atom_t))))
		return NULL;
	atom->type = value ? KI_TRUE : KI_FALSE;
	return atom;
}

ki_atom_t*
ki_atom_new_int(int value)
{
	ki_atom_t* atom;

	if (!(atom = calloc(1, sizeof(ki_atom_t))))
		return NULL;
	atom->type = KI_INT;
	atom->int_value = value;
	return atom;
}

ki_atom_t*
ki_atom_new_number(double value)
{
	ki_atom_t* atom;

	if (!(atom = calloc(1, sizeof(ki_atom_t))))
		return NULL;
	atom->type = KI_NUMBER;
	atom->float_value = value;
	return atom;
}

ki_atom_t*
ki_atom_new_ref(unsigned int value)
{
	ki_atom_t* atom;

	if (!(atom = calloc(1, sizeof(ki_atom_t))))
		return NULL;
	atom->type = KI_REF;
	atom->handle = value;
	return atom;
}

ki_atom_t*
ki_atom_new_string(const char* value)
{
	ki_atom_t* atom;

	if (!(atom = calloc(1, sizeof(ki_atom_t))))
		return NULL;
	atom->type = KI_STRING;
	atom->buffer.data = strdup(value);
	atom->buffer.size = strlen(value);
	return atom;
}

ki_atom_t*
ki_atom_dup(const ki_atom_t* it)
{
	ki_atom_t* atom = NULL;

	if (!(atom = calloc(1, sizeof(ki_atom_t))))
		goto on_error;
	memcpy(atom, it, sizeof(ki_atom_t));
	if (atom->type == KI_STRING || atom->type == KI_BUFFER) {
		if (!(atom->buffer.data = malloc(it->buffer.size + 1)))
			goto on_error;
		memcpy(atom->buffer.data, it->buffer.data, it->buffer.size + 1);
	}
	return atom;

on_error:
	free(atom);
	return NULL;
}

void
ki_atom_free(ki_atom_t* it)
{
	if (it == NULL)
		return;

	if (it->type == KI_STRING || it->type == KI_BUFFER)
		free(it->buffer.data);
	free(it);
}

bool
ki_atom_bool(const ki_atom_t* it)
{
	return it->type == KI_TRUE;
}

unsigned int
ki_atom_handle(const ki_atom_t* it)
{
	return it->type == KI_REF ? it->handle : 0;
}

int
ki_atom_int(const ki_atom_t* it)
{
	return it->type == KI_INT ? it->int_value
		: it->type == KI_NUMBER ? (int)it->float_value
		: 0;
}

double
ki_atom_number(const ki_atom_t* it)
{
	return it->type == KI_NUMBER ? it->float_value
		: it->type == KI_INT ? (double)it->int_value
		: 0.0;
}

const char*
ki_atom_string(const ki_atom_t* it)
{
	return it->type == KI_STRING ? it->buffer.data : NULL;
}

ki_type_t
ki_atom_type(const ki_atom_t* it)
{
	return it->type;
}

void
ki_atom_print(const ki_atom_t* it, bool verbose)
{
	switch (ki_atom_type(it)) {
	case KI_BUFFER:
		printf("{buf:\"%zd bytes\"}", it->buffer.size);
		break;
	case KI_FALSE:
		printf("false");
		break;
	case KI_INT:
		printf("%d", it->int_value);
		break;
	case KI_NULL:
		printf("null");
		break;
	case KI_NUMBER:
		if (isnan(it->float_value))
			printf("NaN");
		else if (isinf(it->float_value))
			printf("%sInfinity", it->float_value < 0.0 ? "-" : "");
		else
			printf("%g", it->float_value);
		break;
	case KI_REF:
		if (!verbose)
			printf("{...}");
		else {
			printf("{...} *%u", it->handle);
		}
		break;
	case KI_STRING:
		printf("\"%s\"", (char*)it->buffer.data);
		break;
	case KI_TRUE:
		printf("true");
		break;
	case KI_UNDEFINED:
		printf("undefined");
		break;
	default:
		printf("*MUNCH*");
	}
}

ki_atom_t*
ki_atom_recv(socket_t* socket)
{
	ki_atom_t* atom;
	uint8_t    data[32];
	uint8_t    ib;
	int        read_size;

	if (!(atom = calloc(1, sizeof(ki_atom_t))))
		return NULL;
	if (socket_read(socket, &ib, 1) == 0)
		goto lost_connection;
	atom->type = (ki_type_t)ib;
	switch (ib) {
	case KI_INT:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		atom->type = KI_INT;
		atom->int_value = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		break;
	case KI_STRING:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		atom->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		atom->buffer.data = calloc(1, atom->buffer.size + 1);
		read_size = (int)atom->buffer.size;
		if (socket_read(socket, atom->buffer.data, read_size) != read_size)
			goto lost_connection;
		atom->type = KI_STRING;
		break;
	case KI_BUFFER:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		atom->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		atom->buffer.data = calloc(1, atom->buffer.size + 1);
		read_size = (int)atom->buffer.size;
		if (socket_read(socket, atom->buffer.data, read_size) != read_size)
			goto lost_connection;
		atom->type = KI_BUFFER;
		break;
	case KI_NUMBER:
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
		atom->type = KI_NUMBER;
		break;
	case KI_REF:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		atom->type = KI_REF;
		atom->handle = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		break;
	}
	return atom;

lost_connection:
	free(atom);
	return NULL;
}

bool
ki_atom_send(const ki_atom_t* it, socket_t* socket)
{
	uint8_t  data[32];
	uint32_t str_length;

	data[0] = (uint8_t)it->type;
	socket_write(socket, data, 1);
	switch (it->type) {
	case KI_NUMBER:
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
	case KI_REF:
		data[0] = (uint8_t)(it->handle >> 24 & 0xFF);
		data[1] = (uint8_t)(it->handle >> 16 & 0xFF);
		data[2] = (uint8_t)(it->handle >> 8 & 0xFF);
		data[3] = (uint8_t)(it->handle & 0xFF);
		socket_write(socket, data, 4);
		break;
	case KI_INT:
		data[0] = (uint8_t)(it->int_value >> 24 & 0xFF);
		data[1] = (uint8_t)(it->int_value >> 16 & 0xFF);
		data[2] = (uint8_t)(it->int_value >> 8 & 0xFF);
		data[3] = (uint8_t)(it->int_value & 0xFF);
		socket_write(socket, data, 4);
		break;
	case KI_STRING:
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

ki_message_t*
ki_message_new(ki_type_t command_tag)
{
	ki_message_t* message;

	if (!(message = calloc(1, sizeof(ki_message_t))))
		return NULL;
	message->atoms = vector_new(sizeof(ki_atom_t*));
	message->command = command_tag;
	return message;
}

void
ki_message_free(ki_message_t* it)
{
	ki_atom_t* atom;

	iter_t iter;

	if (it== NULL)
		return;

	iter = vector_enum(it->atoms);
	while (iter_next(&iter)) {
		atom = *(ki_atom_t**)iter.ptr;
		ki_atom_free(atom);
	}
	vector_free(it->atoms);
	free(it);
}

int
ki_message_len(const ki_message_t* it)
{
	return vector_len(it->atoms);
}

ki_type_t
ki_message_tag(const ki_message_t* it)
{
	return it->command;
}

ki_type_t
ki_message_atom_type(const ki_message_t* it, int index)
{
	ki_atom_t* atom;

	atom = *(ki_atom_t**)vector_get(it->atoms, index);
	return ki_atom_type(atom);
}

const ki_atom_t*
ki_message_atom(const ki_message_t* it, int index)
{
	return *(ki_atom_t**)vector_get(it->atoms, index);
}

bool
ki_message_bool(const ki_message_t* it, int index)
{
	ki_atom_t* atom;

	atom = *(ki_atom_t**)vector_get(it->atoms, index);
	return ki_atom_bool(atom);
}

double
ki_message_number(const ki_message_t* it, int index)
{
	ki_atom_t* atom;

	atom = *(ki_atom_t**)vector_get(it->atoms, index);
	return ki_atom_number(atom);
}

unsigned int
ki_message_handle(const ki_message_t* it, int index)
{
	ki_atom_t* atom;

	atom = *(ki_atom_t**)vector_get(it->atoms, index);
	return ki_atom_handle(atom);
}

int
ki_message_int(const ki_message_t* it, int index)
{
	ki_atom_t* atom;

	atom = *(ki_atom_t**)vector_get(it->atoms, index);
	return ki_atom_int(atom);
}

const char*
ki_message_string(const ki_message_t* it, int index)
{
	ki_atom_t* atom;

	atom = *(ki_atom_t**)vector_get(it->atoms, index);
	return ki_atom_string(atom);
}

void
ki_message_add_atom(ki_message_t* it, const ki_atom_t* atom)
{
	ki_atom_t* dup;

	dup = ki_atom_dup(atom);
	vector_push(it->atoms, &dup);
}

void
ki_message_add_bool(ki_message_t* it, bool value)
{
	ki_atom_t* atom;

	atom = ki_atom_new_bool(value);
	vector_push(it->atoms, &atom);
}

void
ki_message_add_int(ki_message_t* it, int value)
{
	ki_atom_t* atom;

	atom = ki_atom_new_int(value);
	vector_push(it->atoms, &atom);
}

void
ki_message_add_number(ki_message_t* it, double value)
{
	ki_atom_t* atom;

	atom = ki_atom_new_number(value);
	vector_push(it->atoms, &atom);
}

void
ki_message_add_ref(ki_message_t* it, unsigned int value)
{
	ki_atom_t* atom;

	atom = ki_atom_new_ref(value);
	vector_push(it->atoms, &atom);
}

void
ki_message_add_string(ki_message_t* it, const char* value)
{
	ki_atom_t* atom;

	atom = ki_atom_new_string(value);
	vector_push(it->atoms, &atom);
}

ki_message_t*
ki_message_recv(socket_t* socket)
{
	ki_atom_t*    atom;
	ki_message_t* message;

	iter_t iter;

	if (!(message = calloc(1, sizeof(ki_message_t))))
		return NULL;
	message->atoms = vector_new(sizeof(ki_atom_t*));
	if (!(atom = ki_atom_recv(socket)))
		goto lost_dvalue;
	message->command = ki_atom_type(atom);
	ki_atom_free(atom);
	if (!(atom = ki_atom_recv(socket)))
		goto lost_dvalue;
	while (ki_atom_type(atom) != KI_EOM) {
		vector_push(message->atoms, &atom);
		if (!(atom = ki_atom_recv(socket)))
			goto lost_dvalue;
	}
	ki_atom_free(atom);
	return message;

lost_dvalue:
	if (message != NULL) {
		iter = vector_enum(message->atoms);
		while ((atom = iter_next(&iter)))
			ki_atom_free(atom);
		vector_free(message->atoms);
		free(message);
	}
	return NULL;
}

bool
ki_message_send(const ki_message_t* it, socket_t* socket)
{
	ki_atom_t*  atom;
	ki_atom_t** atom_ptr;
	ki_type_t   lead_tag;

	iter_t iter;

	lead_tag = it->command == KI_REQ ? KI_REQ
		: it->command == KI_REP ? KI_REP
		: it->command == KI_ERR ? KI_ERR
		: it->command == KI_NFY ? KI_NFY
		: KI_EOM;
	atom = ki_atom_new(lead_tag);
	ki_atom_send(atom, socket);
	ki_atom_free(atom);
	iter = vector_enum(it->atoms);
	while ((atom_ptr = iter_next(&iter)))
		ki_atom_send(*atom_ptr, socket);
	atom = ki_atom_new(KI_EOM);
	ki_atom_send(atom, socket);
	ki_atom_free(atom);
	return socket_connected(socket);
}
