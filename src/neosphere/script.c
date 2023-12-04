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

#include "neosphere.h"
#include "script.h"

#include "api.h"
#include "jsal.h"
#include "pegasus.h"
#include "source_map.h"
#include "utility.h"

struct script
{
	unsigned int  refcount;
	unsigned int  id;
	js_ref_t*     function;
	bool          in_use;
	js_ref_t*     this_arg;
};

static bool js_onScriptFinished (int num_args, bool is_ctor, intptr_t magic);

static js_ref_t* s_key_then;
static int       s_next_script_id = 1;

void
scripts_init(void)
{
	console_log(1, "initializing JS script manager");

	s_key_then = jsal_new_key("then");
}

void
scripts_uninit(void)
{
	console_log(1, "shutting down JS script manager");

	jsal_unref(s_key_then);
}

bool
script_eval(const char* filename)
{
	path_t*     path = NULL;
	const char* source_name;
	lstring_t*  source_text = NULL;
	int         stack_top;
	char*       slurp;
	size_t      size;

	stack_top = jsal_get_top();

	path = game_full_path(g_game, filename, NULL, false);
	source_name = source_map_alias_of(path_cstr(path));
	if (!(slurp = game_read_file(g_game, filename, &size))) {
		jsal_push_new_error(JS_REF_ERROR, "script not found '%s'\n", filename);
		goto on_error;
	}
	source_text = lstr_from_cp1252(slurp, size);
	free(slurp);

	// ready for launch in T-10...9...*munch*
	jsal_push_lstring_t(source_text);
	if (!jsal_try_compile(source_name))
		goto on_error;
	if (!jsal_try_call(0))
		goto on_error;

	lstr_free(source_text);
	path_free(path);
	return true;

on_error:
	lstr_free(source_text);
	path_free(path);
	return false;
}

script_t*
script_new(const lstring_t* source, const char* fmt_name, ...)
{
	va_list    ap;
	js_ref_t*  function;
	lstring_t* name;
	script_t*  script;

	va_start(ap, fmt_name);
	if (!(name = lstr_vnewf(fmt_name, ap)))
		goto on_error;
	va_end(ap);

	if (!(script = calloc(1, sizeof(script_t))))
		goto on_error;

	console_log(4, "compiling script #%u as '%s'", s_next_script_id, lstr_cstr(name));

	// compile the source.  JSAL gives us a function back; save its heap pointer so
	// we can call the script later.
	jsal_push_lstring_t(source);
	jsal_compile(lstr_cstr(name));
	function = jsal_pop_ref();

	source_map_add_source(lstr_cstr(name), lstr_cstr(source));
	lstr_free(name);

	script->id = s_next_script_id++;
	script->function = function;
	return script_ref(script);

on_error:
	lstr_free(name);
	return NULL;
}

script_t*
script_new_function(int stack_index)
{
	stack_index = jsal_normalize_index(stack_index);
	jsal_push_undefined();
	return script_new_method(stack_index);
}

script_t*
script_new_method(int stack_index)
{
	// this_arg should be on top of the JSAL stack before calling this.
	
	js_ref_t* function;
	script_t* script;
	js_ref_t* this_arg;

	stack_index = jsal_normalize_index(stack_index);
	jsal_require_function(stack_index);
	function = jsal_ref(stack_index);
	this_arg = jsal_pop_ref();

	if (!(script = calloc(1, sizeof(script_t))))
		return NULL;
	script->id = s_next_script_id++;
	script->function = function;
	script->this_arg = this_arg;
	return script_ref(script);
}

script_t*
script_ref(script_t* script)
{
	++script->refcount;
	return script;
}

void
script_unref(script_t* script)
{
	if (script == NULL || --script->refcount > 0)
		return;

	console_log(4, "disposing script #%u no longer in use", script->id);

	jsal_unref(script->function);
	jsal_unref(script->this_arg);
	free(script);
}

void
script_run(script_t* script, bool allow_reentry)
{
	bool was_in_use;
	
	if (script == NULL)  // NULL is allowed, it's a no-op
		return;

	// check whether an instance of the script is already running.
	// if it is, but the script is reentrant, allow it.  otherwise, return early
	// to prevent multiple invocation.
	was_in_use = script->in_use;
	if (was_in_use && !allow_reentry) {
		console_log(4, "skipping execution of script #%u, already in use", script->id);
		return;
	}

	console_log(4, "executing script #%u", script->id);

	// ref the script in case it gets freed during execution.  the owner
	// may be destroyed in the process and we don't want to end up crashing.
	script_ref(script);

	// execute the script!
	script->in_use = true;
	jsal_push_ref_weak(script->function);
	if (script->this_arg != NULL)
		jsal_push_ref_weak(script->this_arg);
	else
		jsal_push_undefined();
	jsal_call_method(0);
	if (jsal_is_object(-1) && jsal_has_prop_key(-1, s_key_then)) {
		jsal_get_prop_string(-1, "then");
		jsal_pull(-2);
		jsal_push_new_function(js_onScriptFinished, "", 0, false, (intptr_t)script);
		jsal_call_method(1);
		jsal_pop(1);
	}
	else {
		jsal_pop(1);
		script->in_use = was_in_use;
		script_unref(script);
	}

}

static bool
js_onScriptFinished(int num_args, bool is_ctor, intptr_t magic)
{
	script_t* script;

	script = (script_t*)magic;
	script->in_use = false;
	script_unref(script);
	return false;
}
