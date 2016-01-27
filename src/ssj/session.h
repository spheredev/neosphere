#ifndef SSJ__SESSION_H__INCLUDED
#define SSJ__SESSION_H__INCLUDED

typedef struct session session_t;

session_t* new_session     (const char* hostname, int port);
void       eval_expression (session_t* sess, const char* expr, size_t frame);
void       print_callstack (session_t* sess, size_t frame);
void       print_variables (session_t* sess, size_t frame);
void       run_session     (session_t* sess);

#endif // SSJ__SESSION_H__INCLUDED
