#ifndef SPHERE__PROFILER_H__INCLUDED
#define SPHERE__PROFILER_H__INCLUDED

#include "jsal.h"

void      profiler_init      (void);
void      profiler_uninit    (void);
js_ref_t* profiler_attach_to (js_ref_t* function, const char* description);

#endif // SPHERE__PROFILER_H__INCLUDED
