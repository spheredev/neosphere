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
static duk_ret_t js_new_Server                      (duk_context* ctx);
static duk_ret_t js_Server_finalize                 (duk_context* ctx);
static duk_ret_t js_Server_close                    (duk_context* ctx);
static duk_ret_t js_Server_accept                   (duk_context* ctx);
static duk_ret_t js_new_IOSocket                    (duk_context* ctx);
static duk_ret_t js_IOSocket_finalize               (duk_context* ctx);
static duk_ret_t js_IOSocket_get_bytesPending       (duk_context* ctx);
static duk_ret_t js_IOSocket_get_remoteAddress      (duk_context* ctx);
static duk_ret_t js_IOSocket_get_remotePort         (duk_context* ctx);
static duk_ret_t js_IOSocket_isConnected            (duk_context* ctx);
static duk_ret_t js_IOSocket_close                  (duk_context* ctx);
static duk_ret_t js_IOSocket_pipe                   (duk_context* ctx);
static duk_ret_t js_IOSocket_read                   (duk_context* ctx);
static duk_ret_t js_IOSocket_readString             (duk_context* ctx);
static duk_ret_t js_IOSocket_unpipe                 (duk_context* ctx);
static duk_ret_t js_IOSocket_write                  (duk_context* ctx);

struct socket
{
	unsigned int refcount;
	unsigned int id;
	dyad_Stream* stream;
	dyad_Stream* stream_ipv6;
	uint8_t*     buffer;
	size_t       buffer_size;
	size_t       pend_size;
	socket_t*    pipe_target;
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

	console_log(2, "connecting socket #%u to %s:%i", s_next_socket_id, hostname, port);
	
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
	console_log(2, "failed to connect socket #%u", s_next_socket_id++);
	if (socket != NULL) {
		free(socket->buffer);
		if (socket->stream != NULL) dyad_close(stream);
		free(socket);
	}
	return NULL;
}

socket_t*
listen_on_port(const char* hostname, int port, size_t buffer_size, int max_backlog)
{
	int          backlog_size;
	socket_t*    socket = NULL;
	dyad_Stream* stream = NULL;

	console_log(2, "opening socket #%u to listen on %i", s_next_socket_id, port);
	if (max_backlog > 0)
		console_log(3, "    backlog: up to %i", max_backlog);
	
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
	if (hostname == NULL) {
		if (!(socket->stream_ipv6 = dyad_newStream())) goto on_error;
		dyad_setNoDelay(socket->stream_ipv6, true);
		dyad_addListener(socket->stream_ipv6, DYAD_EVENT_ACCEPT, on_dyad_accept, socket);
		if (dyad_listenEx(socket->stream, "0.0.0.0", port, max_backlog) == -1)
			goto on_error;
		if (dyad_listenEx(socket->stream_ipv6, "::", port, max_backlog) == -1)
			goto on_error;
	}
	else {
		if (dyad_listenEx(socket->stream, hostname, port, max_backlog) == -1)
			goto on_error;
	}
	
	socket->id = s_next_socket_id++;
	return ref_socket(socket);

on_error:
	console_log(2, "failed to open socket #%u", s_next_socket_id++);
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
	if (socket != NULL)
		++socket->refcount;
	return socket;
}

void
free_socket(socket_t* socket)
{
	unsigned int threshold;
	
	int i;
	
	if (socket == NULL) return;
	
	// handle the case where a socket is piped into itself.
	// the circular reference will otherwise prevent the socket from being freed.
	threshold = socket->pipe_target == socket ? 1 : 0;
	if (--socket->refcount > threshold)
		return;
	
	console_log(3, "disposing socket #%u no longer in use", socket->id);
	for (i = 0; i < socket->num_backlog; ++i)
		dyad_end(socket->backlog[i]);
	dyad_end(socket->stream);
	if (socket->stream_ipv6 != NULL)
		dyad_end(socket->stream_ipv6);
	free_socket(socket->pipe_target);
	free(socket);
}

