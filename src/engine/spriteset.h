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
	unsigned int     id;
	rect_t           base;
	char*            filename;
	int              num_images;
	int              num_poses;
	image_t*         *images;
	spriteset_pose_t *poses;
};

void         initialize_spritesets   (void);
void         shutdown_spritesets     (void);
spriteset_t* clone_spriteset         (const spriteset_t* spriteset);
spriteset_t* load_spriteset          (const char* filename);
spriteset_t* ref_spriteset           (spriteset_t* spriteset);
void         free_spriteset          (spriteset_t* spriteset);
rect_t       get_sprite_base         (const spriteset_t* spriteset);
int          get_sprite_frame_delay  (const spriteset_t* spriteset, const char* pose_name, int frame_index);
void         get_sprite_size         (const spriteset_t* spriteset, int* out_width, int* out_height);
image_t*     get_spriteset_image     (const spriteset_t* spriteset, int index);
void         get_spriteset_info      (const spriteset_t* spriteset, int* out_num_images, int* out_num_poses);
bool         get_spriteset_pose_info (const spriteset_t* spriteset, const char* pose_name, int* out_num_frames);
void         set_spriteset_image     (spriteset_t* spriteset, int index, image_t* image);
void         draw_sprite             (const spriteset_t* spriteset, color_t mask, bool is_flipped, double theta, double scale_x, double scale_y, const char* pose_name, float x, float y, int frame_index);

#endif // MINISPHERE__SPRITESET_H__INCLUDED
