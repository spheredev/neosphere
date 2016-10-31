#ifndef MINISPHERE__ASYNC_H__INCLUDED
#define MINISPHERE__ASYNC_H__INCLUDED

#include "script.h"

bool async_init     (void);
void async_uninit   (void);
void async_update   (void);
bool async_dispatch (script_t* script);

#endif // MINISPHERE__ASYNC_H__INCLUDED
