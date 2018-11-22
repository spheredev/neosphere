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
	int          num_sources;
	vector_t*    ops;
	js_ref_t*    program;
	js_ref_t**   sources;
};

void        compile_query (query_t* it, reduce_op_t opcode);
static void emit_shuffle  (char** p_ptr, const char* array_name);

static vector_t* jit_cache = NULL;

int
query_max_ops()
{
	return (int)floor(log((double)UINT32_MAX) / log((double)QOP_MAX));
}

query_t*
query_new(int num_sources)
{
	int      stack_base;
	query_t* query;

	int i;

	query = calloc(1, sizeof(query_t));
	query->num_sources = num_sources;
	query->ops = vector_new(sizeof(struct op));
	if (num_sources > 0) {
		query->sources = malloc(num_sources * sizeof(js_ref_t*));
		stack_base = -num_sources;
		for (i = 0; i < num_sources; ++i)
			query->sources[i] = jsal_ref(i + stack_base);
	}
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
	int i;
	
	if (--it->refcount > 0)
		return;
	iter = vector_enum(it->ops);
	while (op = iter_next(&iter))
		jsal_unref(op->a);
	for (i = 0; i < it->num_sources; ++i)
		jsal_unref(it->sources[i]);
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
	int i;

	compile_query(it, opcode);

	jsal_push_ref_weak(it->program);
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
	jsal_call(2 + vector_len(it->ops));
	if (it->sources != NULL) {
		for (i = 0; i < it->num_sources; ++i)
			jsal_push_ref_weak(it->sources[i]);
		jsal_call(it->num_sources);
	}
}

