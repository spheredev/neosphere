#include "ssj.h"
#include "remote.h"

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
	//   0x60...0x7F - string, len = (ib - 0x60)
	//   0x80...0xBF - int, value = (ib - 0x80)
	//   0xC0...0xFF - int, value = ((b[0] - 0xC0) << 8) + b[1]
};

static bool   parse_handshake (remote_t* remote);
static size_t receive_bytes   (remote_t* remote, void* buffer, size_t num_bytes);
static void   send_dvalue_ib  (remote_t* remote, enum dvalue_tag tag);

static void on_socket_recv (dyad_Event* e);

struct remote
{
	uint8_t*     recv_buf;
	size_t       recv_buf_size;
	size_t       recv_size;
	dyad_Stream* socket;
};

struct dvalue
{
	enum dvalue_tag tag;
	union {
		struct {
			void*  data;
			size_t size;
		} buffer;
		double   float_value;
		int32_t  int_value;
		uint64_t ptr_value;
	};
};

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

remote_t*
connect_remote(const char* hostname, int port)
{
	remote_t* session;

	session = calloc(1, sizeof(remote_t));
	session->recv_buf_size = 65536;
	session->recv_buf = malloc(session->recv_buf_size);
	
	printf("Connecting to remote '%s:%d'... ", hostname, port);
	session->socket = dyad_newStream();
	dyad_addListener(session->socket, DYAD_EVENT_DATA, on_socket_recv, session);
	if (dyad_connect(session->socket, hostname, port) != 0)
		goto on_error;
	while (dyad_getState(session->socket) == DYAD_STATE_CONNECTING)
		dyad_update();
	if (dyad_getState(session->socket) == DYAD_STATE_CONNECTED)
		printf("OK.\n");
	else
		goto on_error;
	
	if (!parse_handshake(session))
		goto on_error;

	return session;

on_error:
	printf("Failed.\n");
	if (session != NULL) {
		if (session->socket != NULL)
			dyad_close(session->socket);
		free(session);
	}
	return NULL;
}

void
close_remote(remote_t* remote)
{
	dyad_end(remote->socket);
	free(remote);
}

