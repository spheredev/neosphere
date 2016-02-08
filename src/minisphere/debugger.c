#include "minisphere.h"
#include "sockets.h"

#include "debugger.h"

static const int TCP_DEBUG_PORT = 1208;

static bool       attach_debugger     (void);
static void       detach_debugger     (bool is_shutdown);
static void       duk_cb_debug_detach (void* udata);
static duk_size_t duk_cb_debug_peek   (void* udata);
static duk_size_t duk_cb_debug_read   (void* udata, char* buffer, duk_size_t bufsize);
static duk_size_t duk_cb_debug_write  (void* udata, const char* data, duk_size_t size);

static bool      s_is_attached = false;
static socket_t* s_client;
static bool      s_have_source_map;
static socket_t* s_server;
static bool      s_want_attach;

void
initialize_debugger(bool want_attach, bool allow_remote)
{
	void*       data;
	size_t      data_size;
	const char* hostname;

	s_want_attach = want_attach;

	// listen for debugger connections on TCP port 1208.
	// the listening socket will remain active for the duration of
	// the session, allowing a debugger to be attached at any time.
	console_log(1, "Opening TCP port %i to listen for debugger", TCP_DEBUG_PORT);
	hostname = allow_remote ? NULL : "127.0.0.1";
	s_server = listen_on_port(hostname, TCP_DEBUG_PORT, 1024, 1);

	// if the engine was started in debug mode, wait for a debugger
	// to connect before beginning execution.
	if (s_want_attach && !attach_debugger())
		exit_game(true);

	// load the source map, if one is available
	s_have_source_map = false;
	duk_push_global_stash(g_duk);
	duk_del_prop_string(g_duk, -1, "debugMap");
	if (data = sfs_fslurp(g_fs, "sourcemap.json", NULL, &data_size)) {
		duk_push_lstring(g_duk, data, data_size);
		duk_json_decode(g_duk, -1);
		duk_put_prop_string(g_duk, -2, "debugMap");
		duk_push_global_object(g_duk);
		duk_push_string(g_duk, "SourceMap");
		duk_push_lstring(g_duk, data, data_size);
		duk_json_decode(g_duk, -1);
		duk_def_prop(g_duk, -3, DUK_DEFPROP_HAVE_VALUE
			| DUK_DEFPROP_CLEAR_WRITABLE | DUK_DEFPROP_CLEAR_CONFIGURABLE
			| DUK_DEFPROP_CLEAR_ENUMERABLE);
		duk_pop(g_duk);
		free(data);
		s_have_source_map = true;
	}
	duk_pop(g_duk);
}

void
shutdown_debugger()
{
	detach_debugger(true);
	free_socket(s_server);
}

void
update_debugger(void)
{
	socket_t* socket;

	if (socket = accept_next_socket(s_server)) {
		if (s_client != NULL) {
			console_log(2, "Rejecting connection from %s, debugger already attached",
				get_socket_host(socket));
			free_socket(socket);
		}
		else {
			console_log(1, "Connected to debugger at %s", get_socket_host(socket));
			s_client = socket;
			duk_debugger_detach(g_duk);
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

bool
is_debugger_attached(void)
{
	return s_is_attached;
}

const char*
get_source_pathname(const char* pathname)
{
	// note: pathname must be canonicalized using make_sfs_path() otherwise
	//       the source map lookup will fail.

	static char retval[SPHERE_PATH_MAX];

	strcpy(retval, pathname);
	if (!s_have_source_map)
		return retval;
	duk_push_global_stash(g_duk);
	duk_get_prop_string(g_duk, -1, "debugMap");
	if (!duk_get_prop_string(g_duk, -1, "fileMap"))
		duk_pop_3(g_duk);
	else {
		duk_get_prop_string(g_duk, -1, pathname);
		if (duk_is_string(g_duk, -1))
			strcpy(retval, duk_get_string(g_duk, -1));
		duk_pop_n(g_duk, 4);
	}
	return retval;
}

static bool
attach_debugger(void)
{
	double timeout;

	console_log(1, "Waiting for connection from debugger");
	timeout = al_get_time() + 30.0;
	while (s_client == NULL && al_get_time() < timeout) {
		update_debugger();
		delay(0.05);
	}
	if (s_client == NULL)  // did we time out?
		console_log(1, "Timed out waiting for debugger");
	return s_client != NULL;
}

static void
detach_debugger(bool is_shutdown)
{
	if (!s_is_attached) return;
	
	// detach the debugger
	console_log(1, "Detaching debugger");
	s_is_attached = false;
	duk_debugger_detach(g_duk);
	if (s_client != NULL) {
		shutdown_socket(s_client);
		while (is_socket_live(s_client))
			delay(0.05);
	}
	free_socket(s_client);
	s_client = NULL;
	if (s_want_attach && !is_shutdown)
		exit_game(true);  // clean detach, exit
}

static void
duk_cb_debug_detach(void* udata)
{
	// note: if s_client is null, a TCP reset was detected by one of the I/O callbacks.
	// if this is the case, wait a bit for the client to reconnect.
	if (s_client != NULL || !attach_debugger())
		detach_debugger(false);
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

	if (s_client == NULL) return 0;

	// if we return zero, Duktape will drop the session. thus we're forced
	// to block until we can read >= 1 byte.
	while ((n_bytes = peek_socket(s_client)) == 0) {
		if (!is_socket_live(s_client)) {  // did a pig eat it?
			console_log(1, "TCP connection reset while debugging");
			free_socket(s_client); s_client = NULL;
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
	if (s_client == NULL) return 0;

	// make sure we're still connected
	if (!is_socket_live(s_client)) {
		console_log(1, "TCP connection reset while debugging");
		free_socket(s_client); s_client = NULL;
		return 0;
	}

	// send out the data
	write_socket(s_client, data, size);
	return size;
}
