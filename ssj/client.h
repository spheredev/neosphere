#ifndef SSJ__CLIENT_H__INCLUDED
#define SSJ__CLIENT_H__INCLUDED

typedef struct session session_t;
typedef struct dvalue  dvalue_t;

typedef
enum dvalue_type
{
	DVALUE_BOOLEAN,
	DVALUE_FLOAT,
	DVALUE_INT,
	DVALUE_STRING,
} dvalue_type_t;

void          initialize_client (void);
void          shutdown_client   (void);
dvalue_t*     dvalue_new_float  (double value);
dvalue_t*     dvalue_new_int    (int32_t value);
dvalue_t*     dvalue_new_string (const char* value);
void          dvalue_free       (dvalue_t* dvalue);
dvalue_type_t dvalue_get_type   (dvalue_t* dvalue);

#endif // SSJ__CLIENT_H__INCLUDED
