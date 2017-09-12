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

#ifndef SSJ__DMESSAGE_H__INCLUDED
#define SSJ__DMESSAGE_H__INCLUDED

#include "dvalue.h"

typedef struct dmessage dmessage_t;

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
	REQ_BASICINFO = 0x10,
	REQ_SENDSTATUS = 0x11,
	REQ_PAUSE = 0x12,
	REQ_RESUME = 0x13,
	REQ_STEPINTO = 0x14,
	REQ_STEPOVER = 0x15,
	REQ_STEPOUT = 0x16,
	REQ_LISTBREAK = 0x17,
	REQ_ADDBREAK = 0x18,
	REQ_DELBREAK = 0x19,
	REQ_GETVAR = 0x1A,
	REQ_PUTVAR = 0x1B,
	REQ_GETCALLSTACK = 0x1C,
	REQ_GETLOCALS = 0x1D,
	REQ_EVAL = 0x1E,
	REQ_DETACH = 0x1F,
	REQ_DUMPHEAP = 0x20,
	REQ_GETBYTECODE = 0x21,
	REQ_APPREQUEST = 0x22,
	REQ_GETHEAPOBJINFO = 0x23,
	REQ_GETOBJPROPDESC = 0x24,
	REQ_GETOBJPROPDESCRANGE = 0x25,
};

enum nfy_command
{
	NFY_STATUS = 0x01,
	NFY_PRINT = 0x02,
	NFY_ALERT = 0x03,
	NFY_LOG = 0x04,
	NFY_THROW = 0x05,
	NFY_DETACHING = 0x06,
	NFY_APPNOTIFY = 0x07,
};

enum err_command
{
	ERR_UNKNOWN = 0x00,
	ERR_UNSUPPORTED = 0x01,
	ERR_TOO_MANY = 0x02,
	ERR_NOT_FOUND = 0x03,
	ERR_APP_ERROR = 0x04,
};

enum print_op
{
	PRINT_NORMAL,
	PRINT_ASSERT,
	PRINT_DEBUG,
	PRINT_ERROR,
	PRINT_INFO,
	PRINT_TRACE,
	PRINT_WARN,
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
	APPREQ_WATERMARK,
};

dmessage_t*     dmessage_new          (message_tag_t tag);
void            dmessage_free         (dmessage_t* o);
int             dmessage_len          (const dmessage_t* o);
message_tag_t   dmessage_tag          (const dmessage_t* o);
dvalue_tag_t    dmessage_get_atom_tag (const dmessage_t* o, int index);
const dvalue_t* dmessage_get_dvalue   (const dmessage_t* o, int index);
double          dmessage_get_float    (const dmessage_t* o, int index);
int32_t         dmessage_get_int      (const dmessage_t* o, int index);
const char*     dmessage_get_string   (const dmessage_t* o, int index);
void            dmessage_add_dvalue   (dmessage_t* o, const dvalue_t* dvalue);
void            dmessage_add_float    (dmessage_t* o, double value);
void            dmessage_add_heapptr  (dmessage_t* o, remote_ptr_t value);
void            dmessage_add_int      (dmessage_t* o, int value);
void            dmessage_add_string   (dmessage_t* o, const char* value);
dmessage_t*     dmessage_recv         (socket_t* socket);
bool            dmessage_send         (const dmessage_t* o, socket_t* socket);

#endif // SSJ__DMESSAGE_H__INCLUDED