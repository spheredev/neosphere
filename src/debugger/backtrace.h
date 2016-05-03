#ifndef SSJ__BACKTRACE_H__INCLUDED
#define SSJ__BACKTRACE_H__INCLUDED

typedef struct backtrace backtrace_t;

backtrace_t* backtrace_new           (void);
void         backtrace_free          (backtrace_t* obj);
int          backtrace_len           (const backtrace_t* obj);
const char*  backtrace_get_call_name (const backtrace_t* obj, int index);
const char*  backtrace_get_filename  (const backtrace_t* obj, int index);
int          backtrace_get_linenum   (const backtrace_t* obj, int index);
void         backtrace_add           (backtrace_t* obj, const char* call_name, const char* filename, int line_no);
void         backtrace_print         (const backtrace_t* obj, int active_frame, bool show_all);

#endif // SSJ__BACKTRACE_H__INCLUDED
