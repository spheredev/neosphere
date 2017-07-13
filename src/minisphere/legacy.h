#ifndef MINISPHERE__LEGACY_H__INCLUDED
#define MINISPHERE__LEGACY_H__INCLUDED

typedef struct v1_socket v1_socket_t;

v1_socket_t* v1_socket_new_client (const char* hostname, int port);
v1_socket_t* v1_socket_new_server (int port);
v1_socket_t* v1_socket_ref        (v1_socket_t* it);
void         v1_socket_free       (v1_socket_t* it);
bool         v1_socket_connected  (v1_socket_t* it);
void         v1_socket_close      (v1_socket_t* it);
size_t       v1_socket_peek       (const v1_socket_t* it);
size_t       v1_socket_read       (v1_socket_t* it, void* buffer, size_t num_bytes);
void         v1_socket_write      (v1_socket_t* it, const void* data, size_t num_bytes);

#endif // MINISPHERE__LEGACY_H__INCLUDED
