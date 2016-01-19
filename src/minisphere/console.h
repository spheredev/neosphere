#ifndef MINISPHERE__CONSOLE_H__INCLUDED
#define MINISPHERE__CONSOLE_H__INCLUDED

extern int  get_log_verbosity (void);
extern void set_log_verbosity (int verbosity);
extern void console_log       (int level, const char* fmt, ...);

#endif // MINISPHERE__CONSOLE_H__INCLUDED
