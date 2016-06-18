// note: due to Allegro's architecture, this is far from a perfect abstraction. avoid the
//       temptation to create multiple screen objects for any reason as this will not work properly
//       and will almost certainly blow up in your face. :o)

#include "minisphere.h"
#include "screen.h"

#include "debugger.h"
#include "image.h"
#include "matrix.h"

struct screen
{
	bool             avoid_sleep;
	rect_t           clip_rect;
	ALLEGRO_DISPLAY* display;
	int              fps_flips;
	int              fps_frames;
	double           fps_poll_time;
	bool             fullscreen;
	bool             have_shaders;
	double           last_flip_time;
	int              max_skips;
	double           next_frame_time;
	int              num_flips;
	int              num_frames;
	int              num_skips;
	bool             show_fps;
	bool             skip_frame;
	bool             take_screenshot;
	bool             use_shaders;
	int              x_offset;
	float            x_scale;
	int              x_size;
	int              y_offset;
	float            y_scale;
	int              y_size;
};

static void refresh_display (screen_t* obj);

screen_t*
screen_new(const char* title, image_t* icon, int x_size, int y_size, int frameskip, bool avoid_sleep)
{
	ALLEGRO_DISPLAY* display;
	ALLEGRO_BITMAP*  icon_bitmap;
	screen_t*        obj;
	int              bitmap_flags;
	bool             use_shaders = false;
	int              x_scale;
	int              y_scale;

	console_log(1, "initializing render context at %dx%d", x_size, y_size);

	x_scale = x_size <= 400 && y_size <= 300 ? 2.0 : 1.0;
	y_scale = x_scale;
#ifdef MINISPHERE_USE_SHADERS
	al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_PROGRAMMABLE_PIPELINE);
	if (display = al_create_display(x_size * x_scale, y_size * y_scale))
		use_shaders = true;
	else {
		al_set_new_display_flags(ALLEGRO_OPENGL);
		display = al_create_display(x_size * x_scale, y_size * y_scale);
	}
#else
	al_set_new_display_flags(ALLEGRO_OPENGL);
	display = al_create_display(x_size * x_scale, y_size * y_scale);
#endif

	if (display == NULL) {
		fprintf(stderr, "FATAL: unable to initialize render context!");
		return NULL;
	}
	
	console_log(1, "    shader support: %s", use_shaders ? "yes" : "no");

	al_set_window_title(display, title);
	if (icon != NULL) {
		bitmap_flags = al_get_new_bitmap_flags();
		al_set_new_bitmap_flags(
			ALLEGRO_NO_PREMULTIPLIED_ALPHA | ALLEGRO_MIN_LINEAR | ALLEGRO_MAG_LINEAR
			| bitmap_flags);
		icon = image_clone(icon);
		image_rescale(icon, 32, 32);
		icon_bitmap = image_bitmap(icon);
		al_set_new_bitmap_flags(bitmap_flags);
		al_set_display_icon(display, icon_bitmap);
	}

	obj = calloc(1, sizeof(screen_t));
	obj->display = display;
	obj->x_size = x_size;
	obj->y_size = y_size;
	obj->max_skips = frameskip;
	obj->avoid_sleep = avoid_sleep;
	obj->have_shaders = use_shaders;

	obj->fps_poll_time = al_get_time() + 1.0;
	obj->next_frame_time = al_get_time();
	obj->last_flip_time = obj->next_frame_time;

#ifdef MINISPHERE_SPHERUN
	obj->show_fps = true;
#endif

	screen_set_clipping(obj, new_rect(0, 0, x_size, y_size));
	refresh_display(obj);
	return obj;
}

void
screen_free(screen_t* obj)
{
	if (obj == NULL)
		return;
	
	console_log(1, "shutting down render context");
	al_destroy_display(obj->display);
	free(obj);
}

ALLEGRO_DISPLAY*
screen_display(const screen_t* obj)
{
	return obj->display;
}

bool
screen_have_shaders(const screen_t* screen)
{
	return screen->have_shaders;
}

bool
screen_is_skipframe(const screen_t* obj)
{
	return obj->skip_frame;
}

rect_t
screen_get_clipping(screen_t* obj)
{
	return obj->clip_rect;
}

