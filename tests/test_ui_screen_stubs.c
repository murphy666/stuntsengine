/*
 * test_ui_screen_stubs.c — Stubs for external symbols needed by ui_screen.c
 *
 * These provide controllable fake implementations of keyboard, mouse,
 * timer, and sprite functions so the screen stack and event dispatch
 * can be tested in isolation.
 */

#include <string.h>
#include "data_game.h"
#include "keyboard.h"
#include "mouse.h"
#include "timer.h"
#include "highscore.h"
#include "shape2d.h"

/* ---- Controllable state for test injection ---- */

/* Keyboard stub: queued keys */
#define STUB_KEY_QUEUE_MAX 64
static unsigned short s_key_queue[STUB_KEY_QUEUE_MAX];
static int s_key_head = 0;
static int s_key_tail = 0;

void stub_key_reset(void) {
    s_key_head = s_key_tail = 0;
}
void stub_key_enqueue(unsigned short key) {
    if (s_key_tail < STUB_KEY_QUEUE_MAX) {
        s_key_queue[s_key_tail++] = key;
    }
}

/* Mouse stub state */
static unsigned short s_mouse_buttons = 0;
static unsigned short s_mouse_x = 0;
static unsigned short s_mouse_y = 0;

void stub_mouse_set(unsigned short buttons, unsigned short x, unsigned short y) {
    s_mouse_buttons = buttons;
    s_mouse_x = x;
    s_mouse_y = y;
}

/* Timer stub: fixed delta */
static unsigned long s_timer_delta = 10;
void stub_timer_set_delta(unsigned long d) {
    s_timer_delta = d;
}

/* Hittest stub: return value */
static short s_hittest_result = -1;
void stub_hittest_set(short result) {
    s_hittest_result = result;
}

/* Track how many times various functions are called */
static int s_blink_calls = 0;
static int s_poll_calls = 0;
static int s_idle_reset_calls = 0;

int stub_get_blink_calls(void) { return s_blink_calls; }
int stub_get_poll_calls(void) { return s_poll_calls; }
int stub_get_idle_reset_calls(void) { return s_idle_reset_calls; }

void stub_counters_reset(void) {
    s_blink_calls = 0;
    s_poll_calls = 0;
    s_idle_reset_calls = 0;
}

/* ---- data_game.h globals (stub definitions for test binary) ---- */
char kbormouse = 0;
char idle_expired = 0;
unsigned int idle_counter = 0;
unsigned short mousebutinputcode = 0;
unsigned short joyinputcode = 0;
unsigned short newjoyflags = 0;
unsigned short mouse_oldbut = 0;
unsigned short mouse_butstate = 0;
unsigned short camera_view_matrix = 0;
unsigned short object_visibility_state = 0;

void stub_reset_all(void) {
    stub_key_reset();
    stub_mouse_set(0, 0, 0);
    stub_timer_set_delta(10);
    stub_hittest_set(-1);
    stub_counters_reset();
    kbormouse = 0;
    idle_expired = 0;
    idle_counter = 0;
    mousebutinputcode = 0;
    joyinputcode = 0;
}

/* ---- keyboard.h stubs ---- */
void kb_poll_sdl_input(void) {
    s_poll_calls++;
}

int kb_get_char(void) {
    if (s_key_head < s_key_tail) {
        return (int)s_key_queue[s_key_head++];
    }
    return 0;
}

short get_joy_flags(void) {
    return 0;
}

void check_input(void) {
    /* no-op for tests */
}

/* ---- mouse.h stubs ---- */
void mouse_get_state(unsigned short *buttons, unsigned short *x, unsigned short *y) {
    *buttons = s_mouse_buttons;
    *x = s_mouse_x;
    *y = s_mouse_y;
}

short mouse_multi_hittest(short count, unsigned short *x1_array,
                          unsigned short *x2_array, unsigned short *y1_array,
                          unsigned short *y2_array) {
    (void)count; (void)x1_array; (void)x2_array;
    (void)y1_array; (void)y2_array;
    return s_hittest_result;
}

unsigned int mouse_update_menu_blink(unsigned char selected,
                                     unsigned short *x1_arr, unsigned short *x2_arr,
                                     unsigned short *y1_arr, unsigned short *y2_arr,
                                     unsigned short sprite_hi, unsigned short sprite_lo) {
    (void)selected; (void)x1_arr; (void)x2_arr;
    (void)y1_arr; (void)y2_arr;
    (void)sprite_hi; (void)sprite_lo;
    s_blink_calls++;
    return (unsigned int)s_timer_delta;
}

/* ---- timer.h stubs ---- */
unsigned long timer_get_delta_alt(void) {
    return s_timer_delta;
}

/* ---- highscore.h stubs ---- */
void menu_reset_idle_timers(void) {
    s_idle_reset_calls++;
}

/* ---- shape2d.h stubs (needed if any header pulls them in) ---- */
void sprite_fill_rect_clipped(unsigned short x, unsigned short y,
                              unsigned short w, unsigned short h,
                              unsigned char color) {
    (void)x; (void)y; (void)w; (void)h; (void)color;
}

void sprite_draw_rect_outline(unsigned short x1, unsigned short y1,
                               unsigned short x2, unsigned short y2,
                               unsigned short color) {
    (void)x1; (void)y1; (void)x2; (void)y2; (void)color;
}

/* ---- stunts.h additional stubs ---- */
void add_exit_handler(void (*handler)(void)) { (void)handler; }
void draw_beveled_border(unsigned short x, unsigned short y, unsigned short w,
                         unsigned short h, unsigned short col1,
                         unsigned short col2, unsigned short col3) {
    (void)x; (void)y; (void)w; (void)h; (void)col1; (void)col2; (void)col3;
}

void fatal_error(const char *fmt, ...) {
    (void)fmt;
    /* In tests we just abort so ASAN/test infra catches it */
    __builtin_trap();
}
