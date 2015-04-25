#ifndef MINISPHERE__SCRIPT_H__INCLUDED
#define MINISPHERE__SCRIPT_H__INCLUDED

typedef struct script script_t;

extern script_t* compile_script (const lstring_t* script, const char* name);
extern void      free_script    (script_t* script);
extern void      run_script     (script_t* script, bool allow_reentry);

extern script_t* duk_require_sphere_script (duk_context* ctx, duk_idx_t index, const char* name);

#endif // MINISPHERE__SCRIPT_H__INCLUDED
