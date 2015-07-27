#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "sockets.h"

static void on_dyad_accept  (dyad_Event* e);
static void on_dyad_receive (dyad_Event* e);

static duk_ret_t js_GetLocalName                    (duk_context* ctx);
static duk_ret_t js_GetLocalAddress                 (duk_context* ctx);
static duk_ret_t js_ListenOnPort                    (duk_context* ctx);
static duk_ret_t js_OpenAddress                     (duk_context* ctx);
static duk_ret_t js_new_Socket                      (duk_context* ctx);
static duk_ret_t js_Socket_finalize                 (duk_context* ctx);
static duk_ret_t js_Socket_get_remoteAddress        (duk_context* ctx);
static duk_ret_t js_Socket_get_remotePort           (duk_context* ctx);
static duk_ret_t js_Socket_toString                 (duk_context* ctx);
static duk_ret_t js_Socket_isConnected              (duk_context* ctx);
static duk_ret_t js_Socket_getPendingReadSize       (duk_context* ctx);
static duk_ret_t js_Socket_close                    (duk_context* ctx);
static duk_ret_t js_Socket_read                     (duk_context* ctx);
static duk_ret_t js_Socket_readString               (duk_context* ctx);
static duk_ret_t js_Socket_write                    (duk_context* ctx);
static duk_ret_t js_new_ListeningSocket             (duk_context* ctx);
static duk_ret_t js_ListeningSocket_finalize        (duk_context* ctx);
static duk_ret_t js_ListeningSocket_close           (duk_context* ctx);
static duk_ret_t js_ListeningSocket_accept          (duk_context* ctx);
static duk_ret_t js_new_IOSocket                    (duk_context* ctx);
static duk_ret_t js_IOSocket_finalize               (duk_context* ctx);
static duk_ret_t js_IOSocket_get_remoteAddress      (duk_context* ctx);
static duk_ret_t js_IOSocket_get_remotePort         (duk_context* ctx);
static duk_ret_t js_IOSocket_isConnected            (duk_context* ctx);
static duk_ret_t js_IOSocket_getPendingReadSize     (duk_context* ctx);
static duk_ret_t js_IOSocket_close                  (duk_context* ctx);
static duk_ret_t js_IOSocket_read                   (duk_context* ctx);
static duk_ret_t js_IOSocket_readString             (duk_context* ctx);
static duk_ret_t js_IOSocket_write                  (duk_context* ctx);

struct socket
{
	unsigned int refcount;
	unsigned int id;
	dyad_Stream* stream;
	bool         is_data_lost;
	uint8_t*     buffer;
	size_t       buffer_size;
	size_t       pend_size;
	int          max_backlog;
	int          num_backlog;
	dyad_Stream* *backlog;
};

unsigned int s_next_socket_id = 0;

socket_t*
connect_to_host(const char* hostname, int port, size_t buffer_size)
{
	socket_t*    socket = NULL;
	dyad_Stream* stream = NULL;

	console_log(2, "Connecting Socket %u to %s:%i via TCP", s_next_socket_id, hostname, port);
	
	socket = calloc(1, sizeof(socket_t));
	socket->buffer = malloc(buffer_size);
	socket->buffer_size = buffer_size;
	if (!(socket->stream = dyad_newStream()))
		goto on_error;
	dyad_setNoDelay(socket->stream, true);
	dyad_addListener(socket->stream, DYAD_EVENT_DATA, on_dyad_receive, socket);
	if (dyad_connect(socket->stream, hostname, port) == -1)
		goto on_error;
	
	socket->id = s_next_socket_id++;
	return ref_socket(socket);

on_error:
	console_log(2, "Failed to open Socket %u", s_next_socket_id++);
	if (socket != NULL) {
		free(socket->buffer);
		if (socket->stream != NULL) dyad_close(stream);
		free(socket);
	}
	return NULL;
}

