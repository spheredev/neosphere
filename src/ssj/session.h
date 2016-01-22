#ifndef SSJ__SESSION_H__INCLUDED
#define SSJ__SESSION_H__INCLUDED

typedef struct session session_t;

session_t* new_session    ();
bool       attach_session (session_t* sess, const char* hostname, int port);
void       run_session    (session_t* sess);

#endif // SSJ__SESSION_H__INCLUDED
