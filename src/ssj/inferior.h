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

#ifndef SPHERE__INFERIOR_H__INCLUDED
#define SPHERE__INFERIOR_H__INCLUDED

#include "backtrace.h"
#include "ki.h"
#include "listing.h"
#include "objview.h"

typedef struct inferior inferior_t;

typedef
enum resume_op
{
	OP_RESUME,
	OP_STEP_OVER,
	OP_STEP_IN,
	OP_STEP_OUT,
} resume_op_t;

void               inferiors_init            (void);
void               inferiors_uninit          (void);
inferior_t*        inferior_new              (const char* hostname, int port, bool show_trace);
void               inferior_free             (inferior_t* it);
bool               inferior_update           (inferior_t* it);
bool               inferior_attached         (const inferior_t* it);
const char*        inferior_author           (const inferior_t* it);
bool               inferior_running          (const inferior_t* it);
const char*        inferior_title            (const inferior_t* it);
const backtrace_t* inferior_get_calls        (inferior_t* it);
const listing_t*   inferior_get_listing      (inferior_t* it, const char* filename);
objview_t*         inferior_get_object       (inferior_t* it, unsigned int handle, bool get_all);
objview_t*         inferior_get_vars         (inferior_t* it, int frame);
int                inferior_add_breakpoint   (inferior_t* it, const char* filename, int linenum);
bool               inferior_clear_breakpoint (inferior_t* it, int handle);
void               inferior_detach           (inferior_t* it);
ki_atom_t*         inferior_eval             (inferior_t* it, const char* expr, int frame, bool* out_is_error);
bool               inferior_pause            (inferior_t* it);
ki_message_t*      inferior_request          (inferior_t* it, ki_message_t* msg);
bool               inferior_resume           (inferior_t* it, resume_op_t op);

#endif // SPHERE__INFERIOR_H__INCLUDED
