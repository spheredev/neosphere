#include "ssj.h"
#include "message.h"

#include "dvalue.h"

struct message
{
	msg_type_t type;
	vector_t*  dvalues;
};

message_t*
msg_new(msg_type_t type)
{
	message_t* msg;

	msg = calloc(1, sizeof(message_t));
	msg->dvalues = vector_new(sizeof(dvalue_t*));
	msg->type = type;
	return msg;
}

void
msg_free(message_t* obj)
{
	iter_t iter;

	if (obj == NULL)
		return;

	iter = vector_enum(obj->dvalues);
	while (vector_next(&iter)) {
		dvalue_t* dvalue = *(dvalue_t**)iter.ptr;
		dvalue_free(dvalue);
	}
	vector_free(obj->dvalues);
	free(obj);
}

size_t
msg_len(const message_t* msg)
{
	return vector_len(msg->dvalues);
}

msg_type_t
msg_type(const message_t* msg)
{
	return msg->type;
}

dvalue_tag_t
msg_get_atom_tag(const message_t* msg, size_t index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(msg->dvalues, index);
	return dvalue_tag(dvalue);
}

const dvalue_t*
msg_get_dvalue(const message_t* msg, size_t index)
{
	return *(dvalue_t**)vector_get(msg->dvalues, index);
}

double
msg_get_float(const message_t* msg, size_t index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(msg->dvalues, index);
	return dvalue_as_float(dvalue);
}

int
msg_get_int(const message_t* msg, size_t index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(msg->dvalues, index);
	return dvalue_as_int(dvalue);
}

const char*
msg_get_string(const message_t* msg, size_t index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(msg->dvalues, index);
	return dvalue_as_cstr(dvalue);
}

void
msg_add_dvalue(message_t* msg, const dvalue_t* dvalue)
{
	dvalue_t* dup;

	dup = dvalue_dup(dvalue);
	vector_push(msg->dvalues, &dup);
}

void
msg_add_float(message_t* msg, double value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_float(value);
	vector_push(msg->dvalues, &dvalue);
}

void
msg_add_heapptr(message_t* msg, dukptr_t heapptr)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_heapptr(heapptr);
	vector_push(msg->dvalues, &dvalue);
}

void
msg_add_int(message_t* msg, int value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_int(value);
	vector_push(msg->dvalues, &dvalue);
}

void
msg_add_string(message_t* msg, const char* value)
{
	dvalue_t* dvalue;

	dvalue = dvalue_new_string(value);
	vector_push(msg->dvalues, &dvalue);
}

message_t*
msg_recv(socket_t* socket)
{
	message_t* obj;
	dvalue_t* dvalue;

	obj = calloc(1, sizeof(message_t));
	obj->dvalues = vector_new(sizeof(dvalue_t*));
	dvalue = dvalue_recv(socket);
	obj->type = dvalue_tag(dvalue) == DVALUE_REQ ? MSG_TYPE_REQ
		: dvalue_tag(dvalue) == DVALUE_REP ? MSG_TYPE_REP
		: dvalue_tag(dvalue) == DVALUE_ERR ? MSG_TYPE_ERR
		: dvalue_tag(dvalue) == DVALUE_NFY ? MSG_TYPE_NFY
		: MSG_TYPE_UNKNOWN;
	dvalue_free(dvalue);
	dvalue = dvalue_recv(socket);
	while (dvalue_tag(dvalue) != DVALUE_EOM) {
		vector_push(obj->dvalues, &dvalue);
		dvalue = dvalue_recv(socket);
	}
	dvalue_free(dvalue);
	return obj;
}

void
msg_send(const message_t* msg, socket_t* socket)
{
	dvalue_t*    dvalue;
	dvalue_tag_t lead_tag;

	iter_t iter;
	dvalue_t* *p_dvalue;

	lead_tag = msg->type == MSG_TYPE_REQ ? DVALUE_REQ
		: msg->type == MSG_TYPE_REP ? DVALUE_REP
		: msg->type == MSG_TYPE_ERR ? DVALUE_ERR
		: msg->type == MSG_TYPE_NFY ? DVALUE_NFY
		: DVALUE_EOM;
	dvalue = dvalue_new(lead_tag);
	dvalue_send(dvalue, socket);
	dvalue_free(dvalue);
	iter = vector_enum(msg->dvalues);
	while (p_dvalue = vector_next(&iter))
		dvalue_send(*p_dvalue, socket);
	dvalue = dvalue_new(DVALUE_EOM);
	dvalue_send(dvalue, socket);
	dvalue_free(dvalue);
}
