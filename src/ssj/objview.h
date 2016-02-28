#ifndef SSJ__OBJECTVIEW_H__INCLUDED
#define SSJ__OBJECTVIEW_H__INCLUDED

#include "dvalue.h"

typedef struct objview objview_t;

typedef
enum prop_tag
{
	PROP_VALUE,
	PROP_ACCESSOR,
} prop_tag_t;

typedef
enum prop_flag
{
	PROP_ENUMERABLE,
	PROP_WRITABLE,
	PROP_CONFIGURABLE,
} prop_flag_t;

objview_t*      objview_new          (void);
void            objview_free         (objview_t* obj);
int             objview_len          (const objview_t* obj);
const char*     objview_get_key      (const objview_t* obj, int index);
prop_tag_t      objview_get_tag      (const objview_t* obj, int index);
unsigned int    objview_get_flags    (const objview_t* obj, int index);
const dvalue_t* objview_get_getter   (const objview_t* obj, int index);
const dvalue_t* objview_get_setter   (const objview_t* obj, int index);
const dvalue_t* objview_get_value    (const objview_t* obj, int index);
void            objview_add_accessor (objview_t* obj, const char* key, const dvalue_t* getter, const dvalue_t* setter, unsigned int flags);
void            objview_add_value    (objview_t* obj, const char* key, const dvalue_t* value, unsigned int flags);

#endif // SSJ__OBJECTVIEW_H__INCLUDED
