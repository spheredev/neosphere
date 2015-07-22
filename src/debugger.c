#include "minisphere.h"

#include "debugger.h"

static void       duk_cb_debug_detach (void* udata);
static duk_size_t duk_cb_debug_peek   (void* udata);
static duk_size_t duk_cb_debug_read   (void* udata, char* buffer, duk_size_t bufsize);
static duk_size_t duk_cb_debug_write  (void* udata, const char* buffer, duk_size_t bufsize);

void*  s_buffer = NULL;
size_t s_buffer_size = 0;

void
attach_debugger(void)
{
	duk_debugger_attach(g_duk,
		duk_cb_debug_read,
		duk_cb_debug_write,
		duk_cb_debug_peek,
		NULL,
		NULL,
		duk_cb_debug_detach,
		NULL);
}

void detach_debugger(void)
{
	duk_debugger_detach(g_duk);
}

static void
duk_cb_debug_detach(void* udata)
{

}

static duk_size_t
duk_cb_debug_peek(void* udata)
{
	return 0;
}

static duk_size_t
duk_cb_debug_read(void* udata, char* buffer, duk_size_t bufsize)
{
	return 0;
}

static duk_size_t
duk_cb_debug_write(void* udata, const char* buffer, duk_size_t bufsize)
{
	return 0;
}
