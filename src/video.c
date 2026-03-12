/*
 * Copyright (c) 2026 Stunts Engine Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * video.c - SDL2 video mode and palette functions
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "framebuffer.h"
#include "game_timing.h"
#include "timer.h"
#include "data_game.h"
#include "ui.h"

/* Variables moved from data_game.c (private to this translation unit) */
static unsigned char input_queue_buffer = 0;


/* Function pointer type for exit handlers */
typedef void (* voidfunctype)(void);

/* CRITICAL: All SDL state MUST come before sdl_framebuffer to prevent overflow corruption */
static SDL2Context sdl_ctx;
static Framebuffer sdl_fb;
static void* sdl_ctx_saved_window = NULL;
static void* sdl_ctx_saved_renderer = NULL;
static void* sdl_ctx_saved_texture = NULL;
static unsigned char sdl_active = 0;
static unsigned char sdl_init_attempted = 0;
/* Framebuffer LAST - overflows will go past end of .bss, causing immediate crash instead of silent corruption */
static unsigned char sdl_framebuffer[FB_PIXELS];

/* Forward declarations */
void video_add_exithandler(void);
void video_on_exit(void);

unsigned char* video_get_framebuffer(void);
void video_present_frame(void);
void video_refresh(void);
int video_is_sdl_active(void);
void video_wait_menu_refresh(void);

/** @brief Video wait refresh slot.
 * @param next_counter Parameter `next_counter`.
 * @param accum Parameter `accum`.
 * @return Function result.
 */
static void video_wait_refresh_slot(unsigned long *next_counter, unsigned long *accum)
{
	timer_wait_for_counter_rate(timer_get_display_hz(), next_counter, accum);
}

static int sdl_scale = 3;
static int sdl_fullscreen = 0;
static unsigned char video_exit_handler_registered = 0;

/** @brief Increase the SDL render scale by one step.
 */
void video_scale_up(void) {
	if (sdl_fullscreen) return;
	if (sdl_scale >= 3) return;
	sdl_scale++;
	fb_sdl2_set_scale(&sdl_ctx, sdl_scale);
}

/** @brief Decrease the SDL render scale by one step.
 */
void video_scale_down(void) {

	if (sdl_fullscreen) return;
	if (sdl_scale <= 1) return;
	sdl_scale--;
	fb_sdl2_set_scale(&sdl_ctx, sdl_scale);
}

/** @brief Toggle SDL fullscreen mode and reinitialize the video surface.
 */
void video_toggle_fullscreen(void) {

	sdl_fullscreen = !sdl_fullscreen;
	fb_sdl2_toggle_fullscreen(&sdl_ctx);
}

/** @brief Try to initialize SDL video and the main rendering surface.
 */
static void video_try_init_sdl(void) {

	int scale;
	const char* scale_env;

	if (sdl_init_attempted != 0) {
		return;
	}

	sdl_init_attempted = 1;
	fb_init(&sdl_fb);
	memset(sdl_framebuffer, 0, sizeof(sdl_framebuffer));

	scale = 3;
	scale_env = getenv("STUNTS_SCALE");
	if (scale_env != NULL) {
		scale = atoi(scale_env);
		if (scale < 1) scale = 1;
		if (scale > 3) scale = 3;
	}
	sdl_scale = scale;

	if (fb_sdl2_init(&sdl_ctx, "stuntsengine", scale) == 0) {
		sdl_ctx_saved_window   = sdl_ctx.window;
		sdl_ctx_saved_renderer = sdl_ctx.renderer;
		sdl_ctx_saved_texture  = sdl_ctx.texture;
		sdl_active = 1;
	}
}

/** @brief Return the active 8-bit framebuffer.
 * @return Function result.
 */
unsigned char* video_get_framebuffer(void) {

	if (sdl_active != 0) {
		return sdl_framebuffer;
	}
	return NULL;
}

/** @brief Report whether SDL video output is currently active.
 * @return Function result.
 */
int video_is_sdl_active(void) {

	return sdl_active != 0;
}

/** @brief Present the current framebuffer, applying timing and palette updates.
 */
void video_present_frame(void) {

	static unsigned long next_present_counter = 0;
	static unsigned long present_accum = 0;

	if (sdl_active == 0) {
		return;
	}

	/* Silently recover from any pointer corruption. */
	if (sdl_ctx.window   != sdl_ctx_saved_window   ||
	    sdl_ctx.renderer != sdl_ctx_saved_renderer ||
	    sdl_ctx.texture  != sdl_ctx_saved_texture) {
		sdl_ctx.window   = sdl_ctx_saved_window;
		sdl_ctx.renderer = sdl_ctx_saved_renderer;
		sdl_ctx.texture  = sdl_ctx_saved_texture;
	}

	video_wait_refresh_slot(&next_present_counter, &present_accum);

	memcpy(sdl_fb.pixels, sdl_framebuffer, FB_PIXELS);
	fb_sdl2_present(&sdl_ctx, &sdl_fb);
}

/** @brief Video refresh.
 */
void video_refresh(void)
{
	video_present_frame();
}

/** @brief Video wait menu refresh.
 */
void video_wait_menu_refresh(void)
{
	video_present_frame();
}

/**
 * video_clear_color - Clear framebuffer with color value
 */
void video_clear_color(unsigned short color) {
	unsigned int i;
	unsigned char * framebuffer;

	if (sdl_active == 0) {
		return;
	}

	framebuffer = video_get_framebuffer();
	/* FB_PIXELS bytes, writing as shorts so divide by 2 */
	for (i = 0; i < FB_PIXELS / 2; i++) {
		((unsigned short *)framebuffer)[i] = color;
	}
}

/**
 * video_set_palette - Set SDL palette entries
 * 
/** @brief Is.
 * @param start Parameter `start`.
 * @param count Parameter `count`.
 * @param colors Parameter `colors`.
 * @return Function result.
 */
void video_set_palette(unsigned short start, unsigned short count, unsigned char* colors) {
	unsigned int i;

	if (sdl_active == 0) {
		return;
	}

	for (i = 0; i < (unsigned int)count; i++) {
		unsigned int idx = (unsigned int)start + i;
		if (idx >= 256u) {
			break;
		}
		fb_set_palette_entry(&sdl_fb,
			(unsigned char)idx,
			colors[i * 3u + 0u],
			colors[i * 3u + 1u],
			colors[i * 3u + 2u]);
	}
}
/**
/** @brief Mode.
 * @param video_sdl_init Parameter `video_sdl_init`.
 * @return Function result.
 */
void video_sdl_init(void) {
	video_try_init_sdl();
	video_add_exithandler();

	if (sdl_active == 0) {
		return;
	}

	video_clear_color(0);
	video_refresh();
}

/**
 * video_add_exithandler - Register video exit handler
 * 
 * Registers video_on_exit exactly once.
 */
void video_add_exithandler(void) {
	if (video_exit_handler_registered != 0) {
		return;
	}

	if (sdl_active == 0) {
		return;
	}

	video_exit_handler_registered = 1;
	input_queue_buffer = 0;
	add_exit_handler((voidfunctype)video_on_exit);
}

/**
 * video_on_exit - Restore original video mode
 * 
 * Called during program exit to shutdown SDL video resources.
 */
void video_on_exit(void) {
	if (sdl_init_attempted == 0) {
		return;
	}

	fb_sdl2_shutdown(&sdl_ctx);
	sdl_active = 0;
	video_exit_handler_registered = 0;
}
