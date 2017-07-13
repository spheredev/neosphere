#ifndef MINISPHERE__SOCKETS_H__INCLUDED
#define MINISPHERE__SOCKETS_H__INCLUDED

typedef struct server server_t;
typedef struct socket socket_t;

bool        sockets_init     (void);
void        sockets_uninit   (void);
void        sockets_update   (void);
server_t*   server_new       (const char* hostname, int port, size_t buffer_size, int max_backlog);
server_t*   server_ref       (server_t* it);
void        server_free      (server_t* it);
socket_t*   server_accept    (server_t* it);
socket_t*   socket_new       (size_t buffer_size);
socket_t*   socket_ref       (socket_t* it);
void        socket_free      (socket_t* it);
bool        socket_connected (const socket_t* it);
const char* socket_hostname  (const socket_t* it);
int         socket_port      (const socket_t* it);
void        socket_close     (socket_t* it);
bool        socket_connect   (socket_t* it, const char* hostname, int port);
size_t      socket_peek      (const socket_t* it);
size_t      socket_read      (socket_t* it, void* buffer, size_t num_bytes);
void        socket_write     (socket_t* it, const void* data, size_t num_bytes);

#endif // MINISPHERE__SOCKETS_H__INCLUDED
