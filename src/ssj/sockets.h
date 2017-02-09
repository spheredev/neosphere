#ifndef SSJ__SOCKET_H__INCLUDED
#define SSJ__SOCKET_H__INCLUDED

typedef struct socket socket_t;

void      sockets_init   (void);
void      sockets_deinit (void);
socket_t* socket_connect (const char* hostname, int port, double timeout);
void      socket_close   (socket_t* obj);
bool      socket_is_live (socket_t* obj);
int       socket_recv    (socket_t* obj, void* buffer, int num_bytes);
int       socket_send    (socket_t* obj, const void* data, int num_bytes);

#endif // SSJ__SOCKET_H__INCLUDED
