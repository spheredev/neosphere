#ifndef MINISPHERE__ANIMATION_H__INCLUDED
#define MINISPHERE__ANIMATION_H__INCLUDED

typedef struct animation animation_t;

animation_t* animation_new        (const char* path);
animation_t* animation_ref        (animation_t* anim);
void         animation_unref      (animation_t* anim);
int          animation_delay      (const animation_t* anim);
image_t*     animation_frame      (const animation_t* anim);
int          animation_height     (const animation_t* anim);
int          animation_num_frames (const animation_t* anim);
int          animation_width      (const animation_t* anim);
bool         animation_update     (animation_t* anim);

#endif // MINISPHERE__ANIMATION_H__INCLUDED
