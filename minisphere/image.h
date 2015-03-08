#ifndef MINISPHERE__IMAGE_H__INCLUDED
#define MINISPHERE__IMAGE_H__INCLUDED

typedef struct image image_t;

extern image_t*        create_image     (int width, int height);
extern image_t*        clone_image      (const image_t* image);
extern image_t*        load_image       (const char* path);
extern image_t*        read_image       (ALLEGRO_FILE* file, int width, int height);
extern image_t*        ref_image        (image_t* image);
extern void            free_image       (image_t* image);
extern ALLEGRO_BITMAP* get_image_bitmap (const image_t* image);
extern int             get_image_height (const image_t* image);
extern int             get_image_width  (const image_t* image);

extern void init_image_api        (duk_context* ctx);

extern void     duk_push_sphere_image    (duk_context* ctx, image_t* image);
extern image_t* duk_require_sphere_image (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__IMAGE_H__INCLUDED
