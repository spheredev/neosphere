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

#ifndef FATCERBERUS__ECMUNCH_H__INCLUDED
#define FATCERBERUS__ECMUNCH_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>

#include "lstring.h"

typedef struct js_value js_value_t;

typedef js_value_t* (* js_c_function_t) (js_value_t* this, int num_args, js_value_t* args[], bool is_ctor);

bool        js_init               (void);
void        js_uninit             (void);
js_value_t* js_global_object      (void);
js_value_t* js_get_exception      (void);
void        js_set_exception      (js_value_t* value);
js_value_t* js_value_new_error    (const char* message);
js_value_t* js_value_new_eval     (const lstring_t* source);
js_value_t* js_value_new_function (const char* name, js_c_function_t callback, int min_args);
js_value_t* js_value_new_int      (int value);
js_value_t* js_value_new_number   (double value);
js_value_t* js_value_new_object   (void);
js_value_t* js_value_new_string   (const char* value);
js_value_t* js_value_ref          (js_value_t* it);
void        js_value_unref        (js_value_t* it);
int         js_value_as_int       (const js_value_t* it);
double      js_value_as_number    (const js_value_t* it);
const char* js_value_as_string    (const js_value_t* it);
bool        js_value_is_function  (const js_value_t* it);
bool        js_value_is_number    (const js_value_t* it);
bool        js_value_is_object    (const js_value_t* it);
bool        js_value_is_string    (const js_value_t* it);
js_value_t* js_value_get          (js_value_t* it, const char* name);
bool        js_value_set          (js_value_t* it, const char* name, js_value_t* value);

#endif // FATCERBERUS__ECMUNCH_H__INCLUDED
