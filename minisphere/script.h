#ifndef MINISPHERE__SCRIPT_H__INCLUDED
#define MINISPHERE__SCRIPT_H__INCLUDED

extern int  compile_script (const lstring_t* script, const char* name);
extern void free_script    (int script_id);
extern void run_script     (int script_id, bool allow_reentry);

#endif // MINISPHERE__SCRIPT_H__INCLUDED
