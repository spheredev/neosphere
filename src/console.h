#ifndef MINISPHERE__CONSOLE_H__INCLUDED
#define MINISPHERE__CONSOLE_H__INCLUDED

extern int  get_log_level (void);
extern void set_log_level (int log_level);
extern void console_log   (int log_level, const char* fmt, ...);

#endif // MINISPHERE__CONSOLE_H__INCLUDED
