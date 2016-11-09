#ifndef MINISPHERE__ASYNC_H__INCLUDED
#define MINISPHERE__ASYNC_H__INCLUDED

#include "script.h"

typedef struct job job_t;

typedef
enum async_hint
{
	ASYNC_TICK,
	ASYNC_RENDER,
	ASYNC_UPDATE,
	ASYNC_MAX
} async_hint_t;

void    async_init       (void);
void    async_uninit     (void);
bool    async_busy       (void);
void    async_cancel     (int64_t token);
void    async_cancel_all (bool recurring);
int64_t async_defer      (script_t* script, uint32_t timeout, async_hint_t hint);
int64_t async_recur      (script_t* script, double priority, async_hint_t hint);
void    async_run_jobs   (async_hint_t hint);

#endif // MINISPHERE__ASYNC_H__INCLUDED
