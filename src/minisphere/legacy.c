#include "minisphere.h"
#include "legacy.h"

#include "sockets.h"

struct v1_socket
{
	unsigned int refcount;
	socket_t*    client;
	server_t*    server;
};

v1_socket_t*
v1_socket_new_client(const char* hostname, int port)
{
	socket_t*    client;
	v1_socket_t* socket;

	if (!(client = socket_new(4096)))
		goto on_error;
	if (!socket_connect(client, hostname, port))
		goto on_error;

	socket = calloc(1, sizeof(v1_socket_t));
	socket->client = client;
	return v1_socket_ref(socket);

on_error:
	socket_free(client);
	return NULL;
}

v1_socket_t*
v1_socket_new_server(int port)
{
	server_t*     server;
	v1_socket_t* socket;

	server = server_new(NULL, port, 4096, 16);

	socket = calloc(1, sizeof(v1_socket_t));
	socket->server = server;
	return v1_socket_ref(socket);
}

v1_socket_t*
v1_socket_ref(v1_socket_t* it)
{
	++it->refcount;
	return it;
}

void
v1_socket_free(v1_socket_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	socket_free(it->client);
	server_free(it->server);
	free(it);
}

bool
v1_socket_connected(v1_socket_t* it)
{
	socket_t* client;

	if (it->server != NULL) {
		// note: Sphere v1 has "magic sockets": server sockets created using ListenOnPort()
		//       automatically transform into normal client sockets upon first connection.
		if (client = server_accept(it->server)) {
			it->client = client;
			server_free(it->server);
			it->server = NULL;
		}
	}

	if (it->client != NULL)
		return socket_connected(it->client);
	else
		return false;
}

void
v1_socket_close(v1_socket_t* it)
{
	socket_close(it->client);
}

size_t
v1_socket_peek(const v1_socket_t* it)
{
	return socket_peek(it->client);
}

size_t
v1_socket_read(v1_socket_t* it, void* buffer, size_t num_bytes)
{
	return socket_read(it->client, buffer, num_bytes);
}

void
v1_socket_write(v1_socket_t* it, const void* data, size_t num_bytes)
{
	socket_write(it->client, data, num_bytes);
}
