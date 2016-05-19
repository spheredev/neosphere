#ifndef MINISPHERE__COMMONJS_H__INCLUDED
#define MINISPHERE__COMMONJS_H__INCLUDED

duk_int_t duk_peval_module (const char* filename);
path_t*   cjs_resolve      (const char* id, const char* origin, const char* sys_origin);

void init_commonjs_api (void);

#endif // MINISPHERE__COMMONJS_H__INCLUDED
