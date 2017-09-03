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

#ifndef SSJ__DVALUE_H__INCLUDED
#define SSJ__DVALUE_H__INCLUDED

#include "sockets.h"

typedef struct dvalue  dvalue_t;

typedef
struct remote_ptr {
	uintmax_t addr;
	uint8_t   size;
} remote_ptr_t;

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
	DVALUE_BUFFER = 0x13,
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
dvalue_t*    dvalue_new_heapptr (remote_ptr_t value);
dvalue_t*    dvalue_new_int     (int value);
dvalue_t*    dvalue_new_string  (const char* value);
dvalue_t*    dvalue_dup         (const dvalue_t* it);
void         dvalue_free        (dvalue_t* it);
dvalue_tag_t dvalue_tag         (const dvalue_t* it);
const char*  dvalue_as_cstr     (const dvalue_t* it);
remote_ptr_t dvalue_as_ptr      (const dvalue_t* it);
double       dvalue_as_float    (const dvalue_t* it);
int          dvalue_as_int      (const dvalue_t* it);
void         dvalue_print       (const dvalue_t* it, bool is_verbose);
dvalue_t*    dvalue_recv        (socket_t* socket);
bool         dvalue_send        (const dvalue_t* it, socket_t* socket);

#endif // SSJ__DVALUE_H__INCLUDED
