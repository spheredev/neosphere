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
#include "dmessage.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "vector.h"

static void print_duktape_ptr (remote_ptr_t ptr);

const char* const
CLASS_NAMES[] = {
	"unknown",
	"Object",
	"Array",
	"Function",
	"Arguments",
	"Boolean",
	"Date",
	"Error",
	"JSON",
	"Math",
	"Number",
	"RegExp",
	"String",
	"global",
	"Symbol",
	"ObjEnv",
	"DecEnv",
	"Pointer",
	"Thread",
	"ArrayBuffer",
	"DataView",
	"Int8Array",
	"Uint8Array",
	"Uint8ClampedArray",
	"Int16Array",
	"Uint16Array",
	"Int32Array",
	"Uint32Array",
	"Float32Array",
	"Float64Array",
};

struct dmessage
{
	vector_t*      dvalues;
	dmessage_tag_t tag;
};

struct dvalue
{
	enum dvalue_tag tag;
	union {
		double       float_value;
		int          int_value;
		struct {
			remote_ptr_t value;
			uint8_t      class;
		} ptr;
		struct {
			void*  data;
			size_t size;
		} buffer;
	};
};

dmessage_t*
dmessage_new(dmessage_tag_t tag)
{
	dmessage_t* o;

	o = calloc(1, sizeof(dmessage_t));
	o->dvalues = vector_new(sizeof(dvalue_t*));
	o->tag = tag;
	return o;
}

void
dmessage_free(dmessage_t* o)
{
	iter_t iter;

	if (o== NULL)
		return;

	iter = vector_enum(o->dvalues);
	while (iter_next(&iter)) {
		dvalue_t* dvalue = *(dvalue_t**)iter.ptr;
		dvalue_free(dvalue);
	}
	vector_free(o->dvalues);
	free(o);
}

int
dmessage_len(const dmessage_t* o)
{
	return (int)vector_len(o->dvalues);
}

dmessage_tag_t
dmessage_tag(const dmessage_t* o)
{
	return o->tag;
}

dvalue_tag_t
dmessage_get_atom_tag(const dmessage_t* o, int index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(o->dvalues, index);
	return dvalue_tag(dvalue);
}

const dvalue_t*
dmessage_get_dvalue(const dmessage_t* o, int index)
{
	return *(dvalue_t**)vector_get(o->dvalues, index);
}

double
dmessage_get_float(const dmessage_t* o, int index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(o->dvalues, index);
	return dvalue_as_float(dvalue);
}

int
dmessage_get_int(const dmessage_t* o, int index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(o->dvalues, index);
	return dvalue_as_int(dvalue);
}

const char*
dmessage_get_string(const dmessage_t* o, int index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(o->dvalues, index);
	return dvalue_as_cstr(dvalue);
}

void
dmessage_add_dvalue(dmessage_t* o, const dvalue_t* dvalue)
{
	dvalue_t* dup;

	dup = dvalue_dup(dvalue);
	vector_push(o->dvalues, &dup);
}

void
dmessage_add_float(dmessage_t* o, double value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_float(value);
	vector_push(o->dvalues, &dvalue);
}

void
dmessage_add_heapptr(dmessage_t* o, remote_ptr_t heapptr)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_heapptr(heapptr);
	vector_push(o->dvalues, &dvalue);
}

void
dmessage_add_int(dmessage_t* o, int value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_int(value);
	vector_push(o->dvalues, &dvalue);
}

void
dmessage_add_string(dmessage_t* o, const char* value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_string(value);
	vector_push(o->dvalues, &dvalue);
}

dmessage_t*
dmessage_recv(socket_t* socket)
{
	dmessage_t* obj;
	dvalue_t* dvalue;

	iter_t iter;

	obj = calloc(1, sizeof(dmessage_t));
	obj->dvalues = vector_new(sizeof(dvalue_t*));
	if (!(dvalue = dvalue_recv(socket))) goto lost_dvalue;
	obj->tag = dvalue_tag(dvalue) == DVALUE_REQ ? MESSAGE_REQ
		: dvalue_tag(dvalue) == DVALUE_REP ? MESSAGE_REP
		: dvalue_tag(dvalue) == DVALUE_ERR ? MESSAGE_ERR
		: dvalue_tag(dvalue) == DVALUE_NFY ? MESSAGE_NFY
		: MESSAGE_UNKNOWN;
	dvalue_free(dvalue);
	if (!(dvalue = dvalue_recv(socket))) goto lost_dvalue;
	while (dvalue_tag(dvalue) != DVALUE_EOM) {
		vector_push(obj->dvalues, &dvalue);
		if (!(dvalue = dvalue_recv(socket))) goto lost_dvalue;
	}
	dvalue_free(dvalue);
	return obj;

lost_dvalue:
	if (obj != NULL) {
		iter = vector_enum(obj->dvalues);
		while (dvalue = iter_next(&iter))
			dvalue_free(dvalue);
		vector_free(obj->dvalues);
		free(obj);
	}
	return NULL;
}

