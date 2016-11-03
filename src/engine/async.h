#ifndef MINISPHERE__ASYNC_H__INCLUDED
#define MINISPHERE__ASYNC_H__INCLUDED

#include "script.h"

typedef struct job job_t;

typedef
enum async_hint
{
	ASYNC_ASAP,
	ASYNC_RENDER,
	ASYNC_UPDATE,
	ASYNC_MAX
} async_hint_t;

void     async_init     (void);
void     async_uninit   (void);
void     async_cancel   (uint64_t token);
uint64_t async_defer    (script_t* script, double timeout, async_hint_t hint);
uint64_t async_recur    (script_t* script, double priority, async_hint_t hint);
void     async_run_jobs (async_hint_t hint);

#endif // MINISPHERE__ASYNC_H__INCLUDED
