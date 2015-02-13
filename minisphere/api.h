extern void init_api              (duk_context* ctx);
extern void register_api_func     (duk_context* ctx, const char* ctor_name, const char* name, duk_c_function fn);
extern void duk_push_sphere_Font  (duk_context* ctx, ALLEGRO_FONT* font);
extern void duk_push_sphere_Image (duk_context* ctx, ALLEGRO_BITMAP* bitmap);
extern void duk_push_sphere_Log   (duk_context* ctx, ALLEGRO_FILE* file);
extern void duk_push_sphere_Sound (duk_context* ctx, ALLEGRO_AUDIO_STREAM* stream);