static void
compile_query(query_t* query, reduce_op_t opcode)
{
	// HERE BE DRAGONS!
	// this is the query compiler, which compiles from() queries to fast JavaScript.  being a compiler, it
	// is almost certainly going to be completely inscrutable to the uninitiated.  be sure you're up to the
	// task before attempting to make changes here!

	char             arg_list[1024] = "";
	char*            arg_list_ptr;
	char             code[4096] = "";
	char*            code_ptr;
	char             decl_list[1024] = "";
	char*            decl_list_ptr;
	char             epilogue[1024] = "";
	char*            epilogue_ptr;
	char             filename[256];
	bool             has_transforms = false;
	struct jit_entry jit_entry;
	uint64_t         jit_id = 1;
	bool             loop_open = true;
	bool             is_generator = false;
	bool             is_transform_op;
	int              num_overs_open = 0;
	struct op*       op;
	uint64_t         place_value = 1;
	const char*      sort_args;
	char             source_name[32] = "source";
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

	// no JIT cache entry found, time to compile the query!
	arg_list_ptr = arg_list;
	code_ptr = code;
	decl_list_ptr = decl_list;
	epilogue_ptr = epilogue;
	decl_list_ptr += sprintf(decl_list_ptr, "let value = undefined, i = undefined;");
	iter = vector_enum(query->ops);
	while (op = iter_next(&iter)) {
		// note: QOP_SORT_* is treated as a special case because its temp array contains key/value
		//       pairs and therefore can't be used by other opcodes as-is.
		is_transform_op = op->code == QOP_CONCAT
			|| op->code == QOP_RANDOM
			|| op->code == QOP_REVERSE
			|| op->code == QOP_SAMPLE
			|| op->code == QOP_SHUFFLE
			|| op->code == QOP_TAP
			|| op->code == QOP_THRU;
		arg_list_ptr += sprintf(arg_list_ptr, "op%d_a,", iter.index);
		if (loop_open && is_transform_op) {
			sprintf(source_name, "a%d", iter.index);
			decl_list_ptr += sprintf(decl_list_ptr, "let %s = [];", source_name);
			code_ptr += sprintf(code_ptr, "%s.push(value); }", source_name);
			for (i = 0; i < num_overs_open; ++i)  // close loops for `over()`
				code_ptr += sprintf(code_ptr, "}");
			num_overs_open = 0;
			loop_open = false;
			has_transforms = true;
		}
		else if (!loop_open && !is_transform_op) {
			code_ptr += sprintf(code_ptr, "for (let ii = 0, len = %s.length; ii < len; ++ii) { value = %s[ii];",
				source_name, source_name);
			loop_open = true;
		}
		switch (op->code) {
		case QOP_CONCAT:
			code_ptr += sprintf(code_ptr, "if (op%d_a && typeof op%d_a !== 'string' && typeof op%d_a.length === 'number') %s.push(...op%d_a); else %s.push(op%d_a);",
				iter.index, iter.index, iter.index, source_name, iter.index, source_name, iter.index);
			break;
		case QOP_DROP_N:
			decl_list_ptr += sprintf(decl_list_ptr, "let c%d = op%d_a;", iter.index, iter.index);
			code_ptr += sprintf(code_ptr, "if (c%d > 0) { --c%d; continue; }", iter.index, iter.index);
			break;
		case QOP_EACH:
			code_ptr += sprintf(code_ptr, "op%d_a(value, i);", iter.index);
			break;
		case QOP_FILTER:
			code_ptr += sprintf(code_ptr, "if (!op%d_a(value, i)) continue;", iter.index);
			break;
		case QOP_MAP:
			code_ptr += sprintf(code_ptr, "value = op%d_a(value, i);", iter.index);
			break;
		case QOP_OVER:
			++num_overs_open;
			decl_list_ptr += sprintf(decl_list_ptr, "let src%d;", iter.index);
			code_ptr += sprintf(code_ptr, "src%d = op%d_a(value, i);", iter.index, iter.index);
			code_ptr += sprintf(code_ptr, "for (let k%d = 0, len = src%d.length; k%d < len; ++k%d) { value = src%d[k%d];",
				iter.index, iter.index, iter.index, iter.index, iter.index, iter.index);
			break;
		case QOP_RANDOM:
			decl_list_ptr += sprintf(decl_list_ptr, "const a%d_s = [];", iter.index);
			code_ptr += sprintf(code_ptr, "for (let s = 0, len = %s.length; s < op%d_a; ++s) a%d_s.push(%s[Math.floor(Math.random() * len)]);",
				source_name, iter.index, iter.index, source_name);
			sprintf(source_name, "a%d_2", iter.index);
			break;
		case QOP_REVERSE:
			code_ptr += sprintf(code_ptr, "%s.reverse();", source_name);
			break;
		case QOP_SAMPLE:
			decl_list_ptr += sprintf(decl_list_ptr, "let c%d;", iter.index);
			code_ptr += sprintf(code_ptr, "c%d = Math.max(Math.min(op%d_a, %s.length), 0);", iter.index, iter.index, source_name);
			code_ptr += sprintf(code_ptr, "for (let i = 0, len = %s.length; i < c%d; ++i) {", source_name, iter.index);
			code_ptr += sprintf(code_ptr, "const idx = i + Math.floor(Math.random() * (len - i)), v = %s[idx];", source_name);
			code_ptr += sprintf(code_ptr, "%s[idx] = %s[i];", source_name, source_name);
			code_ptr += sprintf(code_ptr, "%s[i] = v; }", source_name);
			code_ptr += sprintf(code_ptr, "%s.length = op%d_a;", source_name, iter.index);
			break;
		case QOP_SHUFFLE:
			// emit a Fisher-Yates shuffle
			code_ptr += sprintf(code_ptr, "for (let i = %s.length - 1; i >= 1; --i) {", source_name);
			code_ptr += sprintf(code_ptr, "const idx = Math.floor(Math.random() * i), v = %s[idx];", source_name);
			code_ptr += sprintf(code_ptr, "%s[idx] = %s[i];", source_name, source_name);
			code_ptr += sprintf(code_ptr, "%s[i] = v; }", source_name);
			break;
		case QOP_SORT_AZ:
		case QOP_SORT_ZA:
			sort_args = op->code == QOP_SORT_AZ ? "(a, b)" : "(b, a)";
			decl_list_ptr += sprintf(decl_list_ptr, "const a%d = [];", iter.index);
			code_ptr += sprintf(code_ptr, "a%d.push([ op%d_a(value, i), value ]); }", iter.index, iter.index);
			for (i = 0; i < num_overs_open; ++i)  // close loops for `over()`
				code_ptr += sprintf(code_ptr, "}");
			num_overs_open = 0;
			code_ptr += sprintf(code_ptr, "a%d.sort(%s => a[0] < b[0] ? -1 : b[0] < a[0] ? 1 : 0);", iter.index, sort_args);
			code_ptr += sprintf(code_ptr, "for (let ii = 0, len = a%d.length; ii < len; ++ii) { value = a%d[ii][1];", iter.index, iter.index);
			has_transforms = true;
			break;
		case QOP_TAKE_N:
			decl_list_ptr += sprintf(decl_list_ptr, "let c%d = op%d_a;", iter.index, iter.index);
			code_ptr += sprintf(code_ptr, "if (c%d-- === 0) break main_loop;", iter.index);
			break;
		case QOP_TAP:
		case QOP_THRU:
			code_ptr += op->code == QOP_THRU
				? sprintf(code_ptr, "%s = op%d_a(%s);", source_name, iter.index, source_name)
				: sprintf(code_ptr, "op%d_a(%s);", iter.index, source_name);
			break;
		default:
			// unsupported opcode, inject an error into the compiled code
			code_ptr += sprintf(code_ptr, "throw Error(`Unsupported QOP %xh (internal error)`);", op->code);
		}
	}
	if (!loop_open && opcode != ROP_ITERATOR && (opcode != ROP_TO_ARRAY || num_overs_open > 0)) {
		// fast path for closed loop is only valid for iterator and toArray(), all others must reopen
		code_ptr += sprintf(code_ptr, "for (let ii = 0, len = %s.length; ii < len; ++ii) { value = %s[i];",
			source_name, source_name);
		loop_open = true;
	}
	switch (opcode) {
	case ROP_CONTAINS:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = false;");
		decl_list_ptr += sprintf(decl_list_ptr, "const is = r1 !== r1 ? (x, y) => x !== x && y !== y : (x, y) => x === y;");
		code_ptr += sprintf(code_ptr, "if (is(value, r1)) return true;");
		break;
	case ROP_COUNT:
		decl_list_ptr += sprintf(decl_list_ptr, "let result = r1 === undefined ? 0 : {};");
		code_ptr += sprintf(code_ptr, "if (r1 === undefined) { ++result; } else { const key = r1(value); if (result[key]!==undefined) ++result[key]; else result[key] = 1; }");
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
	case ROP_FIND_KEY:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = -1;");
		code_ptr += has_transforms
			? sprintf(code_ptr, "throw TypeError(`'findIndex()' cannot be used with transformations`);")
			: sprintf(code_ptr, "if (r1(value)) return i;");
		break;
	case ROP_FIRST:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = undefined;");
		code_ptr += sprintf(code_ptr, "return r1 !== undefined ? r1(value) : value;");
		break;
	case ROP_FOR_EACH:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = undefined;");
		code_ptr += sprintf(code_ptr, "r1(value, i);");
		break;
	case ROP_ITERATOR:
		// note: performance of generator functions in ChakraCore is atrocious so ROP_ITERATOR should ideally
		//       be implemented using something other than `yield` at some point.
		is_generator = true;
		decl_list_ptr += sprintf(decl_list_ptr, "const result = undefined;");
		code_ptr += loop_open
			? sprintf(code_ptr, "yield value;")
			: sprintf(code_ptr, "yield* %s;", source_name);
		break;
	case ROP_LAST:
		decl_list_ptr += sprintf(decl_list_ptr, "let result = undefined;");
		code_ptr += sprintf(code_ptr, "result = value;");
		epilogue_ptr += sprintf(epilogue_ptr, "if (r1 !== undefined) result = r1(result);");
		break;
	case ROP_REDUCE:
		decl_list_ptr += sprintf(decl_list_ptr, "let result = r2;");
		code_ptr += sprintf(code_ptr, "result = r1(result, value);");
		break;
	case ROP_REMOVE:
		decl_list_ptr += sprintf(decl_list_ptr, "const result = undefined;");
		decl_list_ptr += sprintf(decl_list_ptr, "const removals = [];");
		code_ptr += has_transforms
			? sprintf(code_ptr, "throw TypeError(`'remove()' cannot be used with transformations`);")
			: sprintf(code_ptr, "if (r1 === undefined || r1(value, i)) removals.push([ source, i ]);");
		epilogue_ptr += sprintf(epilogue_ptr,
			"let r = 0; for (let m = 0, len = sources.length; m < len; ++m) { const source = sources[m];"
			"if (typeof source.length === 'number') { let j = 0; for (let i = 0, len = source.length; i < len; ++i) { if (i === removals[r][1]) { ++r; continue; } source[j++] = source[i]; } source.length = j; }"
			"else { for (let i = 0, len = removals.length; i < len; ++i) if (removals[i][0] === source) delete source[removals[i][1]]; }"
			"}");
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
		code_ptr += loop_open
			? sprintf(code_ptr, "result.push(value);")
			: sprintf(code_ptr, "return %s;", source_name);
		break;
	case ROP_UPDATE:
		decl_list_ptr += sprintf(decl_list_ptr, "let result = undefined;");
		code_ptr += has_transforms
			? sprintf(code_ptr, "throw TypeError(`'update()' cannot be used with transformations`);")
			: sprintf(code_ptr, "source[i] = r1 !== undefined ? r1(value) : value;");
		break;
	default:
		code_ptr += sprintf(code_ptr, "throw Error(`Unsupported ROP %xh (internal error)`);", opcode);
	}
	for (i = 0; i < num_overs_open; ++i)  // close extra `over()` loops
		code_ptr += sprintf(code_ptr, "}");
	if (loop_open)  // main loop still open?  close it now.
		code_ptr += sprintf(code_ptr, "}");
	sprintf(filename, "%%/fromQuery/%"PRIx64".js", jit_id);
	wrapper = lstr_newf(
		"(function fromQuery(%s r1, r2) { return %s runQuery(...sources) { %s"
		"main_loop: for (let m = 0, len = sources.length; m < len; ++m) { let source = sources[m];"
		"if (typeof source.length === 'number') { for (let i = 0, len = source.length; i < len; ++i) { value = source[i]; %s }"
		"else if (typeof source[Symbol.iterator] === 'function') { for (let value of source) { %s }"
		"else { for (const i of Object.keys(source)) { value = source[i]; %s }"
		"} %s return result; } })",
		arg_list, is_generator ? "function*" : "function", decl_list, code, code, code, epilogue);
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

static void
emit_shuffle(char** p_ptr, const char* array_name)
{
	// emit a Fisher-Yates shuffle of `array_name`.
	*p_ptr += sprintf(*p_ptr, "for (let i = %s.length - 1; i >= 1; --i) {", array_name);
	*p_ptr += sprintf(*p_ptr, "const idx = Math.floor(Math.random() * i), v = %s[idx];", array_name);
	*p_ptr += sprintf(*p_ptr, "%s[idx] = %s[i];", array_name, array_name);
	*p_ptr += sprintf(*p_ptr, "%s[i] = v; }", array_name);
}
