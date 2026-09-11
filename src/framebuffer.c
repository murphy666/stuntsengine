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

#include <SDL3/SDL.h>
#include <string.h>

static uint8_t
fb_expand_6_to_8(uint8_t v6) {
    return (uint8_t)((v6 << 2) | (v6 >> 4));
}

/* On DOS the engine's 8-bit page *is* the hardware format, so the
 * renderer/texture path (expand to ARGB8888, re-upload a 320x200 streaming
 * texture, software-blit it back down) is pure waste — it runs at roughly one
 * frame per second. Instead we take a paletted mode-13h window surface, copy
 * the page into it, and let SDL's DOS driver push system RAM straight to VRAM.
 * fb_sdl_init() falls back to the renderer path if any of this fails. */
#ifdef __DJGPP__
#define FB_HAVE_DIRECT_FB 1
#endif

/** @brief Fb init.
 * @param fb Parameter `fb`.
 */
void
fb_init(Framebuffer *fb) {
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
void
fb_clear(Framebuffer *fb, uint8_t color_index) {
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
void
fb_set_pixel(Framebuffer *fb, int x, int y, uint8_t color_index) {
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
uint8_t
fb_get_pixel(const Framebuffer *fb, int x, int y) {
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
void
fb_set_palette_entry(Framebuffer *fb, uint8_t index, uint8_t r6, uint8_t g6, uint8_t b6) {
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
uint32_t
fb_palette_index_to_rgba(const Framebuffer *fb, uint8_t index) {
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
void
fb_to_rgba(const Framebuffer *fb, uint32_t *out_rgba, size_t out_len) {
    size_t i;
    size_t n = (size_t)FB_PIXELS;

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

#ifdef FB_HAVE_DIRECT_FB
/** @brief Pin the mode-13h fullscreen mode so the window surface is paletted.
 *
 * SDL's DOS driver derives the window-surface format from the current display
 * mode, so an INDEX8 surface requires selecting the 320x200x8 mode explicitly.
 *
 * @param window Window to pin the fullscreen mode on.
 * @return Non-zero on success.
 */
static int
fb_dos_pin_mode13h(SDL_Window *window) {
    SDL_DisplayMode **modes;
    int count = 0;
    int i;
    int pinned = 0;

    modes = SDL_GetFullscreenDisplayModes(SDL_GetPrimaryDisplay(), &count);
    if (modes == 0) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        if (modes[i]->w == FB_WIDTH && modes[i]->h == FB_HEIGHT
            && modes[i]->format == SDL_PIXELFORMAT_INDEX8) {
            pinned = SDL_SetWindowFullscreenMode(window, modes[i]) ? 1 : 0;
            break;
        }
    }

    SDL_free(modes);
    return pinned;
}

/** @brief Try to bring up the DOS direct-framebuffer present path.
 * @param ctx Parameter `ctx`.
 * @param title Parameter `title`.
 * @return 0 on success, -1 to fall back to the renderer path.
 */
static int
fb_sdl_init_direct(SDLContext *ctx, const char *title) {
    SDL_Palette *palette;

    /* Must be set before the first SDL_GetWindowSurface() to improves performance. */
    SDL_SetHint(SDL_HINT_DOS_ALLOW_DIRECT_FRAMEBUFFER, "1");

    ctx->window = SDL_CreateWindow(title, FB_WIDTH, FB_HEIGHT, SDL_WINDOW_FULLSCREEN);
    if (ctx->window == 0) {
        return -1;
    }

    if (!fb_dos_pin_mode13h(ctx->window)) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = 0;
        return -1;
    }
    (void)SDL_SyncWindow(ctx->window);

    ctx->surface = SDL_GetWindowSurface(ctx->window);
    if (ctx->surface == 0 || ctx->surface->format != SDL_PIXELFORMAT_INDEX8) {
        ctx->surface = 0;
        SDL_DestroyWindow(ctx->window);
        ctx->window = 0;
        return -1;
    }

    /* The driver creates the palette with the surface; only make one if it
     * did not. Either way the DAC is driven from it on present. */
    palette = SDL_GetSurfacePalette(ctx->surface);
    if (palette == 0 && SDL_CreateSurfacePalette(ctx->surface) == 0) {
        ctx->surface = 0;
        SDL_DestroyWindow(ctx->window);
        ctx->window = 0;
        return -1;
    }

    /* Shadow starts all-zero from the memset in fb_sdl_init(), matching the
     * driver's all-black startup palette, so the first differing frame pushes. */
    return 0;
}

/** @brief Present via the DOS direct-framebuffer path.
 * @param ctx Parameter `ctx`.
 * @param fb Parameter `fb`.
 */
static void
fb_sdl_present_direct(SDLContext *ctx, const Framebuffer *fb) {
    uint8_t *dst;
    int y;

    if (memcmp(ctx->palette_shadow, fb->palette, sizeof(ctx->palette_shadow)) != 0) {
        SDL_Palette *palette = SDL_GetSurfacePalette(ctx->surface);
        SDL_Color colors[256];
        int i;

        for (i = 0; i < 256; i++) {
            colors[i].r = fb_expand_6_to_8(fb->palette[i][0]);
            colors[i].g = fb_expand_6_to_8(fb->palette[i][1]);
            colors[i].b = fb_expand_6_to_8(fb->palette[i][2]);
            colors[i].a = SDL_ALPHA_OPAQUE;
        }
        (void)SDL_SetPaletteColors(palette, colors, 0, 256);
        memcpy(ctx->palette_shadow, fb->palette, sizeof(ctx->palette_shadow));
    }

    /* Copy row by row: the surface pitch need not equal FB_WIDTH. */
    dst = (uint8_t *)ctx->surface->pixels;
    for (y = 0; y < FB_HEIGHT; y++) {
        memcpy(dst + ((size_t)y * (size_t)ctx->surface->pitch), fb->pixels + ((size_t)y * FB_WIDTH),
               FB_WIDTH);
    }

    (void)SDL_UpdateWindowSurface(ctx->window);
}
#endif /* FB_HAVE_DIRECT_FB */

/** @brief Fb sdl init.
 * @param ctx Parameter `ctx`.
 * @param title Parameter `title`.
 * @param window_scale Parameter `window_scale`.
 * @return Function result.
 */
int
fb_sdl_init(SDLContext *ctx, const char *title, int window_scale) {
    int width;
    int height;

    if (ctx == 0) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));

    if (window_scale <= 0) {
        window_scale = 3;
    }

    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        return -1;
    }

    if (title == 0) {
        title = "stuntsengine";
    }

#ifdef FB_HAVE_DIRECT_FB
    if (fb_sdl_init_direct(ctx, title) == 0) {
        (void)SDL_HideCursor();
        return 0;
    }
    /* Fall through to the renderer path. */
#endif

    width = FB_WIDTH * window_scale;
    height = FB_HEIGHT * window_scale;

    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    ctx->window = SDL_CreateWindow(title, width, height, 0);
    if (ctx->window == 0) {
        fb_sdl_shutdown(ctx);
        return -1;
    }

    (void)SDL_HideCursor();

    ctx->renderer = SDL_CreateRenderer(ctx->window, "software");
    if (ctx->renderer == 0) {
        ctx->renderer = SDL_CreateRenderer(ctx->window, 0);
    }
    if (ctx->renderer == 0) {
        fb_sdl_shutdown(ctx);
        return -1;
    }

    (void)SDL_SetRenderVSync(ctx->renderer, 1);
    (void)SDL_SetRenderLogicalPresentation(ctx->renderer, FB_WIDTH, FB_HEIGHT,
                                           SDL_LOGICAL_PRESENTATION_LETTERBOX);

    ctx->texture = SDL_CreateTexture(ctx->renderer, SDL_PIXELFORMAT_ARGB8888,
                                     SDL_TEXTUREACCESS_STREAMING, FB_WIDTH, FB_HEIGHT);
    if (ctx->texture == 0) {
        fb_sdl_shutdown(ctx);
        return -1;
    }

    (void)SDL_SetTextureScaleMode(ctx->texture, SDL_SCALEMODE_NEAREST);

    return 0;
}

/** @brief Fb sdl present.
 * @param ctx Parameter `ctx`.
 * @param fb Parameter `fb`.
 */
void
fb_sdl_present(SDLContext *ctx, const Framebuffer *fb) {
    if (ctx == 0 || fb == 0) {
        return;
    }

#ifdef FB_HAVE_DIRECT_FB
    if (ctx->surface != 0) {
        fb_sdl_present_direct(ctx, fb);
        return;
    }
#endif

    if (ctx->texture == 0 || ctx->renderer == 0) {
        return;
    }

    fb_to_rgba(fb, ctx->rgba, (size_t)FB_PIXELS);

    (void)SDL_UpdateTexture(ctx->texture, 0, ctx->rgba, FB_WIDTH * (int)sizeof(uint32_t));
    (void)SDL_RenderClear(ctx->renderer);
    (void)SDL_RenderTexture(ctx->renderer, ctx->texture, 0, 0);
    (void)SDL_RenderPresent(ctx->renderer);
}

/** @brief Fb sdl set scale.
 * @param ctx Parameter `ctx`.
 * @param scale Parameter `scale`.
 */
void
fb_sdl_set_scale(SDLContext *ctx, int scale) {
    SDL_WindowFlags flags;
    if (ctx == 0 || ctx->window == 0) {
        return;
    }
    /* The direct-FB path is pinned to the native 320x200 hardware mode. */
    if (ctx->surface != 0) {
        return;
    }
    /* Don't resize while fullscreen */
    flags = SDL_GetWindowFlags(ctx->window);
    if ((flags & SDL_WINDOW_FULLSCREEN) != 0) {
        return;
    }
    (void)SDL_SetWindowSize(ctx->window, FB_WIDTH * scale, FB_HEIGHT * scale);
}

/** @brief Fb sdl toggle fullscreen.
 * @param ctx Parameter `ctx`.
 */
void
fb_sdl_toggle_fullscreen(SDLContext *ctx) {
    SDL_WindowFlags flags;
    if (ctx == 0 || ctx->window == 0) {
        return;
    }
    /* Always fullscreen with no desktop to return to, and
     * dropping the pinned mode would lose the INDEX8 surface. */
    if (ctx->surface != 0) {
        return;
    }
    flags = SDL_GetWindowFlags(ctx->window);
    if ((flags & SDL_WINDOW_FULLSCREEN) != 0) {
        (void)SDL_SetWindowFullscreen(ctx->window, false);
    }
    else {
        (void)SDL_SetWindowFullscreen(ctx->window, true);
    }
}

/** @brief Fb sdl shutdown.
 * @param ctx Parameter `ctx`.
 */
void
fb_sdl_shutdown(SDLContext *ctx) {
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
    /* Owned by the window — destroying the window releases it. */
    ctx->surface = 0;
    if (ctx->window != 0) {
        SDL_DestroyWindow(ctx->window);
        ctx->window = 0;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}
