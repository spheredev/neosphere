#ifndef MINISPHERE__DEBUGGER_H__INCLUDED
#define MINISPHERE__DEBUGGER_H__INCLUDED

extern void        initialize_debugger  (bool want_attach, bool allow_remote);
extern void        shutdown_debugger    (void);
extern void        update_debugger      (void);
extern bool        is_debugger_attached (void);
extern const char* get_source_pathname  (const char* pathname);
extern void        debug_print          (const char* text);

#endif // MINISPHERE__DEBUGGER_H__INCLUDED
