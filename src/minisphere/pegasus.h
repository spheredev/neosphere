/**
 *  miniSphere JavaScript game engine
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

#ifndef SPHERE__PEGASUS_H__INCLUDED
#define SPHERE__PEGASUS_H__INCLUDED

enum pegasus_type
{
	CLASS_COLOR = 0x2000,
	CLASS_DIR_STREAM,
	CLASS_FILE_STREAM,
	CLASS_FONT,
	CLASS_INDEX_LIST,
	CLASS_JOB_TOKEN,
	CLASS_JOYSTICK,
	CLASS_KEYBOARD,
	CLASS_MIXER,
	CLASS_MODEL,
	CLASS_MOUSE,
	CLASS_RNG,
	CLASS_SAMPLE,
	CLASS_SERVER,
	CLASS_SHADER,
	CLASS_SHAPE,
	CLASS_SOCKET,
	CLASS_SOUND,
	CLASS_SOUND_STREAM,
	CLASS_SURFACE,
	CLASS_TEXT_DEC,
	CLASS_TEXT_ENC,
	CLASS_TEXTURE,
	CLASS_TRANSFORM,
	CLASS_VERTEX_LIST,
};

void initialize_pegasus_api (void);
bool pegasus_run            (void);

bool jsal_pegasus_eval_module (const char* filename);

#endif // SPHERE__PEGASUS_H__INCLUDED
