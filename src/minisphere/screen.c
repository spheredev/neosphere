/**
 *  miniSphere JavaScript game engine
 *  Copyright (c) 2015-2017, Fat Cerberus
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name of miniSphere nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
**/

// IMPORTANT!
// due to Allegro's architecture, this is far from a perfect abstraction.  once a screen
// object has been constructed, treat it as a singleton: creating multiple screen objects is
// undefined behavior and will probably blow up in your face. :o)

#include "minisphere.h"
#include "screen.h"

#include "async.h"
#include "debugger.h"
#include "image.h"

struct screen
{
	bool             avoid_sleep;
	image_t*         backbuffer;
	rect_t           clip_rect;
	ALLEGRO_DISPLAY* display;
	int              fps_flips;
	int              fps_frames;
	double           fps_poll_time;
	bool             fullscreen;
	double           last_flip_time;
	int              max_skips;
	double           next_frame_time;
	uint32_t         now;
	int              num_flips;
	int              num_frames;
	int              num_skips;
	bool             show_fps;
	bool             skip_frame;
	bool             take_screenshot;
	int              x_offset;
	float            x_scale;
	int              x_size;
	int              y_offset;
	float            y_scale;
	int              y_size;
};

static void refresh_display (screen_t* screen);

screen_t*
screen_new(const char* title, image_t* icon, size2_t resolution, int frameskip, bool avoid_sleep)
{
	image_t*             backbuffer = NULL;
	int                  bitmap_flags;
	ALLEGRO_DISPLAY*     display;
	ALLEGRO_BITMAP*      icon_bitmap;
	ALLEGRO_MONITOR_INFO desktop_info;
	screen_t*            screen;
	int                  x_scale = 1;
	int                  y_scale = 1;

	console_log(1, "initializing render context at %dx%d", resolution.width, resolution.height);

	al_set_new_window_title(title);
	al_set_new_display_flags(ALLEGRO_OPENGL | ALLEGRO_PROGRAMMABLE_PIPELINE);
	if (al_get_monitor_info(0, &desktop_info)) {
		x_scale = ((desktop_info.x2 - desktop_info.x1) / 2) / resolution.width;
		y_scale = ((desktop_info.y2 - desktop_info.y1) / 2) / resolution.height;
		x_scale = y_scale = fmax(fmin(x_scale, y_scale), 1.0);
	}
	display = al_create_display(resolution.width * x_scale, resolution.height * y_scale);

	// custom backbuffer: this allows pixel-perfect rendering regardless
	// of the actual window size.
	if (display != NULL)
		backbuffer = image_new(resolution.width, resolution.height);
	if (backbuffer == NULL) {
		fprintf(stderr, "FATAL: couldn't initialize render context");
		return NULL;
	}

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

	screen = calloc(1, sizeof(screen_t));
	screen->display = display;
	screen->backbuffer = backbuffer;
	screen->x_size = resolution.width;
	screen->y_size = resolution.height;
	screen->max_skips = frameskip;
	screen->avoid_sleep = avoid_sleep;

	screen->fps_poll_time = al_get_time() + 1.0;
	screen->next_frame_time = al_get_time();
	screen->last_flip_time = screen->next_frame_time;

#ifdef MINISPHERE_SPHERUN
	screen->show_fps = true;
#endif

	refresh_display(screen);
	return screen;
}

void
screen_free(screen_t* it)
{
	if (it == NULL)
		return;

	console_log(1, "shutting down render context");
	image_unref(it->backbuffer);
	al_destroy_display(it->display);
	free(it);
}

image_t*
screen_backbuffer(const screen_t* it)
{
	return it->backbuffer;
}

rect_t
screen_bounds(const screen_t* it)
{
	return rect(0, 0, it->x_size, it->y_size);
}

ALLEGRO_DISPLAY*
screen_display(const screen_t* it)
{
	return it->display;
}

size2_t
screen_size(const screen_t* it)
{
	return size2(it->x_size, it->y_size);
}

bool
screen_skip_frame(const screen_t* it)
{
	return it->skip_frame;
}

uint32_t
screen_now(const screen_t* it)
{
	return it->now;
}

int
screen_get_frameskip(const screen_t* it)
{
	return it->max_skips;
}

bool
screen_get_fullscreen(const screen_t* it)
{
	return it->fullscreen;
}

void
screen_get_mouse_xy(const screen_t* it, int* o_x, int* o_y)
{
	ALLEGRO_MOUSE_STATE mouse_state;

	al_get_mouse_state(&mouse_state);
	*o_x = (mouse_state.x - it->x_offset) / it->x_scale;
	*o_y = (mouse_state.y - it->y_offset) / it->y_scale;
}

void
screen_set_frameskip(screen_t* it, int max_skips)
{
	it->max_skips = max_skips;
}

void
screen_set_fullscreen(screen_t* it, bool fullscreen)
{
	if (debugger_attached())
		fullscreen = false;
	it->fullscreen = fullscreen;
	refresh_display(it);
}

