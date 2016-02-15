#ifndef SSJ__SOCKET_H__INCLUDED
#define SSJ__SOCKET_H__INCLUDED

typedef struct socket socket_t;

void      sockets_init   (void);
void      sockets_deinit (void);
socket_t* socket_connect (const char* hostname, int port, double timeout);
void      socket_close   (socket_t* socket);
size_t    socket_recv    (socket_t* socket, void* buffer, size_t num_bytes);
size_t    socket_send    (socket_t* socket, const void* data, size_t num_bytes);

#endif // SSJ__SOCKET_H__INCLUDED
