typedef struct socket socket_t;

void duk_push_sphere_socket(duk_context* ctx, socket_t* socket);

socket_t* connect_to_host (const char* hostname, int port, size_t buffer_size);
socket_t* listen_on_port  (int port, size_t buffer_size);
socket_t* ref_socket      (socket_t* socket);
void      free_socket     (socket_t* socket);

void init_networking_api (void);
