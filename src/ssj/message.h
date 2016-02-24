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

enum req_command
{
	REQ_BASIC_INFO = 0x10,
	REQ_SEND_STATUS = 0x11,
	REQ_PAUSE = 0x12,
	REQ_RESUME = 0x13,
	REQ_STEP_INTO = 0x14,
	REQ_STEP_OVER = 0x15,
	REQ_STEP_OUT = 0x16,
	REQ_LIST_BREAK = 0x17,
	REQ_ADD_BREAK = 0x18,
	REQ_DEL_BREAK = 0x19,
	REQ_GET_VAR = 0x1A,
	REQ_PUT_VAR = 0x1B,
	REQ_GET_CALLSTACK = 0x1C,
	REQ_GET_LOCALS = 0x1D,
	REQ_EVAL = 0x1E,
	REQ_DETACH = 0x1F,
	REQ_DUMP_HEAP = 0x20,
	REQ_GET_BYTECODE = 0x21,
	REQ_APP_REQUEST = 0x22,
	REQ_INSPECT_OBJ = 0x23,
};

enum nfy_command
{
	NFY_STATUS = 0x01,
	NFY_PRINT = 0x02,
	NFY_ALERT = 0x03,
	NFY_LOG = 0x04,
	NFY_THROW = 0x05,
	NFY_DETACHING = 0x06,
	NFY_APP_NOTIFY = 0x07,
};

enum appnotify
{
	APPNFY_NOP,
	APPNFY_DEBUGPRINT,
};

enum apprequest
{
	APPREQ_NOP,
	APPREQ_GAME_INFO,
	APPREQ_SOURCE,
	APPREQ_SRC_PATH,
};

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
void            msg_add_heapptr  (message_t* msg, dukptr_t value);
void            msg_add_int      (message_t* msg, int value);
void            msg_add_string   (message_t* msg, const char* value);
message_t*      msg_recv         (socket_t* socket);
void            msg_send         (const message_t* msg, socket_t* socket);

#endif // SSJ__MESSAGE_H__INCLUDED
