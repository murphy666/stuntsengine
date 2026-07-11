/*
 * ui_screen.c - SDL2-style event-driven screen manager for Stunts UI
 *
 * Provides a screen stack with event dispatch, replacing per-menu
 * while(1) loops.  Each screen gets UIEvents and renders one frame;
 * the run-loop handles SDL polling, timing, and presentation.
 */

#include <stdlib.h>
#include <string.h>
#include "ui_screen.h"
#include "ui_widgets.h"
#include "ui_keys.h"
#include "stunts.h"
#include "keyboard.h"
#include "mouse.h"
#include "timer.h"
#include "shape2d.h"
#include "data_game.h"
#include "highscore.h"

/* ------------------------------------------------------------------ */
/* Screen allocation                                                   */
/* ------------------------------------------------------------------ */

UIScreen *ui_screen_alloc(void)
{
    UIScreen *s = (UIScreen *)calloc(1, sizeof(UIScreen));
    return s;
}

void ui_screen_free(UIScreen *scr)
{
    if (!scr) return;
    if (scr->on_destroy) {
        scr->on_destroy(scr);
    }
    free(scr);
}

/* ------------------------------------------------------------------ */
/* Screen stack                                                        */
/* ------------------------------------------------------------------ */

static UIScreen *s_stack[UI_SCREEN_STACK_MAX];
static int       s_depth = 0;

void ui_screen_push(UIScreen *scr)
{
    UIEvent ev;

    if (!scr || s_depth >= UI_SCREEN_STACK_MAX) return;

    /* notify old top that it's being covered */
    if (s_depth > 0 && s_stack[s_depth - 1]->on_event) {
        memset(&ev, 0, sizeof(ev));
        ev.type = UI_EVENT_LEAVE;
        s_stack[s_depth - 1]->on_event(s_stack[s_depth - 1], &ev);
    }

    s_stack[s_depth++] = scr;

    /* notify new top that it's now active */
    if (scr->on_event) {
        memset(&ev, 0, sizeof(ev));
        ev.type = UI_EVENT_ENTER;
        scr->on_event(scr, &ev);
    }
}

void ui_screen_pop(void)
{
    UIEvent ev;

    if (s_depth <= 0) return;

    ui_screen_free(s_stack[--s_depth]);
    s_stack[s_depth] = NULL;

    /* notify the screen that's now on top */
    if (s_depth > 0 && s_stack[s_depth - 1]->on_event) {
        memset(&ev, 0, sizeof(ev));
        ev.type = UI_EVENT_ENTER;
        s_stack[s_depth - 1]->on_event(s_stack[s_depth - 1], &ev);
    }
}

UIScreen *ui_screen_top(void)
{
    return (s_depth > 0) ? s_stack[s_depth - 1] : NULL;
}

int ui_screen_depth(void)
{
    return s_depth;
}

/* ------------------------------------------------------------------ */
/* Event translation                                                   */
/* ------------------------------------------------------------------ */

/*
 * Convert the current input state into UIEvents and dispatch them
 * to the top screen.  Returns 0 normally, non-zero if the top screen
 * wants to pop.
 */