socket_t*
listen_on_port(int port, size_t buffer_size, int max_backlog)
{
	int          backlog_size;
	socket_t*    socket = NULL;
	dyad_Stream* stream = NULL;

	console_log(2, "Opening Socket %u to listen on TCP %i", s_next_socket_id, port);
	if (max_backlog > 0)
		console_log(3, "  Backlog: %i connections", max_backlog);
	
	socket = calloc(1, sizeof(socket_t));
	if (max_backlog == 0)
		socket->buffer = malloc(buffer_size);
	else
		socket->backlog = malloc(max_backlog * sizeof(dyad_Stream*));
	socket->max_backlog = max_backlog;
	socket->buffer_size = buffer_size;
	if (!(socket->stream = dyad_newStream())) goto on_error;
	dyad_setNoDelay(socket->stream, true);
	dyad_addListener(socket->stream, DYAD_EVENT_ACCEPT, on_dyad_accept, socket);
	backlog_size = max_backlog > 0 ? max_backlog : 16;
	if (dyad_listenEx(socket->stream, NULL, port, max_backlog) == -1)
		goto on_error;
	
	socket->id = s_next_socket_id++;
	return ref_socket(socket);

on_error:
	console_log(2, "Failed to open Socket %u", s_next_socket_id++);
	if (socket != NULL) {
		free(socket->backlog);
		free(socket->buffer);
		if (socket->stream != NULL) dyad_close(stream);
		free(socket);
	}
	return NULL;
}

socket_t*
ref_socket(socket_t* socket)
{
	++socket->refcount;
	return socket;
}

void
free_socket(socket_t* socket)
{
	int i;
	
	if (socket == NULL || --socket->refcount)
		return;
	console_log(3, "Disposing Socket %u no longer in use", socket->id);
	for (i = 0; i < socket->num_backlog; ++i)
		dyad_end(socket->backlog[i]);
	dyad_end(socket->stream);
	free(socket);
}

bool
is_socket_data_lost(socket_t* socket)
{
	return socket->is_data_lost;
}

bool
is_socket_live(socket_t* socket)
{
	return dyad_getState(socket->stream) == DYAD_STATE_CONNECTED;
}

bool
is_socket_server(socket_t* socket)
{
	return dyad_getState(socket->stream) == DYAD_STATE_LISTENING;
}

const char*
get_socket_host(socket_t* socket)
{
	return dyad_getAddress(socket->stream);
}

int
get_socket_port(socket_t* socket)
{
	return dyad_getPort(socket->stream);
}

size_t
peek_socket(const socket_t* socket)
{
	return socket->pend_size;
}

socket_t*
accept_next_socket(socket_t* listener)
{
	socket_t*    socket;

	int i;

	if (listener->num_backlog == 0)
		return NULL;
	
	console_log(2, "Spawning new Socket %u for connection on Socket %u",
		s_next_socket_id, listener->id);
	console_log(2, "  Remote address: %s:%i",
		dyad_getAddress(listener->backlog[0]),
		dyad_getPort(listener->backlog[0]));
	
	// construct a socket object for the new connection
	socket = calloc(1, sizeof(socket_t));
	socket->buffer = malloc(listener->buffer_size);
	socket->buffer_size = listener->buffer_size;
	socket->stream = listener->backlog[0];
	dyad_addListener(socket->stream, DYAD_EVENT_DATA, on_dyad_receive, socket);
	
	// we accepted the connection, remove it from the backlog
	--listener->num_backlog;
	for (i = 0; i < listener->num_backlog; ++i)
		listener->backlog[i] = listener->backlog[i + 1];
	
	socket->id = s_next_socket_id++;
	return ref_socket(socket);
}

size_t
read_socket(socket_t* socket, uint8_t* buffer, size_t n_bytes)
{
	n_bytes = n_bytes <= socket->pend_size ? n_bytes : socket->pend_size;
	console_log(4, "Reading %z bytes from Socket %u", n_bytes, socket->id);
	memcpy(buffer, socket->buffer, n_bytes);
	memmove(socket->buffer, socket->buffer + n_bytes, socket->pend_size - n_bytes);
	socket->pend_size -= n_bytes;
	return n_bytes;
}

void
write_socket(socket_t* socket, const uint8_t* data, size_t n_bytes)
{
	console_log(4, "Writing %z bytes to Socket %u", n_bytes, socket->id);
	dyad_write(socket->stream, (void*)data, (int)n_bytes);
}

