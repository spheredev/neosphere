#ifndef MINISPHERE__DEBUGGER_H__INCLUDED
#define MINISPHERE__DEBUGGER_H__INCLUDED

void        initialize_debugger  (bool want_attach, bool allow_remote);
void        shutdown_debugger    (void);
void        update_debugger      (void);
bool        is_debugger_attached (void);
const char* get_compiled_name    (const char* source_name);
const char* get_source_name      (const char* pathname);
void        cache_source         (const char* name, const lstring_t* text);
void        debug_print          (const char* text);

#endif // MINISPHERE__DEBUGGER_H__INCLUDED
