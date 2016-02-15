#ifndef SSJ__SESSION_H__INCLUDED
#define SSJ__SESSION_H__INCLUDED

typedef struct session   session_t;
typedef struct file_line file_line_t;

typedef
enum exec_op
{
	EXEC_RESUME,
	EXEC_STEP_OVER,
	EXEC_STEP_IN,
	EXEC_STEP_OUT,
} exec_op_t;

session_t* new_session       (const char* hostname, int port);
void       clear_breakpoint  (session_t* sess, const char* filename, int line_no);
void       execute_next      (session_t* sess, exec_op_t op);
void       print_backtrace   (session_t* sess, int frame, bool show_all);
void       print_breakpoints (session_t* sess);
void       print_eval        (session_t* sess, const char* expr, int frame, bool show_metadata);
void       print_locals      (session_t* sess, int frame);
void       print_source      (session_t* sess, const char* filename, int line_no, int window);
void       run_session       (session_t* sess);
int        set_breakpoint    (session_t* sess, const char* filename, int line_no);

#endif // SSJ__SESSION_H__INCLUDED