void
screen_set_mouse_xy(screen_t* it, int x, int y)
{
	x = x * it->x_scale + it->x_offset;
	y = y * it->y_scale + it->y_offset;
	al_set_mouse_xy(it->display, x, y);
}

void
screen_draw_status(screen_t* it, const char* text, color_t color)
{
	rect_t            bounds;
	ALLEGRO_BITMAP*   old_target;
	int               screen_cx;
	int               screen_cy;
	int               width;
	int               height;

	screen_cx = al_get_display_width(it->display);
	screen_cy = al_get_display_height(it->display);
	width = font_get_width(g_system_font, text) + 20;
	height = font_height(g_system_font) + 10;
	bounds.x1 = 8 + it->x_offset;
	bounds.y1 = screen_cy - it->y_offset - height - 8;
	bounds.x2 = bounds.x1 + width;
	bounds.y2 = bounds.y1 + height;
	old_target = al_get_target_bitmap();
	al_set_target_backbuffer(it->display);
	al_draw_filled_rounded_rectangle(bounds.x1, bounds.y1, bounds.x2, bounds.y2, 4, 4,
		al_map_rgba(16, 16, 16, 192));
	font_draw_text(g_system_font, color_new(0, 0, 0, 255), (bounds.x1 + bounds.x2) / 2 + 1,
		bounds.y1 + 6, TEXT_ALIGN_CENTER, text);
	font_draw_text(g_system_font, color, (bounds.x2 + bounds.x1) / 2,
		bounds.y1 + 5, TEXT_ALIGN_CENTER, text);
	al_set_target_bitmap(old_target);
}

void
screen_flip(screen_t* it, int framerate, bool need_clear)
{
	time_t            datetime;
	char*             filename;
	char              fps_text[20];
	const char*       game_filename;
	const path_t*     game_root;
	bool              is_backbuffer_valid;
	ALLEGRO_STATE     old_state;
	ALLEGRO_BITMAP*   old_target;
	path_t*           path;
	const char*       pathstr;
	rect_t            scissor;
	int               screen_cx;
	int               screen_cy;
	int               serial = 1;
	ALLEGRO_BITMAP*   snapshot;
	double            time_left;
	char              timestamp[100];
	int               x, y;

	size_t i;

	async_run_jobs(ASYNC_RENDER);

	// update FPS with 1s granularity
	if (al_get_time() >= it->fps_poll_time) {
		it->fps_flips = it->num_flips;
		it->fps_frames = it->num_frames;
		it->num_frames = it->num_flips = 0;
		it->fps_poll_time = al_get_time() + 1.0;
	}

	// flip the backbuffer, unless the preceeding frame was skipped
	is_backbuffer_valid = !it->skip_frame;
	screen_cx = al_get_display_width(it->display);
	screen_cy = al_get_display_height(it->display);
	if (is_backbuffer_valid) {
		if (it->take_screenshot) {
			al_store_state(&old_state, ALLEGRO_STATE_NEW_BITMAP_PARAMETERS);
			al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ANY_24_NO_ALPHA);
			snapshot = al_clone_bitmap(image_bitmap(it->backbuffer));
			al_restore_state(&old_state);
			game_root = game_path(g_game);
			game_filename = path_is_file(game_root)
				? path_filename(game_root)
				: path_hop(game_root, path_num_hops(game_root) - 1);
			path = path_rebase(path_new("miniSphere/Screenshots/"), home_path());
			path_mkdir(path);
			time(&datetime);
			strftime(timestamp, 100, "%Y%m%d", localtime(&datetime));
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
			it->take_screenshot = false;
		}
		old_target = al_get_target_bitmap();
		al_set_target_backbuffer(it->display);
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
		al_draw_scaled_bitmap(image_bitmap(it->backbuffer), 0, 0, it->x_size, it->y_size,
			it->x_offset, it->y_offset, it->x_size * it->x_scale, it->y_size * it->y_scale,
			0x0);
		if (debugger_attached())
			screen_draw_status(it, debugger_name(), debugger_color());
		if (it->show_fps) {
			if (framerate > 0)
				sprintf(fps_text, "%d/%d fps", it->fps_flips, it->fps_frames);
			else
				sprintf(fps_text, "%d fps", it->fps_flips);
			x = screen_cx - it->x_offset - 108;
			y = screen_cy - it->y_offset - 24;
			al_draw_filled_rounded_rectangle(x, y, x + 100, y + 16, 4, 4, al_map_rgba(16, 16, 16, 192));
			font_draw_text(g_system_font, color_new(0, 0, 0, 255), x + 51, y + 3, TEXT_ALIGN_CENTER, fps_text);
			font_draw_text(g_system_font, color_new(255, 255, 255, 255), x + 50, y + 2, TEXT_ALIGN_CENTER, fps_text);
		}
		al_set_target_bitmap(old_target);
		al_flip_display();
		it->last_flip_time = al_get_time();
		it->num_skips = 0;
		++it->num_flips;
	}
	else {
		++it->num_skips;
	}

	// if framerate is nonzero and we're backed up on frames, skip frames until we
	// catch up. there is a cap on consecutive frameskips to avoid the situation where
	// the engine "can't catch up" (due to a slow machine, overloaded CPU, etc.). better
	// that we lag instead of never rendering anything at all.
	if (framerate > 0) {
		it->skip_frame = it->num_skips < it->max_skips && it->last_flip_time > it->next_frame_time;
		do {  // kill time while we wait for the next frame
			time_left = it->next_frame_time - al_get_time();
			if (!it->avoid_sleep && time_left > 0.001)  // engine may stall with < 1ms timeout
				al_wait_for_event_timed(g_events, NULL, time_left);
			sphere_run(false);
		} while (al_get_time() < it->next_frame_time);
		sphere_run(true);
		if (it->num_skips >= it->max_skips)  // did we skip too many frames?
			it->next_frame_time = al_get_time() + 1.0 / framerate;
		else
			it->next_frame_time += 1.0 / framerate;
	}
	else {
		it->skip_frame = false;
		sphere_run(true);
		it->next_frame_time = al_get_time();
	}
	++it->num_frames;
	if (!it->skip_frame && need_clear) {
		// disable clipping so we can clear the whole backbuffer.
		scissor = image_get_scissor(it->backbuffer);
		image_set_scissor(it->backbuffer, rect(0, 0, it->x_size, it->y_size));
		image_render_to(it->backbuffer, NULL);
		al_clear_to_color(al_map_rgba(0, 0, 0, 255));
		image_set_scissor(it->backbuffer, scissor);
	}

	async_run_jobs(ASYNC_UPDATE);
	async_run_jobs(ASYNC_TICK);

	++it->now;
}

