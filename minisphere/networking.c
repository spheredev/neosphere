#include "minisphere.h"
#include "api.h"
#include "bytearray.h"

#include "networking.h"

static void on_dyad_accept  (dyad_Event* e);
static void on_dyad_receive (dyad_Event* e);

static duk_ret_t js_GetLocalName              (duk_context* ctx);
static duk_ret_t js_GetLocalAddress           (duk_context* ctx);
static duk_ret_t js_ListenOnPort              (duk_context* ctx);
static duk_ret_t js_OpenAddress               (duk_context* ctx);
static duk_ret_t js_Socket_finalize           (duk_context* ctx);
static duk_ret_t js_Socket_isConnected        (duk_context* ctx);
static duk_ret_t js_Socket_getPendingReadSize (duk_context* ctx);
static duk_ret_t js_Socket_close              (duk_context* ctx);
static duk_ret_t js_Socket_read               (duk_context* ctx);
static duk_ret_t js_Socket_write              (duk_context* ctx);

struct socket
{
	int          refcount;
	dyad_Stream* stream;
	bool         is_data_lost;
	uint8_t*     buffer;
	size_t       buffer_size;
	size_t       pend_size;
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
	dyad_connect(socket->stream, hostname, port);
	return ref_socket(socket);

on_error:
	if (socket != NULL) {
		if (socket->stream != NULL) dyad_close(stream);
		free(socket->buffer);
		free(socket);
	}
	return NULL;
}

