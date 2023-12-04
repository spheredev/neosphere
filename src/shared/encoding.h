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

#ifndef SPHERE__ENCODING_H__INCLUDED
#define SPHERE__ENCODING_H__INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include "lstring.h"

typedef struct decoder decoder_t;
typedef struct encoder encoder_t;

decoder_t* decoder_new        (bool fatal, bool ignore_bom);
decoder_t* decoder_ref        (decoder_t* decoder);
void       decoder_free       (decoder_t* decoder);
bool       decoder_fatal      (const decoder_t* decoder);
bool       decoder_ignore_bom (const decoder_t* decoder);
lstring_t* decoder_finish     (decoder_t* decoder);
lstring_t* decoder_run        (decoder_t* decoder, const uint8_t* buffer, size_t size);
encoder_t* encoder_new        (void);
encoder_t* encoder_ref        (encoder_t* encoder);
void       encoder_free       (encoder_t* encoder);
uint8_t*   encoder_run        (encoder_t* encoder, const lstring_t* string, size_t *out_size);

#endif // SPHERE__ENCODING_H__INCLUDED
