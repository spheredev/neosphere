#ifndef MINISPHERE__TRANSPILER_H__INCLUDED
#define MINISPHERE__TRANSPILER_H__INCLUDED

#include "spherefs.h"

void initialize_transpiler (void);
void shutdown_transpiler   (void);
bool transpile_to_js       (lstring_t** p_source, const char* filename);

#endif // MINISPHERE__TRANSPILER_H__INCLUDED
