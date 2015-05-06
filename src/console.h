#ifndef MINISPHERE__CONSOLE_H__INCLUDED
#define MINISPHERE__CONSOLE_H__INCLUDED

extern void initialize_console (int log_level);
extern void shutdown_console   (void);
extern void console_log        (int log_level, const char* fmt, ...);

#endif // MINISPHERE__CONSOLE_H__INCLUDED
