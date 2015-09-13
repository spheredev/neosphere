#include <stdlib.h>
#include <stdio.h>

#include "duktape.h"

#define CELL_VERSION "2.0.0"

duk_context* g_duk = NULL;

int
main(int argc, char* argv[])
{
	printf("Cell %s for minisphere\n", CELL_VERSION);
	printf("A scriptable build engine for Sphere games\n");
	printf("(c) 2015 Fat Cerberus\n\n");
	g_duk = duk_create_heap_default();
	duk_destroy_heap(g_duk);
	return 0;
}
