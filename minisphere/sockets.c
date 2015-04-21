#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "sockets.h"

static void on_dyad_accept  (dyad_Event* e);
static void on_dyad_receive (dyad_Event* e);

static duk_ret_t js_GetLocalName              (duk_context* ctx);
static duk_ret_t js_GetLocalAddress           (duk_context* ctx);
static duk_ret_t js_ListenOnPort              (duk_context* ctx);
static duk_ret_t js_OpenAddress               (duk_context* ctx);
static duk_ret_t js_new_Socket                (duk_context* ctx);
static duk_ret_t js_Socket_finalize           (duk_context* ctx);
static duk_ret_t js_Socket_toString           (duk_context* ctx);
static duk_ret_t js_Socket_isConnected        (duk_context* ctx);
static duk_ret_t js_Socket_getPendingReadSize (duk_context* ctx);
static duk_ret_t js_Socket_getRemoteAddress   (duk_context* ctx);
static duk_ret_t js_Socket_getRemotePort      (duk_context* ctx);
static duk_ret_t js_Socket_acceptNext         (duk_context* ctx);
static duk_ret_t js_Socket_close              (duk_context* ctx);
static duk_ret_t js_Socket_read               (duk_context* ctx);
static duk_ret_t js_Socket_readString         (duk_context* ctx);
static duk_ret_t js_Socket_write              (duk_context* ctx);

struct socket
{
	int          refcount;
	dyad_Stream* stream;
	bool         is_data_lost;
	uint8_t*     buffer;
	size_t       buffer_size;
	size_t       pend_size;
	int          max_backlog;
	int          num_backlog;
	dyad_Stream* *backlog;
};

