#include "minisphere.h"
#include "sockets.h"

#include "debugger.h"

static const int TCP_DEBUG_PORT = 812;

static void       duk_cb_debug_detach (void* udata);
static duk_size_t duk_cb_debug_peek   (void* udata);
static duk_size_t duk_cb_debug_read   (void* udata, char* buffer, duk_size_t bufsize);
static duk_size_t duk_cb_debug_write  (void* udata, const char* data, duk_size_t size);

bool      s_is_attached = false;
socket_t* s_socket;

void
attach_debugger(void)
{
	if (s_is_attached)
		return;
	
	// open TCP port 812 to listen for a debugger.
	// a magic socket is used for this (max_backlog = 0) which will automatically
	// convert to a full peer on first connection, perfect for bootstrapping a remote
	// session.
	console_log(0, "Waiting for debugger on TCP %i", TCP_DEBUG_PORT);
	s_socket = listen_on_port(TCP_DEBUG_PORT, 1024, 0);
	while (!is_socket_live(s_socket))
		delay(0.05);
	console_log(0, "  Connected!");
	
	// attach the debugger
	console_log(1, "Attaching debugger");
	duk_debugger_attach(g_duk,
		duk_cb_debug_read,
		duk_cb_debug_write,
		duk_cb_debug_peek,
		NULL,
		NULL,
		duk_cb_debug_detach,
		NULL);
	
	s_is_attached = true;
}

void
detach_debugger(void)
{
	if (!s_is_attached)
		return;
	
	// detach the debugger
	console_log(1, "Detaching debugger");
	s_is_attached = false;
	duk_debugger_detach(g_duk);
	free_socket(s_socket);
}

static void
duk_cb_debug_detach(void* udata)
{
	detach_debugger();
}

static duk_size_t
duk_cb_debug_peek(void* udata)
{
	return peek_socket(s_socket);
}

static duk_size_t
duk_cb_debug_read(void* udata, char* buffer, duk_size_t bufsize)
{
	size_t n_bytes;
	
	// if we return zero, Duktape will drop the session. thus we're forced
	// to block until we can read >= 1 byte.
	while ((n_bytes = peek_socket(s_socket)) == 0) {
		if (!is_socket_live(s_socket)) {  // did a pig eat it?
			console_log(0, "TCP connection reset while debugging");
			return 0;  // stupid pig
		}
		
		// so the system doesn't think we locked up...
		delay(0.05);
	}
	
	// let's not overflow the buffer, alright?
	if (n_bytes > bufsize) n_bytes = bufsize;
	read_socket(s_socket, buffer, n_bytes);
	return n_bytes;
}

static duk_size_t
duk_cb_debug_write(void* udata, const char* data, duk_size_t size)
{
	// make sure we're still connected
	if (!is_socket_live(s_socket)) {
		console_log(0, "TCP connection reset while debugging");
		return 0;  // stupid pig!
	}
	
	// send out the data
	write_socket(s_socket, data, size);
	return size;
}
