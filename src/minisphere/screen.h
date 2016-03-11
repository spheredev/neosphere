#ifndef MINISPHERE__DISPLAY_H__INCLUDED
#define MINISPHERE__DISPLAY_H__INCLUDED

#include "image.h"

typedef struct screen screen_t;

screen_t*        screen_new               (int x_size, int y_size, int frameskip, bool avoid_sleep);
void             screen_free              (screen_t* obj);
ALLEGRO_DISPLAY* screen_display           (const screen_t* obj);
bool             screen_is_skipframe      (const screen_t* obj);
rect_t           screen_get_clip          (screen_t* obj);
int              screen_get_frameskip     (const screen_t* obj);
void             screen_get_mouse_xy      (const screen_t* obj, int* o_x, int* o_y);
void             screen_set_clip          (screen_t* obj, rect_t clip_rect);
void             screen_set_frameskip     (screen_t* obj, int max_skips);
void             screen_set_mouse_xy      (screen_t* obj, int x, int y);
void             screen_capture_now       (screen_t* obj);
void             screen_draw_status       (screen_t* obj, const char* text);
void             screen_flip              (screen_t* obj, int framerate);
image_t*         screen_grab              (screen_t* obj, int x, int y, int width, int height);
void             screen_resize            (screen_t* obj, int x_size, int y_size);
void             screen_show_mouse        (screen_t* obj, bool visible);
void             screen_toggle_fullscreen (screen_t* obj);
void             screen_toggle_fps        (screen_t* obj);
void             screen_transform         (const screen_t* obj, ALLEGRO_TRANSFORM* p_trans);
void             screen_unskip_frame      (screen_t* obj);

#endif // MINISPHERE__DISPLAY_H__INCLUDED
