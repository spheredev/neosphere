#ifndef SSJ__SESSION_H__INCLUDED
#define SSJ__SESSION_H__INCLUDED

#include "inferior.h"
#include "parser.h"

typedef struct session session_t;

session_t*  session_new  (inferior_t* inferior);
void        session_free (session_t* ses);
void        session_run  (session_t* ses, bool run_now);

#endif // SSJ__SESSION_H__INCLUDED
