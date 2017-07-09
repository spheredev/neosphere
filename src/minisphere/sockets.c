#include "minisphere.h"
#include "sockets.h"

static void on_dyad_accept  (dyad_Event* e);
static void on_dyad_receive (dyad_Event* e);


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
unsigned int s_num_refs       = 0;

bool
sockets_init(void)
{
	if (++s_num_refs > 1)
		return true;

	console_log(1, "initializing sockets subsystem");
	console_log(2, "    Dyad.c %s", dyad_getVersion());
	dyad_init();
	dyad_setUpdateTimeout(0.0);
	return true;
}

void
sockets_uninit(void)
{
	if (--s_num_refs > 0)
		return;

	console_log(1, "shutting down sockets subsystem");
	dyad_shutdown();
}

void
update_sockets(void)
{
	dyad_update();
}

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

size_t
get_socket_read_size(socket_t* socket)
{
	return socket->pend_size;
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
read_socket(socket_t* socket, void* buffer, size_t n_bytes)
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
write_socket(socket_t* socket, const void* data, size_t n_bytes)
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
