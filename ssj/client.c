#include "ssj.h"
#include "client.h"

#include <dyad.h>

struct session
{
	dyad_Stream* socket;
};

static void
on_socket_recv(dyad_Event* e)
{

}

session_t*
session_new(const char* hostname, int port)
{
	session_t*   session;
	dyad_Stream* socket;

	session = calloc(1, sizeof(session_t));
	socket = dyad_newStream();
	dyad_addListener(socket, DYAD_EVENT_DATA, on_socket_recv, session);
	if (dyad_connect(socket, hostname, port) != 0)
		goto on_error;

	session->socket = socket;
	return session;

on_error:
	if (socket != NULL) dyad_close(socket);
	return NULL;
}

void
session_free(session_t* session)
{
	dyad_end(session->socket);
	free(session);
}
