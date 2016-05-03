#ifndef MINISPHERE__IMAGE_H__INCLUDED
#define MINISPHERE__IMAGE_H__INCLUDED

typedef struct image image_t;

typedef
struct image_lock
{
	color_t*  pixels;
	ptrdiff_t pitch;
	int       num_lines;
} image_lock_t;

image_t*        create_image             (int width, int height);
image_t*        create_subimage          (image_t* parent, int x, int y, int width, int height);
image_t*        clone_image              (const image_t* image);
image_t*        load_image               (const char* filename);
image_t*        read_image               (sfs_file_t* file, int width, int height);
image_t*        read_subimage            (sfs_file_t* file, image_t* parent, int x, int y, int width, int height);
image_t*        ref_image                (image_t* image);
void            free_image               (image_t* image);
ALLEGRO_BITMAP* get_image_bitmap         (image_t* image);
int             get_image_height         (const image_t* image);
color_t         get_image_pixel          (image_t* image, int x, int y);
int             get_image_width          (const image_t* image);
void            set_image_pixel          (image_t* image, int x, int y, color_t color);
bool            apply_color_matrix       (image_t* image, colormatrix_t matrix, int x, int y, int width, int height);
bool            apply_color_matrix_4     (image_t* image, colormatrix_t ul_mat, colormatrix_t ur_mat, colormatrix_t ll_mat, colormatrix_t lr_mat, int x, int y, int width, int height);
bool            apply_image_lookup       (image_t* image, int x, int y, int width, int height, uint8_t red_lu[256], uint8_t green_lu[256], uint8_t blue_lu[256], uint8_t alpha_lu[256]);
void            blit_image               (image_t* image, image_t* target_image, int x, int y);
void            draw_image               (image_t* image, int x, int y);
void            draw_image_masked        (image_t* image, color_t mask, int x, int y);
void            draw_image_scaled        (image_t* image, int x, int y, int width, int height);
void            draw_image_scaled_masked (image_t* image, color_t mask, int x, int y, int width, int height);
void            draw_image_tiled         (image_t* image, int x, int y, int width, int height);
void            draw_image_tiled_masked  (image_t* image, color_t mask, int x, int y, int width, int height);
void            fill_image               (image_t* image, color_t color);
bool            flip_image               (image_t* image, bool is_h_flip, bool is_v_flip);
image_lock_t*   lock_image               (image_t* image);
bool            replace_image_color      (image_t* image, color_t color, color_t new_color);
bool            rescale_image            (image_t* image, int width, int height);
bool            save_image               (image_t* image, const char* filename);
void            unlock_image             (image_t* image, image_lock_t* lock);

void init_image_api (duk_context* ctx);

void     duk_push_sphere_image    (duk_context* ctx, image_t* image);
image_t* duk_require_sphere_image (duk_context* ctx, duk_idx_t index);

#endif // MINISPHERE__IMAGE_H__INCLUDED
