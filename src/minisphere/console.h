#ifndef MINISPHERE__CONSOLE_H__INCLUDED
#define MINISPHERE__CONSOLE_H__INCLUDED

extern void initialize_console (int verbosity);
extern int  get_log_verbosity  (void);
extern void console_log        (int level, const char* fmt, ...);

#endif // MINISPHERE__CONSOLE_H__INCLUDED
