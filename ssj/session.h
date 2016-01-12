#ifndef SSJ__SESSION_H__INCLUDED
#define SSJ__SESSION_H__INCLUDED

typedef struct session session_t;

session_t* new_session (const char* hostname, int port);

#endif // SSJ__SESSION_H__INCLUDED
