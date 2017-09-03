/**
 *  SSj, the Sphere JavaScript debugger
 *  Copyright (c) 2015-2017, Fat Cerberus
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
 *  * Neither the name of miniSphere nor the names of its contributors may be
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

#include "ssj.h"
#include "objview.h"

#include "dmessage.h"

struct entry
{
	char*        key;
	prop_tag_t   tag;
	unsigned int flags;
	dvalue_t*    getter;
	dvalue_t*    setter;
	dvalue_t*    value;
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
	objview_t* obj;

	array = malloc(array_size * sizeof(struct entry));

	obj = calloc(1, sizeof(objview_t));
	obj->props = array;
	obj->array_size = array_size;
	return obj;
}

void
objview_free(objview_t* obj)
{
	int i;

	if (obj == NULL)
		return;

	for (i = 0; i < obj->num_props; ++i) {
		free(obj->props[i].key);
		dvalue_free(obj->props[i].value);
		dvalue_free(obj->props[i].getter);
		dvalue_free(obj->props[i].setter);
	}
	free(obj);
}

unsigned int
objview_get_flags(const objview_t* obj, int index)
{
	return obj->props[index].flags;
}

int
objview_len(const objview_t* obj)
{
	return obj->num_props;
}

prop_tag_t
objview_get_tag(const objview_t* obj, int index)
{
	return obj->props[index].tag;
}

const char*
objview_get_key(const objview_t* obj, int index)
{
	return obj->props[index].key;
}

const dvalue_t*
objview_get_getter(const objview_t* obj, int index)
{
	return obj->props[index].tag == PROP_ACCESSOR
		? obj->props[index].getter : NULL;
}

const dvalue_t*
objview_get_setter(const objview_t* obj, int index)
{
	return obj->props[index].tag == PROP_ACCESSOR
		? obj->props[index].setter : NULL;
}

const dvalue_t*
objview_get_value(const objview_t* obj, int index)
{
	return obj->props[index].tag == PROP_VALUE ? obj->props[index].value : NULL;
}

void
objview_add_accessor(objview_t* obj, const char* key, const dvalue_t* getter, const dvalue_t* setter, unsigned int flags)
{
	int idx;

	idx = obj->num_props++;
	if (obj->num_props > obj->array_size) {
		obj->array_size *= 2;
		obj->props = realloc(obj->props, obj->array_size * sizeof(struct entry));
	}
	obj->props[idx].tag = PROP_ACCESSOR;
	obj->props[idx].key = strdup(key);
	obj->props[idx].value = NULL;
	obj->props[idx].getter = dvalue_dup(getter);
	obj->props[idx].setter = dvalue_dup(setter);
	obj->props[idx].flags = flags;
}

void
objview_add_value(objview_t* obj, const char* key, const dvalue_t* value, unsigned int flags)
{
	int idx;

	idx = obj->num_props++;
	if (obj->num_props > obj->array_size) {
		obj->array_size *= 2;
		obj->props = realloc(obj->props, obj->array_size * sizeof(struct entry));
	}
	obj->props[idx].tag = PROP_VALUE;
	obj->props[idx].key = strdup(key);
	obj->props[idx].value = dvalue_dup(value);
	obj->props[idx].getter = NULL;
	obj->props[idx].setter = NULL;
	obj->props[idx].flags = flags;
}
