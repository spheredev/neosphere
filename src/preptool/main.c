#include "majin.h"

static void print_banner (bool want_copyright, bool want_deps);
static void print_quote  (void);

int
main(int argc, char* argv[])
{
	print_quote();
	return 0;
}

static void
print_banner(bool want_copyright, bool want_deps)
{
	printf("%s %s Sphere project prep tool (%s)\n", PREPTOOL_NAME, VERSION_NAME,
		sizeof(void*) == 8 ? "x64" : "x86");
	if (want_copyright) {
		printf("a Sphere v2 game manifest editor for minisphere\n");
		printf("(c) 2015-2016 Fat Cerberus\n");
	}
	if (want_deps) {
		printf("\n");
	}
}

static void
print_quote(void)
{
	static const char* const MESSAGES[] =
	{
		"Me Buu!  Kill you!",
	};

	srand((unsigned int)time(NULL));
	printf("...and that's why it has to end like this!  You'll have to learn the hard way,\n");
	printf("and know what it feels like to have YOUR life taken away against YOUR will!\n");
	printf("It's WRONG!\n\n");
	printf("    Buu says:\n    \"%s\"\n",
		MESSAGES[rand() % (sizeof MESSAGES / sizeof(const char*))]);
}
