/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of Spherical nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

#ifndef SPHERE__SOCKETS_H__INCLUDED
#define SPHERE__SOCKETS_H__INCLUDED

#include <stdbool.h>
#include <stddef.h>

typedef void (* sockets_on_idle_t) (void);

typedef struct server server_t;
typedef struct socket socket_t;

bool        sockets_init         (sockets_on_idle_t idle_handler);
void        sockets_uninit       (void);
void        sockets_update       (void);
server_t*   server_new           (const char* hostname, int port, size_t buffer_size, int max_backlog, bool sync_mode);
server_t*   server_ref           (server_t* it);
void        server_unref         (server_t* it);
int         server_num_pending   (const server_t* it);
bool        server_get_no_delay  (const server_t* it);
void        server_set_no_delay  (server_t* it, bool enabled);
socket_t*   server_accept        (server_t* it);
socket_t*   socket_new           (size_t buffer_size, bool sync_mode);
socket_t*   socket_ref           (socket_t* it);
void        socket_unref         (socket_t* it);
bool        socket_get_no_delay  (const socket_t* it);
void        socket_set_no_delay  (socket_t* it, bool enabled);
int         socket_bytes_avail   (const socket_t* it);
int         socket_bytes_in      (const socket_t* it);
int         socket_bytes_out     (const socket_t* it);
int         socket_bytes_pending (const socket_t* it);
bool        socket_connected     (const socket_t* it);
bool        socket_closed        (const socket_t* it);
const char* socket_hostname      (const socket_t* it);
int         socket_port          (const socket_t* it);
void        socket_close         (socket_t* it);
bool        socket_connect       (socket_t* it, const char* hostname, int port);
void        socket_disconnect    (socket_t* it);
int         socket_peek          (socket_t* it, void* buffer, int num_bytes);
int         socket_read          (socket_t* it, void* buffer, int num_bytes);
int         socket_write         (socket_t* it, const void* data, int num_bytes);

#endif // SPHERE__SOCKETS_H__INCLUDED