image_t*
screen_grab(screen_t* it, int x, int y, int width, int height)
{
	image_t* image;

	if (!(image = image_new(width, height)))
		goto on_error;
	image_render_to(image, NULL);
	al_draw_bitmap_region(image_bitmap(it->backbuffer), x, y, width, height, 0, 0, 0x0);
	return image;

on_error:
	image_unref(image);
	return NULL;
}

void
screen_queue_screenshot(screen_t* it)
{
	it->take_screenshot = true;
}

void
screen_resize(screen_t* it, int x_size, int y_size)
{
	it->x_size = x_size;
	it->y_size = y_size;

	refresh_display(it);
}

void
screen_show_mouse(screen_t* it, bool visible)
{
	if (visible)
		al_show_mouse_cursor(it->display);
	else
		al_hide_mouse_cursor(it->display);
}

void
screen_toggle_fps(screen_t* it)
{
	it->show_fps = !it->show_fps;
}

void
screen_toggle_fullscreen(screen_t* it)
{
	screen_set_fullscreen(it, !it->fullscreen);
}

void
screen_unskip_frame(screen_t* it)
{
	it->skip_frame = false;
	al_clear_to_color(al_map_rgba(0, 0, 0, 255));
}

static void
refresh_display(screen_t* screen)
{
	ALLEGRO_MONITOR_INFO desktop_info;
	int                  real_width;
	int                  real_height;

	al_set_display_flag(screen->display, ALLEGRO_FULLSCREEN_WINDOW, screen->fullscreen);
	if (screen->fullscreen) {
		real_width = al_get_display_width(screen->display);
		real_height = al_get_display_height(screen->display);
		screen->x_scale = (float)real_width / screen->x_size;
		screen->y_scale = (float)real_height / screen->y_size;
		if (screen->x_scale > screen->y_scale) {
			screen->x_scale = screen->y_scale;
			screen->x_offset = (real_width - screen->x_size * screen->x_scale) / 2;
			screen->y_offset = 0;
		}
		else {
			screen->y_scale = screen->x_scale;
			screen->y_offset = (real_height - screen->y_size * screen->y_scale) / 2;
			screen->x_offset = 0;
		}
	}
	else {
		screen->x_scale = 1.0;
		screen->y_scale = 1.0;
		screen->x_offset = 0;
		screen->y_offset = 0;
		if (al_get_monitor_info(0, &desktop_info)) {
			screen->x_scale = trunc(((desktop_info.x2 - desktop_info.x1) / 2) / screen->x_size);
			screen->y_scale = trunc(((desktop_info.y2 - desktop_info.y1) / 2) / screen->y_size);
			screen->x_scale = screen->y_scale = fmax(fmin(screen->x_scale, screen->y_scale), 1.0);
		}

		// size and recenter the window
		al_resize_display(screen->display, screen->x_size * screen->x_scale, screen->y_size * screen->y_scale);
		al_set_window_position(screen->display,
			(desktop_info.x1 + desktop_info.x2) / 2 - screen->x_size * screen->x_scale / 2,
			(desktop_info.y1 + desktop_info.y2) / 2 - screen->y_size * screen->y_scale / 2);
	}

	image_render_to(screen->backbuffer, NULL);
}
