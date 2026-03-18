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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stddef.h>
#include <stdint.h>

#define FB_WIDTH 320
#define FB_HEIGHT 200
#define FB_PIXELS (FB_WIDTH * FB_HEIGHT)

typedef struct Framebuffer {
    uint8_t pixels[FB_PIXELS];
    uint8_t palette[256][3];
} Framebuffer;

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

typedef struct SDL2Context {
    struct SDL_Window* window;
    struct SDL_Renderer* renderer;
    struct SDL_Texture* texture;
    uint32_t rgba[FB_PIXELS];
} SDL2Context;

void fb_init(Framebuffer* fb);
void fb_clear(Framebuffer* fb, uint8_t color_index);
void fb_set_pixel(Framebuffer* fb, int x, int y, uint8_t color_index);
uint8_t fb_get_pixel(const Framebuffer* fb, int x, int y);

void fb_set_palette_entry(Framebuffer* fb, uint8_t index,
                              uint8_t r6, uint8_t g6, uint8_t b6);

uint32_t fb_palette_index_to_rgba(const Framebuffer* fb, uint8_t index);
void fb_to_rgba(const Framebuffer* fb, uint32_t* out_rgba, size_t out_len);

int fb_sdl2_init(SDL2Context* ctx, const char* title, int window_scale);
void fb_sdl2_present(SDL2Context* ctx, const Framebuffer* fb);
void fb_sdl2_shutdown(SDL2Context* ctx);
void fb_sdl2_set_scale(SDL2Context* ctx, int scale);
void fb_sdl2_toggle_fullscreen(SDL2Context* ctx);

/* Video window controls (defined in video.c) */
void video_scale_up(void);
void video_scale_down(void);
void video_toggle_fullscreen(void);
int  video_get_scale(void);
void video_set_scale(int scale);
void video_set_scale_changed_cb(void (*cb)(void));

#endif