static int dispatch_events(UIScreen *scr, unsigned short delta)
{
    UIEvent ev;
    unsigned short key;
    unsigned short mbut, mx, my;
    unsigned short joy;
    unsigned short changed;
    unsigned char had_keyboard_event = 0;
    static unsigned short prev_mbut = 0;
    static unsigned short prev_mx = 0, prev_my = 0;
    static unsigned short prev_joy = 0;
    static unsigned short joy_repeat_elapsed = 0;
    static unsigned char joy_repeat_started = 0;
    int result;

    if (!scr->on_event) return 0;

    /* --- keyboard events --- */
    key = kb_get_char();
    if (key != 0) {
        kbormouse = 0;
        had_keyboard_event = 1;
        memset(&ev, 0, sizeof(ev));
        ev.type = UI_EVENT_KEY_DOWN;
        ev.key  = key;
        ev.delta = delta;
        result = scr->on_event(scr, &ev);
        if (result != 0) return result;
    }

    /* --- joystick mapped to key events --- */
    joy = get_joy_flags();

    /* kb_get_char() + get_joy_flags() can represent the same physical key.
     * When we already dispatched a keyboard event this frame, only sync
     * held-state trackers and skip joy-derived emission to avoid double steps. */
    if (had_keyboard_event != 0) {
        prev_joy = joy;
        joy_repeat_elapsed = 0;
        joy_repeat_started = 0;
    } else {
        changed = (prev_joy ^ joy) & joy;
        prev_joy = joy;

        if (changed) {
            unsigned short jkey = 0;
            if (changed & 32) jkey = UI_KEY_ENTER;
            else if (changed & 16) jkey = UI_KEY_SPACE;
            else if (changed & 1) jkey = UI_KEY_UP;
            else if (changed & 2) jkey = UI_KEY_DOWN;
            else if (changed & 8) jkey = UI_KEY_LEFT;
            else if (changed & 4) jkey = UI_KEY_RIGHT;

            if (jkey != 0) {
                kbormouse = 0;
                memset(&ev, 0, sizeof(ev));
                ev.type = UI_EVENT_KEY_DOWN;
                ev.key  = jkey;
                ev.delta = delta;
                result = scr->on_event(scr, &ev);
                if (result != 0) return result;
            }
            joy_repeat_elapsed = 0;
            joy_repeat_started = 0;
        } else if (joy != 0) {
            unsigned short jkey = 0;
            unsigned short repeat_threshold = joy_repeat_started ? UI_INPUT_REPEAT_MS : UI_INPUT_REPEAT_INITIAL_MS;

            if (joy_repeat_elapsed + delta < repeat_threshold) {
                joy_repeat_elapsed = (unsigned short)(joy_repeat_elapsed + delta);
            } else {
                joy_repeat_elapsed = 0;
                joy_repeat_started = 1;

                if (joy & 32) jkey = UI_KEY_ENTER;
                else if (joy & 16) jkey = UI_KEY_SPACE;
                else if (joy & 1) jkey = UI_KEY_UP;
                else if (joy & 2) jkey = UI_KEY_DOWN;
                else if (joy & 8) jkey = UI_KEY_LEFT;
                else if (joy & 4) jkey = UI_KEY_RIGHT;

                if (jkey != 0) {
                    kbormouse = 0;
                    memset(&ev, 0, sizeof(ev));
                    ev.type = UI_EVENT_KEY_DOWN;
                    ev.key  = jkey;
                    ev.delta = delta;
                    result = scr->on_event(scr, &ev);
                    if (result != 0) return result;
                }
            }
        } else {
            joy_repeat_elapsed = 0;
            joy_repeat_started = 0;
        }
    }

    /* --- mouse events --- */
    mouse_get_state(&mbut, &mx, &my);

    if (mx != prev_mx || my != prev_my) {
        kbormouse = 1;
        prev_mx = mx;
        prev_my = my;
        memset(&ev, 0, sizeof(ev));
        ev.type = UI_EVENT_MOUSE_MOVE;
        ev.mouse_x = mx;
        ev.mouse_y = my;
        ev.mouse_buttons = mbut;
        ev.delta = delta;
        result = scr->on_event(scr, &ev);
        if (result != 0) return result;
    }

    if (mbut != prev_mbut) {
        unsigned short pressed  = (mbut & ~prev_mbut);
        unsigned short released = (prev_mbut & ~mbut);
        prev_mbut = mbut;

        if (pressed) {
            kbormouse = 1;
            memset(&ev, 0, sizeof(ev));
            ev.type = UI_EVENT_MOUSE_DOWN;
            ev.mouse_x = mx;
            ev.mouse_y = my;
            ev.mouse_buttons = pressed;
            ev.delta = delta;
            result = scr->on_event(scr, &ev);
            if (result != 0) return result;
        }
        if (released) {
            memset(&ev, 0, sizeof(ev));
            ev.type = UI_EVENT_MOUSE_UP;
            ev.mouse_x = mx;
            ev.mouse_y = my;
            ev.mouse_buttons = released;
            ev.delta = delta;
            result = scr->on_event(scr, &ev);
            if (result != 0) return result;
        }
    }

    /* --- tick event (always sent) --- */
    memset(&ev, 0, sizeof(ev));
    ev.type = UI_EVENT_TICK;
    ev.delta = delta;
    ev.mouse_x = mx;
    ev.mouse_y = my;
    ev.mouse_buttons = mbut;
    result = scr->on_event(scr, &ev);
    return result;
}

/* ------------------------------------------------------------------ */
/* Run loops                                                           */
/* ------------------------------------------------------------------ */

void ui_screen_run(void)
{
    while (s_depth > 0) {
        UIScreen *top = s_stack[s_depth - 1];
        unsigned short delta;
        int result;

        /* 1. Pump SDL events into the DOS-compat input layer */
        kb_poll_sdl_input();

        /* 2. Get frame timing */
        delta = (unsigned short)timer_get_delta_alt();
        if (delta == 0) delta = 1;

        /* 3. Dispatch events to the top screen */
        result = dispatch_events(top, delta);

        if (result != 0 || top->_wants_pop) {
            top->_modal_result = result;
            ui_screen_pop();
            continue;
        }

        /* 4. Render (screen is responsible for calling video_refresh,
         *    typically via mouse_update_menu_blink) */
        if (top->on_render) {
            top->on_render(top);
        }
    }
}

