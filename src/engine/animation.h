#ifndef MINISPHERE__ANIMATION_H__INCLUDED
#define MINISPHERE__ANIMATION_H__INCLUDED

typedef struct animation animation_t;

animation_t* load_animation   (const char* path);
animation_t* ref_animation    (animation_t* anim);
void         free_animation   (animation_t* anim);
bool         update_animation (animation_t* anim);

void init_animation_api (void);

#endif // MINISPHERE__ANIMATION_H__INCLUDED
