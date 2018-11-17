/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2018, Fat Cerberus
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

#ifndef SPHERE__QUERY_H__INCLUDED
#define SPHERE__QUERY_H__INCLUDED

typedef struct query query_t;

typedef
enum query_op
{
	QOP_NOP,
	QOP_DROP_N,
	QOP_FILTER,
	QOP_MAP,
	QOP_REDUCE,
	QOP_REVERSE,
	QOP_SHUFFLE,
	QOP_SORT_AZ,
	QOP_SORT_ZA,
	QOP_TAKE_N,
	QOP_TAP,
	QOP_MAX,
} query_op_t;

int      query_max_ops (void);
query_t* query_new     (js_ref_t* source);
query_t* query_ref     (query_t* it);
void     query_unref   (query_t* it);
void     query_add_op  (query_t* it, query_op_t opcode, js_ref_t* a);
void     query_find    (query_t* it, js_ref_t* predicate);
void     query_first   (query_t* it, bool from_end);
void     query_test    (query_t* it, js_ref_t* predicate, bool match_all);
void     query_reduce  (query_t* it, js_ref_t* reducer, js_ref_t* initial_value);
void     query_run     (query_t* it);

#endif // SPHERE__QUERY_H__INCLUDED
