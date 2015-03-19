typedef struct socket socket_t;

void duk_push_sphere_socket(duk_context* ctx, socket_t* socket);

extern socket_t* connect_to_host     (const char* hostname, int port, size_t buffer_size);
extern socket_t* listen_on_port      (int port, size_t buffer_size, int max_backlog);
extern socket_t* ref_socket          (socket_t* socket);
extern void      free_socket         (socket_t* socket);
extern bool      is_socket_data_lost (socket_t* socket);
extern bool      is_socket_live      (socket_t* socket);
extern bool      is_socket_server    (socket_t* socket);
extern socket_t* accept_next_socket  (socket_t* listener);
extern size_t    read_socket         (socket_t* socket, uint8_t* buffer, size_t n_bytes);
extern void      write_socket        (socket_t* socket, const uint8_t* data, size_t n_bytes);

void init_sockets_api (void);
