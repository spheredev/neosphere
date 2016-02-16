#include "ssj.h"
#include "client.h"

#include "message.h"
#include "socket.h"

struct client
{
	socket_t* socket;
};

static bool
parse_handshake(client_t* obj)
{
	static char handshake[128];

	char* next_token;
	char* token;
	char  *p_ch;

	printf("verifying... ");
	memset(handshake, 0, sizeof handshake);
	p_ch = handshake;
	do {
		socket_recv(obj->socket, p_ch, 1);
	} while (*p_ch++ != '\n');
	*(p_ch - 1) = '\0';

	// parse handshake line
	if (!(token = strtok_r(handshake, " ", &next_token)))
		goto on_error;
	if (atoi(token) != 1) goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	if (!(token = strtok_r(NULL, " ", &next_token)))
		goto on_error;
	printf("OK.\n");
	printf("   inferior is \33[36m%s\33[m\n", next_token);
	printf("   duktape \33[36m%s\33[m\n", token);

	return true;

on_error:
	printf("\33[31;1merror!\33[m\n");
	return false;
}

void
clients_init(void)
{
	sockets_init();
}

void
clients_deinit(void)
{
	sockets_deinit();
}

client_t*
client_connect(const char* hostname, int port)
{
	client_t* obj;

	obj = calloc(1, sizeof(client_t));
	
	printf("connecting to \33[36m%s:%d\33[m... ", hostname, port);
	fflush(stdout);
	if (!(obj->socket = socket_connect(hostname, port, 30.0)))
		goto on_error;
	printf("OK.\n");
	if (!parse_handshake(obj))
		goto on_error;
	return obj;

on_error:
	printf("\33[31;1merror.\33[m\n");
	if (obj != NULL) {
		if (obj->socket != NULL)
			socket_close(obj->socket);
		free(obj);
	}
	return NULL;
}

void
client_close(client_t* obj)
{
	socket_close(obj->socket);
	free(obj);
}

message_t*
client_recv_msg(client_t* obj)
{
	return msg_recv(obj->socket);
}

void
client_send_msg(client_t* obj, message_t* msg)
{
	// note: client_send() will free the message after sending it. this is for
	//       convenience, but requires diligence on the part of the caller.

	msg_send(obj->socket, msg);
	msg_free(msg);
}
