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

#ifndef SPHERE__DISPATCH_H__INCLUDED
#define SPHERE__DISPATCH_H__INCLUDED

#include "script.h"

typedef
enum job_type
{
	JOB_ON_EXIT,
	JOB_ON_RENDER,
	JOB_ON_TICK,
	JOB_ON_UPDATE,
	JOB_TYPE_MAX,
} job_type_t;

void    dispatch_init       (void);
void    dispatch_uninit     (void);
bool    dispatch_busy       (void);
bool    dispatch_can_exit   (void);
void    dispatch_cancel     (int64_t token);
void    dispatch_cancel_all (bool recurring, bool also_critical);
int64_t dispatch_defer      (script_t* script, int timeout, job_type_t hint, bool critical);
void    dispatch_pause      (int64_t token, bool paused);
int64_t dispatch_recur      (script_t* script, double priority, bool background, job_type_t hint);
bool    dispatch_run        (job_type_t hint);

#endif // SPHERE__DISPATCH_H__INCLUDED
