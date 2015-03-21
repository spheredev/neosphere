typedef enum js_error js_error_t;

extern void init_api           (duk_context* ctx);
extern void register_api_const (duk_context* ctx, const char* name, double value);
extern void register_api_func  (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
extern void bail_out_game      (void);
extern void js_error           (js_error_t type, int stack_offset, const char* fmt, ...);

#define _JS_C_FUNC_ARGS_ duk_context* ctx

#define js_retval_t duk_ret_t

#define js_begin_api_func(name) const char* _api_name = name; int _n_args = duk_get_top(ctx)
#define js_require_num_args(n) if (_n_args < n) js_error(JS_ERROR, -1, "%s() called with wrong number of arguments (at least %i expected)", _api_name, n)
#define js_cstr_arg(argnum, varname) const char* varname = duk_require_string(ctx, (argnum) - 1)
#define js_int_arg(argnum, varname)  int varname; do { \
	if (!duk_is_number(ctx, (argnum) - 1)) js_error(JS_TYPE_ERROR, -1, "%s() argument %i must be a number", _api_name, argnum); \
	varname = duk_require_int(ctx, (argnum) - 1); \
	} while(0)
#define js_maybe_int_arg(argnum, varname, def_val) int varname; do { \
	varname = _n_args >= (argnum) ? duk_require_int(ctx, (argnum) - 1) : (def_val); \
	} while (0)
#define js_sphereobj_arg(argnum, typename, varname) typename##_t \
	varname = duk_require_sphere_##typename(ctx, (argnum) - 1)

#define js_return return 0
#define js_return_cstr(cstr) return duk_push_string(ctx, cstr), 1
#define js_return_int(val) return duk_push_int(ctx, val), 1
#define js_return_number(val) return duk_push_number(ctx, val), 1
#define js_return_sphereobj(typename, object_ptr) return \
	duk_push_sphere_##typename(ctx, object_ptr), 1

enum js_error
{
	JS_ERROR,
	JS_EVAL_ERROR,
	JS_RANGE_ERROR,
	JS_REF_ERROR,
	JS_TYPE_ERROR,
	JS_URI_ERROR
};
