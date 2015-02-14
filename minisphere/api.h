extern void init_api              (duk_context* ctx);
extern void register_api_const    (duk_context* ctx, const char* name, double value);
extern void register_api_func     (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
extern void duk_push_sphere_Font  (duk_context* ctx, ALLEGRO_FONT* font);

