#ifndef SSJ__SOCKET_H__INCLUDED
#define SSJ__SOCKET_H__INCLUDED

typedef struct socket socket_t;

void      sockets_init   (void);
void      sockets_deinit (void);
socket_t* socket_connect (const char* hostname, int port, double timeout);
void      socket_close   (socket_t* socket);
int       socket_recv    (socket_t* socket, void* buffer, int num_bytes);
int       socket_send    (socket_t* socket, const void* data, int num_bytes);

#endif // SSJ__SOCKET_H__INCLUDED
