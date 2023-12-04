/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2024, Fat Cerberus
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
 *  * Neither the name of Spherical nor the names of its contributors may be
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

#ifndef SPHERE__OBJECTVIEW_H__INCLUDED
#define SPHERE__OBJECTVIEW_H__INCLUDED

#include "ki.h"

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
	PROP_WRITABLE     = 1 << 0,
	PROP_ENUMERABLE   = 1 << 1,
	PROP_CONFIGURABLE = 1 << 2,
} prop_flag_t;

objview_t*       objview_new          (void);
void             objview_free         (objview_t* obj);
int              objview_len          (const objview_t* obj);
const char*      objview_get_class    (const objview_t* obj, int index);
const char*      objview_get_key      (const objview_t* obj, int index);
prop_tag_t       objview_get_tag      (const objview_t* obj, int index);
unsigned int     objview_get_flags    (const objview_t* obj, int index);
const ki_atom_t* objview_get_getter   (const objview_t* obj, int index);
const ki_atom_t* objview_get_setter   (const objview_t* obj, int index);
const ki_atom_t* objview_get_value    (const objview_t* obj, int index);
void             objview_add_accessor (objview_t* obj, const char* key, const ki_atom_t* getter, const ki_atom_t* setter, unsigned int flags);
void             objview_add_value    (objview_t* obj, const char* key, const char* class_name, const ki_atom_t* value, unsigned int flags);

#endif // SPHERE__OBJECTVIEW_H__INCLUDED
