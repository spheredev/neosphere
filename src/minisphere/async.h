#ifndef MINISPHERE__ASYNC_H__INCLUDED
#define MINISPHERE__ASYNC_H__INCLUDED

#include "script.h"

bool initialize_async   (void);
void shutdown_async     (void);
void update_async       (void);
bool queue_async_script (script_t* script);

void init_async_api (void);

#endif // MINISPHERE__ASYNC_H__INCLUDED