static void
on_dyad_accept(dyad_Event* e)
{
	int          new_backlog_len;

	socket_t* socket = e->udata;

	if (socket->max_backlog > 0) {
		// BSD-style socket with backlog: listener stays open, game must accept new sockets
		new_backlog_len = socket->num_backlog + 1;
		if (new_backlog_len <= socket->max_backlog) {
			console_log(4, "Taking connection from %s:%i on Socket %u",
				dyad_getAddress(e->remote), dyad_getPort(e->remote), socket->id);
			socket->backlog[socket->num_backlog++] = e->remote;
		}
		else {
			console_log(4, "Backlog full on Socket %u, refusing %s:%i", socket->id,
				dyad_getAddress(e->remote), dyad_getPort(e->remote), socket->id);
			dyad_close(e->remote);
		}
	}
	else {
		// no backlog: listener closes on first connection (legacy socket)
		console_log(2, "Rebinding Socket %u to %s:%i", socket->id,
			dyad_getAddress(e->remote), dyad_getPort(e->remote));
		dyad_removeAllListeners(socket->stream, DYAD_EVENT_ACCEPT);
		dyad_close(socket->stream);
		socket->stream = e->remote;
		dyad_addListener(socket->stream, DYAD_EVENT_DATA, on_dyad_receive, socket);
	}
}

static void
on_dyad_receive(dyad_Event* e)
{
	uint8_t*  new_buffer;
	size_t    new_pend_size;
	socket_t* socket = e->udata;

	new_pend_size = socket->pend_size + e->size;
	if (new_pend_size > socket->buffer_size) {
		if (new_pend_size <= UINT_MAX
			&& (new_buffer = realloc(socket->buffer, new_pend_size * 2)))
		{
			socket->buffer = new_buffer;
			socket->buffer_size = new_pend_size * 2;
		}
		else {
			socket->is_data_lost = true;
		}
	}
	if (new_pend_size <= socket->buffer_size) {
		memcpy(socket->buffer, e->data + socket->pend_size, e->size);
		socket->pend_size += e->size;
	}
}

void
init_sockets_api(void)
{
	// core Sockets API functions
	register_api_function(g_duk, NULL, "GetLocalAddress", js_GetLocalAddress);
	register_api_function(g_duk, NULL, "GetLocalName", js_GetLocalName);
	
	// Socket object (Sphere 1.5-style socket)
	register_api_function(g_duk, NULL, "ListenOnPort", js_ListenOnPort);
	register_api_function(g_duk, NULL, "OpenAddress", js_OpenAddress);
	register_api_ctor(g_duk, "Socket", js_new_Socket, js_Socket_finalize);
	register_api_prop(g_duk, "Socket", "remoteAddress", js_IOSocket_get_remoteAddress, NULL);
	register_api_prop(g_duk, "Socket", "remotePort", js_IOSocket_get_remotePort, NULL);
	register_api_function(g_duk, "Socket", "toString", js_Socket_toString);
	register_api_function(g_duk, "Socket", "isConnected", js_Socket_isConnected);
	register_api_function(g_duk, "Socket", "getPendingReadSize", js_Socket_getPendingReadSize);
	register_api_function(g_duk, "Socket", "close", js_Socket_close);
	register_api_function(g_duk, "Socket", "read", js_Socket_read);
	register_api_function(g_duk, "Socket", "readString", js_Socket_readString);
	register_api_function(g_duk, "Socket", "write", js_Socket_write);

	// ListeningSocket object
	register_api_ctor(g_duk, "ListeningSocket", js_new_ListeningSocket, js_ListeningSocket_finalize);
	register_api_function(g_duk, "ListeningSocket", "close", js_ListeningSocket_close);
	register_api_function(g_duk, "ListeningSocket", "accept", js_ListeningSocket_accept);
	
	// IOSocket object
	register_api_ctor(g_duk, "IOSocket", js_new_IOSocket, js_IOSocket_finalize);
	register_api_prop(g_duk, "IOSocket", "remoteAddress", js_IOSocket_get_remoteAddress, NULL);
	register_api_prop(g_duk, "IOSocket", "remotePort", js_IOSocket_get_remotePort, NULL);
	register_api_function(g_duk, "IOSocket", "isConnected", js_IOSocket_isConnected);
	register_api_function(g_duk, "IOSocket", "getPendingReadSize", js_IOSocket_getPendingReadSize);
	register_api_function(g_duk, "IOSocket", "close", js_IOSocket_close);
	register_api_function(g_duk, "IOSocket", "read", js_IOSocket_read);
	register_api_function(g_duk, "IOSocket", "readString", js_IOSocket_readString);
	register_api_function(g_duk, "IOSocket", "write", js_IOSocket_write);
}

static duk_ret_t
js_GetLocalAddress(duk_context* ctx)
{
	duk_push_string(ctx, "127.0.0.1");
	return 1;
}

static duk_ret_t
js_GetLocalName(duk_context* ctx)
{
	duk_push_string(ctx, "localhost");
	return 1;
}

