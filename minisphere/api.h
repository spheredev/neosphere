typedef enum js_error js_error_t;

extern void init_api           (duk_context* ctx);
extern void register_api_const (duk_context* ctx, const char* name, double value);
extern void register_api_func  (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
extern void bail_out_game      (void);
extern void js_error           (js_error_t type, int stack_offset, const char* fmt, ...);

#define _JS_C_FUNC_ARG_LIST_ duk_context* ctx

#define js_return_t duk_ret_t

#define js_return            return 0
#define js_return_cstr(cstr) return duk_push_string(ctx, cstr), 1
#define js_return_int(val)   return duk_push_int(ctx, val), 1

#define js_return_sphereobj(typename, object) return duk_push_sphere_##typename(ctx, object), 1

enum js_error
{
	JS_ERROR,
	JS_EVAL_ERROR,
	JS_RANGE_ERROR,
	JS_REF_ERROR,
	JS_TYPE_ERROR,
	JS_URI_ERROR
};
