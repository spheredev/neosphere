#ifndef MINISPHERE__SPRITESET_H__INCLUDED
#define MINISPHERE__SPRITESET_H__INCLUDED

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
	rect_t           base;
	int              num_images;
	int              num_poses;
	ALLEGRO_BITMAP*  *bitmaps;
	spriteset_pose_t *poses;
};

extern spriteset_t* load_spriteset     (const char* path);
extern void         free_spriteset     (spriteset_t* spriteset);
extern rect_t       get_sprite_base    (const spriteset_t* spriteset);
extern void         draw_sprite        (const spriteset_t* spriteset, const char* pose_name, float x, float y, int frame_index);
extern void         init_spriteset_api (duk_context* ctx);

#endif // MINISPHERE__SPRITESET_H__INCLUDED
