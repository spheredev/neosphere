typedef struct socket socket_t;

extern socket_t*   connect_to_host     (const char* hostname, int port, size_t buffer_size);
extern socket_t*   listen_on_port      (const char* hostname, int port, size_t buffer_size, int max_backlog);
extern socket_t*   ref_socket          (socket_t* socket);
extern void        free_socket         (socket_t* socket);
extern bool        is_socket_live      (socket_t* socket);
extern bool        is_socket_server    (socket_t* socket);
extern const char* get_socket_host     (socket_t* socket);
extern int         get_socket_port     (socket_t* socket);
extern size_t      peek_socket         (const socket_t* socket);
extern void        pipe_socket         (socket_t* socket, socket_t* destination);
extern socket_t*   accept_next_socket  (socket_t* listener);
extern size_t      read_socket         (socket_t* socket, uint8_t* buffer, size_t n_bytes);
extern void        write_socket        (socket_t* socket, const uint8_t* data, size_t n_bytes);

void init_sockets_api (void);
