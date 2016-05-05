#ifndef MINISPHERE__ANIMATION_H__INCLUDED
#define MINISPHERE__ANIMATION_H__INCLUDED

typedef struct animation animation_t;

animation_t* animation_new    (const char* path);
animation_t* animation_ref    (animation_t* anim);
void         animation_free   (animation_t* anim);
bool         animation_update (animation_t* anim);

void init_animation_api (void);

#endif // MINISPHERE__ANIMATION_H__INCLUDED
