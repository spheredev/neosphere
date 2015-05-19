#ifndef MINISPHERE__ANIMATION_H__INCLUDED
#define MINISPHERE__ANIMATION_H__INCLUDED

typedef struct animation animation_t;

extern animation_t* load_animation (const char* path);
extern animation_t* ref_animation  (animation_t* animation);
extern void         free_animation (animation_t* animation);

extern void init_animation_api (void);

#endif // MINISPHERE__ANIMATION_H__INCLUDED
