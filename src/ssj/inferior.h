#ifndef SSJ__INFERIOR_H__INCLUDED
#define SSJ__INFERIOR_H__INCLUDED

#include "backtrace.h"
#include "message.h"
#include "source.h"

typedef struct inferior inferior_t;

typedef
enum resume_op
{
	OP_RESUME,
	OP_STEP_OVER,
	OP_STEP_IN,
	OP_STEP_OUT,
} resume_op_t;

void               inferiors_init       (void);
void               inferiors_deinit     (void);
inferior_t*        inferior_new         (const char* hostname, int port);
void               inferior_free        (inferior_t* o);
bool               inferior_tick        (inferior_t* o);
bool               inferior_is_attached (const inferior_t* o);
bool               inferior_is_running  (const inferior_t* o);
const char*        inferior_author      (const inferior_t* o);
const char*        inferior_title       (const inferior_t* o);
const source_t*    inferior_get_source  (inferior_t* o, const char* filename);
const backtrace_t* inferior_get_stack   (inferior_t* o);
void               inferior_detach      (inferior_t* o);
dvalue_t*          inferior_eval        (inferior_t* o, const char* expr, int frame, bool* out_is_error);
bool               inferior_pause       (inferior_t* o);
message_t*         inferior_request     (inferior_t* o, message_t* msg);
bool               inferior_resume      (inferior_t* o, resume_op_t op);

#endif // SSJ__INFERIOR_H__INCLUDED
