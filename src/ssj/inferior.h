#ifndef SSJ__INFERIOR_H__INCLUDED
#define SSJ__INFERIOR_H__INCLUDED

#include "backtrace.h"
#include "message.h"
#include "objview.h"
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
void               inferior_free        (inferior_t* obj);
bool               inferior_update      (inferior_t* obj);
bool               inferior_is_attached (const inferior_t* obj);
bool               inferior_is_running  (const inferior_t* obj);
const char*        inferior_author      (const inferior_t* obj);
const char*        inferior_title       (const inferior_t* obj);
const backtrace_t* inferior_get_calls   (inferior_t* obj);
objview_t*         inferior_get_object  (inferior_t* obj, remote_ptr_t heapptr, bool get_all);
const source_t*    inferior_get_source  (inferior_t* obj, const char* filename);
objview_t*         inferior_get_vars    (inferior_t* obj, int frame);
void               inferior_detach      (inferior_t* obj);
dvalue_t*          inferior_eval        (inferior_t* obj, const char* expr, int frame, bool* out_is_error);
bool               inferior_pause       (inferior_t* obj);
message_t*         inferior_request     (inferior_t* obj, message_t* msg);
bool               inferior_resume      (inferior_t* obj, resume_op_t op);

#endif // SSJ__INFERIOR_H__INCLUDED