bool
is_socket_live(socket_t* socket)
{
	int state;

	state = dyad_getState(socket->stream);
	return state == DYAD_STATE_CONNECTED
		|| state == DYAD_STATE_CLOSING;
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

socket_t*
accept_next_socket(socket_t* listener)
{
	socket_t* socket;

	int i;

	if (listener->num_backlog == 0)
		return NULL;

	console_log(2, "spawning new socket #%u for connection to socket #%u",
		s_next_socket_id, listener->id);
	console_log(2, "    remote address: %s:%d",
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
peek_socket(const socket_t* socket)
{
	return socket->pend_size;
}

void
pipe_socket(socket_t* socket, socket_t* destination)
{
	socket_t* old_target;
	
	console_log(2, "piping socket #%u into destination socket #%u", socket->id, destination->id);
	old_target = socket->pipe_target;
	socket->pipe_target = ref_socket(destination);
	free_socket(old_target);
	if (socket->pipe_target != NULL && socket->pend_size > 0) {
		console_log(4, "piping %zd bytes into socket #%u", socket->pend_size, destination->id);
		write_socket(destination, socket->buffer, socket->pend_size);
		socket->pend_size = 0;
	}
}

size_t
read_socket(socket_t* socket, uint8_t* buffer, size_t n_bytes)
{
	n_bytes = n_bytes <= socket->pend_size ? n_bytes : socket->pend_size;
	console_log(4, "reading %zd bytes from socket #%u", n_bytes, socket->id);
	memcpy(buffer, socket->buffer, n_bytes);
	memmove(socket->buffer, socket->buffer + n_bytes, socket->pend_size - n_bytes);
	socket->pend_size -= n_bytes;
	return n_bytes;
}

void
shutdown_socket(socket_t* socket)
{
	console_log(2, "shutting down socket #%u", socket->id);
	dyad_end(socket->stream);
}

void
write_socket(socket_t* socket, const uint8_t* data, size_t n_bytes)
{
	console_log(4, "writing %zd bytes to socket #%u", n_bytes, socket->id);
	dyad_write(socket->stream, data, (int)n_bytes);
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
			console_log(4, "taking connection from %s:%i on socket #%u",
				dyad_getAddress(e->remote), dyad_getPort(e->remote), socket->id);
			socket->backlog[socket->num_backlog++] = e->remote;
		}
		else {
			console_log(4, "backlog full fpr socket #%u, refusing %s:%i", socket->id,
				dyad_getAddress(e->remote), dyad_getPort(e->remote), socket->id);
			dyad_close(e->remote);
		}
	}
	else {
		// no backlog: listener closes on first connection (legacy socket)
		console_log(2, "rebinding socket #%u to %s:%i", socket->id,
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
	size_t    new_pend_size;
	socket_t* socket = e->udata;

	if (socket->pipe_target != NULL) {
		// if the socket is a pipe, pass the data through to the destination
		console_log(4, "piping %d bytes from socket #%u to socket #%u", e->size,
			socket->id, socket->pipe_target->id);
		write_socket(socket->pipe_target, e->data, e->size);
	}
	else {
		// buffer any data received until read() is called
		new_pend_size = socket->pend_size + e->size;
		if (new_pend_size > socket->buffer_size) {
			socket->buffer_size = new_pend_size * 2;
			socket->buffer = realloc(socket->buffer, socket->buffer_size);
		}
		memcpy(socket->buffer + socket->pend_size, e->data, e->size);
		socket->pend_size += e->size;
	}
}

void
init_sockets_api(void)
{
	// core Sockets API functions
	api_register_method(g_duk, NULL, "GetLocalAddress", js_GetLocalAddress);
	api_register_method(g_duk, NULL, "GetLocalName", js_GetLocalName);
	
	// Socket object (Sphere 1.5-style socket)
	api_register_method(g_duk, NULL, "ListenOnPort", js_ListenOnPort);
	api_register_method(g_duk, NULL, "OpenAddress", js_OpenAddress);
	api_register_ctor(g_duk, "Socket", js_new_Socket, js_Socket_finalize);
	api_register_prop(g_duk, "Socket", "remoteAddress", js_IOSocket_get_remoteAddress, NULL);
	api_register_prop(g_duk, "Socket", "remotePort", js_IOSocket_get_remotePort, NULL);
	api_register_method(g_duk, "Socket", "toString", js_Socket_toString);
	api_register_method(g_duk, "Socket", "isConnected", js_Socket_isConnected);
	api_register_method(g_duk, "Socket", "getPendingReadSize", js_Socket_getPendingReadSize);
	api_register_method(g_duk, "Socket", "close", js_Socket_close);
	api_register_method(g_duk, "Socket", "read", js_Socket_read);
	api_register_method(g_duk, "Socket", "readString", js_Socket_readString);
	api_register_method(g_duk, "Socket", "write", js_Socket_write);

	// Server object
	api_register_ctor(g_duk, "Server", js_new_Server, js_Server_finalize);
	api_register_method(g_duk, "Server", "close", js_Server_close);
	api_register_method(g_duk, "Server", "accept", js_Server_accept);
	
	// IOSocket object
	api_register_ctor(g_duk, "IOSocket", js_new_IOSocket, js_IOSocket_finalize);
	api_register_prop(g_duk, "IOSocket", "bytesPending", js_IOSocket_get_bytesPending, NULL);
	api_register_prop(g_duk, "IOSocket", "remoteAddress", js_IOSocket_get_remoteAddress, NULL);
	api_register_prop(g_duk, "IOSocket", "remotePort", js_IOSocket_get_remotePort, NULL);
	api_register_method(g_duk, "IOSocket", "isConnected", js_IOSocket_isConnected);
	api_register_method(g_duk, "IOSocket", "close", js_IOSocket_close);
	api_register_method(g_duk, "IOSocket", "pipe", js_IOSocket_pipe);
	api_register_method(g_duk, "IOSocket", "read", js_IOSocket_read);
	api_register_method(g_duk, "IOSocket", "readString", js_IOSocket_readString);
	api_register_method(g_duk, "IOSocket", "unpipe", js_IOSocket_unpipe);
	api_register_method(g_duk, "IOSocket", "write", js_IOSocket_write);
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
		if (socket = listen_on_port(NULL, port, 1024, 0))
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
	if (socket != NULL)
		duk_push_boolean(ctx, is_socket_live(socket));
	else
		duk_push_false(ctx);
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getPendingReadSize(): socket has been closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Socket:read(): must read at least 1 byte (got: %i)", length);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): socket is not connected");
	if (!(read_buffer = malloc(length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): unable to allocate read buffer");
	read_socket(socket, read_buffer, length);
	if (!(array = bytearray_from_buffer(read_buffer, length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): unable to create byte array");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): socket is not connected");
	if (!(buffer = malloc(length)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): unable to allocate read buffer");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): socket is not connected");
	write_socket(socket, payload, write_size);
	return 0;
}

static duk_ret_t
js_new_Server(duk_context* ctx)
{
	int n_args = duk_get_top(ctx);
	int port = duk_require_int(ctx, 0);
	int max_backlog = n_args >= 2 ? duk_require_int(ctx, 1) : 16;

	socket_t* socket;

	if (max_backlog <= 0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Server(): backlog size must be greater than zero (got: %i)", max_backlog);
	if (socket = listen_on_port(NULL, port, 1024, max_backlog))
		duk_push_sphere_obj(ctx, "Server", socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Server_finalize(duk_context* ctx)
{
	socket_t*   socket;

	socket = duk_require_sphere_obj(ctx, 0, "Server");
	free_socket(socket);
	return 0;
}

static duk_ret_t
js_Server_accept(duk_context* ctx)
{
	socket_t* new_socket;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Server");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Server:accept(): socket has been closed");
	new_socket = accept_next_socket(socket);
	if (new_socket)
		duk_push_sphere_obj(ctx, "IOSocket", new_socket);
	else
		duk_push_null(ctx);
	return 1;
}

static duk_ret_t
js_Server_close(duk_context* ctx)
{
	socket_t*   socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Server");
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
js_IOSocket_get_bytesPending(duk_context* ctx)
{
	// get IOSocket:bytesPending
	// Returns the number of bytes in the socket's receive buffer.
	
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:bytesPending: Socket has been closed");
	duk_push_uint(ctx, (duk_uint_t)socket->pend_size);
	return 1;
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
	if (socket != NULL)
		duk_push_boolean(ctx, is_socket_live(socket));
	else
		duk_push_false(ctx);
	return 1;
}

static duk_ret_t
js_IOSocket_close(duk_context* ctx)
{
	// IOSocket:close();
	// Closes the socket, after which no further I/O may be performed
	// with it.
	
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
js_IOSocket_pipe(duk_context* ctx)
{
	// IOSocket:pipe(destSocket);
	// Pipes all data received by a socket into another socket.
	// Arguments:
	//     destSocket: The IOSocket into which to pipe received data.
	
	socket_t* dest_socket;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	dest_socket = duk_require_sphere_obj(ctx, 0, "IOSocket");
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:pipe(): socket has been closed");
	if (dest_socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:pipe(): destination socket has been closed");
	pipe_socket(socket, dest_socket);
	
	// return destination socket (enables pipe chaining)
	duk_dup(ctx, 0);
	return 1;
}

static duk_ret_t
js_IOSocket_unpipe(duk_context* ctx)
{
	// IOSocket:unpipe();
	// Undoes a previous call to pipe(), reverting the socket to normal
	// behavior.
	
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:pipe(): socket has been closed");
	pipe_socket(socket, NULL);
	return 0;
}

static duk_ret_t
js_IOSocket_read(duk_context* ctx)
{
	// IOSocket:read(numBytes);
	// Reads from a socket, returning the data in an ArrayBuffer.
	// Arguments:
	//     numBytes: The maximum number of bytes to read.
	
	void*      buffer;
	size_t     bytes_read;
	duk_size_t num_bytes;
	socket_t*  socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	num_bytes = duk_require_uint(ctx, 0);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:read(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:read(): socket is not connected");
	buffer = duk_push_fixed_buffer(ctx, num_bytes);
	bytes_read = read_socket(socket, buffer, num_bytes);
	duk_push_buffer_object(ctx, -1, 0, bytes_read, DUK_BUFOBJ_ARRAYBUFFER);
	return 1;
}

static duk_ret_t
js_IOSocket_readString(duk_context* ctx)
{
	// IOSocket:read(numBytes);
	// Reads data from a socket and returns it as a UTF-8 string.
	// Arguments:
	//     numBytes: The maximum number of bytes to read.

	uint8_t*  buffer;
	size_t    num_bytes;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	num_bytes = duk_require_uint(ctx, 0);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:readString(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:readString(): socket is not connected");
	read_socket(socket, buffer = malloc(num_bytes), num_bytes);
	duk_push_lstring(ctx, (char*)buffer, num_bytes);
	free(buffer);
	return 1;
}

static duk_ret_t
js_IOSocket_write(duk_context* ctx)
{
	// IOSocket:write(data);
	// Writes data into the socket.
	// Arguments:
	//     data: The data to write; this can be either a JS string or an
	//           ArrayBuffer.
	
	const uint8_t* payload;
	socket_t*      socket;
	duk_size_t     write_size;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "IOSocket");
	duk_pop(ctx);
	if (duk_is_string(ctx, 0))
		payload = (uint8_t*)duk_get_lstring(ctx, 0, &write_size);
	else
		payload = duk_require_buffer_data(ctx, 0, &write_size);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:write(): socket has been closed");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "IOSocket:write(): socket is not connected");
	write_socket(socket, payload, write_size);
	return 0;
}
