#ifndef SSJ__MESSAGE_H__INCLUDED
#define SSJ__MESSAGE_H__INCLUDED

#include "dvalue.h"

typedef struct message message_t;

typedef
enum msg_type
{
	MSG_TYPE_UNKNOWN,
	MSG_TYPE_REQ,
	MSG_TYPE_REP,
	MSG_TYPE_ERR,
	MSG_TYPE_NFY,
} msg_type_t;

message_t*      msg_new          (msg_type_t type);
void            msg_free         (message_t* msg);
size_t          msg_len          (const message_t* message);
msg_type_t      msg_type         (const message_t* message);
dvalue_tag_t    msg_get_atom_tag (const message_t* msg, size_t index);
const dvalue_t* msg_get_dvalue   (const message_t* msg, size_t index);
double          msg_get_float    (const message_t* msg, size_t index);
int32_t         msg_get_int      (const message_t* msg, size_t index);
const char*     msg_get_string   (const message_t* msg, size_t index);
void            msg_add_dvalue   (message_t* msg, const dvalue_t* dvalue);
void            msg_add_float    (message_t* msg, double value);
void            msg_add_heapptr  (message_t* msg, duk_ptr_t value);
void            msg_add_int      (message_t* msg, int value);
void            msg_add_string   (message_t* msg, const char* value);
message_t*      msg_recv         (socket_t* socket);
void            msg_send         (const message_t* msg, socket_t* socket);

#endif // SSJ__MESSAGE_H__INCLUDED