bool
dmessage_send(const dmessage_t* o, socket_t* socket)
{
	dvalue_t*    dvalue;
	dvalue_tag_t lead_tag;

	iter_t iter;
	dvalue_t* *p_dvalue;

	lead_tag = o->tag == MESSAGE_REQ ? DVALUE_REQ
		: o->tag == MESSAGE_REP ? DVALUE_REP
		: o->tag == MESSAGE_ERR ? DVALUE_ERR
		: o->tag == MESSAGE_NFY ? DVALUE_NFY
		: DVALUE_EOM;
	dvalue = dvalue_new(lead_tag);
	dvalue_send(dvalue, socket);
	dvalue_free(dvalue);
	iter = vector_enum(o->dvalues);
	while (p_dvalue = iter_next(&iter))
		dvalue_send(*p_dvalue, socket);
	dvalue = dvalue_new(DVALUE_EOM);
	dvalue_send(dvalue, socket);
	dvalue_free(dvalue);
	return socket_connected(socket);
}

dvalue_t*
dvalue_new(dvalue_tag_t tag)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = tag;
	return obj;
}

dvalue_t*
dvalue_new_float(double value)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = DVALUE_FLOAT;
	obj->float_value = value;
	return obj;
}

dvalue_t*
dvalue_new_heapptr(remote_ptr_t value)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = DVALUE_HEAPPTR;
	obj->ptr.value.addr = value.addr;
	obj->ptr.value.size = value.size;
	return obj;
}

dvalue_t*
dvalue_new_int(int value)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = DVALUE_INT;
	obj->int_value = value;
	return obj;
}

dvalue_t*
dvalue_new_string(const char* value)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = DVALUE_STRING;
	obj->buffer.data = strdup(value);
	obj->buffer.size = strlen(value);
	return obj;
}

dvalue_t*
dvalue_dup(const dvalue_t* src)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	memcpy(obj, src, sizeof(dvalue_t));
	if (obj->tag == DVALUE_STRING || obj->tag == DVALUE_BUFFER) {
		obj->buffer.data = malloc(src->buffer.size + 1);
		memcpy(obj->buffer.data, src->buffer.data, src->buffer.size + 1);
	}
	return obj;
}

void
dvalue_free(dvalue_t* obj)
{
	if (obj == NULL)
		return;

	if (obj->tag == DVALUE_STRING || obj->tag == DVALUE_BUFFER)
		free(obj->buffer.data);
	free(obj);
}

dvalue_tag_t
dvalue_tag(const dvalue_t* obj)
{
	return obj->tag;
}

const char*
dvalue_as_cstr(const dvalue_t* obj)
{
	return obj->tag == DVALUE_STRING ? obj->buffer.data : NULL;
}

double
dvalue_as_float(const dvalue_t* obj)
{
	return obj->tag == DVALUE_FLOAT ? obj->float_value
		: obj->tag == DVALUE_INT ? (double)obj->int_value
		: 0.0;
}

remote_ptr_t
dvalue_as_ptr(const dvalue_t* obj)
{
	remote_ptr_t retval;

	memset(&retval, 0, sizeof(remote_ptr_t));
	if (obj->tag == DVALUE_PTR || obj->tag == DVALUE_HEAPPTR || obj->tag == DVALUE_OBJ || obj->tag == DVALUE_LIGHTFUNC) {
		retval.addr = obj->ptr.value.addr;
		retval.size = obj->ptr.value.size;
	}
	return retval;
}

int
dvalue_as_int(const dvalue_t* obj)
{
	return obj->tag == DVALUE_INT ? obj->int_value
		: obj->tag == DVALUE_FLOAT ? (int)obj->float_value
		: 0;
}

