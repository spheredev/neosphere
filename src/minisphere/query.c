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

#include "minisphere.h"
#include "query.h"

#include "debugger.h"

static vector_t* jit_cache = NULL;

struct jit_entry
{
	unsigned int id;
	js_ref_t*    wrapper;
};

struct op
{
	query_op_t code;
	js_ref_t*  a;
};

struct query
{
	unsigned int refcount;
	vector_t*    ops;
	js_ref_t*    program;
	js_ref_t*    source;
};

query_t*
query_new(js_ref_t* source)
{
	query_t* query;

	query = calloc(1, sizeof(query_t));
	query->source = source;
	query->ops = vector_new(sizeof(struct op));
	return query;
}

query_t*
query_ref(query_t* it)
{
	++it->refcount;
	return it;
}

void
query_unref(query_t* it)
{
	if (--it->refcount > 0)
		return;
	jsal_unref(it->source);
	free(it);
}

void
query_add_op(query_t* it, query_op_t opcode, js_ref_t* a)
{
	struct op op;

	op.code = opcode;
	op.a = a;
	vector_push(it->ops, &op);
}

void
query_compile(query_t* it)
{
	char             arg_list[1024] = "";
	char*            arg_list_ptr;
	char             code[65536] = "";
	char*            code_ptr;
	char             decl_list[1024] = "";
	char*            decl_list_ptr;
	char             filename[256];
	struct jit_entry jit_entry;
	unsigned int     jit_id = 0;
	struct op*   op;
	unsigned int     place_value = 1;
	const char*      sort_args;
	lstring_t*       wrapper;

	iter_t iter;

	if (jit_cache == NULL)
		jit_cache = vector_new(sizeof(struct jit_entry));

	// see if this query configuration has already been compiled
	iter = vector_enum(it->ops);
	while (op = iter_next(&iter)) {
		jit_id += op->code * place_value;
		place_value *= QOP_MAX;
	}
	iter = vector_enum(jit_cache);
	while (iter_next(&iter)) {
		jit_entry = *(struct jit_entry*)iter.ptr;
		if (jit_id == jit_entry.id) {
			it->program = jit_entry.wrapper;
			return;
		}
	}

	// no JIT cache entry found, let's compile one up now
	arg_list_ptr = arg_list;
	code_ptr = code;
	decl_list_ptr = decl_list;
	iter = vector_enum(it->ops);
	while (op = iter_next(&iter)) {
		arg_list_ptr += sprintf(arg_list_ptr, "op%d,", iter.index);
		switch (op->code) {
		case QOP_FILTER:
			code_ptr += sprintf(code_ptr, "if (!op%d(value)) continue;", iter.index);
			break;
		case QOP_MAP:
			code_ptr += sprintf(code_ptr, "value = op%d(value);", iter.index);
			break;
		case QOP_SORT_AZ:
		case QOP_SORT_ZA:
			sort_args = op->code == QOP_SORT_AZ ? "(a, b)" : "(b, a)";
			decl_list_ptr += sprintf(decl_list_ptr, "const a%d = [];", iter.index);
			code_ptr += sprintf(code_ptr, "a%d.push([ op%d(value), value ]); }", iter.index, iter.index);
			code_ptr += sprintf(code_ptr, "a%d.sort(%s => a[0] < b[0] ? -1 : b[0] < a[0] ? 1 : 0);", iter.index, sort_args);
			code_ptr += sprintf(code_ptr, "for (let i = 0, len = a%d.length; i < len; ++i) { value = a%d[i][1];", iter.index, iter.index);
			break;
		case QOP_TAKE_N:
			code_ptr += sprintf(code_ptr, "if (op%d-- === 0) break;", iter.index);
			break;
		case QOP_TAP:
			code_ptr += sprintf(code_ptr, "op%d(value);", iter.index);
			break;
		default:
			// unsupported opcode, inject an error into the compiled code
			decl_list_ptr += sprintf(decl_list_ptr, "throw new Error(`Unsupported QOP %xh (internal error)`);", op->code);
		}
	}
	code_ptr += sprintf(code_ptr, "result = reducer(result, value);");
	sprintf(filename, "%%/fromQuery/%.4x.js", jit_id);
	wrapper = lstr_newf(
		"(source,%s reducer,result) => { %s for (let i = 0, len = source.length; i < len; ++i) { let value = source[i]; %s } return result; }",
		arg_list, decl_list, code);
	debugger_add_source(filename, wrapper);
	jsal_push_lstring_t(wrapper);
	lstr_free(wrapper);
	jsal_compile(filename);
	jsal_call(0);
	it->program = jsal_ref(-1);
	jit_entry.id = jit_id;
	jit_entry.wrapper = it->program;
	vector_push(jit_cache, &jit_entry);
	jsal_pop(1);

}

void
query_reduce(query_t* it, js_ref_t* reducer, js_ref_t* initial_value)
{
	struct op* op;

	iter_t iter;

	jsal_push_ref_weak(it->program);
	jsal_push_ref_weak(it->source);
	iter = vector_enum(it->ops);
	while (op = iter_next(&iter))
		jsal_push_ref_weak(op->a);
	jsal_push_ref_weak(reducer);
	jsal_push_ref_weak(initial_value);
	jsal_call(3 + vector_len(it->ops));
}