socket_t*
connect_to_host(const char* hostname, int port, size_t buffer_size)
{
	socket_t*    socket = NULL;
	dyad_Stream* stream = NULL;

	if (!(socket = calloc(1, sizeof(socket_t)))) goto on_error;
	if (!(socket->buffer = malloc(buffer_size))) goto on_error;
	socket->buffer_size = buffer_size;
	if (!(socket->stream = dyad_newStream())) goto on_error;
	dyad_setNoDelay(socket->stream, true);
	dyad_addListener(socket->stream, DYAD_EVENT_DATA, on_dyad_receive, socket);
	if (dyad_connect(socket->stream, hostname, port) == -1)
		goto on_error;
	return ref_socket(socket);

on_error:
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
	socket_t*    socket = NULL;
	dyad_Stream* stream = NULL;

	if (!(socket = calloc(1, sizeof(socket_t)))) goto on_error;
	if (max_backlog == 0 && !(socket->buffer = malloc(buffer_size))) goto on_error;
	if (max_backlog > 0 && !(socket->backlog = malloc(max_backlog * sizeof(dyad_Stream*))))
		goto on_error;
	socket->max_backlog = max_backlog;
	socket->buffer_size = buffer_size;
	if (!(socket->stream = dyad_newStream())) goto on_error;
	dyad_setNoDelay(socket->stream, true);
	dyad_addListener(socket->stream, DYAD_EVENT_ACCEPT, on_dyad_accept, socket);
	if (dyad_listen(socket->stream, port) == -1)
		goto on_error;
	return ref_socket(socket);

on_error:
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

socket_t*
accept_next_socket(socket_t* listener)
{
	socket_t*    socket;

	int i;

	if (listener->num_backlog == 0)
		return NULL;
	if (!(socket = calloc(1, sizeof(socket_t)))) goto on_error;
	if (!(socket->buffer = malloc(listener->buffer_size))) goto on_error;
	socket->buffer_size = listener->buffer_size;
	socket->stream = listener->backlog[0];
	dyad_addListener(socket->stream, DYAD_EVENT_DATA, on_dyad_receive, socket);
	--listener->num_backlog;
	for (i = 0; i < listener->num_backlog; ++i) listener->backlog[i] = listener->backlog[i + 1];
	return ref_socket(socket);

on_error:
	if (socket != NULL) {
		free(socket->buffer);
		free(socket);
	}
	return NULL;
}

size_t
read_socket(socket_t* socket, uint8_t* buffer, size_t n_bytes)
{
	n_bytes = n_bytes <= socket->pend_size ? n_bytes : socket->pend_size;
	memcpy(buffer, socket->buffer, n_bytes);
	memmove(socket->buffer, socket->buffer + n_bytes, socket->pend_size - n_bytes);
	socket->pend_size -= n_bytes;
	return n_bytes;
}

void
write_socket(socket_t* socket, const uint8_t* data, size_t n_bytes)
{
	dyad_write(socket->stream, (void*)data, (int)n_bytes);
}

static void
on_dyad_accept(dyad_Event* e)
{
	int          new_backlog_len;
	dyad_Stream* *backlog;

	socket_t* socket = e->udata;

	if (socket->max_backlog > 0) {
		// BSD-style socket with backlog: listener stays open, game must accept new sockets
		new_backlog_len = socket->num_backlog + 1;
		backlog = socket->backlog;
		if (new_backlog_len > socket->max_backlog) {
			if (!(backlog = realloc(backlog, new_backlog_len * 2 * sizeof(dyad_Stream*))))
				goto on_error;
			socket->backlog = backlog;
			socket->max_backlog = new_backlog_len * 2;
		}
		socket->backlog[socket->num_backlog] = e->remote;
		++socket->num_backlog;
	}
	else {
		// no backlog: listener closes on first connection (legacy socket)
		dyad_removeAllListeners(socket->stream, DYAD_EVENT_ACCEPT);
		dyad_close(socket->stream);
		socket->stream = e->remote;
		dyad_addListener(socket->stream, DYAD_EVENT_DATA, on_dyad_receive, socket);
	}
	return;

on_error:
	dyad_close(e->remote);
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
	register_api_func(g_duktape, NULL, "GetLocalAddress", js_GetLocalAddress);
	register_api_func(g_duktape, NULL, "GetLocalName", js_GetLocalName);
	
	// Socket object
	register_api_func(g_duktape, NULL, "ListenOnPort", js_ListenOnPort);
	register_api_func(g_duktape, NULL, "OpenAddress", js_OpenAddress);
	register_api_ctor(g_duktape, "Socket", js_new_Socket, js_Socket_finalize);
	register_api_func(g_duktape, "Socket", "toString", js_Socket_toString);
	register_api_func(g_duktape, "Socket", "acceptNext", js_Socket_acceptNext);
	register_api_func(g_duktape, "Socket", "isConnected", js_Socket_isConnected);
	register_api_func(g_duktape, "Socket", "getPendingReadSize", js_Socket_getPendingReadSize);
	register_api_func(g_duktape, "Socket", "getRemoteAddress", js_Socket_getRemoteAddress);
	register_api_func(g_duktape, "Socket", "getRemotePort", js_Socket_getRemotePort);
	register_api_func(g_duktape, "Socket", "close", js_Socket_close);
	register_api_func(g_duktape, "Socket", "read", js_Socket_read);
	register_api_func(g_duktape, "Socket", "readString", js_Socket_readString);
	register_api_func(g_duktape, "Socket", "write", js_Socket_write);
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
	int n_args = duk_get_top(ctx);
	duk_require_int(ctx, 0);
	if (n_args >= 2)
		duk_require_int(ctx, 1);
	else
		duk_push_int(ctx, 0);
	
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
	int n_args = duk_get_top(ctx);
	
	const char* ip;
	int         max_backlog;
	int         port;
	socket_t*   socket;

	if (duk_is_number(ctx, 0)) {
		port = duk_require_int(ctx, 0);
		max_backlog = n_args >= 2 ? duk_require_int(ctx, 1) : 16;

		socket_t* socket;

		if (socket = listen_on_port(port, 1024, max_backlog))
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
		if (is_socket_server(socket) && socket->max_backlog > 0)
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:isConnected(): Not valid on listen-only sockets");
		if (is_socket_data_lost(socket))
			duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:isConnected(): Socket has dropped incoming data due to allocation failure");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getPendingReadSize(): Socket has already been closed");
	if (is_socket_server(socket) && socket->max_backlog > 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getPendingReadSize(): Not valid on listen-only sockets");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getPendingReadSize(): Socket has dropped incoming data due to allocation failure");
	duk_push_uint(ctx, (duk_uint_t)socket->pend_size);
	return 1;
}

static duk_ret_t
js_Socket_getRemoteAddress(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getRemoteAddress(): Socket has already been closed");
	if (is_socket_server(socket) && socket->max_backlog > 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getRemoteAddress(): Not valid on listen-only sockets");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getRemoteAddress(): Socket has dropped incoming data due to allocation failure");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getRemoteAddress(): Socket is not connected");
	duk_push_string(ctx, dyad_getAddress(socket->stream));
	return 1;
}

static duk_ret_t
js_Socket_getRemotePort(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getRemotePort(): Socket has already been closed");
	if (is_socket_server(socket) && socket->max_backlog > 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getRemotePort(): Not valid on listen-only sockets");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getRemotePort(): Socket has dropped incoming data due to allocation failure");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:getRemotePort(): Socket is not connected");
	duk_push_int(ctx, dyad_getPort(socket->stream));
	return 1;
}

static duk_ret_t
js_Socket_acceptNext(duk_context* ctx)
{
	socket_t* new_socket;
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:acceptNext(): Socket has already been closed");
	if (!is_socket_server(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:acceptNext(): Not valid on non-listening socket");
	new_socket = accept_next_socket(socket);
	if (new_socket) {
		duk_push_sphere_obj(ctx, "Socket", new_socket);
	}
	else {
		duk_push_null(ctx);
	}
	return 1;
}

static duk_ret_t
js_Socket_close(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	socket = duk_require_sphere_obj(ctx, -1, "Socket");
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:close(): Socket has already been closed");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Socket has already been closed");
	if (is_socket_server(socket) && socket->max_backlog > 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Not valid on listen-only sockets");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:read(): Socket has dropped incoming data due to allocation failure");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): Socket has already been closed");
	if (is_socket_server(socket) && socket->max_backlog > 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): Not valid on listen-only sockets");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:readString(): Socket has dropped incoming data due to allocation failure");
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
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): Socket has already been closed");
	if (is_socket_server(socket) && socket->max_backlog > 0)
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): Not valid on listen-only sockets");
	if (!is_socket_live(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): Socket is not connected");
	if (is_socket_data_lost(socket))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Socket:write(): Socket has dropped incoming data due to allocation failure");
	write_socket(socket, payload, write_size);
	return 0;
}
