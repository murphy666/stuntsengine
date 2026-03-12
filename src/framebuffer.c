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

#include "framebuffer.h"

#include <SDL2/SDL.h>
#include <string.h>

static uint8_t fb_expand_6_to_8(uint8_t v6)
{
    return (uint8_t)((v6 << 2) | (v6 >> 4));
}

/** @brief Fb init.
 * @param fb Parameter `fb`.
 */
void fb_init(Framebuffer* fb)
{
    int i;
    if (fb == 0) {
        return;
    }

    memset(fb->pixels, 0, sizeof(fb->pixels));

    for (i = 0; i < 256; i++) {
        uint8_t v = (uint8_t)(i & 63);
        fb->palette[i][0] = v;
        fb->palette[i][1] = v;
        fb->palette[i][2] = v;
    }
}

/** @brief Fb clear.
 * @param fb Parameter `fb`.
 * @param color_index Parameter `color_index`.
 */
void fb_clear(Framebuffer* fb, uint8_t color_index)
{
    if (fb == 0) {
        return;
    }
    memset(fb->pixels, color_index, sizeof(fb->pixels));
}

/** @brief Fb set pixel.
 * @param fb Parameter `fb`.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @param color_index Parameter `color_index`.
 */
void fb_set_pixel(Framebuffer* fb, int x, int y, uint8_t color_index)
{
    if (fb == 0) {
        return;
    }
    if (x < 0 || y < 0 || x >= FB_WIDTH || y >= FB_HEIGHT) {
        return;
    }
    fb->pixels[(y * FB_WIDTH) + x] = color_index;
}

/** @brief Fb get pixel.
 * @param fb Parameter `fb`.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @return Function result.
 */
uint8_t fb_get_pixel(const Framebuffer* fb, int x, int y)
{
    if (fb == 0) {
        return 0;
    }
    if (x < 0 || y < 0 || x >= FB_WIDTH || y >= FB_HEIGHT) {
        return 0;
    }
    return fb->pixels[(y * FB_WIDTH) + x];
}

/** @brief Fb set palette entry.
 * @param fb Parameter `fb`.
 * @param index Parameter `index`.
 * @param r6 Parameter `r6`.
 * @param g6 Parameter `g6`.
 * @param b6 Parameter `b6`.
 */
void fb_set_palette_entry(Framebuffer* fb, uint8_t index,
                              uint8_t r6, uint8_t g6, uint8_t b6)
{
    if (fb == 0) {
        return;
    }
    fb->palette[index][0] = (uint8_t)(r6 & 63);
    fb->palette[index][1] = (uint8_t)(g6 & 63);
    fb->palette[index][2] = (uint8_t)(b6 & 63);
}

/** @brief Fb palette index to rgba.
 * @param fb Parameter `fb`.
 * @param index Parameter `index`.
 * @return Function result.
 */
uint32_t fb_palette_index_to_rgba(const Framebuffer* fb, uint8_t index)
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    if (fb == 0) {
        return 4278190080u;
    }

    r = fb_expand_6_to_8(fb->palette[index][0]);
    g = fb_expand_6_to_8(fb->palette[index][1]);
    b = fb_expand_6_to_8(fb->palette[index][2]);

    return (uint32_t)(4278190080u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b);
}

/** @brief Fb to rgba.
 * @param fb Parameter `fb`.
 * @param out_rgba Parameter `out_rgba`.
 * @param out_len Parameter `out_len`.
 */
void fb_to_rgba(const Framebuffer* fb, uint32_t* out_rgba, size_t out_len)
{
    size_t i;
    size_t n = FB_PIXELS;

    if (fb == 0 || out_rgba == 0) {
        return;
    }

    if (out_len < n) {
        n = out_len;
    }

    for (i = 0; i < n; i++) {
        out_rgba[i] = fb_palette_index_to_rgba(fb, fb->pixels[i]);
    }
}

