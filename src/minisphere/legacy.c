#include "minisphere.h"
#include "legacy.h"

#include "font.h"
#include "image.h"
#include "kev_file.h"
#include "sockets.h"
#include "windowstyle.h"

struct socket_v1
{
	unsigned int refcount;
	socket_t*    client;
	server_t*    server;
};

image_t*       s_default_arrow = NULL;
image_t*       s_default_arrow_down = NULL;
image_t*       s_default_arrow_up = NULL;
font_t*        s_default_font;
windowstyle_t* s_default_windowstyle = NULL;

bool
legacy_init(void)
{
	path_t*     path;
	kev_file_t* system_ini;
	
	console_log(1, "initializing legacy subsystem");

	system_ini = kev_open(NULL, "#/system.ini", false);
	
	// system default font
	path = fs_build_path(
		kev_read_string(system_ini, "Font", "system.rfn"),
		"#/", true);
	s_default_font = font_load(path_cstr(path));
	path_free(path);

	// system default windowstyle
	path = fs_build_path(
		kev_read_string(system_ini, "WindowStyle", "system.rws"),
		"#/", true);
	s_default_windowstyle = load_windowstyle(path_cstr(path));
	path_free(path);

	// system default pointer image
	path = fs_build_path(
		kev_read_string(system_ini, "Arrow", "pointer.png"),
		"#/", true);
	s_default_arrow = image_load(path_cstr(path));
	path_free(path);

	// system default up arrow image
	path = fs_build_path(
		kev_read_string(system_ini, "UpArrow", "up_arrow.png"),
		"#/", true);
	s_default_arrow_up = image_load(path_cstr(path));
	path_free(path);

	// system default down arrow image
	path = fs_build_path(
		kev_read_string(system_ini, "DownArrow", "down_arrow.png"),
		"#/", true);
	s_default_arrow_down = image_load(path_cstr(path));
	path_free(path);

	kev_close(system_ini);
	return true;
}

void
legacy_uninit(void)
{
	console_log(1, "shutting down legacy subsystem");
}

image_t*
legacy_default_arrow_image(void)
{
	return s_default_arrow;
}

image_t*
legacy_default_arrow_down_image(void)
{
	return s_default_arrow_down;
}

image_t*
legacy_default_arrow_up_image(void)
{
	return s_default_arrow_up;
}

font_t*
legacy_default_font(void)
{
	return s_default_font;
}

windowstyle_t*
legacy_default_windowstyle(void)
{
	return s_default_windowstyle;
}

socket_v1_t*
socket_v1_new_client(const char* hostname, int port)
{
	socket_t*    client;
	socket_v1_t* socket;

	if (!(client = socket_new(4096)))
		goto on_error;
	if (!socket_connect(client, hostname, port))
		goto on_error;

	socket = calloc(1, sizeof(socket_v1_t));
	socket->client = client;
	return socket_v1_ref(socket);

on_error:
	socket_free(client);
	return NULL;
}

socket_v1_t*
socket_v1_new_server(int port)
{
	server_t*     server;
	socket_v1_t* socket;

	server = server_new(NULL, port, 4096, 16);

	socket = calloc(1, sizeof(socket_v1_t));
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
socket_v1_free(socket_v1_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	socket_free(it->client);
	server_free(it->server);
	free(it);
}

bool
socket_v1_connected(socket_v1_t* it)
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
socket_v1_close(socket_v1_t* it)
{
	socket_close(it->client);
}

size_t
socket_v1_peek(const socket_v1_t* it)
{
	return socket_peek(it->client);
}

size_t
socket_v1_read(socket_v1_t* it, void* buffer, size_t num_bytes)
{
	return socket_read(it->client, buffer, num_bytes);
}

void
socket_v1_write(socket_v1_t* it, const void* data, size_t num_bytes)
{
	socket_write(it->client, data, num_bytes);
}
