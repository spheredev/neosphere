#include "minisphere.h"
#include "sockets.h"

struct socket
{
	unsigned int refcount;
	unsigned int id;
	dyad_Stream* stream;
	dyad_Stream* stream_ipv6;
	uint8_t*     buffer;
	size_t       buffer_size;
	size_t       pend_size;
	int          max_backlog;
	int          num_backlog;
	dyad_Stream* *backlog;
};

static void on_dyad_accept(dyad_Event* e);
static void on_dyad_receive(dyad_Event* e);

static unsigned int s_next_socket_id = 0;
static unsigned int s_num_refs       = 0;

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
sockets_update(void)
{
	dyad_update();
}

socket_t*
socket_new_client(const char* hostname, int port, size_t buffer_size)
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
	return socket_ref(socket);

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
socket_new_server(const char* hostname, int port, size_t buffer_size, int max_backlog)
{
	int          backlog_size;
	socket_t*    socket = NULL;
	dyad_Stream* stream = NULL;

	console_log(2, "opening socket #%u to listen on %i", s_next_socket_id, port);
	if (max_backlog > 0)
		console_log(3, "    backlog size: %i", max_backlog);
	
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
	return socket_ref(socket);

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
socket_ref(socket_t* it)
{
	if (it != NULL)
		++it->refcount;
	return it;
}

void
socket_free(socket_t* it)
{
	int i;
	
	if (it == NULL || --it->refcount > 0)
		return;

	console_log(3, "disposing socket #%u no longer in use", it->id);
	for (i = 0; i < it->num_backlog; ++i)
		dyad_end(it->backlog[i]);
	dyad_end(it->stream);
	if (it->stream_ipv6 != NULL)
		dyad_end(it->stream_ipv6);
	free(it);
}

bool
socket_connected(socket_t* it)
{
	int state;

	state = dyad_getState(it->stream);
	return state == DYAD_STATE_CONNECTED
		|| state == DYAD_STATE_CLOSING;
}

const char*
socket_hostname(socket_t* it)
{
	return dyad_getAddress(it->stream);
}

bool
socket_is_server(socket_t* it)
{
	return dyad_getState(it->stream) == DYAD_STATE_LISTENING;
}

size_t
socket_num_bytes(socket_t* it)
{
	return it->pend_size;
}

int
socket_port(socket_t* it)
{
	return dyad_getPort(it->stream);
}

socket_t*
socket_accept(socket_t* it)
{
	socket_t* client;

	int i;

	if (it->num_backlog == 0)
		return NULL;

	console_log(2, "spawning new socket #%u for connection to socket #%u",
		s_next_socket_id, it->id);
	console_log(2, "    remote address: %s:%d",
		dyad_getAddress(it->backlog[0]),
		dyad_getPort(it->backlog[0]));

	// construct a socket object for the new connection
	client = calloc(1, sizeof(socket_t));
	client->buffer = malloc(it->buffer_size);
	client->buffer_size = it->buffer_size;
	client->stream = it->backlog[0];
	dyad_addListener(client->stream, DYAD_EVENT_DATA, on_dyad_receive, client);

	// we accepted the connection, remove it from the backlog
	--it->num_backlog;
	for (i = 0; i < it->num_backlog; ++i)
		it->backlog[i] = it->backlog[i + 1];

	client->id = s_next_socket_id++;
	return socket_ref(client);
}

void
socket_close(socket_t* it)
{
	console_log(2, "shutting down socket #%u", it->id);
	dyad_end(it->stream);
}

size_t
socket_peek(const socket_t* it)
{
	return it->pend_size;
}

size_t
socket_read(socket_t* it, void* buffer, size_t n_bytes)
{
	n_bytes = n_bytes <= it->pend_size ? n_bytes : it->pend_size;
	console_log(4, "reading %zd bytes from socket #%u", n_bytes, it->id);
	memcpy(buffer, it->buffer, n_bytes);
	memmove(it->buffer, it->buffer + n_bytes, it->pend_size - n_bytes);
	it->pend_size -= n_bytes;
	return n_bytes;
}

void
socket_write(socket_t* it, const void* data, size_t n_bytes)
{
	console_log(4, "writing %zd bytes to socket #%u", n_bytes, it->id);
	dyad_write(it->stream, data, (int)n_bytes);
}

static void
on_dyad_accept(dyad_Event* e)
{
	int       new_backlog_len;
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

	// buffer any data received until read() is called
	new_pend_size = socket->pend_size + e->size;
	if (new_pend_size > socket->buffer_size) {
		socket->buffer_size = new_pend_size * 2;
		socket->buffer = realloc(socket->buffer, socket->buffer_size);
	}
	memcpy(socket->buffer + socket->pend_size, e->data, e->size);
	socket->pend_size += e->size;
}
