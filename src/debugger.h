#ifndef MINISPHERE__DEBUGGER_H__INCLUDED
#define MINISPHERE__DEBUGGER_H__INCLUDED

extern void initialize_debugger (bool want_attach, bool allow_remote);
extern void shutdown_debugger   (void);
extern void update_debugger     (void);

#endif // MINISPHERE__DEBUGGER_H__INCLUDED
