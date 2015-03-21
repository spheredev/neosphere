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