dvalue_t*
receive_dvalue(remote_t* remote)
{
	uint8_t   data[32];
	dvalue_t* dvalue;
	uint8_t   ib;
	uint8_t   ptr_size;

	dvalue = calloc(1, sizeof(dvalue_t));
	receive_bytes(remote, &ib, 1);
	switch (ib) {
	case DVALUE_TAG_EOM:
	case DVALUE_TAG_REQ:
	case DVALUE_TAG_REP:
	case DVALUE_TAG_ERR:
	case DVALUE_TAG_NFY:
		dvalue->tag = (enum dvalue_tag)ib;
		break;
	case DVALUE_TAG_INT32:
		receive_bytes(remote, data, 4);
		dvalue->tag = DVALUE_TAG_INT32;
		dvalue->int_value = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		break;
	case DVALUE_TAG_STR32:
		receive_bytes(remote, data, 4);
		dvalue->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		dvalue->buffer.data = calloc(1, dvalue->buffer.size + 1);
		receive_bytes(remote, dvalue->buffer.data, dvalue->buffer.size);
		dvalue->tag = DVALUE_TAG_STR32;
		break;
	case DVALUE_TAG_STR16:
		receive_bytes(remote, data, 2);
		dvalue->buffer.size = (data[0] << 8) + data[1];
		dvalue->buffer.data = calloc(1, dvalue->buffer.size + 1);
		receive_bytes(remote, dvalue->buffer.data, dvalue->buffer.size);
		dvalue->tag = DVALUE_TAG_STR32;
		break;
	case DVALUE_TAG_BUF32:
		receive_bytes(remote, data, 4);
		dvalue->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		dvalue->buffer.data = calloc(1, dvalue->buffer.size + 1);
		receive_bytes(remote, dvalue->buffer.data, dvalue->buffer.size);
		dvalue->tag = DVALUE_TAG_BUF32;
		break;
	case DVALUE_TAG_BUF16:
		receive_bytes(remote, data, 2);
		dvalue->buffer.size = (data[0] << 8) + data[1];
		dvalue->buffer.data = calloc(1, dvalue->buffer.size + 1);
		receive_bytes(remote, dvalue->buffer.data, dvalue->buffer.size);
		dvalue->tag = DVALUE_TAG_BUF32;
		break;
	case DVALUE_TAG_UNUSED:
	case DVALUE_TAG_UNDEF:
	case DVALUE_TAG_NULL:
	case DVALUE_TAG_TRUE:
	case DVALUE_TAG_FALSE:
		dvalue->tag = (enum dvalue_tag)ib;
		break;
	case DVALUE_TAG_FLOAT:
		receive_bytes(remote, data, 8);
		((uint8_t*)&dvalue->float_value)[0] = data[7];
		((uint8_t*)&dvalue->float_value)[1] = data[6];
		((uint8_t*)&dvalue->float_value)[2] = data[5];
		((uint8_t*)&dvalue->float_value)[3] = data[4];
		((uint8_t*)&dvalue->float_value)[4] = data[3];
		((uint8_t*)&dvalue->float_value)[5] = data[2];
		((uint8_t*)&dvalue->float_value)[6] = data[1];
		((uint8_t*)&dvalue->float_value)[7] = data[0];
		dvalue->tag = DVALUE_TAG_FLOAT;
		break;
	case DVALUE_TAG_OBJ:
		receive_bytes(remote, data, 1);
		receive_bytes(remote, &ptr_size, 1);
		receive_bytes(remote, data, ptr_size);
		dvalue->tag = DVALUE_TAG_UNUSED;
		break;
	case DVALUE_TAG_PTR:
		receive_bytes(remote, &ptr_size, 1);
		receive_bytes(remote, data, ptr_size);
		dvalue->tag = DVALUE_TAG_UNUSED;
		break;
	case DVALUE_TAG_LIGHTFUNC:
		receive_bytes(remote, data, 2);
		receive_bytes(remote, &ptr_size, 1);
		receive_bytes(remote, data, ptr_size);
		dvalue->tag = DVALUE_TAG_UNUSED;
		break;
	case DVALUE_TAG_HEAPPTR:
		receive_bytes(remote, &ptr_size, 1);
		receive_bytes(remote, data, ptr_size);
		dvalue->tag = DVALUE_TAG_UNUSED;
		break;
	default:
		if (ib >= 0x60 && ib <= 0x7F) {
			dvalue->tag = DVALUE_TAG_STR32;
			dvalue->buffer.size = ib - 0x60;
			dvalue->buffer.data = calloc(1, dvalue->buffer.size + 1);
			receive_bytes(remote, dvalue->buffer.data, dvalue->buffer.size);
		}
		else if (ib >= 0x80 && ib <= 0xBF) {
			dvalue->tag = DVALUE_TAG_INT32;
			dvalue->int_value = ib - 0x80;
		}
		else if (ib >= 0xC0) {
			receive_bytes(remote, data, 1);
			dvalue->tag = DVALUE_TAG_INT32;
			dvalue->int_value = ((ib - 0xC0) << 8) + data[0];
		}
		else
			goto on_error;
	}
	return dvalue;

on_error:
	free(dvalue);
	return NULL;
}

void
send_dvalue(remote_t* remote, const dvalue_t* dvalue)
{
	uint8_t  data[32];
	uint32_t str_length;

	switch (dvalue->tag) {
	case DVALUE_TAG_EOM:
	case DVALUE_TAG_REQ:
	case DVALUE_TAG_REP:
	case DVALUE_TAG_ERR:
	case DVALUE_TAG_NFY:
		send_dvalue_ib(remote, dvalue->tag);
		break;
	case DVALUE_TAG_INT32:
		data[0] = (uint8_t)(dvalue->int_value >> 24 & 0xFF);
		data[1] = (uint8_t)(dvalue->int_value >> 16 & 0xFF);
		data[2] = (uint8_t)(dvalue->int_value >> 8 & 0xFF);
		data[3] = (uint8_t)(dvalue->int_value & 0xFF);
		send_dvalue_ib(remote, DVALUE_TAG_INT32);
		dyad_write(remote->socket, data, 4);
		break;
	case DVALUE_TAG_STR32:
		str_length = (uint32_t)strlen(dvalue->buffer.data);
		data[0] = (uint8_t)(str_length >> 24 & 0xFF);
		data[1] = (uint8_t)(str_length >> 16 & 0xFF);
		data[2] = (uint8_t)(str_length >> 8 & 0xFF);
		data[3] = (uint8_t)(str_length & 0xFF);
		send_dvalue_ib(remote, DVALUE_TAG_STR32);
		dyad_write(remote->socket, data, 4);
		dyad_write(remote->socket, dvalue->buffer.data, (int)str_length);
		break;
	case DVALUE_TAG_UNUSED:
	case DVALUE_TAG_UNDEF:
	case DVALUE_TAG_NULL:
	case DVALUE_TAG_TRUE:
	case DVALUE_TAG_FALSE:
		send_dvalue_ib(remote, dvalue->tag);
		break;
	case DVALUE_TAG_FLOAT:
		data[0] = ((uint8_t*)&dvalue->float_value)[7];
		data[1] = ((uint8_t*)&dvalue->float_value)[6];
		data[2] = ((uint8_t*)&dvalue->float_value)[5];
		data[3] = ((uint8_t*)&dvalue->float_value)[4];
		data[4] = ((uint8_t*)&dvalue->float_value)[3];
		data[5] = ((uint8_t*)&dvalue->float_value)[2];
		data[6] = ((uint8_t*)&dvalue->float_value)[1];
		data[7] = ((uint8_t*)&dvalue->float_value)[0];
		send_dvalue_ib(remote, DVALUE_TAG_FLOAT);
		dyad_write(remote->socket, data, 8);
		break;
	default:
		send_dvalue_ib(remote, DVALUE_TAG_UNUSED);
	}
}

