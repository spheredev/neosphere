/**
 *  miniSphere JavaScript game engine
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

#ifndef FATCERBERUS__JSAL_H__INCLUDED
#define FATCERBERUS__JSAL_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef
enum jsal_error_type
{
	JS_ERROR,
	JS_ERROR_RANGE,
	JS_ERROR_REF,
	JS_ERROR_SYNTAX,
	JS_ERROR_TYPE,
	JS_ERROR_URI,
} jsal_error_type_t;

typedef int (* jsal_callback_t) (bool is_ctor);

bool jsal_init               (void);
void jsal_uninit             (void);
bool jsal_call               (int num_args);
bool jsal_is_array           (int stack_index);
bool jsal_is_boolean         (int stack_index);
bool jsal_is_buffer_object   (int stack_index);
bool jsal_is_error           (int stack_index);
bool jsal_is_number          (int stack_index);
bool jsal_is_object          (int stack_index);
bool jsal_is_string          (int stack_index);
bool jsal_is_symbol          (int stack_index);
void jsal_pop                (int num_values);
int  jsal_push_boolean       (bool value);
int  jsal_push_eval          (const char* source);
int  jsal_push_global_object (void);
int  jsal_push_int           (int value);
int  jsal_push_known_symbol  (const char* name);
int  jsal_push_new_array     (void);
int  jsal_push_new_error     (const char* message, jsal_error_type_t type);
int  jsal_push_new_object    (void);
int  jsal_push_new_symbol    (const char* description);
int  jsal_push_null          (void);
int  jsal_push_number        (double value);
int  jsal_push_sprintf       (const char* format, ...);
int  jsal_push_string        (const char* value);
int  jsal_push_undefined     (void);
void jsal_throw              (void);

#endif // FATCERBERUS__JSAL_H__INCLUDED
