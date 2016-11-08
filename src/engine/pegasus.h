#ifndef MINISPHERE__PEGASUS_H__INCLUDED
#define MINISPHERE__PEGASUS_H__INCLUDED

void initialize_pegasus_api (duk_context* ctx);
void pegasus_run            (void);

duk_bool_t duk_pegasus_eval_module (duk_context* ctx, const char* filename);

#endif // MINISPHERE__PEGASUS_H__INCLUDED
