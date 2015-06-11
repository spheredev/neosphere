#ifndef MINISPHERE__IMAGE_H__INCLUDED
#define MINISPHERE__IMAGE_H__INCLUDED

typedef struct image      image_t;
typedef struct image_lock image_lock_t;

extern image_t*        create_image             (int width, int height);
extern image_t*        create_subimage          (image_t* parent, int x, int y, int width, int height);
extern image_t*        clone_image              (const image_t* image);
extern image_t*        load_image               (const char* path);
extern image_t*        read_image               (FILE* file, int width, int height);
extern image_t*        read_subimage            (FILE* file, image_t* parent, int x, int y, int width, int height);
extern image_t*        ref_image                (image_t* image);
extern void            free_image               (image_t* image);
extern ALLEGRO_BITMAP* get_image_bitmap         (image_t* image);
extern int             get_image_height         (const image_t* image);
extern color_t         get_image_pixel          (image_t* image, int x, int y);
extern int             get_image_width          (const image_t* image);
extern void            set_image_pixel          (image_t* image, int x, int y, color_t color);
extern bool            apply_color_matrix       (image_t* image, colormatrix_t matrix, int x, int y, int width, int height);
extern bool            apply_color_matrix_4     (image_t* image, colormatrix_t ul_mat, colormatrix_t ur_mat, colormatrix_t ll_mat, colormatrix_t lr_mat, int x, int y, int width, int height);
extern bool            apply_image_lookup       (image_t* image, int x, int y, int width, int height, uint8_t red_lu[256], uint8_t green_lu[256], uint8_t blue_lu[256], uint8_t alpha_lu[256]);
extern void            draw_image               (image_t* image, int x, int y);
extern void            draw_image_masked        (image_t* image, color_t mask, int x, int y);
extern void            draw_image_scaled        (image_t* image, int x, int y, int width, int height);
extern void            draw_image_scaled_masked (image_t* image, color_t mask, int x, int y, int width, int height);
extern void            draw_image_tiled         (image_t* image, int x, int y, int width, int height);
extern void            draw_image_tiled_masked  (image_t* image, color_t mask, int x, int y, int width, int height);
extern void            fill_image               (image_t* image, color_t color);
extern bool            flip_image               (image_t* image, bool is_h_flip, bool is_v_flip);
extern image_lock_t*   lock_image               (image_t* image);
extern bool            replace_image_color      (image_t* image, color_t color, color_t new_color);
extern bool            rescale_image            (image_t* image, int width, int height);
extern void            unlock_image             (image_t* image, image_lock_t* lock);

extern image_t* create_surface (int width, int height);
extern image_t* clone_surface  (const image_t* image);

extern void init_image_api (duk_context* ctx);

extern void     duk_push_sphere_image    (duk_context* ctx, image_t* image);
extern image_t* duk_require_sphere_image (duk_context* ctx, duk_idx_t index);

struct image_lock
{
	color_t*  pixels;
	ptrdiff_t pitch;
};

#endif // MINISPHERE__IMAGE_H__INCLUDED
