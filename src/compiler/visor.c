#include "cell.h"
#include "visor.h"

static void print_indent (int level);

struct visor
{
	int indent_level;
	int num_errors;
	int num_warns;
};

visor_t*
visor_new(void)
{
	visor_t* visor;

	visor = calloc(1, sizeof(visor_t));
	return visor;
}

void
visor_free(visor_t* visor)
{
	free(visor);
}

int
visor_num_errors(const visor_t* visor)
{
	return visor->num_errors;
}

int
visor_num_warns(const visor_t* visor)
{
	return visor->num_warns;
}

void
visor_begin_op(visor_t* visor, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	print_indent(visor->indent_level);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

	++visor->indent_level;
}

void
visor_end_op(visor_t* visor)
{
	--visor->indent_level;
}

void
visor_error(visor_t* visor, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	print_indent(visor->indent_level);
	printf("ERROR: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

	++visor->num_errors;
}

void
visor_info(visor_t* visor, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	print_indent(visor->indent_level);
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);
}

void
visor_warn(visor_t* visor, const char* fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	print_indent(visor->indent_level);
	printf("warning: ");
	vprintf(fmt, ap);
	printf("\n");
	va_end(ap);

	++visor->num_warns;
}

void
print_indent(int level)
{
	int i;

	for (i = 0; i < level; ++i)
		printf("   ");
}