int
screen_get_frameskip(const screen_t* obj)
{
	return obj->max_skips;
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
screen_set_clipping(screen_t* obj, rect_t clip_rect)
{
	obj->clip_rect = clip_rect;
	clip_rect.x1 = clip_rect.x1 * obj->x_scale + obj->x_offset;
	clip_rect.y1 = clip_rect.y1 * obj->y_scale + obj->y_offset;
	clip_rect.x2 = clip_rect.x2 * obj->x_scale + obj->x_offset;
	clip_rect.y2 = clip_rect.y2 * obj->y_scale + obj->y_offset;
	al_set_clipping_rectangle(clip_rect.x1, clip_rect.y1,
		clip_rect.x2 - clip_rect.x1, clip_rect.y2 - clip_rect.y1);
}

void
screen_set_frameskip(screen_t* obj, int max_skips)
{
	obj->max_skips = max_skips;
}

void
screen_set_mouse_xy(screen_t* obj, int x, int y)
{
	x = x * obj->x_scale + obj->x_offset;
	y = y * obj->y_scale + obj->y_offset;
	al_set_mouse_xy(obj->display, x, y);
}

void
screen_draw_status(screen_t* obj, const char* text)
{
	rect_t            bounds;
	int               screen_cx;
	int               screen_cy;
	ALLEGRO_TRANSFORM trans;
	int               width;
	int               height;

	screen_cx = al_get_display_width(obj->display);
	screen_cy = al_get_display_height(obj->display);
	width = font_get_width(g_sys_font, text) + 20;
	height = font_height(g_sys_font) + 10;
	bounds.x1 = 8 + obj->x_offset;
	bounds.y1 = screen_cy - obj->y_offset - height - 8;
	bounds.x2 = bounds.x1 + width;
	bounds.y2 = bounds.y1 + height;
	al_identity_transform(&trans);
	al_use_transform(&trans);
	al_draw_filled_rounded_rectangle(bounds.x1, bounds.y1, bounds.x2, bounds.y2, 4, 4,
		al_map_rgba(16, 16, 16, 192));
	font_draw_text(g_sys_font, color_new(0, 0, 0, 255), (bounds.x1 + bounds.x2) / 2 + 1,
		bounds.y1 + 6, TEXT_ALIGN_CENTER, text);
	font_draw_text(g_sys_font, color_new(255, 255, 255, 255), (bounds.x2 + bounds.x1) / 2,
		bounds.y1 + 5, TEXT_ALIGN_CENTER, text);
	screen_transform(obj, NULL);
}

void
screen_flip(screen_t* obj, int framerate)
{
	char*             filename;
	char              fps_text[20];
	const char*       game_filename;
	const path_t*     game_path;
	bool              is_backbuffer_valid;
	time_t            now;
	ALLEGRO_STATE     old_state;
	path_t*           path;
	const char*       pathstr;
	int               screen_cx;
	int               screen_cy;
	int               serial = 1;
	ALLEGRO_BITMAP*   snapshot;
	double            time_left;
	char              timestamp[100];
	ALLEGRO_TRANSFORM trans;
	int               x, y;

	size_t i;

	// update FPS with 1s granularity
	if (al_get_time() >= obj->fps_poll_time) {
		obj->fps_flips = obj->num_flips;
		obj->fps_frames = obj->num_frames;
		obj->num_frames = obj->num_flips = 0;
		obj->fps_poll_time = al_get_time() + 1.0;
	}

	// flip the backbuffer, unless the preceeding frame was skipped
	is_backbuffer_valid = !obj->skip_frame;
	screen_cx = al_get_display_width(obj->display);
	screen_cy = al_get_display_height(obj->display);
	if (is_backbuffer_valid) {
		if (obj->take_screenshot) {
			al_store_state(&old_state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
			al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA);
			snapshot = al_clone_bitmap(al_get_backbuffer(obj->display));
			al_restore_state(&old_state);
			game_path = fs_path(g_fs);
			game_filename = path_is_file(game_path)
				? path_filename_cstr(game_path)
				: path_hop_cstr(game_path, path_num_hops(game_path) - 1);
			path = path_rebase(path_new("minisphere/screens/"), homepath());
			path_mkdir(path);
			time(&now);
			strftime(timestamp, 100, "%Y%m%d", localtime(&now));
			do {
				filename = strnewf("%s-%s-%d.png", game_filename, timestamp, serial++);
				for (i = 0; filename[i] != '\0'; ++i)
					filename[i];
				path_strip(path);
				path_append(path, filename);
				pathstr = path_cstr(path);
				free(filename);
			} while (al_filename_exists(pathstr));
			al_save_bitmap(pathstr, snapshot);
			al_destroy_bitmap(snapshot);
			path_free(path);
			obj->take_screenshot = false;
		}
		if (is_debugger_attached())
			screen_draw_status(obj, "SSJ");
		if (obj->show_fps) {
			if (framerate > 0)
				sprintf(fps_text, "%d/%d fps", obj->fps_flips, obj->fps_frames);
			else
				sprintf(fps_text, "%d fps", obj->fps_flips);
			x = screen_cx - obj->x_offset - 108;
			y = screen_cy - obj->y_offset - 24;
			al_identity_transform(&trans);
			al_use_transform(&trans);
			al_draw_filled_rounded_rectangle(x, y, x + 100, y + 16, 4, 4, al_map_rgba(16, 16, 16, 192));
			font_draw_text(g_sys_font, color_new(0, 0, 0, 255), x + 51, y + 3, TEXT_ALIGN_CENTER, fps_text);
			font_draw_text(g_sys_font, color_new(255, 255, 255, 255), x + 50, y + 2, TEXT_ALIGN_CENTER, fps_text);
			screen_transform(g_screen, NULL);
		}
		al_flip_display();
		obj->last_flip_time = al_get_time();
		obj->num_skips = 0;
		++obj->num_flips;
	}
	else {
		++obj->num_skips;
	}

	// if framerate is nonzero and we're backed up on frames, skip frames until we
	// catch up. there is a cap on consecutive frameskips to avoid the situation where
	// the engine "can't catch up" (due to a slow machine, overloaded CPU, etc.). better
	// that we lag instead of never rendering anything at all.
	if (framerate > 0) {
		obj->skip_frame = obj->num_skips < obj->max_skips && obj->last_flip_time > obj->next_frame_time;
		do {  // kill time while we wait for the next frame
			time_left = obj->next_frame_time - al_get_time();
			if (!obj->avoid_sleep && time_left > 0.001)  // engine may stall with < 1ms timeout
				al_wait_for_event_timed(g_events, NULL, time_left);
			do_events();
		} while (al_get_time() < obj->next_frame_time);
		if (obj->num_skips >= obj->max_skips)  // did we skip too many frames?
			obj->next_frame_time = al_get_time() + 1.0 / framerate;
		else
			obj->next_frame_time += 1.0 / framerate;
	}
	else {
		obj->skip_frame = false;
		do_events();
		obj->next_frame_time = al_get_time();
		obj->next_frame_time = al_get_time();
	}
	++obj->num_frames;
	if (!obj->skip_frame) {
		// disable clipping momentarily so we can clear the letterbox area.
		// this prevents artifacts which manifest with some graphics drivers.
		al_set_clipping_rectangle(0, 0, screen_cx, screen_cy);
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
		screen_set_clipping(obj, obj->clip_rect);
	}
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
	
	if (!(image = image_new(scale_width, scale_height)))
		goto on_error;
	backbuffer = al_get_backbuffer(obj->display);
	al_set_target_bitmap(image_bitmap(image));
	al_draw_bitmap_region(backbuffer, x, y, scale_width, scale_height, 0, 0, 0x0);
	al_set_target_backbuffer(obj->display);
	if (!image_rescale(image, width, height))
		goto on_error;
	return image;

on_error:
	image_free(image);
	return NULL;
}

void
screen_queue_screenshot(screen_t* obj)
{
	obj->take_screenshot = true;
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
screen_toggle_fps(screen_t* obj)
{
	obj->show_fps = !obj->show_fps;
}

void
screen_toggle_fullscreen(screen_t* obj)
{
	obj->fullscreen = !obj->fullscreen;
	refresh_display(obj);
}

void
screen_transform(screen_t* obj, const matrix_t* matrix)
{
	ALLEGRO_TRANSFORM transform;
	
	al_identity_transform(&transform);
	if (matrix != NULL)
		al_compose_transform(&transform, matrix_transform(matrix));
	if (al_get_target_bitmap() == al_get_backbuffer(obj->display)) {
		al_scale_transform(&transform, obj->x_scale, obj->y_scale);
		al_translate_transform(&transform, obj->x_offset, obj->y_offset);
	}
	al_use_transform(&transform);
}

void
screen_unskip_frame(screen_t* obj)
{
	obj->skip_frame = false;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
}

static void
refresh_display(screen_t* obj)
{
	ALLEGRO_MONITOR_INFO monitor;
	int                  real_width;
	int                  real_height;

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
	
	screen_transform(obj, NULL);
	screen_set_clipping(obj, obj->clip_rect);
}