socket_t*
listen_on_port(int port, size_t buffer_size)
{
	socket_t*    socket = NULL;
	dyad_Stream* stream = NULL;

	if (!(socket = calloc(1, sizeof(socket_t)))) goto on_error;
	if (!(socket->buffer = malloc(buffer_size))) goto on_error;
	socket->buffer_size = buffer_size;
	if (!(socket->stream = dyad_newStream())) goto on_error;
	dyad_setNoDelay(socket->stream, true);
	dyad_addListener(socket->stream, DYAD_EVENT_ACCEPT, on_dyad_accept, socket);
	dyad_listen(socket->stream, port);
	return ref_socket(socket);

on_error:
	if (socket != NULL) {
		if (socket->stream != NULL) dyad_close(stream);
		free(socket->buffer);
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
	if (socket == NULL || --socket->refcount)
		return;
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
write_socket(socket_t* socket, uint8_t* data, size_t n_bytes)
{
	dyad_write(socket->stream, data, n_bytes);
}

void
duk_push_sphere_socket(duk_context* ctx, socket_t* socket)
{
	ref_socket(socket);
	duk_push_object(ctx);
	duk_push_string(ctx, "socket"); duk_put_prop_string(ctx, -2, "\xFF" "sphere_type");
	duk_push_pointer(ctx, socket); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_push_c_function(ctx, js_Socket_finalize, DUK_VARARGS); duk_set_finalizer(ctx, -2);
	duk_push_c_function(ctx, js_Socket_isConnected, DUK_VARARGS); duk_put_prop_string(ctx, -2, "isConnected");
	duk_push_c_function(ctx, js_Socket_getPendingReadSize, DUK_VARARGS); duk_put_prop_string(ctx, -2, "getPendingReadSize");
	duk_push_c_function(ctx, js_Socket_close, DUK_VARARGS); duk_put_prop_string(ctx, -2, "close");
	duk_push_c_function(ctx, js_Socket_read, DUK_VARARGS); duk_put_prop_string(ctx, -2, "read");
	duk_push_c_function(ctx, js_Socket_write, DUK_VARARGS); duk_put_prop_string(ctx, -2, "write");
}

void
init_networking_api(void)
{
	register_api_func(g_duktape, NULL, "GetLocalAddress", js_GetLocalAddress);
	register_api_func(g_duktape, NULL, "GetLocalName", js_GetLocalName);
	register_api_func(g_duktape, NULL, "ListenOnPort", js_ListenOnPort);
	register_api_func(g_duktape, NULL, "OpenAddress", js_OpenAddress);
}

static void
on_dyad_accept(dyad_Event* e)
{
	socket_t* socket = e->udata;

	dyad_addListener(e->remote, DYAD_EVENT_DATA, on_dyad_receive, socket);
	dyad_end(socket->stream);
	socket->stream = e->remote;
}

static void
on_dyad_receive(dyad_Event* e)
{
	uint8_t*  new_buffer;
	size_t    new_pend_size;
	socket_t* socket = e->udata;

	new_pend_size = socket->pend_size + e->size;
	if (new_pend_size > socket->buffer_size) {
		if (new_buffer = realloc(socket->buffer, new_pend_size * 2)) {
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
	int port = duk_require_int(ctx, 0);
	
	socket_t* socket;

	socket = listen_on_port(port, 1024);
	duk_push_sphere_socket(ctx, socket);
	free_socket(socket);
	return 1;
}

static duk_ret_t
js_OpenAddress(duk_context* ctx)
{
	const char* ip = duk_require_string(ctx, 0);
	int port = duk_require_int(ctx, 1);
	
	socket_t* socket;

	socket = connect_to_host(ip, port, 1024);
	duk_push_sphere_socket(ctx, socket);
	free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_finalize(duk_context* ctx)
{
	socket_t* socket;

	duk_get_prop_string(ctx, 0, "\xFF" "ptr"); socket = duk_get_pointer(ctx, -1); duk_pop(ctx);
	free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_isConnected(duk_context* ctx)
{
	socket_t* socket;
	
	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); socket = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (socket == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "Socket:isConnected(): Tried to use socket after it was closed");
	if (is_socket_data_lost(socket))
		duk_error(ctx, DUK_ERR_ERROR, "Socket:isConnected(): Socket has dropped incoming data due to allocation failure (internal error)");
	duk_push_boolean(ctx, is_socket_live(socket));
	return 1;
}

static duk_ret_t
js_Socket_getPendingReadSize(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); socket = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (socket == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "Socket:getPendingReadSize(): Tried to use socket after it was closed");
	if (is_socket_data_lost(socket))
		duk_error(ctx, DUK_ERR_ERROR, "Socket:getPendingReadSize(): Socket has dropped incoming data due to allocation failure (internal error)");
	duk_push_uint(ctx, socket->pend_size);
	return 1;
}

static duk_ret_t
js_Socket_close(duk_context* ctx)
{
	socket_t* socket;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); socket = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_push_null(ctx); duk_put_prop_string(ctx, -2, "\xFF" "ptr");
	duk_pop(ctx);
	if (socket == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "Socket:close(): Tried to use socket after it was closed");
	free_socket(socket);
	return 1;
}

static duk_ret_t
js_Socket_read(duk_context* ctx)
{
	size_t length = duk_require_uint(ctx, 0);

	uint8_t*  buffer;
	socket_t* socket;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); socket = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (socket == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "Socket:read(): Tried to use socket after it was closed");
	if (is_socket_data_lost(socket))
		duk_error(ctx, DUK_ERR_ERROR, "Socket:read(): Socket has dropped incoming data due to allocation failure (internal error)");
	if (!(buffer = malloc(length)))
		duk_error(ctx, DUK_ERR_ERROR, "Socket:read(): Failed to allocate read buffer (internal error)");
	read_socket(socket, buffer, length);
	duk_push_sphere_bytearray(ctx, buffer, length);
	return 1;
}

static duk_ret_t
js_Socket_write(duk_context* ctx)
{
	duk_require_object_coercible(ctx, 0);
	uint8_t* data; duk_get_prop_string(ctx, 0, "\xFF" "buffer"); data = duk_get_pointer(ctx, -1); duk_pop(ctx);
	int length = duk_get_length(ctx, 0);

	socket_t* socket;

	duk_push_this(ctx);
	duk_get_prop_string(ctx, -1, "\xFF" "ptr"); socket = duk_get_pointer(ctx, -1); duk_pop(ctx);
	duk_pop(ctx);
	if (socket == NULL)
		duk_error(ctx, DUK_ERR_ERROR, "Socket:write(): Tried to use socket after it was closed");
	if (is_socket_data_lost(socket))
		duk_error(ctx, DUK_ERR_ERROR, "Socket:write(): Socket has dropped incoming data due to allocation failure (internal error)");
	write_socket(socket, data, length);
	return 0;
}
