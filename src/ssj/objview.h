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

objview_t*      objview_new          (void);
void            objview_free         (objview_t* dis);
int             objview_len          (const objview_t* dis);
const char*     objview_get_key      (const objview_t* dis, int index);
prop_tag_t      objview_get_tag      (const objview_t* dis, int index);
const dvalue_t* objview_get_getter   (const objview_t* dis, int index);
const dvalue_t* objview_get_setter   (const objview_t* dis, int index);
const dvalue_t* objview_get_value    (const objview_t* dis, int index);
void            objview_add_accessor (objview_t* dis, const char* key, const dvalue_t* getter, const dvalue_t* setter);
void            objview_add_value    (objview_t* dis, const char* key, const dvalue_t* value);

#endif // SSJ__OBJECTVIEW_H__INCLUDED