int ui_screen_run_modal(UIScreen *scr)
{
    int saved_depth = s_depth;

    ui_screen_push(scr);

    /* Run until this screen pops (depth drops back to saved_depth) */
    while (s_depth > saved_depth) {
        UIScreen *top = s_stack[s_depth - 1];
        unsigned short delta;
        int result;

        kb_poll_sdl_input();

        delta = (unsigned short)timer_get_delta_alt();
        if (delta == 0) delta = 1;

        result = dispatch_events(top, delta);

        if (result != 0 || top->_wants_pop) {
            int modal = result ? result : top->_modal_result;
            ui_screen_pop();
            if (s_depth <= saved_depth) {
                return modal;
            }
            continue;
        }

        if (top->on_render) {
            top->on_render(top);
        }
    }

    return 0;
}

/* ------------------------------------------------------------------ */
/* Button menu screen adapter                                          */
/* ------------------------------------------------------------------ */

/*
 * Wraps a UIButtonMenu into the event-driven screen model.
 * The blink/hittest/navigation logic mirrors ui_button_menu_run()
 * but is driven by UIEvents instead of a polling loop.
 */

typedef struct {
    UIButtonMenu *menu;
    unsigned char  selected;
    unsigned char  prev_selected;
    unsigned short idle_counter;
    unsigned char  initialized;
} BtnMenuScreenState;

static unsigned char ui_nav_geometric(
    unsigned char current,
    int dir_x,
    int dir_y,
    const unsigned short *x1,
    const unsigned short *x2,
    const unsigned short *y1,
    const unsigned short *y2,
    unsigned short count)
{
    unsigned short best = current;
    unsigned short best_primary = 65535;
    unsigned short best_secondary = 65535;
    int cur_x = (int)(x1[current] + x2[current]) / 2;
    int cur_y = (int)(y1[current] + y2[current]) / 2;
    unsigned short i;

    for (i = 0; i < count; ++i) {
        int dx;
        int dy;
        unsigned short primary;
        unsigned short secondary;

        if (i == current) {
            continue;
        }

        dx = ((int)(x1[i] + x2[i]) / 2) - cur_x;
        dy = ((int)(y1[i] + y2[i]) / 2) - cur_y;

        if (dir_x != 0) {
            if (dx * dir_x <= 0) {
                continue;
            }
            primary = (unsigned short)((dx < 0) ? -dx : dx);
            secondary = (unsigned short)((dy < 0) ? -dy : dy);
        } else if (dir_y != 0) {
            if (dy * dir_y <= 0) {
                continue;
            }
            primary = (unsigned short)((dy < 0) ? -dy : dy);
            secondary = (unsigned short)((dx < 0) ? -dx : dx);
        } else {
            continue;
        }

        if (primary < best_primary || (primary == best_primary && secondary < best_secondary)) {
            best = i;
            best_primary = primary;
            best_secondary = secondary;
        }
    }

    return (unsigned char)best;
}

