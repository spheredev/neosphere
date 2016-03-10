#include "minisphere.h"
#include "screen.h"

#include "image.h"

struct screen
{
	ALLEGRO_DISPLAY* display;
	rect_t           clip_rect;
	bool             fullscreen;
	bool             use_shaders;
	float            x_offset;
	float            x_scale;
	int              x_size;
	float            y_offset;
	float            y_scale;
	int              y_size;
};

static void refresh_display (screen_t* obj);

screen_t*
screen_new(int x_size, int y_size, bool fullscreen)
{
	ALLEGRO_DISPLAY* display;
	screen_t*        obj;
	bool             use_shaders = false;
	int              x_scale;
	int              y_scale;

	console_log(1, "creating render context at %dx%d", x_size, y_size);

#ifdef MINISPHERE_USE_SHADERS
	al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_PROGRAMMABLE_PIPELINE);
	x_scale = x_size <= 400 && y_size <= 300 ? 2.0 : 1.0;
	y_scale = x_scale;
	if (display = al_create_display(320 * x_scale, 240 * y_scale))
		use_shaders = true;
	else {
		al_set_new_display_flags(ALLEGRO_OPENGL);
		display = al_create_display(320 * x_scale, 240 * y_scale);
	}
#else
	al_set_new_display_flags(ALLEGRO_OPENGL);
	display = al_create_display(320, 240);
#endif

	obj = calloc(1, sizeof(screen_t));
	obj->display = display;
	obj->x_size = x_size;
	obj->y_size = y_size;
	obj->fullscreen = fullscreen;
	
	screen_set_clip(obj, new_rect(0, 0, x_size, y_size));
	refresh_display(obj);
	return obj;
}

void
screen_free(screen_t* obj)
{
	if (obj == NULL)
		return;
	
	console_log(1, "freeing render context");
	al_destroy_display(obj->display);
	free(obj);
}

ALLEGRO_DISPLAY*
screen_display(const screen_t* obj)
{
	return obj->display;
}

rect_t
screen_get_clip(screen_t* obj)
{
	return obj->clip_rect;
}

void
screen_get_mouse_xy(const screen_t* obj, int* o_x, int* o_y)
{
	ALLEGRO_MOUSE_STATE mouse_state;

	al_get_mouse_state(&mouse_state);
	*o_x = (mouse_state.x - obj->x_offset) / obj->x_scale;
	*o_y = (mouse_state.y - obj->y_offset) / obj->y_scale;
}

void
screen_set_mouse_xy(screen_t* obj, int x, int y)
{
	x = x * obj->x_scale + obj->x_offset;
	y = y * obj->y_scale + obj->y_offset;
	al_set_mouse_xy(obj->display, x, y);
}

void
screen_set_clip(screen_t* obj, rect_t clip_rect)
{
	obj->clip_rect = clip_rect;
	clip_rect.x1 = clip_rect.x1 * obj->x_scale + obj->x_offset;
	clip_rect.y1 = clip_rect.y1 * obj->y_scale + obj->y_offset;
	clip_rect.x2 = clip_rect.x2 * obj->x_scale + obj->x_offset;
	clip_rect.y2 = clip_rect.y2 * obj->y_scale + obj->y_offset;
	al_set_clipping_rectangle(clip_rect.x1, clip_rect.y1,
		clip_rect.x2 - clip_rect.x1, clip_rect.y2 - clip_rect.y1);
}

image_t*
screen_grab(screen_t* obj, int x, int y, int width, int height)
{
	ALLEGRO_BITMAP* backbuffer;
	image_t*        image;
	int             scale_width;
	int             scale_height;
	
	x = x * obj->x_scale + obj->x_offset;
	y = y * obj->y_scale + obj->y_offset;
	scale_width = width * obj->x_scale;
	scale_height = height * obj->y_scale;
	
	if (!(image = create_image(scale_width, scale_height)))
		goto on_error;
	backbuffer = al_get_backbuffer(obj->display);
	al_set_target_bitmap(get_image_bitmap(image));
	al_draw_bitmap_region(backbuffer, x, y, scale_width, scale_height, 0, 0, 0x0);
	al_set_target_backbuffer(obj->display);
	if (!rescale_image(image, width, height))
		goto on_error;
	return image;

on_error:
	free_image(image);
	return NULL;
}

