#ifndef SSJ__SOCKET_H__INCLUDED
#define SSJ__SOCKET_H__INCLUDED

typedef struct socket socket_t;

void      sockets_init      (void);
void      sockets_deinit    (void);
socket_t* socket_new_client (const char* hostname, int port, double timeout);
void      socket_close      (socket_t* it);
bool      socket_connected  (socket_t* it);
int       socket_recv       (socket_t* it, void* buffer, int num_bytes);
int       socket_send       (socket_t* it, const void* data, int num_bytes);

#endif // SSJ__SOCKET_H__INCLUDED
