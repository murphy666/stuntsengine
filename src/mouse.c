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

/* mouse.c — Mouse management extracted from stunts.c */
#include <SDL3/SDL.h>
#include <time.h>
#include "stunts.h"
#include "shape2d.h"
#include "timer.h"
#include "mouse.h"

enum {
    MOUSE_DEFAULT_MAX_X = 319,
    MOUSE_DEFAULT_MAX_Y = 199,
    MOUSE_CENTER_DIVISOR = 2
};

typedef struct {
    bool driver_enabled;
    unsigned short x;
    unsigned short y;
    unsigned short min_x;
    unsigned short min_y;
    unsigned short max_x;
    unsigned short max_y;
    unsigned short buttons;
} MouseBackendState;

static MouseBackendState g_mouse_backend
    = { false, 0, 0, 0, 0, MOUSE_DEFAULT_MAX_X, MOUSE_DEFAULT_MAX_Y, 0 };

unsigned short mouse_buttons = 0;
unsigned short mouse_x = 0;
unsigned short mouse_y = 0;
bool mouse_cursor_dirty = false;
bool mouse_control_enabled = false;
bool mouse_cursor_enabled = false;
bool mouse_input_active = false;

static void
mouse_clamp_backend_position(void) {
    if (g_mouse_backend.x < g_mouse_backend.min_x)
        g_mouse_backend.x = g_mouse_backend.min_x;
    if (g_mouse_backend.y < g_mouse_backend.min_y)
        g_mouse_backend.y = g_mouse_backend.min_y;
    if (g_mouse_backend.x > g_mouse_backend.max_x)
        g_mouse_backend.x = g_mouse_backend.max_x;
    if (g_mouse_backend.y > g_mouse_backend.max_y)
        g_mouse_backend.y = g_mouse_backend.max_y;
}

static void
mouse_sync_globals_from_backend(void) {
    mouse_buttons = g_mouse_backend.buttons;
    mouse_x = g_mouse_backend.x;
    mouse_y = g_mouse_backend.y;
}

static void
mouse_warp_host_pointer(unsigned short x, unsigned short y) {
    SDL_Window *window = SDL_GetMouseFocus();
    int window_width = 0;
    int window_height = 0;
    int range_x;
    int range_y;
    int window_x;
    int window_y;

    if (window == 0) {
        return;
    }

    SDL_GetWindowSize(window, &window_width, &window_height);
    range_x = (int)g_mouse_backend.max_x - (int)g_mouse_backend.min_x;
    range_y = (int)g_mouse_backend.max_y - (int)g_mouse_backend.min_y;
    if (window_width <= 1 || window_height <= 1 || range_x <= 0 || range_y <= 0) {
        return;
    }

    window_x = ((int)(x - g_mouse_backend.min_x) * (window_width - 1)) / range_x;
    window_y = ((int)(y - g_mouse_backend.min_y) * (window_height - 1)) / range_y;
    SDL_WarpMouseInWindow(window, (float)window_x, (float)window_y);
}

