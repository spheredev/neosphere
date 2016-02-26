#include "ssj.h"
#include "message.h"

#include "dvalue.h"

struct message
{
	message_tag_t tag;
	vector_t*     dvalues;
};

message_t*
message_new(message_tag_t tag)
{
	message_t* o;

	o = calloc(1, sizeof(message_t));
	o->dvalues = vector_new(sizeof(dvalue_t*));
	o->tag = tag;
	return o;
}

void
message_free(message_t* o)
{
	iter_t iter;

	if (o== NULL)
		return;

	iter = vector_enum(o->dvalues);
	while (vector_next(&iter)) {
		dvalue_t* dvalue = *(dvalue_t**)iter.ptr;
		dvalue_free(dvalue);
	}
	vector_free(o->dvalues);
	free(o);
}

int
message_len(const message_t* o)
{
	return (int)vector_len(o->dvalues);
}

message_tag_t
message_tag(const message_t* o)
{
	return o->tag;
}

dvalue_tag_t
message_get_atom_tag(const message_t* o, int index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(o->dvalues, index);
	return dvalue_tag(dvalue);
}

const dvalue_t*
message_get_dvalue(const message_t* o, int index)
{
	return *(dvalue_t**)vector_get(o->dvalues, index);
}

double
message_get_float(const message_t* o, int index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(o->dvalues, index);
	return dvalue_as_float(dvalue);
}

int
message_get_int(const message_t* o, int index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(o->dvalues, index);
	return dvalue_as_int(dvalue);
}

const char*
message_get_string(const message_t* o, int index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(o->dvalues, index);
	return dvalue_as_cstr(dvalue);
}

void
message_add_dvalue(message_t* o, const dvalue_t* dvalue)
{
	dvalue_t* dup;

	dup = dvalue_dup(dvalue);
	vector_push(o->dvalues, &dup);
}

void
message_add_float(message_t* o, double value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_float(value);
	vector_push(o->dvalues, &dvalue);
}

void
message_add_heapptr(message_t* o, dukptr_t heapptr)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_heapptr(heapptr);
	vector_push(o->dvalues, &dvalue);
}

void
message_add_int(message_t* o, int value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_int(value);
	vector_push(o->dvalues, &dvalue);
}

void
message_add_string(message_t* o, const char* value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_string(value);
	vector_push(o->dvalues, &dvalue);
}

message_t*
message_recv(socket_t* socket)
{
	message_t* obj;
	dvalue_t* dvalue;

	iter_t iter;

	obj = calloc(1, sizeof(message_t));
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
		while (dvalue = vector_next(&iter))
			dvalue_free(dvalue);
		vector_free(obj->dvalues);
		free(obj);
	}
	return NULL;
}

void
message_send(const message_t* o, socket_t* socket)
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
	while (p_dvalue = vector_next(&iter))
		dvalue_send(*p_dvalue, socket);
	dvalue = dvalue_new(DVALUE_EOM);
	dvalue_send(dvalue, socket);
	dvalue_free(dvalue);
}
