#ifndef SSJ__SESSION_H__INCLUDED
#define SSJ__SESSION_H__INCLUDED

typedef struct session session_t;

typedef
enum exec_op
{
	EXEC_RESUME,
	EXEC_STEP_OVER,
	EXEC_STEP_IN,
	EXEC_STEP_OUT,
} exec_op_t;

session_t* new_session     (const char* hostname, int port);
void       execute_next    (session_t* sess, exec_op_t op);
void       print_backtrace (session_t* sess, size_t frame, bool show_all);
void       print_eval      (session_t* sess, const char* expr, size_t frame);
void       print_locals    (session_t* sess, size_t frame);
void       print_source    (session_t* sess, size_t line, size_t window);
void       run_session     (session_t* sess);

#endif // SSJ__SESSION_H__INCLUDED
