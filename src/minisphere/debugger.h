#ifndef MINISPHERE__DEBUGGER_H__INCLUDED
#define MINISPHERE__DEBUGGER_H__INCLUDED

typedef
enum print_op
{
	PRINT_NORMAL,
	PRINT_ASSERT,
	PRINT_DEBUG,
	PRINT_ERROR,
	PRINT_INFO,
	PRINT_TRACE,
	PRINT_WARN,
} print_op_t;

void        debugger_init          (bool want_attach, bool allow_remote);
void        debugger_uninit        (void);
void        debugger_update        (void);
bool        debugger_attached      (void);
color_t     debugger_color         (void);
const char* debugger_name          (void);
const char* debugger_compiled_name (const char* source_name);
const char* debugger_source_name   (const char* pathname);
void        debugger_cache_source  (const char* name, const lstring_t* text);
void        debugger_log           (const char* text, print_op_t op, bool use_console);

#endif // MINISPHERE__DEBUGGER_H__INCLUDED
