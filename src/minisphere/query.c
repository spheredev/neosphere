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

static vector_t* jit_cache = NULL;

struct jit_entry
{
	unsigned int id;
	js_ref_t*    wrapper;
};

struct opcode
{
	query_op_t type;
	union {
		js_ref_t* argument;
		double    count;
	};
};

struct query
{
	unsigned int refcount;
	vector_t*    opcodes;
	js_ref_t*    program;
	js_ref_t*    source;
};

query_t*
query_new(js_ref_t* source)
{
	query_t* query;

	query = calloc(1, sizeof(query_t));
	query->source = source;
	query->opcodes = vector_new(sizeof(struct opcode));
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
query_add_op(query_t* it, query_op_t type, js_ref_t* argument)
{
	struct opcode op;

	op.type = type;
	op.argument = argument;
	vector_push(it->opcodes, &op);
}

void
query_compile(query_t* it)
{
	char             arg_name[64];
	char             arg_list[1024] = "";
	char             code[65536] = "";
	char             decl[256] = "";
	char             decl_list[65536] = "";
	struct jit_entry jit_entry;
	unsigned int     jit_id = 0;
	char             line[256] = "";
	struct opcode*   op;
	unsigned int     place_value = 1;
	char             wrapper[65536];

	iter_t iter;

	if (jit_cache == NULL)
		jit_cache = vector_new(sizeof(struct jit_entry));
	
	// see if this query configuration has already been compiled
	iter = vector_enum(it->opcodes);
	while (op = iter_next(&iter)) {
		jit_id += op->type * place_value;
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
	iter = vector_enum(it->opcodes);
	while (op = iter_next(&iter)) {
		sprintf(arg_name, "op%d,", iter.index);
		strcat(arg_list, arg_name);
		switch (op->type) {
		case QOP_FILTER:
			sprintf(line, "if (!op%d(value)) continue;", iter.index);
			break;
		case QOP_MAP:
			sprintf(line, "value = op%d(value);", iter.index);
			break;
		case QOP_TAKE_N:
			sprintf(line, "if (op%d-- === 0) break;", iter.index);
		}
		strcat(code, line);
	}
	sprintf(line, "result = reducer(result, value);");
	strcat(code, line);
	sprintf(wrapper,
		"(source, %s reducer,result) => { for (let i = 0, len = source.length; i < len; ++i) { let value = source[i]; %s } return result; }",
		arg_list, code);
	jsal_push_string(wrapper);
	jsal_compile("%/jittedQuery.js");
	jsal_call(0);
	it->program = jsal_ref(-1);
	jit_entry.id = jit_id;
	jit_entry.wrapper = it->program;
	vector_push(jit_cache, &jit_entry);
	jsal_pop(1);

}

void
query_run(query_t* it, js_ref_t* reducer, js_ref_t* initial_value)
{
	struct opcode* op;
	
	iter_t iter;
	
	jsal_push_ref_weak(it->program);
	jsal_push_ref_weak(it->source);
	iter = vector_enum(it->opcodes);
	while (op = iter_next(&iter))
		jsal_push_ref_weak(op->argument);
	jsal_push_ref_weak(reducer);
	jsal_push_ref_weak(initial_value);
	jsal_call(3 + vector_len(it->opcodes));
}
