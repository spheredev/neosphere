#ifndef MINISPHERE__SPRITESET_H__INCLUDED
#define MINISPHERE__SPRITESET_H__INCLUDED

#include "image.h"

typedef struct spriteset       spriteset_t;
typedef struct spriteset_base  spriteset_base_t;
typedef struct spriteset_pose  spriteset_pose_t;
typedef struct spriteset_frame spriteset_frame_t;

struct spriteset_frame
{
	int image_idx;
	int delay;
};

struct spriteset_pose
{
	int               num_frames;
	char*             name;
	spriteset_frame_t *frames;
};

struct spriteset
{
	int              c_refs;
	rect_t           base;
	int              num_images;
	int              num_poses;
	image_t*         *images;
	spriteset_pose_t *poses;
};

extern spriteset_t* load_spriteset         (const char* path);
extern spriteset_t* ref_spriteset          (spriteset_t* spriteset);
extern void         free_spriteset         (spriteset_t* spriteset);
extern rect_t       get_sprite_base        (const spriteset_t* spriteset);
extern int          get_sprite_frame_delay (const spriteset_t* spriteset, const char* pose_name, int frame_index);
extern void         draw_sprite            (const spriteset_t* spriteset, const char* pose_name, float x, float y, int frame_index);

extern void         init_spriteset_api    (duk_context* ctx);
extern void         duk_push_spriteset    (duk_context* ctx, spriteset_t* spriteset);
extern spriteset_t* duk_require_spriteset (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__SPRITESET_H__INCLUDED
