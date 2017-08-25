/**
 *  SSj, the Sphere JavaScript debugger
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

#include "ssj.h"
#include "dmessage.h"

#include "dvalue.h"

struct dmessage
{
	message_tag_t tag;
	vector_t*     dvalues;
};

dmessage_t*
dmessage_new(message_tag_t tag)
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

message_tag_t
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
