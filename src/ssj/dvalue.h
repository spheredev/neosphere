#ifndef SSJ__DVALUE_H__INCLUDED
#define SSJ__DVALUE_H__INCLUDED

#include "socket.h"

typedef struct dvalue  dvalue_t;

typedef
struct duk_ptr {
	uintmax_t value;
	uint8_t   size;
} duk_ptr_t;

typedef
enum dvalue_tag
{
	DVALUE_EOM = 0x00,
	DVALUE_REQ = 0x01,
	DVALUE_REP = 0x02,
	DVALUE_ERR = 0x03,
	DVALUE_NFY = 0x04,
	DVALUE_INT = 0x10,
	DVALUE_STRING = 0x11,
	DVALUE_STRING16 = 0x12,
	DVALUE_BUF = 0x13,
	DVALUE_BUF16 = 0x14,
	DVALUE_UNUSED = 0x15,
	DVALUE_UNDEF = 0x16,
	DVALUE_NULL = 0x17,
	DVALUE_TRUE = 0x18,
	DVALUE_FALSE = 0x19,
	DVALUE_FLOAT = 0x1A,
	DVALUE_OBJ = 0x1B,
	DVALUE_PTR = 0x1C,
	DVALUE_LIGHTFUNC = 0x1D,
	DVALUE_HEAPPTR = 0x1E,
} dvalue_tag_t;

dvalue_t*    dvalue_new         (dvalue_tag_t tag);
dvalue_t*    dvalue_new_float   (double value);
dvalue_t*    dvalue_new_heapptr (duk_ptr_t value);
dvalue_t*    dvalue_new_int     (int value);
dvalue_t*    dvalue_new_string  (const char* value);
dvalue_t*    dvalue_dup         (const dvalue_t* obj);
void         dvalue_free        (dvalue_t* obj);
dvalue_tag_t dvalue_tag         (const dvalue_t* obj);
const char*  dvalue_as_cstr     (const dvalue_t* obj);
duk_ptr_t    dvalue_as_ptr      (const dvalue_t* obj);
double       dvalue_as_float    (const dvalue_t* obj);
int          dvalue_as_int      (const dvalue_t* obj);
void         dvalue_print       (const dvalue_t* obj);
dvalue_t*    dvalue_recv        (socket_t* socket);
void         dvalue_send        (const dvalue_t* obj, socket_t* socket);

#endif // SSJ__DVALUE_H__INCLUDED
