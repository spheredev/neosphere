#ifndef MINISPHERE__DISPLAY_H__INCLUDED
#define MINISPHERE__DISPLAY_H__INCLUDED

#include "image.h"
#include "matrix.h"

typedef struct screen screen_t;

screen_t*        screen_new               (const char* title, image_t* icon, int x_size, int y_size, int frameskip, bool avoid_sleep);
void             screen_free              (screen_t* screen);
ALLEGRO_BITMAP*  screen_backbuffer        (const screen_t* screen);
ALLEGRO_DISPLAY* screen_display           (const screen_t* screen);
bool             screen_fullscreen        (const screen_t* screen);
bool             screen_have_shaders      (const screen_t* screen);
bool             screen_is_skipframe      (const screen_t* screen);
void             screen_set_fullscreen    (screen_t* screen, bool fullscreen);
uint32_t         screen_now               (const screen_t* screen);
rect_t           screen_get_clipping      (screen_t* screen);
int              screen_get_frameskip     (const screen_t* screen);
void             screen_get_mouse_xy      (const screen_t* screen, int* o_x, int* o_y);
matrix_t*        screen_get_transform     (const screen_t* screen);
void             screen_set_clipping      (screen_t* screen, rect_t clip_rect);
void             screen_set_frameskip     (screen_t* screen, int max_skips);
void             screen_set_mouse_xy      (screen_t* screen, int x, int y);
void             screen_set_transform     (screen_t* screen, matrix_t* matrix);
void             screen_draw_status       (screen_t* screen, const char* text, color_t color);
void             screen_flip              (screen_t* screen, int framerate);
image_t*         screen_grab              (screen_t* screen, int x, int y, int width, int height);
void             screen_queue_screenshot  (screen_t* screen);
void             screen_render_to         (screen_t* screen, matrix_t* transform);
void             screen_resize            (screen_t* screen, int x_size, int y_size);
void             screen_show_mouse        (screen_t* screen, bool visible);
void             screen_toggle_fps        (screen_t* screen);
void             screen_toggle_fullscreen (screen_t* screen);
void             screen_unskip_frame      (screen_t* screen);

#endif // MINISPHERE__DISPLAY_H__INCLUDED
