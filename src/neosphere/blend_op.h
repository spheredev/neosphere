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

#ifndef SPHERE__BLEND_OP_H__INCLUDED
#define SPHERE__BLEND_OP_H__INCLUDED

typedef struct blend_op blend_op_t;

typedef
enum blend_type
{
	BLEND_OP_ADD,
	BLEND_OP_SUB,
	BLEND_OP_SUB_INV,
	BLEND_OP_MAX,
} blend_type_t;

typedef
enum blend_factor
{
	BLEND_ALPHA,
	BLEND_CONST,
	BLEND_DEST,
	BLEND_INV_ALPHA,
	BLEND_INV_CONST,
	BLEND_INV_DEST,
	BLEND_INV_SRC,
	BLEND_ONE,
	BLEND_SRC,
	BLEND_ZERO,
	BLEND_FACTOR_MAX,
} blend_factor_t;

blend_op_t* blend_op_new_asym  (blend_type_t color_op, blend_factor_t sfc, blend_factor_t tfc, blend_type_t alpha_op, blend_factor_t sfa, blend_factor_t tfa);
blend_op_t* blend_op_new_sym   (blend_type_t op_type, blend_factor_t sf, blend_factor_t tf);
blend_op_t* blend_op_ref       (blend_op_t* it);
void        blend_op_unref     (blend_op_t* it);
void        blend_op_set_const (blend_op_t* it, float r, float g, float b, float a);
void        blend_op_apply     (const blend_op_t* it);

#endif // SPHERE__BLEND_OP_H__INCLUDED
