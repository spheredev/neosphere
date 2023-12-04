/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#include "neosphere.h"
#include "legacy.h"

#include "sockets.h"

struct socket_v1
{
	unsigned int refcount;
	socket_t*    client;
	server_t*    server;
};

socket_v1_t*
socket_v1_new_client(const char* hostname, int port)
{
	socket_t*    client;
	socket_v1_t* socket;

	if (!(client = socket_new(4096, false)))
		goto on_error;
	if (!socket_connect(client, hostname, port))
		goto on_error;

	if (!(socket = calloc(1, sizeof(socket_v1_t))))
		goto on_error;
	socket->client = client;
	return socket_v1_ref(socket);

on_error:
	socket_unref(client);
	return NULL;
}

socket_v1_t*
socket_v1_new_server(int port)
{
	server_t*     server;
	socket_v1_t* socket;

	server = server_new(NULL, port, 4096, 16, false);

	if (!(socket = calloc(1, sizeof(socket_v1_t))))
		return NULL;
	socket->server = server;
	return socket_v1_ref(socket);
}

socket_v1_t*
socket_v1_ref(socket_v1_t* it)
{
	++it->refcount;
	return it;
}

void
socket_v1_unref(socket_v1_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	socket_unref(it->client);
	server_unref(it->server);
	free(it);
}

int
socket_v1_bytes_avail(const socket_v1_t* it)
{
	return socket_bytes_avail(it->client);
}

bool
socket_v1_connected(socket_v1_t* it)
{
	socket_t* client;

	if (it->server != NULL) {
		// note: Sphere v1 has "magic sockets": server sockets created using ListenOnPort()
		//       automatically transform into normal client sockets upon first connection.
		if ((client = server_accept(it->server))) {
			it->client = client;
			server_unref(it->server);
			it->server = NULL;
		}
	}

	if (it->client != NULL)
		return socket_connected(it->client);
	else
		return false;
}

void
socket_v1_close(socket_v1_t* it)
{
	socket_close(it->client);
}

int
socket_v1_read(socket_v1_t* it, void* buffer, int num_bytes)
{
	return socket_read(it->client, buffer, num_bytes);
}

void
socket_v1_write(socket_v1_t* it, const void* data, int num_bytes)
{
	socket_write(it->client, data, num_bytes);
}
