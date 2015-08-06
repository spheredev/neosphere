#include "minisphere.h"
#include "sockets.h"

#include "debugger.h"

static const int TCP_DEBUG_PORT = 812;

static void       detach_debugger     (void);
static void       duk_cb_debug_detach (void* udata);
static duk_size_t duk_cb_debug_peek   (void* udata);
static duk_size_t duk_cb_debug_read   (void* udata, char* buffer, duk_size_t bufsize);
static duk_size_t duk_cb_debug_write  (void* udata, const char* data, duk_size_t size);

bool      s_is_attached = false;
socket_t* s_client;
socket_t* s_server;
bool      s_want_attach;

void
initialize_debugger(bool want_attach)
{
	s_want_attach = want_attach;
	
	// listen for debugger connections on TCP port 812.
	// the listening socket will remain active for the duration of
	// the session, allowing a debugger to be attached at any time.
	console_log(0, "Opening TCP %i to listen for debugger", TCP_DEBUG_PORT);
	s_server = listen_on_port(TCP_DEBUG_PORT, 1024, 1);
	
	// if the engine was started in debug mode, wait for a debugger
	// to connect before beginning execution.
	if (s_want_attach) {
		console_log(0, "Waiting for debugger to connect");
		while (s_client == NULL) {
			update_debugger();
			delay(0.05);
		}
	}
}

void
shutdown_debugger(void)
{
	free_socket(s_client);
	free_socket(s_server);
}

void
update_debugger(void)
{
	socket_t* socket;
	
	if (socket = accept_next_socket(s_server)) {
		if (s_client != NULL) {
			console_log(2, "Rejecting %s:%i, debugger already attached",
				get_socket_host(socket), get_socket_port(socket));
			free_socket(socket);
		}
		else {
			console_log(0, "Attaching to debugger at %s:%i",
				get_socket_host(socket), get_socket_port(socket));
			s_client = socket;
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
	}
}

static void
detach_debugger(void)
{
	if (!s_is_attached)
		return;

	// detach the debugger
	console_log(1, "Detaching debugger");
	s_is_attached = false;
	duk_debugger_detach(g_duk);
	free_socket(s_client); s_client = NULL;
}

static void
duk_cb_debug_detach(void* udata)
{
	detach_debugger();
}

static duk_size_t
duk_cb_debug_peek(void* udata)
{
	return peek_socket(s_client);
}

static duk_size_t
duk_cb_debug_read(void* udata, char* buffer, duk_size_t bufsize)
{
	size_t n_bytes;
	
	// if we return zero, Duktape will drop the session. thus we're forced
	// to block until we can read >= 1 byte.
	while ((n_bytes = peek_socket(s_client)) == 0) {
		if (!is_socket_live(s_client)) {  // did a pig eat it?
			console_log(0, "TCP connection reset while debugging");
			return 0;  // stupid pig
		}
		
		// so the system doesn't think we locked up...
		delay(0.05);
	}
	
	// let's not overflow the buffer, alright?
	if (n_bytes > bufsize) n_bytes = bufsize;
	read_socket(s_client, buffer, n_bytes);
	return n_bytes;
}

static duk_size_t
duk_cb_debug_write(void* udata, const char* data, duk_size_t size)
{
	// make sure we're still connected
	if (!is_socket_live(s_client)) {
		console_log(0, "TCP connection reset while debugging");
		return 0;  // stupid pig!
	}
	
	// send out the data
	write_socket(s_client, data, size);
	return size;
}
