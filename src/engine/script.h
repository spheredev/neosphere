#ifndef MINISPHERE__SCRIPT_H__INCLUDED
#define MINISPHERE__SCRIPT_H__INCLUDED

typedef struct script script_t;

void      scripts_init    (void);
void      scripts_uninit  (void);
bool      build_exec     (const char* filename, bool as_module);
script_t* script_new      (const lstring_t* script, const char* fmt_name, ...);
script_t* script_new_func (duk_context* ctx, duk_idx_t idx);
script_t* script_ref      (script_t* script);
void      script_free     (script_t* script);
void      script_run      (script_t* script, bool allow_reentry);

#endif // MINISPHERE__SCRIPT_H__INCLUDED
