#include "minisphere.h"
#include "api.h"

#include "animation.h"

static duk_ret_t js_LoadAnimation             (duk_context* ctx);
static duk_ret_t js_new_Animation             (duk_context* ctx);
static duk_ret_t js_Animation_finalize        (duk_context* ctx);
static duk_ret_t js_Animation_get_height      (duk_context* ctx);
static duk_ret_t js_Animation_get_width       (duk_context* ctx);
static duk_ret_t js_Animation_getDelay        (duk_context* ctx);
static duk_ret_t js_Animation_getNumFrames    (duk_context* ctx);
static duk_ret_t js_Animation_drawFrame       (duk_context* ctx);
static duk_ret_t js_Animation_drawZoomedFrame (duk_context* ctx);
static duk_ret_t js_Animation_readNextFrame   (duk_context* ctx);

struct animation
{
	unsigned int refcount;
	unsigned int id;
};

static unsigned int s_next_animation_id = 0;

animation_t*
load_animation(const char* path)
{
	animation_t* anim;

	if (!(anim = calloc(1, sizeof(animation_t))))
		goto on_error;
	anim->id = s_next_animation_id++;
	return anim;

on_error:
	free(anim);
	return NULL;
}

animation_t*
ref_animation(animation_t* animation)
{
	++animation->refcount;
	return animation;
}

void
free_animation(animation_t* animation)
{
	if (animation == NULL || --animation->refcount > 0)
		return;
	free(animation);
}

void
init_animation_api(void)
{
	register_api_function(g_duk, NULL, "LoadAnimation", js_LoadAnimation);
	register_api_ctor(g_duk, "Animation", js_new_Animation, js_Animation_finalize);
	register_api_prop(g_duk, "Animation", "width", js_Animation_get_width, NULL);
	register_api_prop(g_duk, "Animation", "height", js_Animation_get_height, NULL);
	register_api_function(g_duk, "Animation", "getDelay", js_Animation_getDelay);
	register_api_function(g_duk, "Animation", "getNumFrames", js_Animation_getNumFrames);
	register_api_function(g_duk, "Animation", "drawFrame", js_Animation_drawFrame);
	register_api_function(g_duk, "Animation", "drawZoomedFrame", js_Animation_drawZoomedFrame);
	register_api_function(g_duk, "Animation", "readNextFrame", js_Animation_readNextFrame);
}

static duk_ret_t
js_LoadAnimation(duk_context* ctx)
{
	duk_require_string(ctx, 0);

	js_new_Animation(ctx);
	return 1;
}

static duk_ret_t
js_new_Animation(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	animation_t* anim;
	char*        path;

	path = get_asset_path(filename, "animations", false);
	if (!(anim = load_animation(path)))
		duk_error_ni(ctx, -1, DUK_ERR_ERROR, "Animation(): Failed to load animation file '%s'", filename);
	free(path);
	duk_push_sphere_obj(ctx, "Animation", anim);
	return 1;
}

static duk_ret_t
js_Animation_finalize(duk_context* ctx)
{
	animation_t* anim;

	anim = duk_require_sphere_obj(ctx, 0, "Animation");
	free_animation(anim);
	return 0;
}

static duk_ret_t
js_Animation_get_height(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_int(ctx, 0);
	return 1;
}

static duk_ret_t
js_Animation_get_width(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_int(ctx, 0);
	return 1;
}

static duk_ret_t
js_Animation_getDelay(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_int(ctx, 0);
	return 1;
}

static duk_ret_t
js_Animation_getNumFrames(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_int(ctx, 0);
	return 1;
}

static duk_ret_t
js_Animation_drawFrame(duk_context* ctx)
{
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	return 0;
}

static duk_ret_t
js_Animation_drawZoomedFrame(duk_context* ctx)
{
	int x = duk_require_number(ctx, 0);
	int y = duk_require_number(ctx, 1);
	float scale = duk_require_number(ctx, 2);

	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	if (scale < 0.0)
		duk_error_ni(ctx, -1, DUK_ERR_RANGE_ERROR, "Animation:drawZoomedFrame(): Scale must not be negative");
	return 0;
}

static duk_ret_t
js_Animation_readNextFrame(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	return 0;
}
