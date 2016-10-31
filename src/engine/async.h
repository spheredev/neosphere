#ifndef MINISPHERE__ASYNC_H__INCLUDED
#define MINISPHERE__ASYNC_H__INCLUDED

#include "script.h"

typedef
enum async_hint
{
	ASYNC_ASAP,
	ASYNC_RENDER,
	ASYNC_UPDATE,
	ASYNC_MAX
} async_hint_t;


void async_init     (void);
void async_uninit   (void);
bool async_dispatch (script_t* script, async_hint_t hint);
bool async_recur    (script_t* script, async_hint_t hint);
void async_run      (async_hint_t hint);

#endif // MINISPHERE__ASYNC_H__INCLUDED