void
dvalue_print(const dvalue_t* obj, bool is_verbose)
{
	switch (dvalue_tag(obj)) {
	case DVALUE_UNDEF: printf("undefined"); break;
	case DVALUE_UNUSED: printf("unused"); break;
	case DVALUE_NULL: printf("null"); break;
	case DVALUE_TRUE: printf("true"); break;
	case DVALUE_FALSE: printf("false"); break;
	case DVALUE_FLOAT: printf("%g", obj->float_value); break;
	case DVALUE_INT: printf("%d", obj->int_value); break;
	case DVALUE_STRING: printf("\"%s\"", (char*)obj->buffer.data); break;
	case DVALUE_BUFFER: printf("{buf:\"%zd bytes\"}", obj->buffer.size); break;
	case DVALUE_HEAPPTR:
		printf("{heap:\"");
		print_duktape_ptr(dvalue_as_ptr(obj));
		printf("\"}");
		break;
	case DVALUE_LIGHTFUNC:
		printf("{lightfunc:\"");
		print_duktape_ptr(dvalue_as_ptr(obj));
		printf("\"}");
		break;
	case DVALUE_OBJ:
		if (!is_verbose)
			printf("{...}");
		else {
			printf("{ obj:'%s' }", CLASS_NAMES[obj->ptr.class]);
		}
		break;
	case DVALUE_PTR:
		printf("{ptr:\"");
		print_duktape_ptr(dvalue_as_ptr(obj));
		printf("\"}");
		break;
	default:
		printf("*munch*");
	}
}

dvalue_t*
dvalue_recv(socket_t* socket)
{
	dvalue_t* obj;
	uint8_t data[32];
	uint8_t ib;
	int     read_size;

	ptrdiff_t i, j;

	obj = calloc(1, sizeof(dvalue_t));
	if (socket_read(socket, &ib, 1) == 0)
		goto lost_connection;
	obj->tag = (enum dvalue_tag)ib;
	switch (ib) {
	case DVALUE_INT:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		obj->tag = DVALUE_INT;
		obj->int_value = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		break;
	case DVALUE_STRING:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		obj->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		obj->buffer.data = calloc(1, obj->buffer.size + 1);
		read_size = (int)obj->buffer.size;
		if (socket_read(socket, obj->buffer.data, read_size) != read_size)
			goto lost_connection;
		obj->tag = DVALUE_STRING;
		break;
	case DVALUE_STRING16:
		if (socket_read(socket, data, 2) == 0)
			goto lost_connection;
		obj->buffer.size = (data[0] << 8) + data[1];
		obj->buffer.data = calloc(1, obj->buffer.size + 1);
		read_size = (int)obj->buffer.size;
		if (socket_read(socket, obj->buffer.data, read_size) != read_size)
			goto lost_connection;
		obj->tag = DVALUE_STRING;
		break;
	case DVALUE_BUFFER:
		if (socket_read(socket, data, 4) == 0)
			goto lost_connection;
		obj->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		obj->buffer.data = calloc(1, obj->buffer.size + 1);
		read_size = (int)obj->buffer.size;
		if (socket_read(socket, obj->buffer.data, read_size) != read_size)
			goto lost_connection;
		obj->tag = DVALUE_BUFFER;
		break;
	case DVALUE_BUF16:
		if (socket_read(socket, data, 2) == 0)
			goto lost_connection;
		obj->buffer.size = (data[0] << 8) + data[1];
		obj->buffer.data = calloc(1, obj->buffer.size + 1);
		read_size = (int)obj->buffer.size;
		if (socket_read(socket, obj->buffer.data, read_size) != read_size)
			goto lost_connection;
		obj->tag = DVALUE_BUFFER;
		break;
	case DVALUE_FLOAT:
		if (socket_read(socket, data, 8) == 0)
			goto lost_connection;
		((uint8_t*)&obj->float_value)[0] = data[7];
		((uint8_t*)&obj->float_value)[1] = data[6];
		((uint8_t*)&obj->float_value)[2] = data[5];
		((uint8_t*)&obj->float_value)[3] = data[4];
		((uint8_t*)&obj->float_value)[4] = data[3];
		((uint8_t*)&obj->float_value)[5] = data[2];
		((uint8_t*)&obj->float_value)[6] = data[1];
		((uint8_t*)&obj->float_value)[7] = data[0];
		obj->tag = DVALUE_FLOAT;
		break;
	case DVALUE_OBJ:
		if (socket_read(socket, &obj->ptr.class, 1) == 0)
			goto lost_connection;
		if (socket_read(socket, &obj->ptr.value.size, 1) == 0)
			goto lost_connection;
		if (socket_read(socket, data, obj->ptr.value.size) == 0)
			goto lost_connection;
		obj->tag = DVALUE_OBJ;
		for (i = 0, j = obj->ptr.value.size - 1; j >= 0; ++i, --j)
			((uint8_t*)&obj->ptr.value.addr)[i] = data[j];
		break;
	case DVALUE_PTR:
		if (socket_read(socket, &obj->ptr.value.size, 1) == 0)
			goto lost_connection;
		if (socket_read(socket, data, obj->ptr.value.size) == 0)
			goto lost_connection;
		obj->tag = DVALUE_PTR;
		for (i = 0, j = obj->ptr.value.size - 1; j >= 0; ++i, --j)
			((uint8_t*)&obj->ptr.value.addr)[i] = data[j];
		break;
	case DVALUE_LIGHTFUNC:
		if (socket_read(socket, data, 2) == 0)
			goto lost_connection;
		if (socket_read(socket, &obj->ptr.value.size, 1) == 0)
			goto lost_connection;
		if (socket_read(socket, data, obj->ptr.value.size) == 0)
			goto lost_connection;
		obj->tag = DVALUE_LIGHTFUNC;
		for (i = 0, j = obj->ptr.value.size - 1; j >= 0; ++i, --j)
			((uint8_t*)&obj->ptr.value.addr)[i] = data[j];
		break;
	case DVALUE_HEAPPTR:
		if (socket_read(socket, &obj->ptr.value.size, 1) == 0)
			goto lost_connection;
		if (socket_read(socket, data, obj->ptr.value.size) == 0)
			goto lost_connection;
		obj->tag = DVALUE_HEAPPTR;
		for (i = 0, j = obj->ptr.value.size - 1; j >= 0; ++i, --j)
			((uint8_t*)&obj->ptr.value.addr)[i] = data[j];
		break;
	default:
		if (ib >= 0x60 && ib <= 0x7F) {
			obj->tag = DVALUE_STRING;
			obj->buffer.size = ib - 0x60;
			obj->buffer.data = calloc(1, obj->buffer.size + 1);
			read_size = (int)obj->buffer.size;
			if (socket_read(socket, obj->buffer.data, read_size) != read_size)
				goto lost_connection;
		}
		else if (ib >= 0x80 && ib <= 0xBF) {
			obj->tag = DVALUE_INT;
			obj->int_value = ib - 0x80;
		}
		else if (ib >= 0xC0) {
			if (socket_read(socket, data, 1) == 0)
				goto lost_connection;
			obj->tag = DVALUE_INT;
			obj->int_value = ((ib - 0xC0) << 8) + data[0];
		}
	}
	return obj;

lost_connection:
	free(obj);
	return NULL;
}

