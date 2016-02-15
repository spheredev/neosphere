#include "ssj.h"
#include "dvalue.h"

#include "socket.h"

struct dvalue
{
	enum dvalue_tag tag;
	union {
		struct {
			void*  data;
			size_t size;
		} buffer;
		struct {
			uintmax_t value;
			uint8_t   size;
		} ptr;
		double    float_value;
		int       int_value;
	};
};

dvalue_t*
dvalue_new(dvalue_tag_t tag)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = tag;
	return obj;
}

dvalue_t*
dvalue_new_float(double value)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = DVALUE_FLOAT;
	obj->float_value = value;
	return obj;
}

dvalue_t*
dvalue_new_int(int value)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = DVALUE_INT;
	obj->int_value = value;
	return obj;
}

dvalue_t*
dvalue_new_string(const char* value)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	obj->tag = DVALUE_STRING;
	obj->buffer.data = strdup(value);
	obj->buffer.size = strlen(value);
	return obj;
}

dvalue_t*
dvalue_dup(const dvalue_t* src)
{
	dvalue_t* obj;

	obj = calloc(1, sizeof(dvalue_t));
	memcpy(obj, src, sizeof(dvalue_t));
	return obj;
}

dvalue_t*
dvalue_recv(socket_t* socket)
{
	dvalue_t* obj;
	uint8_t data[32];
	uint8_t ib;

	ptrdiff_t i, j;

	obj = calloc(1, sizeof(dvalue_t));
	socket_recv(socket, &ib, 1);
	obj->tag = (enum dvalue_tag)ib;
	switch (ib) {
	case DVALUE_INT:
		socket_recv(socket, data, 4);
		obj->tag = DVALUE_INT;
		obj->int_value = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		break;
	case DVALUE_STRING:
		socket_recv(socket, data, 4);
		obj->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		obj->buffer.data = calloc(1, obj->buffer.size + 1);
		socket_recv(socket, obj->buffer.data, obj->buffer.size);
		obj->tag = DVALUE_STRING;
		break;
	case DVALUE_STRING16:
		socket_recv(socket, data, 2);
		obj->buffer.size = (data[0] << 8) + data[1];
		obj->buffer.data = calloc(1, obj->buffer.size + 1);
		socket_recv(socket, obj->buffer.data, obj->buffer.size);
		obj->tag = DVALUE_STRING;
		break;
	case DVALUE_BUF:
		socket_recv(socket, data, 4);
		obj->buffer.size = (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
		obj->buffer.data = calloc(1, obj->buffer.size + 1);
		socket_recv(socket, obj->buffer.data, obj->buffer.size);
		obj->tag = DVALUE_BUF;
		break;
	case DVALUE_BUF16:
		socket_recv(socket, data, 2);
		obj->buffer.size = (data[0] << 8) + data[1];
		obj->buffer.data = calloc(1, obj->buffer.size + 1);
		socket_recv(socket, obj->buffer.data, obj->buffer.size);
		obj->tag = DVALUE_BUF;
		break;
	case DVALUE_FLOAT:
		socket_recv(socket, data, 8);
		((uint8_t*)&obj->float_value)[0] = data[7];
		((uint8_t*)&obj->float_value)[1] = data[6];
		((uint8_t*)&obj->float_value)[2] = data[5];
		((uint8_t*)&obj->float_value)[3] = data[4];
		((uint8_t*)&obj->float_value)[4] = data[3];
		((uint8_t*)&obj->float_value)[5] = data[2];
		((uint8_t*)&obj->float_value)[6] = data[1];
		((uint8_t*)&obj->float_value)[7] = data[0];
		obj->tag = DVALUE_FLOAT;
		break;
	case DVALUE_OBJ:
		socket_recv(socket, data, 1);
		socket_recv(socket, &obj->ptr.size, 1);
		socket_recv(socket, data, obj->ptr.size);
		obj->tag = DVALUE_OBJ;
		for (i = 0, j = obj->ptr.size - 1; j >= 0; ++i, --j)
			((uint8_t*)&obj->ptr.value)[i] = data[j];
		break;
	case DVALUE_PTR:
		socket_recv(socket, &obj->ptr.size, 1);
		socket_recv(socket, data, obj->ptr.size);
		obj->tag = DVALUE_PTR;
		for (i = 0, j = obj->ptr.size - 1; j >= 0; ++i, --j)
			((uint8_t*)&obj->ptr.value)[i] = data[j];
		break;
	case DVALUE_LIGHTFUNC:
		socket_recv(socket, data, 2);
		socket_recv(socket, &obj->ptr.size, 1);
		socket_recv(socket, data, obj->ptr.size);
		obj->tag = DVALUE_LIGHTFUNC;
		for (i = 0, j = obj->ptr.size - 1; j >= 0; ++i, --j)
			((uint8_t*)&obj->ptr.value)[i] = data[j];
		break;
	case DVALUE_HEAPPTR:
		socket_recv(socket, &obj->ptr.size, 1);
		socket_recv(socket, data, obj->ptr.size);
		obj->tag = DVALUE_HEAPPTR;
		for (i = 0, j = obj->ptr.size - 1; j >= 0; ++i, --j)
			((uint8_t*)&obj->ptr.value)[i] = data[j];
		break;
	default:
		if (ib >= 0x60 && ib <= 0x7F) {
			obj->tag = DVALUE_STRING;
			obj->buffer.size = ib - 0x60;
			obj->buffer.data = calloc(1, obj->buffer.size + 1);
			socket_recv(socket, obj->buffer.data, obj->buffer.size);
		}
		else if (ib >= 0x80 && ib <= 0xBF) {
			obj->tag = DVALUE_INT;
			obj->int_value = ib - 0x80;
		}
		else if (ib >= 0xC0) {
			socket_recv(socket, data, 1);
			obj->tag = DVALUE_INT;
			obj->int_value = ((ib - 0xC0) << 8) + data[0];
		}
	}
	return obj;
}

void
dvalue_free(dvalue_t* obj)
{
	if (obj->tag == DVALUE_STRING || obj->tag == DVALUE_BUF)
		free(obj->buffer.data);
	free(obj);
}

const char*
dvalue_as_cstr(const dvalue_t* obj)
{
	return obj->tag == DVALUE_STRING ? obj->buffer.data : NULL;
}

double
dvalue_as_float(const dvalue_t* obj)
{
	return obj->tag == DVALUE_FLOAT ? obj->float_value : 0.0;
}

int
dvalue_as_int(const dvalue_t* obj)
{
	return obj->tag == DVALUE_INT ? obj->int_value : 0;
}

void
dvalue_send(const dvalue_t* obj, socket_t* socket)
{
	uint8_t  data[32];
	uint32_t str_length;

	ptrdiff_t i, j;

	data[0] = (uint8_t)obj->tag;
	socket_send(socket, data, 1);
	switch (obj->tag) {
	case DVALUE_INT:
		data[0] = (uint8_t)(obj->int_value >> 24 & 0xFF);
		data[1] = (uint8_t)(obj->int_value >> 16 & 0xFF);
		data[2] = (uint8_t)(obj->int_value >> 8 & 0xFF);
		data[3] = (uint8_t)(obj->int_value & 0xFF);
		socket_send(socket, data, 4);
		break;
	case DVALUE_STRING:
		str_length = (uint32_t)strlen(obj->buffer.data);
		data[0] = (uint8_t)(str_length >> 24 & 0xFF);
		data[1] = (uint8_t)(str_length >> 16 & 0xFF);
		data[2] = (uint8_t)(str_length >> 8 & 0xFF);
		data[3] = (uint8_t)(str_length & 0xFF);
		socket_send(socket, data, 4);
		socket_send(socket, obj->buffer.data, (int)str_length);
		break;
	case DVALUE_FLOAT:
		data[0] = ((uint8_t*)&obj->float_value)[7];
		data[1] = ((uint8_t*)&obj->float_value)[6];
		data[2] = ((uint8_t*)&obj->float_value)[5];
		data[3] = ((uint8_t*)&obj->float_value)[4];
		data[4] = ((uint8_t*)&obj->float_value)[3];
		data[5] = ((uint8_t*)&obj->float_value)[2];
		data[6] = ((uint8_t*)&obj->float_value)[1];
		data[7] = ((uint8_t*)&obj->float_value)[0];
		socket_send(socket, data, 8);
		break;
	case DVALUE_HEAPPTR:
		for (i = 0, j = obj->ptr.size - 1; j >= 0; ++i, --j)
			data[i] = ((uint8_t*)&obj->ptr.value)[j];
		socket_send(socket, &obj->ptr.size, 1);
		socket_send(socket, data, obj->ptr.size);
		break;
	}
}

dvalue_tag_t
dvalue_tag(const dvalue_t* obj)
{
	return obj->tag;
}
