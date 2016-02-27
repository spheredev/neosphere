#ifndef SSJ__MESSAGE_H__INCLUDED
#define SSJ__MESSAGE_H__INCLUDED

#include "dvalue.h"

typedef struct message message_t;

typedef
enum message_tag
{
	MESSAGE_UNKNOWN,
	MESSAGE_REQ,
	MESSAGE_REP,
	MESSAGE_ERR,
	MESSAGE_NFY,
} message_tag_t;

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
	REQ_INSPECT_PROP_KEY = 0x24,
	REQ_INSPECT_PROPS = 0x25,
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

enum err_command
{
	ERR_UNKNOWN = 0x00,
	ERR_UNSUPPORTED = 0x01,
	ERR_TOO_MANY = 0x02,
	ERR_NOT_FOUND = 0x03,
	ERR_APP_ERROR = 0x04,
};

enum appnotify
{
	APPNFY_NOP,
	APPNFY_DEBUG_PRINT,
};

enum apprequest
{
	APPREQ_NOP,
	APPREQ_GAME_INFO,
	APPREQ_SOURCE,
};

message_t*      message_new          (message_tag_t tag);
void            message_free         (message_t* o);
int             message_len          (const message_t* o);
message_tag_t   message_tag          (const message_t* o);
dvalue_tag_t    message_get_atom_tag (const message_t* o, int index);
const dvalue_t* message_get_dvalue   (const message_t* o, int index);
double          message_get_float    (const message_t* o, int index);
int32_t         message_get_int      (const message_t* o, int index);
const char*     message_get_string   (const message_t* o, int index);
void            message_add_dvalue   (message_t* o, const dvalue_t* dvalue);
void            message_add_float    (message_t* o, double value);
void            message_add_heapptr  (message_t* o, dukptr_t value);
void            message_add_int      (message_t* o, int value);
void            message_add_string   (message_t* o, const char* value);
message_t*      message_recv         (socket_t* socket);
void            message_send         (const message_t* o, socket_t* socket);

#endif // SSJ__MESSAGE_H__INCLUDED
