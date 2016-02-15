#include "ssj.h"
#include "message.h"

#include "dvalue.h"

struct message
{
	msg_class_t msg_class;
	vector_t*   dvalues;
};

message_t*
msg_new(msg_class_t msg_class)
{
	message_t* msg;

	msg = calloc(1, sizeof(message_t));
	msg->dvalues = vector_new(sizeof(dvalue_t*));
	msg->msg_class = msg_class;
	return msg;
}

message_t*
msg_recv(socket_t* socket)
{
	message_t* obj;
	dvalue_t* dvalue;

	obj = calloc(1, sizeof(message_t));
	obj->dvalues = vector_new(sizeof(dvalue_t*));
	dvalue = dvalue_recv(socket);
	obj->msg_class = dvalue_tag(dvalue) == DVALUE_REQ ? MSG_CLASS_REQ
		: dvalue_tag(dvalue) == DVALUE_REP ? MSG_CLASS_REP
		: dvalue_tag(dvalue) == DVALUE_ERR ? MSG_CLASS_ERR
		: dvalue_tag(dvalue) == DVALUE_NFY ? MSG_CLASS_NFY
		: MSG_CLASS_UNKNOWN;
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
msg_free(message_t* obj)
{
	iter_t iter;
	dvalue_t* *p;

	if (obj == NULL)
		return;

	iter = vector_enum(obj->dvalues);
	while (p = vector_next(&iter))
		dvalue_free(*p);
	free(obj);
}

msg_class_t
msg_get_class(const message_t* msg)
{
	return msg->msg_class;
}

size_t
msg_len(const message_t* msg)
{
	return vector_len(msg->dvalues);
}

void
msg_send(socket_t* socket, const message_t* msg)
{
	dvalue_t*    dvalue;
	dvalue_tag_t lead_tag;

	iter_t iter;
	dvalue_t* *p_dvalue;

	lead_tag = msg->msg_class == MSG_CLASS_REQ ? DVALUE_REQ
		: msg->msg_class == MSG_CLASS_REP ? DVALUE_REP
		: msg->msg_class == MSG_CLASS_ERR ? DVALUE_ERR
		: msg->msg_class == MSG_CLASS_NFY ? DVALUE_NFY
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

dvalue_tag_t
msg_atom_tag(const message_t* msg, size_t index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(msg->dvalues, index);
	return dvalue_tag(dvalue);
}

const dvalue_t*
msg_atom_dvalue(const message_t* msg, size_t index)
{
	return *(dvalue_t**)vector_get(msg->dvalues, index);
}

double
msg_atom_float(const message_t* msg, size_t index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(msg->dvalues, index);
	return dvalue_as_float(dvalue);
}

int32_t
msg_atom_int(const message_t* msg, size_t index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(msg->dvalues, index);
	return dvalue_as_int(dvalue);
}

const char*
msg_atom_string(const message_t* msg, size_t index)
{
	dvalue_t* dvalue;

	dvalue = *(dvalue_t**)vector_get(msg->dvalues, index);
	return dvalue_as_cstr(dvalue);
}
