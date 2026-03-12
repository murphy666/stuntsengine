/*
 * ui_screen.h - SDL2-style event-driven screen system for Stunts UI
 *
 * Replaces the per-menu while(1) polling loops with a proper
 * screen stack.  Each screen provides callbacks for events, updates,
 * and rendering.  A single run-loop drives them all.
 *
 * Usage pattern:
 *
 *   // Define a screen
 *   static int my_screen_event(UIScreen *scr, const UIEvent *ev);
 *   static void my_screen_render(UIScreen *scr);
 *   static void my_screen_destroy(UIScreen *scr);
 *
 *   UIScreen *make_my_screen(void) {
 *       UIScreen *s = ui_screen_alloc();
 *       s->on_event  = my_screen_event;
 *       s->on_render = my_screen_render;
 *       s->on_destroy = my_screen_destroy;
 *       s->userdata  = my_state;
 *       return s;
 *   }
 *
 *   // Push it
 *   ui_screen_push(make_my_screen());
 *
 *   // From inside a blocking context that needs a result:
 *   int result = ui_screen_run_modal(make_my_screen());
 */

#ifndef UI_SCREEN_H_
#define UI_SCREEN_H_

/* -------- Event types ----------------------------------------------- */

#define UI_EVENT_NONE        0
#define UI_EVENT_KEY_DOWN    1   /* key pressed                         */
#define UI_EVENT_KEY_UP      2   /* key released                        */
#define UI_EVENT_MOUSE_MOVE  3   /* mouse moved                         */
#define UI_EVENT_MOUSE_DOWN  4   /* mouse button pressed                */
#define UI_EVENT_MOUSE_UP    5   /* mouse button released               */
#define UI_EVENT_TICK        6   /* timer tick (per-frame update)        */
#define UI_EVENT_ENTER       7   /* screen became topmost               */
#define UI_EVENT_LEAVE       8   /* screen is no longer topmost         */

/*
 * UIEvent — unified input event, replacing the scattered polling of
 * kb_get_char / mouse_get_state / get_joy_flags.
 */
typedef struct {
    unsigned short type;           /* UI_EVENT_* constant               */
    unsigned short key;            /* DOS BIOS key code (for KEY_DOWN/UP) */
    unsigned short mouse_x;        /* Current mouse X                   */
    unsigned short mouse_y;        /* Current mouse Y                   */
    unsigned short mouse_buttons;  /* Bitmask: bit0=left, bit1=right    */
    unsigned short delta;          /* Frame delta (for TICK events)      */
} UIEvent;

/* -------- Screen struct --------------------------------------------- */

/*
 * UIScreen — one "page" of UI.  Pushed onto a stack.
 *
 * on_event:   Handle one event.  Return 0 to keep running, non-zero
 *             to pop the screen.  The return value becomes the modal
 *             result for ui_screen_run_modal().
 *
 * on_render:  Draw one frame.  Called after events are dispatched.
 *             The screen should blit its content; video_refresh()
 *             is called automatically by the run-loop afterwards.
 *
 * on_destroy: Free any resources owned by this screen.
 */
typedef struct UIScreen UIScreen;
struct UIScreen {
    int  (*on_event)(UIScreen *self, const UIEvent *ev);
    void (*on_render)(UIScreen *self);
    void (*on_destroy)(UIScreen *self);
    void *userdata;

    /* private — managed by the screen stack */
    int   _modal_result;
    unsigned char _wants_pop;
};

/* -------- Screen lifecycle ------------------------------------------ */

/*
 * Allocate a zeroed UIScreen.  The caller fills in the callbacks.
 */
UIScreen *ui_screen_alloc(void);

/*
 * Free a screen (calls on_destroy if set, then frees memory).
 */
void ui_screen_free(UIScreen *scr);

/* -------- Screen stack ---------------------------------------------- */

#define UI_SCREEN_STACK_MAX 8

/*
 * Push a screen onto the stack.  It becomes the active screen.
 * The previous screen receives a UI_EVENT_LEAVE.
 * The new screen receives a UI_EVENT_ENTER.
 */
void ui_screen_push(UIScreen *scr);

/*
 * Pop the topmost screen.  It receives on_destroy and is freed.
 * The screen below (if any) receives UI_EVENT_ENTER.
 */
void ui_screen_pop(void);

/*
 * Return the topmost screen, or NULL if the stack is empty.
 */
UIScreen *ui_screen_top(void);

/*
 * Return the current stack depth.
 */
int ui_screen_depth(void);

/* -------- Run loop -------------------------------------------------- */

/*
 * Run the screen loop until the stack is empty.
 *
 * Each iteration:
 *   1. Poll SDL events via kb_poll_sdl_input()
 *   2. Convert to UIEvent and dispatch to the top screen's on_event
 *   3. Send a UI_EVENT_TICK with the frame delta
 *   4. Call on_render on the top screen
 *   5. Present the frame (mouse blink, video_refresh)
 *
 * This replaces all the individual while(1) loops.
 */
void ui_screen_run(void);

/*
 * Push a screen and run until it pops itself.  Returns the modal
 * result (the non-zero value returned by on_event that triggered
 * the pop).
 *
 * This is the bridge for existing code that calls menu functions
 * synchronously: instead of `run_opponent_menu()` blocking in its
 * own loop, it does `result = ui_screen_run_modal(opponent_screen)`.
 */
int ui_screen_run_modal(UIScreen *scr);

/* -------- Button menu screen helper --------------------------------- */

/*
 * Create a ready-to-use button menu screen from a UIButtonMenu
 * definition.  This replaces ui_button_menu_run() for the new
 * event-driven model.
 *
 * The screen internally handles blink, hittest, navigation, and
 * idle timeout.  on_event returns the button index + 1 (so 0 stays
 * "keep running"), UI_SEL_CANCEL+1, or UI_SEL_TIMEOUT+1.
 *
 * After ui_screen_run_modal(), decode: result-1 gives the original
 * UIButtonMenu return value.
 *
 * Optional callbacks (frame_cb, selection_cb) are forwarded from
 * the UIButtonMenu struct.
 */
typedef struct UIButtonMenu UIButtonMenu;
UIScreen *ui_screen_from_button_menu(UIButtonMenu *menu);

/* Decode the modal result back to a UIButtonMenu result value */
#define UI_MODAL_TO_SEL(modal_result) ((unsigned char)((modal_result) - 1))

#endif /* UI_SCREEN_H_ */
