#ifndef MINISPHERE__ASYNC_H__INCLUDED
#define MINISPHERE__ASYNC_H__INCLUDED

#include "script.h"

extern bool initialize_async   (void);
extern void shutdown_async     (void);
extern void update_async       (void);
extern bool queue_async_script (script_t* script);

extern void init_async_api (void);

#endif // MINISPHERE__ASYNC_H__INCLUDED
