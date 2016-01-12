#ifndef SSJ__REMOTE_H__INCLUDED
#define SSJ__REMOTE_H__INCLUDED

typedef struct remote remote_t;
typedef struct dvalue dvalue_t;

typedef
enum dvalue_type
{
	DVALUE_BOOL,
	DVALUE_BUFFER,
	DVALUE_FLOAT,
	DVALUE_INT,
	DVALUE_STRING,
} dvalue_type_t;

void          initialize_client (void);
void          shutdown_client   (void);
remote_t*     connect_remote    (const char* hostname, int port);
void          close_remote      (remote_t* remote);
dvalue_t*     receive_dvalue    (remote_t* remote);
void          send_dvalue       (remote_t* remote, const dvalue_t* dvalue);
dvalue_t*     dvalue_new_float  (double value);
dvalue_t*     dvalue_new_int    (int32_t value);
dvalue_t*     dvalue_new_string (const char* value);
void          dvalue_free       (dvalue_t* dvalue);
dvalue_type_t dvalue_get_type   (dvalue_t* dvalue);

#endif // SSJ__REMOTE_H__INCLUDED