static duk_ret_t
js_ListenOnPort(duk_context* ctx)
{
	duk_require_int(ctx, 0);
	
	js_new_Socket(ctx);
	return 1;
}

static duk_ret_t
js_OpenAddress(duk_context* ctx)
{
	duk_require_string(ctx, 0);
	duk_require_int(ctx, 1);
	
	js_new_Socket(ctx);
	return 1;
}

static duk_ret_t
js_new_Socket(duk_context* ctx)
{
	const char* ip;
	int         port;
	socket_t*   socket;

	if (duk_is_number(ctx, 0)) {
		port = duk_require_int(ctx, 0);
		if (socket = listen_on_port(port, 1024, 0))
			duk_push_sphere_obj(ctx, "Socket", socket);
		else
			duk_push_null(ctx);
	}
	else {
		ip = duk_require_string(ctx, 0);
		port = duk_require_int(ctx, 1);
		if ((socket = connect_to_host(ip, port, 1024)) != NULL)
			duk_push_sphere_obj(ctx, "Socket", socket);
		else
			duk_push_null(ctx);
	}
	return 1;
}

static duk_ret_t
js_Socket_finalize(duk_context* ctx)
{
	socket_t* socket;

	socket = duk_require_sphere_obj(ctx, 0, "Socket");
	free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_get_remoteAddress(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remoteAddress - Socket has been closed");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remoteAddress - Allocation failure while receiving data");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remoteAddress - Socket is not connected");
	duk_push_string(ctx, dyad_getAddress(socket->stream));
	return 1;
}

static duk_ret_t
js_Socket_get_remotePort(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remotePort - Socket has been closed");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remotePort - Allocation failure while receiving data");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:remotePort - Socket is not connected");
	duk_push_int(ctx, dyad_getPort(socket->stream));
	return 1;
}

static duk_ret_t
js_Socket_toString(duk_context* ctx)
{
	duk_push_string(ctx, "[object socket]");
	return 1;
}

static duk_ret_t
js_Socket_isConnected(duk_context* ctx)
{
	socket_t* socket;
	
	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket != NULL) {
		if (is_socket_data_lost(socket))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:isConnected(): Allocation failure while receiving data");
		duk_push_boolean(ctx, is_socket_live(socket));
	}
	else {
		duk_push_false(ctx);
	}
	return 1;
}

static duk_ret_t
js_Socket_getPendingReadSize(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getPendingReadSize(): Socket has been closed");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getPendingReadSize(): Allocation failure while receiving data");
	duk_push_uint(ctx, (duk_uint_t)peek_socket(socket));
	return 1;
}

