/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2020, Fat Cerberus
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

#include "fs.h"

#include <stdlib.h>

struct fs
{
	fs_fexist_impl_t fexist_impl;
	fs_fslurp_impl_t fslurp_impl;
	void*            userdata;
};

fs_t*
fs_new(void* userdata)
{
	fs_t* fs;

	if (!(fs = calloc(1, sizeof(fs_t))))
		return NULL;
	fs->userdata = userdata;
	return fs;
}

void
fs_free(fs_t* it)
{
	free(it);
}

void
fs_on_fexist(fs_t* it, fs_fexist_impl_t impl)
{
	it->fexist_impl = impl;
}

void
fs_on_fslurp(fs_t* it, fs_fslurp_impl_t impl)
{
	it->fslurp_impl = impl;
}

bool
fs_fexist(const fs_t* it, const char* filename)
{
	return it->fexist_impl(filename, it->userdata);
}

void*
fs_fslurp(const fs_t* it, const char* filename, size_t* out_size)
{
	return it->fslurp_impl(filename, out_size, it->userdata);
}
