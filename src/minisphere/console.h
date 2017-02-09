#ifndef MINISPHERE__CONSOLE_H__INCLUDED
#define MINISPHERE__CONSOLE_H__INCLUDED

void initialize_console (int verbosity);
int  get_log_verbosity  (void);
void console_log        (int level, const char* fmt, ...);

#endif // MINISPHERE__CONSOLE_H__INCLUDED