static duk_ret_t
js_Socket_close(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	if (socket != NULL)
		free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_read(duk_context* ctx)
{
	int length = duk_require_int(ctx, 0);

	bytearray_t* array;
	void*        read_buffer;
	socket_t*    socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (length <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Socket:read(): At least 1 byte must be read (%i)", length);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Allocation failure while receiving data");
	if (!(read_buffer = malloc(length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Failed to allocate read buffer");
	read_socket(socket, read_buffer, length);
	if (!(array = bytearray_from_buffer(read_buffer, length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Failed to create byte array");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_Socket_readString(duk_context* ctx)
{
	size_t length = duk_require_uint(ctx, 0);

	uint8_t*  buffer;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): Socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): Allocation failure while receiving data");
	if (!(buffer = malloc(length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): Failed to allocate read buffer");
	read_socket(socket, buffer, length);
	duk_push_lstring(ctx, (char*)buffer, length);
	free(buffer);
	return 1;
}

static duk_ret_t
js_Socket_write(duk_context* ctx)
{
	bytearray_t*   array;
	const uint8_t* payload;
	socket_t*      socket;
	size_t         write_size;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (duk_is_string(ctx, 0))
		payload = (uint8_t*)duk_get_lstring(ctx, 0, &write_size);
	else {
		array = duk_require_sphere_bytearray(ctx, 0);
		payload = get_bytearray_buffer(array);
		write_size = get_bytearray_size(array);
	}
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): Socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): Allocation failure while receiving data");
	write_socket(socket, payload, write_size);
	return 0;
}

static duk_ret_t
js_new_ListeningSocket(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int port = duk_require_int(ctx, 0);
	int max_backlog = n_args >= 2 ? duk_require_int(ctx, 1) : 16;

	socket_t* socket;

	if (max_backlog <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "ListeningSocket(): Backlog size must be greater than zero (%i)", max_backlog);
	if (socket = listen_on_port(port, 1024, max_backlog))
		duk_push_sphere_obj(ctx, "ListeningSocket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_ListeningSocket_finalize(duk_context* ctx)
{
	socket_t*   socket;

	socket = duk_require_sphere_obj(ctx, 0, "ListeningSocket");
	free_socket(socket);
	return 0;
}

static duk_ret_t
js_ListeningSocket_accept(duk_context* ctx)
{
	socket_t* new_socket;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "ListeningSocket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "ListeningSocket:accept(): Socket has been closed");
	new_socket = accept_next_socket(socket);
	if (new_socket)
		duk_push_sphere_obj(ctx, "IOSocket", new_socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_ListeningSocket_close(duk_context* ctx)
{
	socket_t*   socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "ListeningSocket");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	if (socket != NULL)
		free_socket(socket);
	return 0;
}

static duk_ret_t
js_new_IOSocket(duk_context* ctx)
{
	const char* hostname = duk_require_string(ctx, 0);
	int port = duk_require_int(ctx, 1);
	
	socket_t*   socket;

	if ((socket = connect_to_host(hostname, port, 1024)) != NULL)
		duk_push_sphere_obj(ctx, "IOSocket", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_IOSocket_finalize(duk_context* ctx)
{
	socket_t* socket;

	socket = duk_require_sphere_obj(ctx, 0, "IOSocket");
	free_socket(socket);
	return 0;
}

static duk_ret_t
js_IOSocket_get_remoteAddress(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:remoteAddress - Socket has been closed");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:remoteAddress - Allocation failure while receiving data");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:remoteAddress - Socket is not connected");
	duk_push_string(ctx, dyad_getAddress(socket->stream));
	return 1;
}

static duk_ret_t
js_IOSocket_get_remotePort(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:remotePort - Socket has been closed");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:remotePort - Allocation failure while receiving data");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:remotePort - Socket is not connected");
	duk_push_int(ctx, dyad_getPort(socket->stream));
	return 1;
}

static duk_ret_t
js_IOSocket_isConnected(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (socket != NULL) {
		if (is_socket_data_lost(socket))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:isConnected(): Allocation failure while receiving data");
		duk_push_boolean(ctx, is_socket_live(socket));
	}
	else {
		duk_push_false(ctx);
	}
	return 1;
}

static duk_ret_t
js_IOSocket_getPendingReadSize(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:getPendingReadSize(): Socket has been closed");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:getPendingReadSize(): Allocation failure while receiving data");
	duk_push_uint(ctx, (duk_uint_t)socket->pend_size);
	return 1;
}

static duk_ret_t
js_IOSocket_close(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "udata");
	duk_pop(ctx);
	if (socket != NULL)
		free_socket(socket);
	return 0;
}

static duk_ret_t
js_IOSocket_read(duk_context* ctx)
{
	int length = duk_require_int(ctx, 0);

	bytearray_t* array;
	void*        read_buffer;
	socket_t*    socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (length <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "IOSocket:read(): At least 1 byte must be read (%i)", length);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:read(): Socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:read(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:read(): Allocation failure while receiving data");
	if (!(read_buffer = malloc(length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:read(): Failed to allocate read buffer");
	read_socket(socket, read_buffer, length);
	if (!(array = bytearray_from_buffer(read_buffer, length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:read(): Failed to create byte array");
	duk_push_sphere_bytearray(ctx, array);
	return 1;
}

static duk_ret_t
js_IOSocket_readString(duk_context* ctx)
{
	size_t length = duk_require_uint(ctx, 0);

	uint8_t*  buffer;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:readString(): Socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:readString(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:readString(): Allocation failure while receiving data");
	if (!(buffer = malloc(length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:readString(): Failed to allocate read buffer");
	read_socket(socket, buffer, length);
	duk_push_lstring(ctx, (char*)buffer, length);
	free(buffer);
	return 1;
}

static duk_ret_t
js_IOSocket_write(duk_context* ctx)
{
	bytearray_t*   array;
	const uint8_t* payload;
	socket_t*      socket;
	size_t         write_size;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (duk_is_string(ctx, 0))
		payload = (uint8_t*)duk_get_lstring(ctx, 0, &write_size);
	else {
		array = duk_require_sphere_bytearray(ctx, 0);
		payload = get_bytearray_buffer(array);
		write_size = get_bytearray_size(array);
	}
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:write(): Socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:write(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:write(): Allocation failure while receiving data");
	write_socket(socket, payload, write_size);
	return 0;
}

