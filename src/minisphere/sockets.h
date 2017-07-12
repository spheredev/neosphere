#ifndef MINISPHERE__SOCKETS_H__INCLUDED
#define MINISPHERE__SOCKETS_H__INCLUDED

typedef struct client client_t;
typedef struct server server_t;
typedef struct socket socket_t;

bool        sockets_init      (void);
void        sockets_uninit    (void);
void        sockets_update    (void);

client_t*   client_new        (const char* hostname, int port, size_t buffer_size);
client_t*   client_ref        (client_t* it);
void        client_free       (client_t* it);
bool        client_connected  (const client_t* it);
const char* client_hostname   (const client_t* it);
int         client_port       (const client_t* it);
void        client_close      (client_t* it);
size_t      client_peek       (const client_t* it);
size_t      client_read       (client_t* it, void* buffer, size_t num_bytes);
void        client_write      (client_t* it, const void* data, size_t num_bytes);
server_t*   server_new        (const char* hostname, int port, size_t buffer_size, int max_backlog);
server_t*   server_ref        (server_t* it);
void        server_free       (server_t* it);
client_t*   server_accept     (server_t* it);
socket_t*   socket_new_client (const char* hostname, int port, size_t buffer_size);
socket_t*   socket_new_server (const char* hostname, int port, size_t buffer_size, int max_backlog);
socket_t*   socket_ref        (socket_t* it);
void        socket_free       (socket_t* it);
bool        socket_connected  (socket_t* it);
bool        socket_is_server  (socket_t* it);
const char* socket_hostname   (socket_t* it);
size_t      socket_num_bytes  (socket_t* it);
int         socket_port       (socket_t* it);
socket_t*   socket_accept     (socket_t* listener);
void        socket_close      (socket_t* it);
size_t      socket_peek       (const socket_t* socket);
size_t      socket_read       (socket_t* it, void* buffer, size_t n_bytes);
void        socket_write      (socket_t* it, const void* data, size_t n_bytes);

#endif // MINISPHERE__SOCKETS_H__INCLUDED
