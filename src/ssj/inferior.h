#ifndef SSJ__INFERIOR_H__INCLUDED
#define SSJ__INFERIOR_H__INCLUDED

#include "message.h"

typedef struct inferior inferior_t;

typedef
enum resume_op
{
	OP_RESUME,
	OP_STEP_OVER,
	OP_STEP_IN,
	OP_STEP_OUT,
} resume_op_t;

void        inferiors_init    (void);
void        inferiors_deinit  (void);
inferior_t* inferior_new      (const char* hostname, int port);
void        inferior_free     (inferior_t* inf);
bool        inferior_is_ready (inferior_t* inf);
message_t*  inferior_request  (inferior_t* inf, message_t* msg);
bool        inferior_pause    (inferior_t* inf);
bool        inferior_resume   (inferior_t* inf, resume_op_t op);
void        inferior_run      (inferior_t* inf);

#endif // SSJ__INFERIOR_H__INCLUDED
