#include "ssj.h"
#include "sockets.h"

#include <dyad.h>

struct socket
{
	bool         has_closed;
	dyad_Stream* stream;
	uint8_t*     recv_buffer;
	int          recv_size;
	int          buf_size;
};

static void
dyad_on_stream_close(dyad_Event* e)
{
	socket_t* obj;

	obj = e->udata;
	obj->has_closed = true;
}

static void
dyad_on_stream_recv(dyad_Event* e)
{
	socket_t* obj;
	bool  need_resize = false;
	char* write_ptr;

	obj = e->udata;
	while (obj->recv_size + e->size > obj->buf_size) {
		obj->buf_size *= 2;
		need_resize = true;
	}
	if (need_resize)
		obj->recv_buffer = realloc(obj->recv_buffer, obj->buf_size);
	write_ptr = obj->recv_buffer + obj->recv_size;
	obj->recv_size += e->size;
	memcpy(write_ptr, e->data, e->size);
}

void
sockets_init()
{
	dyad_init();
	dyad_setUpdateTimeout(0.05);
}

void
sockets_deinit()
{
	dyad_shutdown();
}

socket_t*
socket_connect(const char* hostname, int port, double timeout)
{
	clock_t   end_time;
	bool      is_connected;
	socket_t* obj;
	int       state;
	
	obj = calloc(1, sizeof(socket_t));
	
	obj->stream = dyad_newStream();
	dyad_addListener(obj->stream, DYAD_EVENT_DATA, dyad_on_stream_recv, obj);
	dyad_setNoDelay(obj->stream, true);
	obj->buf_size = 4096;
	obj->recv_buffer = malloc(obj->buf_size);
	is_connected = false;
	end_time = clock() + (clock_t)(timeout * CLOCKS_PER_SEC);
	do {
		state = dyad_getState(obj->stream);
		if (state == DYAD_STATE_CLOSED)
			dyad_connect(obj->stream, hostname, port);
		is_connected = state == DYAD_STATE_CONNECTED;
		if (clock() >= end_time)
			goto on_timeout;
		dyad_update();
	} while (state != DYAD_STATE_CONNECTED);
	dyad_addListener(obj->stream, DYAD_EVENT_CLOSE, dyad_on_stream_close, obj);
	return obj;

on_timeout:
	dyad_close(obj->stream);
	free(obj->recv_buffer);
	free(obj);
	return NULL;
}

void
socket_close(socket_t* obj)
{
	if (!obj->has_closed) {
		dyad_end(obj->stream);
		while (!obj->has_closed)
			dyad_update();
	}
	free(obj->recv_buffer);
	free(obj);
}

int
socket_recv(socket_t* obj, void* buffer, int num_bytes)
{
	while (obj->recv_size < num_bytes && !obj->has_closed)
		dyad_update();
	if (obj->recv_size < num_bytes)
		return 0;
	else {
		memcpy(buffer, obj->recv_buffer, num_bytes);
		obj->recv_size -= num_bytes;
		memmove(obj->recv_buffer, obj->recv_buffer + num_bytes, obj->recv_size);
		return num_bytes;
	}
}

int
socket_send(socket_t* obj, const void* data, int num_bytes)
{
	if (obj->has_closed)
		return 0;

	dyad_write(obj->stream, data, num_bytes);
	if (num_bytes > 0)
		dyad_update();
	return num_bytes;
}
