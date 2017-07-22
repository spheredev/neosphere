#include "minisphere.h"
#include "console.h"

static int s_verbosity = 1;

void
console_init(int verbosity)
{
	s_verbosity = verbosity;
}

void
console_log(int level, const char* fmt, ...)
{
	va_list ap;

	if (level > s_verbosity)
		return;
	va_start(ap, fmt);
	vprintf(fmt, ap);
	fputc('\n', stdout);
	va_end(ap);
}
