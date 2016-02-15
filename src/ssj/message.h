#ifndef SSJ__MESSAGE_H__INCLUDED
#define SSJ__MESSAGE_H__INCLUDED

#include "dvalue.h"

typedef struct message message_t;

typedef
enum msg_class
{
	MSG_CLASS_UNKNOWN,
	MSG_CLASS_REQ,
	MSG_CLASS_REP,
	MSG_CLASS_ERR,
	MSG_CLASS_NFY,
} msg_class_t;

message_t*      msg_new           (msg_class_t msg_class);
message_t*      msg_recv          (socket_t* socket);
void            msg_free          (message_t* msg);
msg_class_t     msg_get_class     (const message_t* message);
size_t          msg_len           (const message_t* message);
void            msg_send          (socket_t* socket, const message_t* msg);
void            msg_add_dvalue    (message_t* msg, const dvalue_t* dvalue);
void            msg_add_float     (message_t* msg, double value);
void            msg_add_heapptr   (message_t* msg, duk_ptr_t value);
void            msg_add_int       (message_t* msg, int value);
void            msg_add_string    (message_t* msg, const char* value);
dvalue_tag_t    msg_atom_tag      (const message_t* msg, size_t index);
const dvalue_t* msg_atom_dvalue   (const message_t* msg, size_t index);
double          msg_atom_float    (const message_t* msg, size_t index);
int32_t         msg_atom_int      (const message_t* msg, size_t index);
const char*     msg_atom_string   (const message_t* msg, size_t index);

#endif // SSJ__MESSAGE_H__INCLUDED
