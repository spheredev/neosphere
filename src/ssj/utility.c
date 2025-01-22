/**
 *  Sphere: the JavaScript game platform
 *  Copyright (c) 2015-2025, Where'd She Go? LLC
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

#include "ssj.h"
#include "utility.h"

#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#endif

void*
fslurp(const char* filename, size_t* out_size)
{
	char*  buffer;
	FILE*  file = NULL;
	size_t read_size;

	if (!(file = fopen(filename, "rb")))
		return NULL;
	fseek(file, 0, SEEK_END);
	read_size = (size_t)ftell(file);
	if (out_size != NULL)
		*out_size = read_size;
	if (!(buffer = (char*)malloc(read_size + 1)))
		goto on_error;
	fseek(file, 0, SEEK_SET);
	if (fread(buffer, 1, read_size, file) != read_size)
		goto on_error;
	buffer[read_size] = '\0';
	fclose(file);
	return buffer;

on_error:
	fclose(file);
	return NULL;
}

void
launch_url(const char* url)
{
	char* command;

#if defined(_WIN32)
	command = strnewf("start %s", url);
#elif defined(__APPLE__)
	command = strnewf("open %s", url);
#else
	command = strnewf("xdg-open %s", url);
#endif
	system(command);
	free(command);
}

void
stall(double time)
{
#if defined(_WIN32)
	Sleep((DWORD)(1000.0 * time));
#else
	usleep((useconds_t)(1000000.0 * time));
#endif
}
