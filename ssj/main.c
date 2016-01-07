#include "ssj.h"

static void print_banner (bool want_copyright);

int
main(int argc, char* argv[])
{
	print_banner(true);
}

static void
print_banner(bool want_copyright)
{
	printf("%s Sphere Game Debugger %s\n", DEBUGGER_NAME, sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("A powerful JavaScript debugger for minisphere\n");
		printf("(c) 2016 Fat Cerberus\n");
	}
}