bool
dvalue_send(const dvalue_t* obj, socket_t* socket)
{
	uint8_t  data[32];
	uint32_t str_length;

	ptrdiff_t i, j;

	data[0] = (uint8_t)obj->tag;
	socket_write(socket, data, 1);
	switch (obj->tag) {
	case DVALUE_INT:
		data[0] = (uint8_t)(obj->int_value >> 24 & 0xFF);
		data[1] = (uint8_t)(obj->int_value >> 16 & 0xFF);
		data[2] = (uint8_t)(obj->int_value >> 8 & 0xFF);
		data[3] = (uint8_t)(obj->int_value & 0xFF);
		socket_write(socket, data, 4);
		break;
	case DVALUE_STRING:
		str_length = (uint32_t)strlen(obj->buffer.data);
		data[0] = (uint8_t)(str_length >> 24 & 0xFF);
		data[1] = (uint8_t)(str_length >> 16 & 0xFF);
		data[2] = (uint8_t)(str_length >> 8 & 0xFF);
		data[3] = (uint8_t)(str_length & 0xFF);
		socket_write(socket, data, 4);
		socket_write(socket, obj->buffer.data, (int)str_length);
		break;
	case DVALUE_FLOAT:
		data[0] = ((uint8_t*)&obj->float_value)[7];
		data[1] = ((uint8_t*)&obj->float_value)[6];
		data[2] = ((uint8_t*)&obj->float_value)[5];
		data[3] = ((uint8_t*)&obj->float_value)[4];
		data[4] = ((uint8_t*)&obj->float_value)[3];
		data[5] = ((uint8_t*)&obj->float_value)[2];
		data[6] = ((uint8_t*)&obj->float_value)[1];
		data[7] = ((uint8_t*)&obj->float_value)[0];
		socket_write(socket, data, 8);
		break;
	case DVALUE_HEAPPTR:
		for (i = 0, j = obj->ptr.value.size - 1; j >= 0; ++i, --j)
			data[i] = ((uint8_t*)&obj->ptr.value.addr)[j];
		socket_write(socket, &obj->ptr.value.size, 1);
		socket_write(socket, data, obj->ptr.value.size);
		break;
	}
	return socket_connected(socket);
}

static void
print_duktape_ptr(remote_ptr_t ptr)
{
	if (ptr.size == 8)  // x64 pointer
		printf("%016"PRIx64"h", (uint64_t)ptr.addr);
	else if (ptr.size == 4)  // x86 pointer
		printf("%08"PRIx32"h", (uint32_t)ptr.addr);
}