void
screen_resize(screen_t* obj, int x_size, int y_size)
{
	obj->x_size = x_size;
	obj->y_size = y_size;
	
	refresh_display(obj);
}

void
screen_show_mouse(screen_t* obj, bool visible)
{
	if (visible)
		al_show_mouse_cursor(obj->display);
	else
		al_hide_mouse_cursor(obj->display);
}

void
screen_status(screen_t* obj, const char* text)
{
	int               screen_cx;
	int               screen_cy;
	ALLEGRO_TRANSFORM trans;
	int               width;
	int               height;

	screen_cx = al_get_display_width(obj->display);
	screen_cy = al_get_display_height(obj->display);
	width = get_text_width(g_sys_font, text) + 20;
	height = get_font_line_height(g_sys_font) + 10;
	al_identity_transform(&trans);
	al_use_transform(&trans);
	al_draw_filled_rounded_rectangle(
		screen_cx - 16 - width, screen_cy - 16 - height, screen_cx - 16, screen_cy - 16,
		4, 4, al_map_rgba(16, 16, 16, 255));
	draw_text(g_sys_font, rgba(0, 0, 0, 255), screen_cx - 16 - width / 2 + 1, screen_cy - 16 - height + 6, TEXT_ALIGN_CENTER, text);
	draw_text(g_sys_font, rgba(255, 255, 255, 255), screen_cx - 16 - width / 2, screen_cy - 16 - height + 5, TEXT_ALIGN_CENTER, text);
	screen_transform(obj, &trans);
	al_use_transform(&trans);
}

void
screen_toggle(screen_t* obj)
{
	obj->fullscreen = !obj->fullscreen;
	refresh_display(obj);
}

void
screen_transform(const screen_t* obj, ALLEGRO_TRANSFORM* p_trans)
{
	al_scale_transform(p_trans, obj->x_scale, obj->y_scale);
	al_translate_transform(p_trans, obj->x_offset, obj->y_offset);
}

static void
refresh_display(screen_t* obj)
{
	ALLEGRO_MONITOR_INFO monitor;
	int                  real_width;
	int                  real_height;
	ALLEGRO_TRANSFORM    trans;

	al_set_display_flag(obj->display, ALLEGRO_FULLSCREEN_WINDOW, obj->fullscreen);
	if (obj->fullscreen) {
		real_width = al_get_display_width(obj->display);
		real_height = al_get_display_height(obj->display);
		obj->x_scale = (float)real_width / obj->x_size;
		obj->y_scale = (float)real_height / obj->y_size;
		if (obj->x_scale > obj->y_scale) {
			obj->x_scale = obj->y_scale;
			obj->x_offset = (real_width - obj->x_size * obj->x_scale) / 2;
			obj->y_offset = 0.0;
		}
		else {
			obj->y_scale = obj->x_scale;
			obj->y_offset = (real_height - obj->y_size * obj->y_scale) / 2;
			obj->x_offset = 0.0;
		}
	}
	else {
		obj->x_scale = obj->x_size <= 400 && obj->y_size <= 300
			? 2.0 : 1.0;
		obj->y_scale = obj->x_scale;
		obj->x_offset = obj->y_offset = 0.0;
		
		// size and recenter the window
		al_resize_display(obj->display, obj->x_size * obj->x_scale, obj->y_size * obj->y_scale);
		al_get_monitor_info(0, &monitor);
		al_set_window_position(obj->display,
			(monitor.x1 + monitor.x2) / 2 - obj->x_size * obj->x_scale / 2,
			(monitor.y1 + monitor.y2) / 2 - obj->y_size * obj->y_scale / 2);
	}
	
	al_identity_transform(&trans);
	screen_transform(obj, &trans);
	al_use_transform(&trans);
	
	screen_set_clip(obj, obj->clip_rect);
}
