#ifndef SSJ__CLIENT_H__INCLUDED
#define SSJ__CLIENT_H__INCLUDED

#include "message.h"

typedef struct client client_t;

void          clients_init    (void);
void          clients_deinit  (void);
client_t*     client_connect  (const char* hostname, int port);
void          client_close    (client_t* obj);
message_t*    client_recv_msg (client_t* obj);
void          client_send_msg (client_t* obj, message_t* msg);

#endif // SSJ__CLIENT_H__INCLUDED
