#include "minisphere.h"

#include "console.h"

static int s_log_level = -1;

void
initialize_console(int log_level)
{
	printf("Initializing console logger\n");
	s_log_level = log_level;
}

void
shutdown_console(void)
{
	printf("Shutting down console logger\n");
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
