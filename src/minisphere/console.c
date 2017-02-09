#include "minisphere.h"
#include "console.h"

static int s_verbosity = 1;

void
initialize_console(int verbosity)
{
	s_verbosity = verbosity;
}

int
get_log_verbosity(void)
{
	return s_verbosity;
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
