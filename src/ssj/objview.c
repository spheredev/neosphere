#include "ssj.h"
#include "objview.h"

#include "dvalue.h"

struct entry
{
	char*      key;
	prop_tag_t tag;
	dvalue_t*  getter;
	dvalue_t*  setter;
	dvalue_t*  value;
};

struct objview
{
	int           num_props;
	int           array_size;
	struct entry* props;
};

objview_t*
objview_new(void)
{
	struct entry* array;
	int           array_size = 16;
	objview_t* dis;

	array = malloc(array_size * sizeof(struct entry));
	
	dis = calloc(1, sizeof(objview_t));
	dis->props = array;
	dis->array_size = array_size;
	return dis;
}

void
objview_free(objview_t* dis)
{
	int i;

	for (i = 0; i < dis->num_props; ++i) {
		free(dis->props[i].key);
		dvalue_free(dis->props[i].value);
	}
	free(dis);
}

int
objview_len(const objview_t* dis)
{
	return dis->num_props;
}

prop_tag_t
objview_get_tag(const objview_t* dis, int index)
{
	return dis->props[index].tag;
}

const char*
objview_get_key(const objview_t* dis, int index)
{
	return dis->props[index].key;
}

const dvalue_t*
objview_get_getter(const objview_t* dis, int index)
{
	return dis->props[index].tag == PROP_ACCESSOR
		? dis->props[index].getter : NULL;
}

const dvalue_t*
objview_get_setter(const objview_t* dis, int index)
{
	return dis->props[index].tag == PROP_ACCESSOR
		? dis->props[index].setter : NULL;
}

const dvalue_t*
objview_get_value(const objview_t* dis, int index)
{
	return dis->props[index].tag == PROP_VALUE ? dis->props[index].value : NULL;
}

void
objview_add_accessor(objview_t* dis, const char* key, const dvalue_t* getter, const dvalue_t* setter)
{
	int idx;

	idx = dis->num_props++;
	if (dis->num_props > dis->array_size) {
		dis->array_size *= 2;
		dis->props = realloc(dis->props, dis->array_size * sizeof(struct entry));
	}
	dis->props[idx].tag = PROP_ACCESSOR;
	dis->props[idx].key = strdup(key);
	dis->props[idx].getter = dvalue_dup(getter);
	dis->props[idx].setter = dvalue_dup(setter);
}

void
objview_add_value(objview_t* dis, const char* key, const dvalue_t* value)
{
	int idx;

	idx = dis->num_props++;
	if (dis->num_props > dis->array_size) {
		dis->array_size *= 2;
		dis->props = realloc(dis->props, dis->array_size * sizeof(struct entry));
	}
	dis->props[idx].tag = PROP_VALUE;
	dis->props[idx].key = strdup(key);
	dis->props[idx].value = dvalue_dup(value);
}
