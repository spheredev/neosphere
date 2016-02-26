#ifndef SSJ__BACKTRACE_H__INCLUDED
#define SSJ__BACKTRACE_H__INCLUDED

typedef struct backtrace backtrace_t;

backtrace_t* backtrace_new          (void);
void         backtrace_free         (backtrace_t* bt);
int          backtrace_len          (const backtrace_t* bt);
const char*  backtrace_get_filename (const backtrace_t* bt, int index);
int          backtrace_get_lineno   (const backtrace_t* bt, int index);
const char*  backtrace_get_name     (const backtrace_t* bt, int index);
void         backtrace_add          (backtrace_t* bt, const char* name, const char* filename, int line_no);
void         backtrace_print        (const backtrace_t* bt, int active_frame, bool show_all);

#endif // SSJ__BACKTRACE_H__INCLUDED
