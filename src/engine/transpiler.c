#include "minisphere.h"
#include "transpiler.h"

#include "debugger.h"

static bool load_coffeescript (void);
static bool load_typescript   (void);

void
initialize_transpiler(void)
{
	console_log(1, "initializing JS transpiler");
	load_coffeescript();
	load_typescript();
}

void
shutdown_transpiler(void)
{
	console_log(1, "shutting down JS transpiler");
}

bool
transpile_to_js(lstring_t** p_source, const char* filename)
{
	const char* extension;
	lstring_t*  js_text;

	// is it a CoffeeScript file? you know, I don't even like coffee.
	// now, Monster drinks on the other hand...
	extension = strrchr(filename, '.');
	if (extension != NULL && strcasecmp(extension, ".coffee") == 0) {
		if (!duk_get_global_string(g_duk, "CoffeeScript")) {
			duk_pop(g_duk);
			duk_push_error_object(g_duk, DUK_ERR_ERROR, "no CoffeeScript support (%s)", filename);
			goto on_error;
		}
		duk_get_prop_string(g_duk, -1, "compile");
		duk_push_lstring_t(g_duk, *p_source);
		duk_push_object(g_duk);
		duk_push_string(g_duk, filename);
		duk_put_prop_string(g_duk, -2, "filename");
		duk_push_true(g_duk);
		duk_put_prop_string(g_duk, -2, "bare");
		if (duk_pcall(g_duk, 2) != DUK_EXEC_SUCCESS) {
			duk_remove(g_duk, -2);  // remove `CoffeeScript` from stack
			goto on_error;
		}
		js_text = duk_require_lstring_t(g_duk, -1);
		cache_source(filename, js_text);
		duk_pop_2(g_duk);
		lstr_free(*p_source);
		*p_source = js_text;
	}
	// TypeScript?
	else if (extension != NULL && strcasecmp(extension, ".ts") == 0) {
		if (!duk_get_global_string(g_duk, "ts")) {
			duk_pop(g_duk);
			duk_push_error_object(g_duk, DUK_ERR_ERROR, "no TypeScript support (%s)", filename);
			goto on_error;
		}
		duk_get_prop_string(g_duk, -1, "transpile");
		duk_push_lstring_t(g_duk, *p_source);
		duk_push_object(g_duk);
		duk_push_boolean(g_duk, true);
		duk_put_prop_string(g_duk, -2, "noImplicitUseStrict");
		duk_push_string(g_duk, filename);
		if (duk_pcall(g_duk, 3) != DUK_EXEC_SUCCESS) {
			duk_remove(g_duk, -2);  // remove `ts` from stack
			goto on_error;
		}
		js_text = duk_require_lstring_t(g_duk, -1);
		cache_source(filename, js_text);
		duk_pop_2(g_duk);
		lstr_free(*p_source);
		*p_source = js_text;
	}
	return true;

on_error:
	// note: when we return false, the caller expects the JS error which caused the
	//       operation to fail to be left on top of the Duktape value stack.
	return false;
}

static bool
load_coffeescript(void)
{
	if (sfs_fexist(g_fs, "#/coffee-script.js", NULL)) {
		if (evaluate_script("#/coffee-script.js")) {
			if (!duk_get_global_string(g_duk, "CoffeeScript")) {
				duk_pop_2(g_duk);
				console_log(1, "    'CoffeeScript' not defined");
				goto on_error;
			}
			duk_get_prop_string(g_duk, -1, "VERSION");
			console_log(1, "    CoffeeScript %s", duk_get_string(g_duk, -1));
			duk_pop_3(g_duk);
		}
		else {
			console_log(1, "    error evaluating coffee-script.js");
			console_log(1, "    %s", duk_to_string(g_duk, -1));
			duk_pop(g_duk);
			goto on_error;
		}
	}
	else {
		console_log(1, "  coffee-script.js is missing");
		goto on_error;
	}
	return true;

on_error:
	console_log(1, "    CoffeeScript support not enabled");
	return false;
}

static bool
load_typescript(void)
{
	if (sfs_fexist(g_fs, "#/typescriptServices.js", NULL)) {
		if (evaluate_script("#/typescriptServices.js")) {
			if (!duk_get_global_string(g_duk, "ts")) {
				duk_pop_2(g_duk);
				console_log(1, "    'ts' not defined");
				goto on_error;
			}
			duk_get_prop_string(g_duk, -1, "version");
			console_log(1, "    TypeScript %s", duk_get_string(g_duk, -1));
			duk_pop_3(g_duk);
		}
		else {
			console_log(1, "    error evaluating typescriptServices.js");
			console_log(1, "    %s", duk_to_string(g_duk, -1));
			duk_pop(g_duk);
			goto on_error;
		}
	}
	else {
		console_log(1, "  typescriptServices.js is missing");
		goto on_error;
	}
	return true;

on_error:
	console_log(1, "    TypeScript support not enabled");
	return false;
}
