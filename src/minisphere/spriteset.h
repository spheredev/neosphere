#ifndef MINISPHERE__SPRITESET_H__INCLUDED
#define MINISPHERE__SPRITESET_H__INCLUDED

#include "image.h"

typedef struct spriteset spriteset_t;

void         spritesets_init         (void);
void         spritesets_uninit       (void);
spriteset_t* spriteset_new           (void);
spriteset_t* spriteset_load          (const char* filename);
spriteset_t* spriteset_clone         (const spriteset_t* it);
spriteset_t* spriteset_ref           (spriteset_t* it);
void         spriteset_free          (spriteset_t* it);
int          spriteset_num_frames    (const spriteset_t* it, const char* pose_name);
int          spriteset_num_images    (const spriteset_t* it);
int          spriteset_num_poses     (const spriteset_t* it);
void         spriteset_frame_info    (const spriteset_t* it, const char* pose_name, int index, int* out_image_index, int* out_delay);
const char*  spriteset_pose_name     (const spriteset_t* it, int index);
rect_t       spriteset_get_base      (const spriteset_t* it);
void         spriteset_set_base      (spriteset_t* it, rect_t new_base);
void         spriteset_add_frame     (spriteset_t* it, const char* pose_name, int image_idx, int delay);
void         spriteset_add_image     (spriteset_t* it, image_t* image);
void         spriteset_add_pose      (spriteset_t* it, const char* name);
void         spriteset_draw          (const spriteset_t* it, color_t mask, bool is_flipped, double theta, double scale_x, double scale_y, const char* pose_name, float x, float y, int frame_index);
int          get_sprite_frame_delay  (const spriteset_t* it, const char* pose_name, int frame_index);
void         get_sprite_size         (const spriteset_t* it, int* out_width, int* out_height);
image_t*     get_spriteset_image     (const spriteset_t* it, int index);
void         get_spriteset_info      (const spriteset_t* it, int* out_num_images, int* out_num_poses);
const char*  get_spriteset_path      (const spriteset_t* it);
bool         get_spriteset_pose_info (const spriteset_t* it, const char* pose_name, int* out_num_frames);
void         set_spriteset_image     (spriteset_t* it, int index, image_t* image);

#endif // MINISPHERE__SPRITESET_H__INCLUDED
