#include "minisphere.h"
#include "api.h"
#include "image.h"

#include "animation.h"

#include <libmng.h>

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

static mng_ptr    mng_cb_malloc        (mng_size_t size);
static void       mng_cb_free          (mng_ptr ptr, mng_size_t size);
static mng_bool   mng_cb_openstream    (mng_handle stream);
static mng_bool   mng_cb_closestream   (mng_handle stream);
static mng_ptr    mng_cb_getcanvasline (mng_handle stream, mng_uint32 line_num);
static mng_uint32 mng_cb_gettickcount  (mng_handle stream);
static mng_bool   mng_cb_processheader (mng_handle stream, mng_uint32 width, mng_uint32 height);
static mng_bool   mng_cb_readdata      (mng_handle stream, mng_ptr buf, mng_uint32 n_bytes, mng_uint32p p_readsize);
static mng_bool   mng_cb_refresh       (mng_handle stream, mng_uint32 x, mng_uint32 y, mng_uint32 width, mng_uint32 height);
static mng_bool   mng_cb_settimer      (mng_handle stream, mng_uint32 msecs);

struct animation
{
	unsigned int  refcount;
	unsigned int  id;
	unsigned int  delay;
	sfs_file_t*   file;
	image_t*      frame;
	bool          is_frame_ready;
	image_lock_t* lock;
	mng_handle    stream;
	unsigned int  w, h;
};

static unsigned int s_next_animation_id = 0;

bool
initialize_animation(void)
{
	console_log(1, "Initializing animation");
	return true;
}

void
shutdown_animation(void)
{
	console_log(1, "Shutting down animation");
}

animation_t*
load_animation(const char* path)
{
	animation_t* anim;

	if (!(anim = calloc(1, sizeof(animation_t))))
		goto on_error;
	if (!(anim->stream = mng_initialize(anim, mng_cb_malloc, mng_cb_free, NULL)))
		goto on_error;
	mng_setcb_openstream(anim->stream, mng_cb_openstream);
	mng_setcb_closestream(anim->stream, mng_cb_closestream);
	mng_setcb_getcanvasline(anim->stream, mng_cb_getcanvasline);
	mng_setcb_gettickcount(anim->stream, mng_cb_gettickcount);
	mng_setcb_processheader(anim->stream, mng_cb_processheader);
	mng_setcb_readdata(anim->stream, mng_cb_readdata);
	mng_setcb_refresh(anim->stream, mng_cb_refresh);
	mng_setcb_settimer(anim->stream, mng_cb_settimer);
	if (!(anim->file = sfs_fopen(g_fs, path, "animations", "rb")))
		goto on_error;
	if (mng_read(anim->stream) != MNG_NOERROR) goto on_error;
	anim->id = s_next_animation_id++;
	
	if (!update_animation(anim))
		goto on_error;
	
	console_log(3, "[anim %u] Loaded %u*%u animation %s\n", anim->id, anim->w, anim->h, path);
	return ref_animation(anim);

on_error:
	if (anim != NULL) {
		if (anim->stream != NULL) mng_cleanup(&anim->stream);
		if (anim->file != NULL) sfs_fclose(anim->file);
		if (anim->frame != NULL) {
			free_image(anim->frame);
		}
		free(anim);
	}
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
	mng_cleanup(&animation->stream);
	sfs_fclose(animation->file);
	free_image(animation->frame);
	free(animation);
}

bool
update_animation(animation_t* anim)
{
	if (!(anim->lock = lock_image(anim->frame)))
		return false;
	if (!anim->is_frame_ready) mng_display(anim->stream);
	else if (mng_display_resume(anim->stream) != MNG_NEEDTIMERWAIT)
		mng_display_reset(anim->stream);
	unlock_image(anim->frame, anim->lock);
	anim->is_frame_ready = true;
	return true;
}

static mng_ptr
mng_cb_malloc(mng_size_t size)
{
	return calloc(1, size);
}

static void
mng_cb_free(mng_ptr ptr, mng_size_t size)
{
	free(ptr);
}

static mng_bool
mng_cb_openstream(mng_handle stream)
{
	return MNG_TRUE;
}

static mng_bool
mng_cb_closestream(mng_handle stream)
{
	return MNG_TRUE;
}

static mng_ptr
mng_cb_getcanvasline(mng_handle stream, mng_uint32 line_num)
{
	animation_t* anim = mng_get_userdata(stream);

	return anim->lock->pixels + line_num * anim->lock->pitch;
}

static mng_uint32
mng_cb_gettickcount(mng_handle stream)
{
	return al_get_time() * 1000;
}

static mng_bool
mng_cb_processheader(mng_handle stream, mng_uint32 width, mng_uint32 height)
{
	animation_t* anim = mng_get_userdata(stream);

	anim->w = width; anim->h = height;
	free_image(anim->frame);
	if (!(anim->frame = create_surface(anim->w, anim->h)))
		goto on_error;
	mng_set_canvasstyle(stream, MNG_CANVAS_RGBA8);
	return MNG_TRUE;

on_error:
	free_image(anim->frame);
	return MNG_FALSE;
}

static mng_bool
mng_cb_readdata(mng_handle stream, mng_ptr buf, mng_uint32 n_bytes, mng_uint32p p_readsize)
{
	animation_t* anim = mng_get_userdata(stream);

	*p_readsize = (mng_uint32)sfs_fread(buf, 1, n_bytes, anim->file);
	return MNG_TRUE;
}

static mng_bool
mng_cb_refresh(mng_handle stream, mng_uint32 x, mng_uint32 y, mng_uint32 width, mng_uint32 height)
{
	return MNG_TRUE;
}

static mng_bool
mng_cb_settimer(mng_handle stream, mng_uint32 msecs)
{
	animation_t* anim = mng_get_userdata(stream);

	anim->delay = msecs;
	return MNG_TRUE;
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
	duk_push_int(ctx, anim->h);
	return 1;
}

static duk_ret_t
js_Animation_get_width(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_int(ctx, anim->w);
	return 1;
}

static duk_ret_t
js_Animation_getDelay(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_uint(ctx, anim->delay);
	return 1;
}

static duk_ret_t
js_Animation_getNumFrames(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	duk_push_uint(ctx, mng_get_framecount(anim->stream));
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
	draw_image(anim->frame, x, y);
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
	draw_image_scaled(anim->frame, x, y, anim->w * scale, anim->h * scale);
	return 0;
}

static duk_ret_t
js_Animation_readNextFrame(duk_context* ctx)
{
	animation_t* anim;

	duk_push_this(ctx);
	anim = duk_require_sphere_obj(ctx, -1, "Animation");
	duk_pop(ctx);
	update_animation(anim);
	return 0;
}