static void
mouse_refresh_from_host(void) {
    float raw_x = 0.0f;
    float raw_y = 0.0f;
    SDL_Window *window;
    SDL_MouseButtonFlags button_state;
    int window_width = 0;
    int window_height = 0;
    int scaled_x;
    int scaled_y;
    int range_x;
    int range_y;

    if (!g_mouse_backend.driver_enabled) {
        g_mouse_backend.buttons = 0;
        mouse_sync_globals_from_backend();
        return;
    }

    SDL_PumpEvents();
    button_state = SDL_GetMouseState(&raw_x, &raw_y);
    window = SDL_GetMouseFocus();
    if (window != 0) {
        SDL_GetWindowSize(window, &window_width, &window_height);
    }

    scaled_x = (int)raw_x;
    scaled_y = (int)raw_y;
    range_x = (int)g_mouse_backend.max_x - (int)g_mouse_backend.min_x;
    range_y = (int)g_mouse_backend.max_y - (int)g_mouse_backend.min_y;

    if (window_width > 1 && range_x > 0) {
        scaled_x = (int)g_mouse_backend.min_x + (scaled_x * range_x) / (window_width - 1);
    }
    if (window_height > 1 && range_y > 0) {
        scaled_y = (int)g_mouse_backend.min_y + (scaled_y * range_y) / (window_height - 1);
    }

    g_mouse_backend.x = (unsigned short)scaled_x;
    g_mouse_backend.y = (unsigned short)scaled_y;
    mouse_clamp_backend_position();
    g_mouse_backend.buttons = 0;
    if ((button_state & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0u) {
        g_mouse_backend.buttons |= MOUSE_BUTTON_PRIMARY;
    }
    if ((button_state & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0u) {
        g_mouse_backend.buttons |= MOUSE_BUTTON_SECONDARY;
    }
    if ((button_state & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE)) != 0u) {
        g_mouse_backend.buttons |= MOUSE_BUTTON_MIDDLE;
    }

    mouse_sync_globals_from_backend();
}

static void
mouse_copy_state(unsigned short *buttons, unsigned short *x, unsigned short *y) {
    if (buttons) {
        *buttons = mouse_buttons;
    }
    if (x) {
        *x = mouse_x;
    }
    if (y) {
        *y = mouse_y;
    }
}

/* --- mouse_get_state --- */

/** @brief Poll host input and refresh the shared mouse globals. */
void
mouse_poll_input(void) {
    if (!g_mouse_backend.driver_enabled) {
        mouse_buttons = 0;
        return;
    }

    mouse_refresh_from_host();
}

/**
 * mouse_get_state - Poll host pointer state for buttons and position
 *
 * Reads the current SDL pointer state, remaps it into the active game-space
 * bounds, and updates the shared mouse globals.
 */
void
mouse_get_state(unsigned short *buttons, unsigned short *x, unsigned short *y) {
    mouse_poll_input();
    mouse_copy_state(buttons, x, y);
}

bool
mouse_has_action_buttons(unsigned short buttons) {
    return (buttons & MOUSE_BUTTON_ACTION_MASK) != 0;
}

unsigned short
mouse_buttons_to_keycode(unsigned short buttons) {
    if ((buttons & MOUSE_BUTTON_PRIMARY) != 0) {
        return MOUSE_KEYCODE_PRIMARY_ACTION;
    }
    if ((buttons & MOUSE_BUTTON_SECONDARY) != 0) {
        return MOUSE_KEYCODE_SECONDARY_ACTION;
    }
    return 0;
}

/* --- mouse_set_pixratio --- */

/**
 * mouse_set_pixratio - Set mouse mickey-to-pixel ratio
 *
 * INT 33h function 0Fh - set horizontal and vertical mickey-to-pixel ratios.
 * Both parameters are the number of mickeys per 8 pixels.
 * @hpix: Horizontal mickeys per 8 pixels
 * @vpix: Vertical mickeys per 8 pixels
 */
void
mouse_set_pixratio(unsigned short hpix, unsigned short vpix) {
    (void)hpix;
    (void)vpix;
}

/* --- mouse_init --- */

/**
 * mouse_init - Initialize host mouse input
 *
 * Enables the host pointer backend, sets initial bounds, and centers the
 * pointer in the current game-space rectangle.
 * @width: Screen width for mouse limits (e.g. 320)
 * @height: Screen height for mouse limits (e.g. 200)
 * Returns: Non-zero if mouse driver present, 0 otherwise
 */
unsigned short
mouse_init(unsigned short width, unsigned short height) {
    /* Host build: SDL pointer support is always available. */
    g_mouse_backend.driver_enabled = true;
    g_mouse_backend.buttons = 0;
    if (g_mouse_backend.driver_enabled) {
        mouse_set_minmax(0, 0, width - 1, height - 1);
        mouse_set_position(width / MOUSE_CENTER_DIVISOR, height / MOUSE_CENTER_DIVISOR);
        mouse_set_pixratio(16, 16);
    }

    return g_mouse_backend.driver_enabled ? 1u : 0u;
}

/* --- mouse_set_minmax --- */

/**
 * mouse_set_minmax - Set mouse cursor movement limits
 *
 * Updates the logical game-space bounds used when projecting host pointer
 * coordinates into the engine's 320x200-style coordinate system.
 * @hmin: Minimum horizontal position
 * @vmin: Minimum vertical position
 * @hmax: Maximum horizontal position
 * @vmax: Maximum vertical position
 */
void
mouse_set_minmax(int hmin, int vmin, int hmax, int vmax) {
    g_mouse_backend.min_x = (unsigned short)hmin;
    g_mouse_backend.min_y = (unsigned short)vmin;
    g_mouse_backend.max_x = (unsigned short)hmax;
    g_mouse_backend.max_y = (unsigned short)vmax;
    mouse_clamp_backend_position();
    mouse_sync_globals_from_backend();
}

/* --- mouse_set_position --- */

/**
 * mouse_set_position - Set mouse cursor position
 *
 * Stores a logical pointer position and, when a window is focused, warps the
 * host cursor to the equivalent window coordinates.
 * @x: Horizontal position
 * @y: Vertical position
 */
void
mouse_set_position(int x, int y) {
    g_mouse_backend.x = (unsigned short)x;
    g_mouse_backend.y = (unsigned short)y;
    mouse_clamp_backend_position();
    mouse_warp_host_pointer(g_mouse_backend.x, g_mouse_backend.y);
    mouse_sync_globals_from_backend();
}

/* --- mouse_draw_transparent_check --- */

// Draw mouse cursor transparent (show cursor)
/** @brief Draw the mouse cursor transparently when redraw conditions are met. */
void
mouse_draw_transparent_check(void) {
    mouse_cursor_enabled = true;
    if (!mouse_input_active)
        return;
    if (mouse_cursor_dirty)
        return;
    mouse_draw_transparent();
}

/* --- mouse_draw_opaque_check --- */

// Draw mouse cursor opaque (hide/restore background)
/** @brief Restore the background under the mouse cursor when needed. */
void
mouse_draw_opaque_check(void) {
    mouse_cursor_enabled = false;
    if (!mouse_cursor_dirty)
        return;
    mouse_draw_opaque();
}

/* --- mouse_multi_hittest --- */

// Check if mouse is inside any of an array of rectangles
// Returns index of hit rectangle, or -1 if none
/** @brief Return the first rectangle hit by the current mouse cursor position. */
short
mouse_multi_hittest(short count, unsigned short *x1_array, unsigned short *x2_array,
                    unsigned short *y1_array, unsigned short *y2_array) {
    if (!mouse_input_active) {
        return -1;
    }

    return sprite_hittest_point(mouse_x, mouse_y, count, (const unsigned short *)x1_array,
                                (const unsigned short *)x2_array, (const unsigned short *)y1_array,
                                (const unsigned short *)y2_array);
}

/* --- mouse_draw_opaque --- */

// Draw mouse cursor opaque (solid)
/** @brief Draw the mouse sprite opaquely at the current cursor coordinates. */
void
mouse_draw_opaque(void) {
    // Save both sprite buffers (sprite_copy_both_to_arg copies sprite1 and sprite2)
    static struct SPRITE local_sprites[2];
    if (mouseunkspriteptr == 0) {
        mouse_cursor_dirty = false;
        return;
    }
    sprite_copy_both_to_arg(local_sprites);
    sprite_copy_2_to_1();
    sprite_putimage(mouseunkspriteptr->sprite_bitmapptr);
    sprite_copy_arg_to_both(local_sprites);
    mouse_cursor_dirty = false;
}

/* --- mouse_draw_transparent --- */

// Draw mouse cursor with transparency
/** @brief Draw the mouse sprite using mask/overlay transparency. */
void
mouse_draw_transparent(void) {
    // Save both sprite buffers (sprite_copy_both_to_arg copies sprite1 and sprite2)
    static struct SPRITE local_sprites[2];

    if (smouspriteptr == 0 || mmouspriteptr == 0 || mouseunkspriteptr == 0) {
        return;
    }

    sprite_copy_both_to_arg(local_sprites);
    sprite_copy_2_to_1();
    sprite_clear_shape_alt(mouseunkspriteptr->sprite_bitmapptr, mouse_x, mouse_y);
    sprite_putimage_and(mmouspriteptr->sprite_bitmapptr, mouse_x, mouse_y);
    sprite_putimage_or(smouspriteptr->sprite_bitmapptr, mouse_x, mouse_y);
    sprite_copy_arg_to_both(local_sprites);
    mouse_cursor_dirty = true;
}

/* --- mouse_update_menu_blink --- */

// Timer-based sprite update for menu highlighting.
// Draws the outline/cursor state for the current frame and returns the elapsed
// time; callers remain responsible for presenting the frame.
unsigned int
mouse_update_menu_blink(unsigned char selected, unsigned short *x1_arr, unsigned short *x2_arr,
                        unsigned short *y1_arr, unsigned short *y2_arr, unsigned short sprite_hi,
                        unsigned short sprite_lo) {

    unsigned short delta;
    unsigned short sprite_idx;
    unsigned short idx;

    /* Use a private counter instead of timer_get_delta_alt() so that the
	 * blink phase advances correctly even when the outer ui_screen_run_modal
	 * loop has already called timer_get_delta_alt() this frame (which would
	 * reset last_timer_callback_counter and make a second call return 0). */
    {
        static unsigned long blink_prev_counter = 0;
        unsigned long curr = timer_get_counter();
        unsigned long diff = curr - blink_prev_counter;
        blink_prev_counter = curr;
        /* Cap to period to avoid excessive looping in sprite_phase_add_wrap. */
        delta = (diff > 300UL) ? 300U : (unsigned short)diff;
    }
    if (delta == 0) {
        struct timespec ts_yield;
        ts_yield.tv_sec = 0;
        ts_yield.tv_nsec = GAME_YIELD_NS;
        nanosleep(&ts_yield, NULL);
    }
    /* timer_get_counter() units: g_timer_counter_units_per_tick=5 per 100Hz tick.
	 * At 60fps delta≈5-10 units/frame.  Period=300, threshold=150 → 0.5s cycle. */
    distance_accumulator_counter = sprite_phase_add_wrap(distance_accumulator_counter, delta, 300);
    sprite_idx = sprite_pick_blink_frame(distance_accumulator_counter, 150, sprite_hi, sprite_lo);

    if (x1_arr == 0 || x2_arr == 0 || y1_arr == 0 || y2_arr == 0) {
        return delta;
    }

    /* Always redraw the outline so callers that restore the base sprite each
	 * frame (sprite_copy_2_to_1_2 before this call) don't lose it between
	 * blink state changes. */
    if (game_timer_milliseconds != sprite_idx) {
        game_timer_milliseconds = sprite_idx;
    }
    mouse_draw_opaque_check();
    idx = selected;
    sprite_draw_rect_outline(x1_arr[idx], y1_arr[idx], x2_arr[idx], y2_arr[idx],
                             game_timer_milliseconds);
    mouse_draw_transparent_check();

    return delta;
}
