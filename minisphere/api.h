typedef enum js_error js_error_t;

extern void init_api           (duk_context* ctx);
extern void register_api_const (duk_context* ctx, const char* name, double value);
extern void register_api_func  (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
extern void bail_out_game      (void);
extern void js_error           (js_error_t type, int stack_offset, const char* fmt, ...);

enum js_error
{
	JS_ERROR,
	JS_EVAL_ERROR,
	JS_RANGE_ERROR,
	JS_REF_ERROR,
	JS_TYPE_ERROR,
	JS_URI_ERROR
};


// the following set of macros is a blight on humanity and has no right to exist; alas, these are
// necessary for JS engine independence.
#define js_retval_t duk_ret_t
#define _JS_C_FUNC_ARG_LIST_ duk_context* ctx

#define js_begin_api_func(name, min_args) const char* _api_name = name; int _n_args = duk_get_top(ctx); \
	if (_n_args < min_args) js_error(JS_ERROR, -1, "%s(): Wrong number of arguments (%i or more required)", _api_name, min_args)

#define js_bool_arg(argnum, varname) bool varname; \
	else if (!duk_is_boolean(ctx, (argnum) - 1)) js_error(JS_TYPE_ERROR, -1, "%s(): Parameter %i must be boolean (true/false)\n\Value: %s", _api_name, argnum, duk_to_string(ctx, (argnum) - 1)); \
	else varname = duk_require_boolean(ctx, (argnum) - 1)
#define js_maybe_bool_arg(argnum, varname, defval) bool varname; \
	if (_n_args < argnum) varname = defval; \
	else if (!duk_is_boolean(ctx, (argnum) - 1)) js_error(JS_TYPE_ERROR, -1, "%s(): Optional parameter %i must be boolean (true/false)\n\nValue: %s", _api_name, argnum, duk_to_string(ctx, (argnum) - 1)); \
	else varname = duk_require_boolean(ctx, (argnum) - 1)

#define js_int_arg(argnum, varname) int varname; \
	if (!duk_is_number(ctx, (argnum) - 1)) js_error(JS_TYPE_ERROR, -1, "%s(): Parameter %i must be a number\n\nValue: %s", _api_name, argnum, duk_to_string(ctx, (argnum) - 1)); \
	else varname = duk_require_int(ctx, (argnum) - 1)
#define js_maybe_int_arg(argnum, varname, defval) int varname; \
	if (_n_args < (argnum)) varname = defval; \
	else if (!duk_is_number(ctx, (argnum) - 1)) js_error(JS_TYPE_ERROR, -1, "%s(): Optional parameter %i must be a number\n\nValue: %s", _api_name, argnum, duk_to_string(ctx, (argnum) - 1)); \
	else varname = duk_require_int(ctx, (argnum) - 1)

#define js_cstr_arg(argnum, varname) const char* varname = duk_require_string(ctx, (argnum) - 1)
#define js_sphereobj_arg(argnum, typename, varname) typename##_t \
	varname = duk_require_sphere_##typename(ctx, (argnum) - 1)

#define js_return return 0
#define js_return_cstr(cstr) return duk_push_string(ctx, cstr), 1
#define js_return_int(val) return duk_push_int(ctx, val), 1
#define js_return_number(val) return duk_push_number(ctx, val), 1
#define js_return_sphereobj(typename, object_ptr) return \
	duk_push_sphere_##typename(ctx, object_ptr), 1
