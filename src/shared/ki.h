/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

#ifndef SPHERE__KI_H__INCLUDED
#define SPHERE__KI_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include "sockets.h"

#define KI_VERSION 1

typedef struct ki_atom    ki_atom_t;
typedef struct ki_message ki_message_t;

typedef
enum ki_attribute
{
	KI_ATTR_NONE         = 0x00,
	KI_ATTR_ACCESSOR     = 0x01,
	KI_ATTR_CONFIGURABLE = 0x02,
	KI_ATTR_ENUMERABLE   = 0x04,
	KI_ATTR_SYMBOL       = 0x08,
	KI_ATTR_WRITABLE     = 0x10,
} ki_attribute_t;

typedef
enum ki_log_op
{
	KI_LOG_NORMAL,
	KI_LOG_WARN,
	KI_LOG_TRACE,
	KI_LOG_ERROR,
} ki_log_op_t;

typedef
enum ki_type
{
	KI_EOM,
	KI_REQ,
	KI_REP,
	KI_ERR,
	KI_NFY,
	KI_BUFFER,
	KI_FALSE,
	KI_INT,
	KI_NULL,
	KI_NUMBER,
	KI_REF,
	KI_STRING,
	KI_TRUE,
	KI_UNDEFINED,
} ki_type_t;

enum ki_error
{
	KI_ERR_NOP,
	KI_ERR_NOT_FOUND,
	KI_ERR_TOO_MANY,
	KI_ERR_UNSUPPORTED,
};

enum ki_notify
{
	KI_NFY_NOP,
	KI_NFY_DETACH,
	KI_NFY_LOG,
	KI_NFY_PAUSE,
	KI_NFY_RESUME,
	KI_NFY_THROW,
};

enum ki_request
{
	KI_REQ_NOP,
	KI_REQ_ADD_BREAK,
	KI_REQ_DEL_BREAK,
	KI_REQ_DETACH,
	KI_REQ_DOWNLOAD,
	KI_REQ_EVAL,
	KI_REQ_GAME_INFO,
	KI_REQ_INSPECT_BREAKS,
	KI_REQ_INSPECT_LOCALS,
	KI_REQ_INSPECT_OBJ,
	KI_REQ_INSPECT_STACK,
	KI_REQ_PAUSE,
	KI_REQ_RESUME,
	KI_REQ_STEP_IN,
	KI_REQ_STEP_OUT,
	KI_REQ_STEP_OVER,
	KI_REQ_WATERMARK,
};

ki_atom_t*       ki_atom_new           (ki_type_t type);
ki_atom_t*       ki_atom_new_bool      (bool value);
ki_atom_t*       ki_atom_new_int       (int value);
ki_atom_t*       ki_atom_new_number    (double value);
ki_atom_t*       ki_atom_new_ref       (unsigned int handle);
ki_atom_t*       ki_atom_new_string    (const char* value);
ki_atom_t*       ki_atom_dup           (const ki_atom_t* it);
void             ki_atom_free          (ki_atom_t* it);
bool             ki_atom_bool          (const ki_atom_t* it);
unsigned int     ki_atom_handle        (const ki_atom_t* it);
int              ki_atom_int           (const ki_atom_t* it);
double           ki_atom_number        (const ki_atom_t* it);
const char*      ki_atom_string        (const ki_atom_t* it);
ki_type_t        ki_atom_type          (const ki_atom_t* it);
void             ki_atom_print         (const ki_atom_t* it, bool verbose);
ki_atom_t*       ki_atom_recv          (socket_t* socket);
bool             ki_atom_send          (const ki_atom_t* it, socket_t* socket);
ki_message_t*    ki_message_new        (ki_type_t command_tag);
void             ki_message_free       (ki_message_t* it);
const ki_atom_t* ki_message_atom       (const ki_message_t* it, int index);
ki_type_t        ki_message_atom_type  (const ki_message_t* it, int index);
bool             ki_message_bool       (const ki_message_t* it, int index);
unsigned int     ki_message_handle     (const ki_message_t* it, int index);
int32_t          ki_message_int        (const ki_message_t* it, int index);
int              ki_message_len        (const ki_message_t* it);
double           ki_message_number     (const ki_message_t* it, int index);
const char*      ki_message_string     (const ki_message_t* it, int index);
ki_type_t        ki_message_tag        (const ki_message_t* it);
void             ki_message_add_atom   (ki_message_t* it, const ki_atom_t* atom);
void             ki_message_add_bool   (ki_message_t* it, bool value);
void             ki_message_add_number (ki_message_t* it, double value);
void             ki_message_add_int    (ki_message_t* it, int value);
void             ki_message_add_ref    (ki_message_t* it, unsigned int handle);
void             ki_message_add_string (ki_message_t* it, const char* value);
ki_message_t*    ki_message_recv       (socket_t* socket);
bool             ki_message_send       (const ki_message_t* it, socket_t* socket);

#endif // SPHERE__KI_H__INCLUDED
