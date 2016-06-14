#ifndef MINISPHERE__SOCKETS_H__INCLUDED
#define MINISPHERE__SOCKETS_H__INCLUDED

typedef struct socket socket_t;

bool        initialize_sockets   (void);
void        shutdown_sockets     (void);
void        update_sockets       (void);
socket_t*   connect_to_host      (const char* hostname, int port, size_t buffer_size);
socket_t*   listen_on_port       (const char* hostname, int port, size_t buffer_size, int max_backlog);
socket_t*   ref_socket           (socket_t* socket);
void        free_socket          (socket_t* socket);
bool        is_socket_live       (socket_t* socket);
bool        is_socket_server     (socket_t* socket);
const char* get_socket_host      (socket_t* socket);
int         get_socket_port      (socket_t* socket);
size_t      get_socket_read_size (socket_t* socket);
socket_t*   accept_next_socket   (socket_t* listener);
size_t      peek_socket          (const socket_t* socket);
void        pipe_socket          (socket_t* socket, socket_t* destination);
size_t      read_socket          (socket_t* socket, uint8_t* buffer, size_t n_bytes);
void        shutdown_socket      (socket_t* socket);
void        write_socket         (socket_t* socket, const uint8_t* data, size_t n_bytes);

#endif // MINISPHERE__SOCKETS_H__INCLUDED
