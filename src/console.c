#include "minisphere.h"
#include "logger.h"

#include "console.h"

static int s_verbosity = 0;

void
initialize_console(int verbosity)
{
	s_verbosity = verbosity;
}

void
shutdown_console(void)
{
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
