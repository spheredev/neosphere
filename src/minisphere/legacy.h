#ifndef MINISPHERE__LEGACY_H__INCLUDED
#define MINISPHERE__LEGACY_H__INCLUDED

#include "font.h"
#include "image.h"
#include "windowstyle.h"

typedef struct socket_v1 socket_v1_t;

bool           legacy_init                     (void);
void           legacy_uninit                   (void);
font_t*        legacy_default_font             (void);
windowstyle_t* legacy_default_windowstyle      (void);
image_t*       legacy_default_arrow_image      (void);
image_t*       legacy_default_arrow_down_image (void);
image_t*       legacy_default_arrow_up_image   (void);
socket_v1_t*   socket_v1_new_client            (const char* hostname, int port);
socket_v1_t*   socket_v1_new_server            (int port);
socket_v1_t*   socket_v1_ref                   (socket_v1_t* it);
void           socket_v1_unref                 (socket_v1_t* it);
bool           socket_v1_connected             (socket_v1_t* it);
void           socket_v1_close                 (socket_v1_t* it);
size_t         socket_v1_peek                  (const socket_v1_t* it);
size_t         socket_v1_read                  (socket_v1_t* it, void* buffer, size_t num_bytes);
void           socket_v1_write                 (socket_v1_t* it, const void* data, size_t num_bytes);

#endif // MINISPHERE__LEGACY_H__INCLUDED
