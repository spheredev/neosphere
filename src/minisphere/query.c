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

struct jit_entry
{
	uint64_t  id;
	js_ref_t* wrapper;
};

struct op
{
	query_op_t code;
	js_ref_t*  a;
};

struct query
{
	unsigned int refcount;
	uint64_t     jit_id;
	vector_t*    ops;
	js_ref_t*    program;
	js_ref_t*    source;
};

void compile_query (query_t* it, reduce_op_t opcode);

static vector_t* jit_cache = NULL;

int
query_max_ops()
{
	return (int)floor(log((double)UINT32_MAX) / log((double)QOP_MAX));
}

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
	struct op* op;
	
	iter_t iter;
	
	if (--it->refcount > 0)
		return;
	iter = vector_enum(it->ops);
	while (op = iter_next(&iter))
		jsal_unref(op->a);
	jsal_unref(it->source);
	free(it);
}

void
query_add_op(query_t* it, query_op_t opcode, js_ref_t* a)
{
	struct op op;

	jsal_push_undefined();
	op.code = opcode;
	op.a = a != NULL ? a : jsal_ref(-1);
	vector_push(it->ops, &op);
	jsal_pop(1);
}

void
query_run(query_t* it, reduce_op_t opcode, js_ref_t* r1, js_ref_t* r2)
{
	struct op* op;

	iter_t iter;

	compile_query(it, opcode);

	jsal_push_ref_weak(it->program);
	jsal_push_ref_weak(it->source);
	iter = vector_enum(it->ops);
	while (op = iter_next(&iter))
		jsal_push_ref_weak(op->a);
	if (r1 != NULL)
		jsal_push_ref_weak(r1);
	else
		jsal_push_undefined();
	if (r2 != NULL)
		jsal_push_ref_weak(r2);
	else
		jsal_push_undefined();
	jsal_call(3 + vector_len(it->ops));
}

