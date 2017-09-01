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

#include "minisphere.h"
#include "script.h"

#include "api.h"
#include "debugger.h"
#include "jsal.h"
#include "pegasus.h"
#include "utility.h"

struct script
{
	unsigned int  refcount;
	unsigned int  id;
	bool          is_in_use;
	js_ref_t*     function;
};

static int s_next_script_id = 1;

void
scripts_init(void)
{
	console_log(1, "initializing JS script manager");
}

void
scripts_uninit(void)
{
	console_log(1, "shutting down JS script manager");
}

bool
script_eval(const char* filename, bool as_module)
{
	file_t*     file = NULL;
	path_t*     path = NULL;
	const char* source_name;
	lstring_t*  source_text = NULL;
	int         stack_top;
	char*       slurp;
	size_t      size;

	stack_top = jsal_get_top();

	if (as_module) {
		// the existence check here is needed because eval_module() will segfault if the
		// file doesn't exist.  it's an ugly hack, but a proper fix needs some refactoring
		// that I'm not up for right now.
		if (!game_file_exists(g_game, filename))
			goto on_error;
		if (!jsal_pegasus_eval_module(filename))
			goto on_error;
		return true;
	}
	else {
		path = game_full_path(g_game, filename, NULL, false);
		source_name = debugger_source_name(path_cstr(path));
		if (!(slurp = game_read_file(g_game, filename, &size)))
			goto on_error;
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
	}

on_error:
	lstr_free(source_text);
	path_free(path);
	if (jsal_get_top() == stack_top)  // note: ugly hack, refactor
		jsal_push_new_error(JS_REF_ERROR, "script not found '%s'\n", filename);
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
	name = lstr_vnewf(fmt_name, ap);
	va_end(ap);
	script = calloc(1, sizeof(script_t));

	console_log(3, "compiling script #%u as '%s'", s_next_script_id, lstr_cstr(name));

	// compile the source.  Duktape gives us a function back; save its heap pointer so
	// we can call the script later.
	jsal_push_lstring_t(source);
	jsal_compile(lstr_cstr(name));
	function = jsal_ref(-1);
	jsal_pop(1);

	debugger_cache_source(lstr_cstr(name), source);
	lstr_free(name);

	script->id = s_next_script_id++;
	script->function = function;
	return script_ref(script);
}

script_t*
script_new_func(int idx)
{
	js_ref_t* function;
	script_t* script;

	idx = jsal_normalize_index(idx);
	jsal_require_function(idx);
	function = jsal_ref(idx);

	if (!(script = calloc(1, sizeof(script_t))))
		return NULL;
	script->id = s_next_script_id++;
	script->function = function;
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

	console_log(3, "disposing script #%u no longer in use", script->id);

	jsal_unref(script->function);
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
	if (script->is_in_use && !allow_reentry) {
		console_log(3, "skipping execution of script #%u, already in use", script->id);
		return;
	}
	was_in_use = script->is_in_use;

	console_log(3, "executing script #%u", script->id);

	// ref the script in case it gets freed during execution.  the owner
	// may be destroyed in the process and we don't want to end up crashing.
	script_ref(script);

	// execute the script!
	script->is_in_use = true;
	jsal_push_ref(script->function);
	jsal_call(0);
	jsal_pop(1);
	script->is_in_use = was_in_use;

	script_unref(script);
}
