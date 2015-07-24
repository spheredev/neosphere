#include "minisphere.h"
#include "bytearray.h"
#include "sockets.h"

#include "debugger.h"

static const int TCP_DEBUG_PORT = 812;

static void       duk_cb_debug_detach      (void* udata);
static duk_size_t duk_cb_debug_peek        (void* udata);
static duk_size_t duk_cb_debug_read        (void* udata, char* buffer, duk_size_t bufsize);
static duk_size_t duk_cb_debug_write       (void* udata, const char* data, duk_size_t size);
static void       duk_cb_debug_write_flush (void* udata);

bool         s_is_attached = false;
socket_t*    s_socket;
bytearray_t* s_write_buf;
size_t       s_write_size;

void
attach_debugger(void)
{
	if (s_is_attached)
		return;
	
	// allocate a 64 KB write buffer to start. the buffer can be resized later if
	// needed, but this should be more than enough in 99% of cases.
	s_write_buf = new_bytearray(65536);
	
	// open TCP port 812 to listen for a debugger.
	// a 'magic' legacy socket is used for this (max_backlog = 0). this avoids
	// having to juggle two sockets, plus it ensures the port closes immediately
	// upon connection.
	console_log(0, "Waiting for debugger on TCP %i", TCP_DEBUG_PORT);
	s_socket = listen_on_port(TCP_DEBUG_PORT, 0, 0);
	while (!is_socket_live(s_socket))
		delay(0.05);
	console_log(0, "  Connected!");
	
	// attach the debugger
	console_log(1, "Starting debug session, debugger attached");
	duk_debugger_attach(g_duk,
		duk_cb_debug_read,
		duk_cb_debug_write,
		duk_cb_debug_peek,
		NULL,
		duk_cb_debug_write_flush,
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
	console_log(1, "Resuming execution, debugger detached");
	s_is_attached = false;
	duk_debugger_detach(g_duk);
	free_socket(s_socket);

	// wrap things up
	free_bytearray(s_write_buf);
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
	uint8_t* p_out;
	
	// make sure we're still connected
	if (!is_socket_live(s_socket)) {
		console_log(0, "TCP connection reset while debugging");
		return 0;  // stupid pig!
	}
	
	// buffer the data until Duktape requests a write flush. this
	// improves the chances the message arrives in one piece.
	if (s_write_size + size > get_bytearray_size(s_write_buf))
		resize_bytearray(s_write_buf, get_bytearray_size(s_write_buf) * 2);
	p_out = get_bytearray_buffer(s_write_buf) + s_write_size;
	s_write_size += size;
	memcpy(p_out, data, size);
	return size;
}

static void
duk_cb_debug_write_flush(void* udata)
{
	write_socket(s_socket, get_bytearray_buffer(s_write_buf),
		s_write_size);
	s_write_size = 0;
}
