#include "minisphere.h"

#include "console.h"

static int s_log_level = 1;

int
get_log_level(void)
{
	return s_log_level;
}

void
set_log_level(int log_level)
{
	s_log_level = log_level;
}

void
console_log(int log_level, const char* fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	if (log_level <= s_log_level)
		vprintf(fmt, ap);
	va_end(ap);
}
