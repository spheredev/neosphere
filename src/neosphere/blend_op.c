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

#include "neosphere.h"
#include "blend_op.h"

struct blend_op
{
	unsigned int  refcount;
	int           alpha_op;
	int           alpha_s_factor;
	int           alpha_t_factor;
	int           color_op;
	int           color_s_factor;
	int           color_t_factor;
	ALLEGRO_COLOR color;
};

static int get_allegro_blend_factor (blend_factor_t factor);
static int get_allegro_blend_op     (blend_type_t type);

blend_op_t*
blend_op_new_asym(blend_type_t color_op, blend_factor_t sfc, blend_factor_t tfc, blend_type_t alpha_op, blend_factor_t sfa, blend_factor_t tfa)
{
	blend_op_t* op;

	if (!(op = calloc(1, sizeof(blend_op_t))))
		return NULL;
	op->color_op = get_allegro_blend_op(color_op);
	op->color_s_factor = get_allegro_blend_factor(sfc);
	op->color_t_factor = get_allegro_blend_factor(tfc);
	op->alpha_op = get_allegro_blend_op(alpha_op);
	op->alpha_s_factor = get_allegro_blend_factor(sfa);
	op->alpha_t_factor = get_allegro_blend_factor(tfa);
	op->color = al_map_rgba_f(1.0, 1.0, 1.0, 1.0);
	return blend_op_ref(op);
}

blend_op_t*
blend_op_new_sym(blend_type_t op_type, blend_factor_t sf, blend_factor_t tf)
{
	blend_op_t* op;

	if (!(op = calloc(1, sizeof(blend_op_t))))
		return NULL;
	op->color_op = get_allegro_blend_op(op_type);
	op->color_s_factor = get_allegro_blend_factor(sf);
	op->color_t_factor = get_allegro_blend_factor(tf);
	op->alpha_op = op->color_op;
	op->alpha_s_factor = op->color_s_factor;
	op->alpha_t_factor = op->color_t_factor;
	op->color = al_map_rgba_f(1.0, 1.0, 1.0, 1.0);
	return blend_op_ref(op);
}

blend_op_t*
blend_op_ref(blend_op_t* it)
{
	if (it != NULL)
		++it->refcount;
	return it;
}

void
blend_op_unref(blend_op_t* it)
{
	if (it == NULL || --it->refcount > 0)
		return;
	free(it);
}

void
blend_op_set_const(blend_op_t* it, float r, float g, float b, float a)
{
	it->color = al_map_rgba_f(r, g, b, a);
}

void
blend_op_apply(const blend_op_t* it)
{
	if (it != NULL) {
		al_set_blend_color(it->color);
		al_set_separate_blender(
			it->color_op, it->color_s_factor, it->color_t_factor,
			it->alpha_op, it->alpha_s_factor, it->alpha_t_factor);
	}
	else {
		// default blend op: alpha blend
		al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);
	}
}

static int
get_allegro_blend_factor(blend_factor_t factor)
{
	return factor == BLEND_ONE ? ALLEGRO_ONE
		: factor == BLEND_INV_ALPHA ? ALLEGRO_INVERSE_ALPHA
		: factor == BLEND_INV_CONST ? ALLEGRO_INVERSE_CONST_COLOR
		: factor == BLEND_INV_SRC ? ALLEGRO_INVERSE_SRC_COLOR
		: factor == BLEND_INV_DEST ? ALLEGRO_INVERSE_DEST_COLOR
		: factor == BLEND_ALPHA ? ALLEGRO_ALPHA
		: factor == BLEND_CONST ? ALLEGRO_CONST_COLOR
		: factor == BLEND_SRC ? ALLEGRO_SRC_COLOR
		: factor == BLEND_DEST ? ALLEGRO_DEST_COLOR
		: ALLEGRO_ZERO;
}

static int
get_allegro_blend_op(blend_type_t type)
{
	return type == BLEND_OP_SUB_INV ? ALLEGRO_SRC_MINUS_DEST
		: type == BLEND_OP_SUB ? ALLEGRO_DEST_MINUS_SRC
		: ALLEGRO_ADD;
}