dvalue_t*
dvalue_new_float(double value)
{
	dvalue_t* dvalue;

	dvalue = calloc(1, sizeof(dvalue_t));
	dvalue->tag = DVALUE_TAG_FLOAT;
	dvalue->float_value = value;
	return dvalue;
}

dvalue_t*
dvalue_new_int(int32_t value)
{
	dvalue_t* dvalue;

	dvalue = calloc(1, sizeof(dvalue_t));
	dvalue->tag = DVALUE_TAG_INT32;
	dvalue->int_value = value;
	return dvalue;
}

dvalue_t*
dvalue_new_string(const char* value)
{
	dvalue_t* dvalue;

	dvalue = calloc(1, sizeof(dvalue_t));
	dvalue->tag = DVALUE_TAG_STR32;
	dvalue->buffer.size = strlen(value);
	dvalue->buffer.data = malloc(dvalue->buffer.size + 1);
	strcpy(dvalue->buffer.data, value);
	return dvalue;
}

void
dvalue_free(dvalue_t* dvalue)
{
	if (dvalue->tag == DVALUE_TAG_STR32 || dvalue->tag == DVALUE_TAG_BUF32)
		free(dvalue->buffer.data);
	free(dvalue);
}

dvalue_type_t
dvalue_get_type(dvalue_t* dvalue)
{
	switch (dvalue->tag) {
	case DVALUE_TAG_INT32:
		return DVALUE_INT;
	case DVALUE_TAG_STR32:
		return DVALUE_STRING;
	case DVALUE_TAG_BUF32:
		return DVALUE_BUFFER;
	case DVALUE_TAG_TRUE:
	case DVALUE_TAG_FALSE:
		return DVALUE_BOOL;
	}
}

static bool
parse_handshake(remote_t* remote)
{
	static char handshake[128];

	char* next_token;
	char* token;
	char  *p_ch;

	printf("Handshaking... ");
	memset(handshake, 0, sizeof handshake);
	p_ch = handshake;
	do {
		receive_bytes(remote, p_ch, 1);
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
	printf("  Connected to %s\n", next_token);
	printf("  Duktape version is %s\n", token);
	printf("\n");

	return true;

on_error:
	printf("ERROR!\n");
	return false;
}

static size_t
receive_bytes(remote_t* remote, void* buffer, size_t num_bytes)
{
	while (remote->recv_size < num_bytes)
		dyad_update();
	memcpy(buffer, remote->recv_buf, num_bytes);
	memmove(remote->recv_buf, remote->recv_buf + num_bytes,
		remote->recv_size -= num_bytes);
	return num_bytes;
}

static void
send_dvalue_ib(remote_t* remote, enum dvalue_tag tag)
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
	dyad_write(remote->socket, &ib, 1);
}

static void
on_socket_recv(dyad_Event* e)
{
	bool      need_resize = false;
	remote_t* remote;
	char*     p_next_write;

	remote = e->udata;
	while (remote->recv_size + e->size > remote->recv_buf_size) {
		remote->recv_buf_size *= 2;
		need_resize = true;
	}
	if (need_resize)
		remote->recv_buf = realloc(remote->recv_buf, remote->recv_buf_size);
	p_next_write = remote->recv_buf + remote->recv_size;
	remote->recv_size += e->size;
	memcpy(p_next_write, e->data, e->size);
}
