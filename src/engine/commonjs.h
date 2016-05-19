#ifndef MINISPHERE__COMMONJS_H__INCLUDED
#define MINISPHERE__COMMONJS_H__INCLUDED

bool    cjs_require (const char* id, const char* caller_id);
path_t* cjs_resolve (const char* id, const char* caller_id);

void init_commonjs_api (void);

#endif // MINISPHERE__COMMONJS_H__INCLUDED