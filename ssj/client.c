#include "ssj.h"
#include "client.h"

#include <dyad.h>

enum dvalue_tag
{
	DVALUE_TAG_EOM = 0x00,
	DVALUE_TAG_REQ = 0x01,
	DVALUE_TAG_REP = 0x02,
	DVALUE_TAG_ERR = 0x03,
	DVALUE_TAG_NFY = 0x04,
	DVALUE_TAG_INT32 = 0x10,
	DVALUE_TAG_STR32 = 0x11,
	DVALUE_TAG_STR16 = 0x12,
	DVALUE_TAG_BUF32 = 0x13,
	DVALUE_TAG_BUF16 = 0x14,
	DVALUE_TAG_UNUSED = 0x15,
	DVALUE_TAG_UNDEF = 0x16,
	DVALUE_TAG_NULL = 0x17,
	DVALUE_TAG_TRUE = 0x18,
	DVALUE_TAG_FALSE = 0x19,
	DVALUE_TAG_FLOAT = 0x1A,
	DVALUE_TAG_OBJ = 0x1B,
	DVALUE_TAG_PTR = 0x1C,
	DVALUE_TAG_LIGHTFUNC = 0x1D,
	DVALUE_TAG_HEAPPTR = 0x1E,

	// Special cases:
	// 0x60...0x7F - string, len = (ib - 0x60)
	// 0x80...0xBF - int, value = (ib - 0x80)
	// 0xC0...0xFF - int, value = ((b[0] - 0xC0) << 8) + b[1]
};

struct session
{
	uint8_t*     recv_buf;
	size_t       recv_buf_size;
	size_t       recv_size;
	dyad_Stream* socket;
	path_t*      source_root;
	vector_t*    dvalues;
	unsigned int req_id;
	unsigned int rep_id;
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
	while (session->recv_size + e->size > session->recv_buf_size) {
		session->recv_buf_size *= 2;
		need_resize = true;
	}
	if (need_resize)
		session->recv_buf = realloc(session->recv_buf, session->recv_buf_size);
	p_next_write = session->recv_buf + session->recv_size;
	session->recv_size += e->size;
	memcpy(p_next_write, e->data, e->size);
}

static size_t
receive_all(session_t* session, void* buffer, size_t num_bytes)
{
	while (session->recv_size < num_bytes)
		dyad_update();
	memcpy(buffer, session->recv_buf, num_bytes);
	memmove(session->recv_buf, session->recv_buf + num_bytes,
		session->recv_size -= num_bytes);
	return num_bytes;
}

static void
send_dvalue_ib(session_t* session, enum dvalue_tag tag)
{
	uint8_t ib;

	ib = tag == DVALUE_TAG_EOM ? 0x00
		: tag == DVALUE_TAG_REQ ? 0x01
		: tag == DVALUE_TAG_REP ? 0x02
		: tag == DVALUE_TAG_ERR ? 0x03
		: tag == DVALUE_TAG_NFY ? 0x04
		: tag == DVALUE_TAG_INT32 ? 0x10
		: tag == DVALUE_TAG_STR32 ? 0x11
		: tag == DVALUE_TAG_FLOAT ? 0x1A
		: 0x15;
	dyad_write(session->socket, &ib, 1);
}

void
initialize_client(void)
{
	dyad_init();
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
	session->recv_size = 0;
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
session_update(session_t* session)
{
	uint8_t       data[8];
	struct dvalue dvalue;
	uint8_t       ib;
	uint8_t*      p_number;
	
	dyad_update();
	if (session->recv_size > 0) {
		do {
			memset(&dvalue, 0, sizeof(struct dvalue));
			receive_all(session, &ib, 1);
			switch (dvalue.tag) {
			case DVALUE_TAG_EOM:
			case DVALUE_TAG_REQ:
			case DVALUE_TAG_REP:
			case DVALUE_TAG_ERR:
			case DVALUE_TAG_NFY:
				dvalue.tag = (enum dvalue_tag)ib;
				break;
			case DVALUE_TAG_INT32:
				dvalue.tag = DVALUE_TAG_INT32;
				receive_all(session, data, 4);
				dvalue.number = (double)((data[0] << 24) + (data[1] << 16)
					+ (data[2] << 8) + data[3]);
				break;
			case DVALUE_TAG_STR32:
				dvalue.tag = DVALUE_TAG_STR32;
				receive_all(session, data, 4);
				dvalue.buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
				dvalue.buffer.data = calloc(1, dvalue.buffer.size + 1);
				receive_all(session, dvalue.buffer.data, dvalue.buffer.size);
				break;
			case DVALUE_TAG_STR16:
				dvalue.tag = DVALUE_TAG_STR32;
				receive_all(session, data, 2);
				dvalue.buffer.size = (data[0] << 8) + data[1];
				dvalue.buffer.data = calloc(1, dvalue.buffer.size + 1);
				receive_all(session, dvalue.buffer.data, dvalue.buffer.size);
				break;
			case DVALUE_TAG_UNUSED:
			case DVALUE_TAG_UNDEF:
			case DVALUE_TAG_NULL:
			case DVALUE_TAG_TRUE:
			case DVALUE_TAG_FALSE:
				dvalue.tag = (enum dvalue_tag)ib;
				break;
			case DVALUE_TAG_FLOAT:
				receive_all(session, data, 8);
				p_number = (uint8_t*)&dvalue.number;
				p_number[0] = data[7];
				p_number[1] = data[6];
				p_number[2] = data[5];
				p_number[3] = data[4];
				p_number[4] = data[3];
				p_number[5] = data[2];
				p_number[6] = data[1];
				p_number[7] = data[0];
				break;
			}
			vector_push(session->dvalues, &dvalue);
		} while (dvalue.tag != DVALUE_TAG_EOM);
	}
}

dvalue_t*
receive_dvalue(session_t* session)
{

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
	
	send_dvalue_ib(session, DVALUE_TAG_STR32);
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
	
	send_dvalue_ib(session, DVALUE_TAG_INT32);
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

	send_dvalue_ib(session, DVALUE_TAG_FLOAT);
	dyad_write(session->socket, data, 8);
}
