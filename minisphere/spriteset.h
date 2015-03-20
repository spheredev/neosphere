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
	lstring_t*        name;
	int               num_frames;
	spriteset_frame_t *frames;
};

struct spriteset
{
	int              refcount;
	rect_t           base;
	lstring_t*       filename;
	int              num_images;
	int              num_poses;
	image_t*         *images;
	spriteset_pose_t *poses;
};

extern spriteset_t* clone_spriteset         (const spriteset_t* spriteset);
extern spriteset_t* load_spriteset          (const char* path);
extern spriteset_t* ref_spriteset           (spriteset_t* spriteset);
extern void         free_spriteset          (spriteset_t* spriteset);
extern rect_t       get_sprite_base         (const spriteset_t* spriteset);
extern int          get_sprite_frame_delay  (const spriteset_t* spriteset, const char* pose_name, int frame_index);
extern void         get_sprite_size         (const spriteset_t* spriteset, int* out_width, int* out_height);
extern void         get_spriteset_info      (const spriteset_t* spriteset, int* out_num_images, int* out_num_poses);
extern bool         get_spriteset_pose_info (const spriteset_t* spriteset, const char* pose_name, int* out_num_frames);
extern void         draw_sprite             (const spriteset_t* spriteset, ALLEGRO_COLOR mask, bool is_flipped, double theta, double scale_x, double scale_y, const char* pose_name, float x, float y, int frame_index);

extern void         init_spriteset_api    (duk_context* ctx);
extern void         duk_push_spriteset    (duk_context* ctx, spriteset_t* spriteset);
extern spriteset_t* duk_require_spriteset (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__SPRITESET_H__INCLUDED
