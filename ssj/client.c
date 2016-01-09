#include "ssj.h"
#include "client.h"

#include <dyad.h>

enum dvalue_tag
{
	DVALUE_EOM,
	DVALUE_REQ,
	DVALUE_REP,
	DVALUE_ERR,
	DVALUE_NFY,
	DVALUE_UNUSED,
	DVALUE_UNDEF,
	DVALUE_INT,
	DVALUE_STRING,
	DVALUE_FLOAT,
	DVALUE_NULL,
	DVALUE_OBJ,
	DVALUE_HEAPPTR,
	DVALUE_PTR,
	DVALUE_LIGHTFUNC,
};

struct session
{
	uint8_t*     recv_buf;
	size_t       recv_buf_size;
	size_t       num_bytes;
	dyad_Stream* socket;
	path_t*      source_root;
	vector_t*    dvalues;
};

struct dvalue
{
	enum dvalue_tag tag;
	union {
		struct {
			void*  data;
			size_t size;
		} buffer;
		double number;
		void*  ptr;
	};
};

static void
on_socket_recv(dyad_Event* e)
{
	bool       need_resize = false;
	session_t* session;
	char*      p_next_write;
	
	session = e->udata;
	while (session->num_bytes + e->size > session->recv_buf_size) {
		session->recv_buf_size *= 2;
		need_resize = true;
	}
	if (need_resize)
		session->recv_buf = realloc(session->recv_buf, session->recv_buf_size);
	p_next_write = session->recv_buf + session->num_bytes;
	session->num_bytes += e->size;
	memcpy(p_next_write, e->data, e->size);
}

static size_t
receive_all(session_t* session, void* buffer, size_t num_bytes)
{
	num_bytes = num_bytes >= session->num_bytes ? num_bytes : session->num_bytes;
	memcpy(buffer, session->recv_buf, num_bytes);
	memmove(session->recv_buf, session->recv_buf + num_bytes,
		session->num_bytes -= num_bytes);
	return num_bytes;
}

void
initialize_client(void)
{
	dyad_init();
	dyad_setUpdateTimeout(0.0);
}

void
shutdown_client(void)
{
	dyad_shutdown();
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
	session->recv_buf_size = 65536;
	session->recv_buf = malloc(session->recv_buf_size);
	session->num_bytes = 0;
	session->dvalues = vector_new(sizeof(struct dvalue));
	return session;

on_error:
	if (socket != NULL) dyad_close(socket);
	return NULL;
}

void
session_free(session_t* session)
{
	dyad_end(session->socket);
	path_free(session->source_root);
	vector_free(session->dvalues);
	free(session);
}

void
session_tick(session_t* session)
{
	struct dvalue dvalue;
	uint8_t       ib;
	
	dyad_update();
	if (session->num_bytes > 0) {
		memset(&dvalue, 0, sizeof(struct dvalue));
		receive_all(session, &ib, 1);
		switch (ib) {
		case 0x00: dvalue.tag = DVALUE_EOM; break;
		case 0x01: dvalue.tag = DVALUE_REQ; break;
		case 0x02: dvalue.tag = DVALUE_REP; break;
		case 0x03: dvalue.tag = DVALUE_ERR; break;
		case 0x04: dvalue.tag = DVALUE_NFY; break;
		default:
			dvalue.tag = DVALUE_UNUSED;
			break;
		}
		vector_push(session->dvalues, &dvalue);
	}
}

void
send_dvalue_ib(session_t* session, enum dvalue_tag tag)
{
	uint8_t ib;

	ib = tag == DVALUE_EOM ? 0x00
		: tag == DVALUE_REQ ? 0x01
		: tag == DVALUE_REP ? 0x02
		: tag == DVALUE_ERR ? 0x03
		: tag == DVALUE_NFY ? 0x04
		: tag == DVALUE_INT ? 0x10
		: tag == DVALUE_STRING ? 0x11
		: tag == DVALUE_FLOAT ? 0x1A
		: 0x15;
	dyad_write(session->socket, &ib, 1);
}

void
send_dvalue_string(session_t* session, const char* string)
{
	uint32_t length;
	uint8_t  len_data[4];

	// use network byte order (big endian) for string length
	length = (int)strlen(string);
	len_data[0] = (uint8_t)(length >> 24 & 0xFF);
	len_data[1] = (uint8_t)(length >> 16 & 0xFF);
	len_data[2] = (uint8_t)(length >> 8 & 0xFF);
	len_data[3] = (uint8_t)(length & 0xFF);
	
	send_dvalue_ib(session, DVALUE_STRING);
	dyad_write(session->socket, len_data, 4);
	dyad_write(session->socket, string, (int)length);
}

void
send_dvalue_int(session_t* session, int32_t value)
{
	uint8_t data[4];
	
	// use network byte order (big endian)
	data[0] = (uint8_t)(value >> 24 & 0xFF);
	data[1] = (uint8_t)(value >> 16 & 0xFF);
	data[2] = (uint8_t)(value >> 8 & 0xFF);
	data[3] = (uint8_t)(value & 0xFF);
	
	send_dvalue_ib(session, DVALUE_INT);
	dyad_write(session->socket, data, 4);
}

void
send_dvalue_float(session_t* session, double value)
{
	uint8_t data[8];

	// use network byte order (big endian)
	data[0] = ((uint8_t*)&value)[7];
	data[1] = ((uint8_t*)&value)[6];
	data[2] = ((uint8_t*)&value)[5];
	data[3] = ((uint8_t*)&value)[4];
	data[4] = ((uint8_t*)&value)[3];
	data[5] = ((uint8_t*)&value)[2];
	data[6] = ((uint8_t*)&value)[1];
	data[7] = ((uint8_t*)&value)[0];

	send_dvalue_ib(session, DVALUE_FLOAT);
	dyad_write(session->socket, data, 8);
}