static int btnmenu_on_event(UIScreen *self, const UIEvent *ev)
{
    BtnMenuScreenState *st = (BtnMenuScreenState *)self->userdata;
    UIButtonMenu *m = st->menu;

    if (ev->type == UI_EVENT_ENTER) {
        if (!st->initialized) {
            st->selected = m->default_sel;
            if (st->selected >= m->count) st->selected = 0;
            st->prev_selected = 255;
            st->idle_counter = 0;
            st->initialized = 1;
            check_input();
            mousebutinputcode = 0;
            joyinputcode = 0;
        }
        return 0;
    }

    if (ev->type == UI_EVENT_KEY_DOWN) {
        unsigned short key = ev->key;

        st->idle_counter = 0;
        menu_reset_idle_timers();

        /* confirm */
        if (UI_IS_CONFIRM(key)) {
            return (int)st->selected + 1;
        }
        /* cancel */
        if (UI_IS_CANCEL(key)) {
            return (int)UI_SEL_CANCEL + 1;
        }

        /* navigation */
        if (m->nav_mode == UI_NAV_HORIZONTAL) {
            if (key == UI_KEY_LEFT) {
                st->selected = (st->selected == 0)
                    ? (unsigned char)(m->count - 1)
                    : (unsigned char)(st->selected - 1);
            } else if (key == UI_KEY_RIGHT) {
                st->selected = (st->selected >= m->count - 1)
                    ? 0 : (unsigned char)(st->selected + 1);
            }
        } else if (m->nav_mode == UI_NAV_VERTICAL) {
            if (key == UI_KEY_UP) {
                st->selected = (st->selected == 0)
                    ? (unsigned char)(m->count - 1)
                    : (unsigned char)(st->selected - 1);
            } else if (key == UI_KEY_DOWN) {
                st->selected = (st->selected >= m->count - 1)
                    ? 0 : (unsigned char)(st->selected + 1);
            }
        } else if (m->nav_mode == UI_NAV_BOTH_LR_SWAP) {
            if (key == UI_KEY_RIGHT || key == UI_KEY_UP) {
                st->selected = (st->selected == 0)
                    ? (unsigned char)(m->count - 1)
                    : (unsigned char)(st->selected - 1);
            } else if (key == UI_KEY_LEFT || key == UI_KEY_DOWN) {
                st->selected = (st->selected >= m->count - 1)
                    ? 0 : (unsigned char)(st->selected + 1);
            }
        } else if (m->nav_mode == UI_NAV_GEOMETRIC) {
            if (key == UI_KEY_LEFT) {
                st->selected = ui_nav_geometric(st->selected, -1, 0,
                    m->x1, m->x2, m->y1, m->y2, m->count);
            } else if (key == UI_KEY_RIGHT) {
                st->selected = ui_nav_geometric(st->selected, 1, 0,
                    m->x1, m->x2, m->y1, m->y2, m->count);
            } else if (key == UI_KEY_UP) {
                st->selected = ui_nav_geometric(st->selected, 0, -1,
                    m->x1, m->x2, m->y1, m->y2, m->count);
            } else if (key == UI_KEY_DOWN) {
                st->selected = ui_nav_geometric(st->selected, 0, 1,
                    m->x1, m->x2, m->y1, m->y2, m->count);
            }
        } else {
            if (UI_IS_NAV_PREV(key)) {
                st->selected = (st->selected == 0)
                    ? (unsigned char)(m->count - 1)
                    : (unsigned char)(st->selected - 1);
            } else if (UI_IS_NAV_NEXT(key)) {
                st->selected = (st->selected >= m->count - 1)
                    ? 0 : (unsigned char)(st->selected + 1);
            }
        }
        return 0;
    }

    if (ev->type == UI_EVENT_MOUSE_DOWN || ev->type == UI_EVENT_MOUSE_MOVE) {
        short hit = mouse_multi_hittest(
            (short)m->count,
            m->x1, m->x2, m->y1, m->y2
        );
        if (hit >= 0 && hit < (short)m->count) {
            if (kbormouse != 0) {
                st->selected = (unsigned char)hit;
            }
            if (ev->type == UI_EVENT_MOUSE_DOWN && kbormouse != 0) {
                return (int)st->selected + 1;
            }
        }
        return 0;
    }

    if (ev->type == UI_EVENT_TICK) {
        /* selection change callback */
        if (st->prev_selected != st->selected) {
            if (m->selection_cb) {
                m->selection_cb(st->selected, m->selection_cb_ctx);
            }
            st->prev_selected = st->selected;
            menu_reset_idle_timers();
            st->idle_counter = 0;
        }

        /* frame callback */
        if (m->frame_cb) {
            unsigned short cb_key = m->frame_cb(st->selected, ev->delta, m->frame_cb_ctx);
            if (cb_key != 0) {
                if (UI_IS_CONFIRM(cb_key)) return (int)st->selected + 1;
                if (UI_IS_CANCEL(cb_key)) return (int)UI_SEL_CANCEL + 1;
            }
        }

        /* idle timeout */
        if (m->idle_timeout != 0) {
            st->idle_counter += ev->delta;
            idle_counter += ev->delta;
            if (idle_expired != 0 || st->idle_counter >= m->idle_timeout) {
                idle_expired++;
                return (int)UI_SEL_TIMEOUT + 1;
            }
        }

        /* blink */
        mouse_update_menu_blink(
            st->selected,
            m->x1, m->x2, m->y1, m->y2,
            m->sprite_hi, m->sprite_lo
        );
        return 0;
    }

    return 0;
}

static void btnmenu_on_destroy(UIScreen *self)
{
    if (self->userdata) {
        free(self->userdata);
        self->userdata = NULL;
    }
}

UIScreen *ui_screen_from_button_menu(UIButtonMenu *menu)
{
    UIScreen *scr;
    BtnMenuScreenState *st;

    scr = ui_screen_alloc();
    st = (BtnMenuScreenState *)calloc(1, sizeof(BtnMenuScreenState));
    st->menu = menu;

    scr->on_event   = btnmenu_on_event;
    scr->on_destroy = btnmenu_on_destroy;
    scr->userdata   = st;

    return scr;
}