static void
compile_query(query_t* query, reduce_op_t opcode)
{
	char             arg_list[1024] = "";
	char*            arg_list_ptr;
	char             code[4096] = "";
	char*            code_ptr;
	char             decl_list[1024] = "";
	char*            decl_list_ptr;
	char             filename[256];
	struct jit_entry jit_entry;
	uint64_t         jit_id = 1;
	bool             loop_closed = false;
	bool             make_generator = false;
	int              num_overs = 0;
	struct op*       op;
	uint64_t         place_value = 1;
	const char*      sort_args;
	lstring_t*       wrapper;

	iter_t iter;
	int    i;

	if (jit_cache == NULL)
		jit_cache = vector_new(sizeof(struct jit_entry));

	// see if this kind of query has already been compiled.  note that ROP affects code generation.  in general,
	// it is of paramount importance that queries requiring a different JS implementation get different JIT IDs.
	// if this is violated, I take no responsibility for whatever you get eaten by next.
	iter = vector_enum(query->ops);
	while (op = iter_next(&iter)) {
		jit_id += (uint64_t)op->code * place_value;
		place_value *= QOP_MAX;
	}
	jit_id += (uint64_t)opcode * place_value;
	iter = vector_enum(jit_cache);
	while (iter_next(&iter)) {
		jit_entry = *(struct jit_entry*)iter.ptr;
		if (jit_id == jit_entry.id) {
			query->program = jit_entry.wrapper;
			return;
		}
	}

	// no JIT cache entry found, let's compile one up now
	arg_list_ptr = arg_list;
	code_ptr = code;
	decl_list_ptr = decl_list;
	decl_list_ptr += sprintf(decl_list_ptr, "let value = undefined;");
	iter = vector_enum(query->ops);
	while (op = iter_next(&iter)) {
		arg_list_ptr += sprintf(arg_list_ptr, "op%d_a,", iter.index);
		if (loop_closed) {
			code_ptr += sprintf(code_ptr, "for (let i = 0, len = a%d.length; i < len; ++i) { value = a%d[i];",
				iter.index - 1, iter.index - 1);
			loop_closed = false;
		}
		switch (op->code) {
		case QOP_DROP_N:
			code_ptr += sprintf(code_ptr, "if (op%d_a > 0) { --op%d_a; continue; }", iter.index, iter.index);
			break;
		case QOP_EACH:
			code_ptr += sprintf(code_ptr, "op%d_a(value);", iter.index);
			break;
		case QOP_FILTER:
			code_ptr += sprintf(code_ptr, "if (!op%d_a(value)) continue;", iter.index);
			break;
		case QOP_INJECT:
			decl_list_ptr += sprintf(decl_list_ptr, "const a%d = [];", iter.index);
			code_ptr += sprintf(code_ptr, "a%d.push(value); }", iter.index);
			code_ptr += sprintf(code_ptr, "if (typeof op%d_a === 'object' && 'length' in op%d_a) a%d.push(...op%d_a); else a%d.push(op%d_a);",
				iter.index, iter.index, iter.index, iter.index, iter.index, iter.index);
			loop_closed = true;
			break;
		case QOP_MAP:
			code_ptr += sprintf(code_ptr, "value = op%d_a(value);", iter.index);
			break;
		case QOP_OVER:
			++num_overs;
			decl_list_ptr += sprintf(decl_list_ptr, "let src%d;", iter.index);
			code_ptr += sprintf(code_ptr, "src%d = op%d_a(value);", iter.index, iter.index);
			code_ptr += sprintf(code_ptr, "for (let k%d = 0, len = src%d.length; k%d < len; ++k%d) { value = src%d[k%d];",
				iter.index, iter.index, iter.index, iter.index, iter.index, iter.index);
			break;
		case QOP_REVERSE:
			decl_list_ptr += sprintf(decl_list_ptr, "const a%d = [];", iter.index);
			code_ptr += sprintf(code_ptr, "a%d.push(value); }", iter.index);
			code_ptr += sprintf(code_ptr, "a%d.reverse();", iter.index);
			loop_closed = true;
			break;
		case QOP_SHUFFLE:
			// emit a Fisher-Yates shuffle
			decl_list_ptr += sprintf(decl_list_ptr, "const a%d = [];", iter.index);
			code_ptr += sprintf(code_ptr, "a%d.push(value); }", iter.index);
			code_ptr += sprintf(code_ptr, "for (let i = a%d.length - 1; i >= 1; --i) {", iter.index);
			code_ptr += sprintf(code_ptr, "const idx = Math.floor(Math.random() * i), v = a%d[idx];", iter.index);
			code_ptr += sprintf(code_ptr, "a%d[idx] = a%d[i];", iter.index, iter.index);
			code_ptr += sprintf(code_ptr, "a%d[i] = v; }", iter.index);
			loop_closed = true;
			break;
		case QOP_SORT_AZ:
		case QOP_SORT_ZA:
			sort_args = op->code == QOP_SORT_AZ ? "(a, b)" : "(b, a)";
			decl_list_ptr += sprintf(decl_list_ptr, "const a%d = [];", iter.index);
			code_ptr += sprintf(code_ptr, "a%d.push([ op%d_a(value), value ]); }", iter.index, iter.index);
			code_ptr += sprintf(code_ptr, "a%d.sort(%s => a[0] < b[0] ? -1 : b[0] < a[0] ? 1 : 0);", iter.index, sort_args);
			code_ptr += sprintf(code_ptr, "for (let i = 0, len = a%d.length; i < len; ++i) { value = a%d[i][1];", iter.index, iter.index);
			break;
		case QOP_TAKE_N:
			code_ptr += sprintf(code_ptr, "if (op%d_a-- === 0) break;", iter.index);
			break;
		case QOP_TAP:
		case QOP_THRU:
			decl_list_ptr += sprintf(decl_list_ptr, "let a%d = [];", iter.index);
			code_ptr += sprintf(code_ptr, "a%d.push(value); }", iter.index);
			code_ptr += op->code == QOP_THRU
				? sprintf(code_ptr, "a%d = op%d_a(a%d);", iter.index, iter.index, iter.index)
				: sprintf(code_ptr, "op%d_a(a%d);", iter.index, iter.index);
			loop_closed = true;
			break;
		default:
			// unsupported opcode, inject an error into the compiled code
			code_ptr += sprintf(code_ptr, "throw new Error(`Unsupported QOP %xh (internal error)`);", op->code);
		}
	}
	if (loop_closed && opcode != ROP_ITERATOR && (opcode != ROP_TO_ARRAY || num_overs > 0)) {
		// closed-loop fast path is only for ROP_ITERATOR and ROP_TO_ARRAY, all others must reopen
		code_ptr += sprintf(code_ptr, "for (let i = 0, len = a%d.length; i < len; ++i) { value = a%d[i];",
			iter.index - 1, iter.index - 1);
		loop_closed = false;
	}
	switch (opcode) {
	case ROP_CONTAINS:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = false;");
		decl_list_ptr += sprintf(decl_list_ptr, "const is = r1 !== r1 ? (x, y) => x !== x && y !== y : (x, y) => x === y;");
		code_ptr += sprintf(code_ptr, "if (is(value, r1)) return true;");
		break;
	case ROP_EACH:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = undefined;");
		code_ptr += sprintf(code_ptr, "r1(value);");
		break;
	case ROP_EVERY:
	case ROP_EVERY_IN:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = true;");
		code_ptr += opcode == ROP_EVERY_IN
			? sprintf(code_ptr, "if (!r1.includes(value)) return false;")
			: sprintf(code_ptr, "if (!r1(value)) return false;");
		break;
	case ROP_FIND:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = undefined;");
		code_ptr += sprintf(code_ptr, "if (r1(value)) return value;");
		break;
	case ROP_FIRST:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = undefined;");
		code_ptr += sprintf(code_ptr, "return r1 !== undefined ? r1(value) : value;");
		break;
	case ROP_ITERATOR:
		// note: performance of generator functions in ChakraCore is atrocious so ROP_ITERATOR should ideally
		//       be implemented using something other than `yield` at some point.
		make_generator = true;
		decl_list_ptr += sprintf(decl_list_ptr, "const result = undefined;");
		code_ptr += loop_closed
			? sprintf(code_ptr, "yield* a%d;", iter.index - 1)
			: sprintf(code_ptr, "yield value;");
		break;
	case ROP_LAST:
		decl_list_ptr += sprintf(decl_list_ptr, "let result = undefined;");
		code_ptr += sprintf(code_ptr, "result = value;");
		break;
	case ROP_REDUCE:
		decl_list_ptr += sprintf(decl_list_ptr, "let result = r2;");
		code_ptr += sprintf(code_ptr, "result = r1(result, value);");
		break;
	case ROP_SOME:
	case ROP_SOME_IN:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = false;");
		code_ptr += opcode == ROP_SOME_IN
			? sprintf(code_ptr, "if (r1.includes(value)) return true;")
			: sprintf(code_ptr, "if (r1(value)) return true;");
		break;
	case ROP_TO_ARRAY:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = [];");
		code_ptr += loop_closed
			? sprintf(code_ptr, "return a%d;", iter.index - 1)
			: sprintf(code_ptr, "result.push(value);");
		break;
	default:
		code_ptr += sprintf(code_ptr, "throw new Error(`Unsupported ROP %xh (internal error)`);", opcode);
	}
	for (i = 0; i < num_overs; ++i)
		code_ptr += sprintf(code_ptr, "}");
	if (!loop_closed)
		code_ptr += sprintf(code_ptr, "}");
	if (opcode == ROP_LAST)
		code_ptr += sprintf(code_ptr, "if (r1 !== undefined) result = r1(result);");
	sprintf(filename, "%%/fromQuery/%"PRIx64".js", jit_id);
	wrapper = lstr_newf(
		"(%s fromQuery(source, %s r1, r2) { %s for (let i = 0, len = source.length; i < len; ++i) { value = source[i]; %s return result; })",
		make_generator ? "function*" : "function", arg_list, decl_list, code);
	debugger_add_source(filename, wrapper);
	jsal_push_lstring_t(wrapper);
	lstr_free(wrapper);
	jsal_compile(filename);
	jsal_call(0);
	query->program = jsal_ref(-1);
	jit_entry.id = jit_id;
	jit_entry.wrapper = query->program;
	vector_push(jit_cache, &jit_entry);
	jsal_pop(1);

}
