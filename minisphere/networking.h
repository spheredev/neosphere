typedef struct socket socket_t;

void duk_push_sphere_socket(duk_context* ctx, socket_t* socket);

socket_t* connect_to_host        (const char* hostname, int port, size_t buffer_size);
socket_t* listen_on_port         (int port, size_t buffer_size);
socket_t* ref_socket             (socket_t* socket);
void      free_socket            (socket_t* socket);
bool      is_socket_data_lost    (socket_t* socket);
bool      is_socket_live         (socket_t* socket);
size_t    read_socket            (socket_t* socket, uint8_t* buffer, size_t n_bytes);
void      write_socket           (socket_t* socket, uint8_t* data, size_t n_bytes);

void init_networking_api (void);