/** @brief Fb sdl2 init.
 * @param ctx Parameter `ctx`.
 * @param title Parameter `title`.
 * @param window_scale Parameter `window_scale`.
 * @return Function result.
 */
int fb_sdl2_init(SDL2Context* ctx, const char* title, int window_scale)
{
    int width;
    int height;

    if (ctx == 0) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));

    if (window_scale <= 0) {
        window_scale = 3;
    }

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        return -1;
    }

    width = FB_WIDTH * window_scale;
    height = FB_HEIGHT * window_scale;

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    ctx->window = SDL_CreateWindow(title ? title : "stuntsengine",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   width,
                                   height,
                                   SDL_WINDOW_SHOWN);
    if (ctx->window == 0) {
        fb_sdl2_shutdown(ctx);
        return -1;
    }

    SDL_ShowCursor(SDL_DISABLE);

    ctx->renderer = SDL_CreateRenderer(ctx->window, -1, SDL_RENDERER_SOFTWARE);
    if (ctx->renderer == 0) {
        ctx->renderer = SDL_CreateRenderer(ctx->window, -1,
                                           SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    }
    if (ctx->renderer == 0) {
        fb_sdl2_shutdown(ctx);
        return -1;
    }

    SDL_RenderSetLogicalSize(ctx->renderer, FB_WIDTH, FB_HEIGHT);

    ctx->texture = SDL_CreateTexture(ctx->renderer,
                                     SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING,
                                     FB_WIDTH,
                                     FB_HEIGHT);
    if (ctx->texture == 0) {
        fb_sdl2_shutdown(ctx);
        return -1;
    }

    return 0;
}

/** @brief Fb sdl2 present.
 * @param ctx Parameter `ctx`.
 * @param fb Parameter `fb`.
 */
void fb_sdl2_present(SDL2Context* ctx, const Framebuffer* fb)
{
    if (ctx == 0 || fb == 0 || ctx->texture == 0 || ctx->renderer == 0) {
        return;
    }

    fb_to_rgba(fb, ctx->rgba, FB_PIXELS);

    SDL_UpdateTexture(ctx->texture, 0, ctx->rgba, FB_WIDTH * (int)sizeof(uint32_t));
    SDL_RenderClear(ctx->renderer);
    SDL_RenderCopy(ctx->renderer, ctx->texture, 0, 0);
    SDL_RenderPresent(ctx->renderer);
}

/** @brief Fb sdl2 set scale.
 * @param ctx Parameter `ctx`.
 * @param scale Parameter `scale`.
 */
void fb_sdl2_set_scale(SDL2Context* ctx, int scale)
{
    Uint32 flags;
    if (ctx == 0 || ctx->window == 0) {
        return;
    }
    /* Don't resize while fullscreen */
    flags = SDL_GetWindowFlags(ctx->window);
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        return;
    }
    SDL_SetWindowSize(ctx->window, FB_WIDTH * scale, FB_HEIGHT * scale);
}

/** @brief Fb sdl2 toggle fullscreen.
 * @param ctx Parameter `ctx`.
 */
void fb_sdl2_toggle_fullscreen(SDL2Context* ctx)
{
    Uint32 flags;
    if (ctx == 0 || ctx->window == 0) {
        return;
    }
    flags = SDL_GetWindowFlags(ctx->window);
    if (flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) {
        SDL_SetWindowFullscreen(ctx->window, 0);
    } else {
        SDL_SetWindowFullscreen(ctx->window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    }
}

/** @brief Fb sdl2 shutdown.
 * @param ctx Parameter `ctx`.
 */
void fb_sdl2_shutdown(SDL2Context* ctx)
{
    if (ctx == 0) {
        return;
    }

    if (ctx->texture != 0) {
        SDL_DestroyTexture(ctx->texture);
        ctx->texture = 0;
    }
    if (ctx->renderer != 0) {
        SDL_DestroyRenderer(ctx->renderer);
        ctx->renderer = 0;
    }
    if (ctx->window != 0) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = 0;
    }

    SDL_Quit();
}
