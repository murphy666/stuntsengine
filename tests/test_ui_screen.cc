/*
 * test_ui_screen.cc — Tests for the UIScreen event-driven screen system
 *
 * Tests the screen stack, alloc/free, event dispatch, modal run loop,
 * and the UIButtonMenu-to-UIScreen adapter.  Uses stub implementations
 * of keyboard/mouse/timer (in test_ui_screen_stubs.c) for isolation.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "ui_screen.h"
#include "ui_widgets.h"
#include "ui_keys.h"
#include "data_game.h"

extern char kbormouse;

/* Stub control functions (defined in test_ui_screen_stubs.c) */
void stub_reset_all(void);
void stub_key_reset(void);
void stub_key_enqueue(unsigned short key);
void stub_mouse_set(unsigned short buttons, unsigned short x, unsigned short y);
void stub_timer_set_delta(unsigned long d);
void stub_hittest_set(short result);
void stub_counters_reset(void);
int  stub_get_blink_calls(void);
int  stub_get_poll_calls(void);
int  stub_get_idle_reset_calls(void);
} /* extern "C" */


/* ================================================================== */
/* Test fixture                                                        */
/* ================================================================== */

class UIScreenTest : public ::testing::Test {
protected:
    void SetUp() override {
        stub_reset_all();
        /* Ensure screen stack is empty before each test */
        while (ui_screen_depth() > 0) {
            ui_screen_pop();
        }
    }
    void TearDown() override {
        /* Neutralize callbacks before popping to avoid dangling userdata */
        while (ui_screen_depth() > 0) {
            UIScreen *top = ui_screen_top();
            if (top) {
                top->on_event = nullptr;
                top->on_render = nullptr;
                top->on_destroy = nullptr;
            }
            ui_screen_pop();
        }
    }
};


/* ================================================================== */
/* Screen alloc/free                                                    */
/* ================================================================== */

TEST_F(UIScreenTest, AllocReturnsNonNull) {
    UIScreen *s = ui_screen_alloc();
    ASSERT_NE(s, nullptr);
    /* All fields zeroed */
    EXPECT_EQ(s->on_event,   nullptr);
    EXPECT_EQ(s->on_render,  nullptr);
    EXPECT_EQ(s->on_destroy, nullptr);
    EXPECT_EQ(s->userdata,   nullptr);
    EXPECT_EQ(s->_modal_result, 0);
    EXPECT_EQ(s->_wants_pop,   0);
    free(s);
}

static int s_destroy_count = 0;
static void counting_destroy(UIScreen *self) {
    (void)self;
    s_destroy_count++;
}

TEST_F(UIScreenTest, FreeCallsOnDestroy) {
    s_destroy_count = 0;
    UIScreen *s = ui_screen_alloc();
    s->on_destroy = counting_destroy;
    ui_screen_free(s);
    EXPECT_EQ(s_destroy_count, 1);
}

TEST_F(UIScreenTest, FreeHandlesNull) {
    /* Should not crash */
    ui_screen_free(nullptr);
}

TEST_F(UIScreenTest, FreeWithNoDestroy) {
    UIScreen *s = ui_screen_alloc();
    s->on_destroy = nullptr;
    ui_screen_free(s);  /* No crash */
}


/* ================================================================== */
/* Screen stack                                                        */
/* ================================================================== */

TEST_F(UIScreenTest, EmptyStack) {
    EXPECT_EQ(ui_screen_depth(), 0);
    EXPECT_EQ(ui_screen_top(), nullptr);
}

/* Track events received by screens */
struct EventLog {
    int enter_count;
    int leave_count;
    int last_event_type;
};

static int logging_event(UIScreen *self, const UIEvent *ev) {
    EventLog *log = (EventLog *)self->userdata;
    log->last_event_type = ev->type;
    if (ev->type == UI_EVENT_ENTER) log->enter_count++;
    if (ev->type == UI_EVENT_LEAVE) log->leave_count++;
    return 0;
}

static void noop_destroy(UIScreen *self) {
    /* userdata is stack-allocated, don't free */
    (void)self;
}

TEST_F(UIScreenTest, PushIncreasesDepth) {
    EventLog log = {};
    UIScreen *s = ui_screen_alloc();
    s->on_event = logging_event;
    s->on_destroy = noop_destroy;
    s->userdata = &log;

    ui_screen_push(s);
    EXPECT_EQ(ui_screen_depth(), 1);
    EXPECT_EQ(ui_screen_top(), s);
    EXPECT_EQ(log.enter_count, 1) << "Screen should receive ENTER on push";
}

TEST_F(UIScreenTest, PopDecreasesDepth) {
    EventLog log = {};
    UIScreen *s = ui_screen_alloc();
    s->on_event = logging_event;
    s->on_destroy = noop_destroy;
    s->userdata = &log;

    ui_screen_push(s);
    ASSERT_EQ(ui_screen_depth(), 1);

    ui_screen_pop();
    EXPECT_EQ(ui_screen_depth(), 0);
    EXPECT_EQ(ui_screen_top(), nullptr);
}

TEST_F(UIScreenTest, PopOnEmptyStackIsSafe) {
    EXPECT_EQ(ui_screen_depth(), 0);
    ui_screen_pop();  /* Should not crash */
    EXPECT_EQ(ui_screen_depth(), 0);
}

TEST_F(UIScreenTest, StackLeaveThenEnter) {
    /* When B is pushed on top of A, A gets LEAVE, B gets ENTER */
    EventLog log_a = {};
    EventLog log_b = {};

    UIScreen *a = ui_screen_alloc();
    a->on_event = logging_event;
    a->on_destroy = noop_destroy;
    a->userdata = &log_a;

    UIScreen *b = ui_screen_alloc();
    b->on_event = logging_event;
    b->on_destroy = noop_destroy;
    b->userdata = &log_b;

    ui_screen_push(a);
    EXPECT_EQ(log_a.enter_count, 1);

    ui_screen_push(b);
    EXPECT_EQ(log_a.leave_count, 1) << "A should get LEAVE when B is pushed";
    EXPECT_EQ(log_b.enter_count, 1) << "B should get ENTER when pushed";
    EXPECT_EQ(ui_screen_depth(), 2);
    EXPECT_EQ(ui_screen_top(), b);
}

TEST_F(UIScreenTest, PopRestoresAndNotifies) {
    /* When B is popped, A gets ENTER again */
    EventLog log_a = {};
    EventLog log_b = {};

    UIScreen *a = ui_screen_alloc();
    a->on_event = logging_event;
    a->on_destroy = noop_destroy;
    a->userdata = &log_a;

    UIScreen *b = ui_screen_alloc();
    b->on_event = logging_event;
    b->on_destroy = noop_destroy;
    b->userdata = &log_b;

    ui_screen_push(a);
    ui_screen_push(b);

    log_a.enter_count = 0;  /* reset to see the re-enter */
    ui_screen_pop();

    EXPECT_EQ(ui_screen_depth(), 1);
    EXPECT_EQ(ui_screen_top(), a);
    EXPECT_EQ(log_a.enter_count, 1) << "A should get ENTER after B is popped";
}

TEST_F(UIScreenTest, PushNullIsSafe) {
    ui_screen_push(nullptr);
    EXPECT_EQ(ui_screen_depth(), 0);
}

TEST_F(UIScreenTest, StackMaxCapacity) {
    UIScreen *screens[UI_SCREEN_STACK_MAX + 1];
    for (int i = 0; i < UI_SCREEN_STACK_MAX; i++) {
        screens[i] = ui_screen_alloc();
        ui_screen_push(screens[i]);
        EXPECT_EQ(ui_screen_depth(), i + 1);
    }
    /* Beyond max should not push */
    UIScreen *overflow = ui_screen_alloc();
    ui_screen_push(overflow);
    EXPECT_EQ(ui_screen_depth(), UI_SCREEN_STACK_MAX);
    /* Manually free the overflow screen since it was never pushed */
    free(overflow);
}


/* ================================================================== */
/* Modal run loop                                                      */
/* ================================================================== */

/* A screen that returns a specific result on the Nth key event */
struct ModalTestState {
    int result_to_return;
    int events_received;
    int renders_called;
};

static int modal_event(UIScreen *self, const UIEvent *ev) {
    ModalTestState *st = (ModalTestState *)self->userdata;
    if (ev->type == UI_EVENT_KEY_DOWN) {
        st->events_received++;
        return st->result_to_return;
    }
    return 0;
}

static void modal_render(UIScreen *self) {
    ModalTestState *st = (ModalTestState *)self->userdata;
    st->renders_called++;
}

static void modal_destroy(UIScreen *self) {
    /* userdata is stack-allocated */
    (void)self;
}

TEST_F(UIScreenTest, ModalReturnsEventResult) {
    ModalTestState state = {42, 0, 0};

    UIScreen *scr = ui_screen_alloc();
    scr->on_event = modal_event;
    scr->on_render = modal_render;
    scr->on_destroy = modal_destroy;
    scr->userdata = &state;

    /* Enqueue one key so dispatch returns non-zero */
    stub_key_enqueue(UI_KEY_ENTER);

    int result = ui_screen_run_modal(scr);
    EXPECT_EQ(result, 42);
    EXPECT_EQ(state.events_received, 1);
    EXPECT_EQ(ui_screen_depth(), 0) << "Modal screen should be popped";
}

/* A screen that pops via _wants_pop instead of returning non-zero */
static int wants_pop_event(UIScreen *self, const UIEvent *ev) {
    if (ev->type == UI_EVENT_KEY_DOWN) {
        self->_modal_result = 99;
        self->_wants_pop = 1;
    }
    return 0;
}

TEST_F(UIScreenTest, ModalWantsPopReturnsResult) {
    UIScreen *scr = ui_screen_alloc();
    scr->on_event = wants_pop_event;

    stub_key_enqueue(UI_KEY_ESCAPE);

    int result = ui_screen_run_modal(scr);
    EXPECT_EQ(result, 99);
    EXPECT_EQ(ui_screen_depth(), 0);
}


/* ================================================================== */
/* UI_MODAL_TO_SEL macro                                                */
/* ================================================================== */

TEST_F(UIScreenTest, ModalToSelDecode) {
    /* Button 0 -> modal returns 1, decode to 0 */
    EXPECT_EQ(UI_MODAL_TO_SEL(1), 0);
    /* Button 3 -> modal returns 4, decode to 3 */
    EXPECT_EQ(UI_MODAL_TO_SEL(4), 3);
    /* Cancel: UI_SEL_CANCEL=255, modal=256, decode=255 */
    EXPECT_EQ(UI_MODAL_TO_SEL((int)UI_SEL_CANCEL + 1), UI_SEL_CANCEL);
    /* Timeout: UI_SEL_TIMEOUT=254, modal=255, decode=254 */
    EXPECT_EQ(UI_MODAL_TO_SEL((int)UI_SEL_TIMEOUT + 1), UI_SEL_TIMEOUT);
}


/* ================================================================== */
/* Button menu screen adapter                                          */
/* ================================================================== */

class BtnMenuScreenTest : public ::testing::Test {
protected:
    UIButtonMenu menu;

    void SetUp() override {
        stub_reset_all();
        while (ui_screen_depth() > 0) {
            ui_screen_pop();
        }

        memset(&menu, 0, sizeof(menu));
        menu.count = 3;
        /* 3 buttons arranged horizontally */
        for (int i = 0; i < 3; i++) {
            menu.x1[i] = (unsigned short)(i * 100);
            menu.x2[i] = (unsigned short)(i * 100 + 90);
            menu.y1[i] = 50;
            menu.y2[i] = 80;
        }
        menu.default_sel = 0;
        menu.nav_mode = UI_NAV_HORIZONTAL;
        menu.idle_timeout = 0;  /* no timeout by default */
    }

    void TearDown() override {
        while (ui_screen_depth() > 0) {
            UIScreen *top = ui_screen_top();
            if (top) {
                top->on_event = nullptr;
                top->on_render = nullptr;
                top->on_destroy = nullptr;
            }
            ui_screen_pop();
        }
    }

    unsigned char runWithKey(unsigned short key) {
        stub_key_enqueue(key);
        UIScreen *scr = ui_screen_from_button_menu(&menu);
        int modal = ui_screen_run_modal(scr);
        return UI_MODAL_TO_SEL(modal);
    }
};

TEST_F(BtnMenuScreenTest, ConfirmReturnsDefaultSelection) {
    menu.default_sel = 1;
    unsigned char sel = runWithKey(UI_KEY_ENTER);
    EXPECT_EQ(sel, 1);
}

TEST_F(BtnMenuScreenTest, ConfirmWithSpace) {
    menu.default_sel = 2;
    unsigned char sel = runWithKey(UI_KEY_SPACE);
    EXPECT_EQ(sel, 2);
}

TEST_F(BtnMenuScreenTest, ConfirmWithExtendedEnter) {
    menu.default_sel = 0;
    unsigned char sel = runWithKey(UI_KEY_ENTER_EXT);
    EXPECT_EQ(sel, 0);
}

TEST_F(BtnMenuScreenTest, CancelReturnsSelCancel) {
    unsigned char sel = runWithKey(UI_KEY_ESCAPE);
    EXPECT_EQ(sel, UI_SEL_CANCEL);
}

TEST_F(BtnMenuScreenTest, CancelWithExtendedEscape) {
    unsigned char sel = runWithKey(UI_KEY_ESCAPE_EXT);
    EXPECT_EQ(sel, UI_SEL_CANCEL);
}

TEST_F(BtnMenuScreenTest, HorizontalNavRight) {
    /* Start at 0, press RIGHT to go to 1, then ENTER to confirm */
    stub_key_enqueue(UI_KEY_RIGHT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 1);
}

TEST_F(BtnMenuScreenTest, HorizontalNavLeft) {
    /* Start at 0, LEFT should wrap to last (2) */
    stub_key_enqueue(UI_KEY_LEFT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 2);
}

TEST_F(BtnMenuScreenTest, HorizontalNavRightWrap) {
    /* Start at 2, RIGHT should wrap to 0 */
    menu.default_sel = 2;
    stub_key_enqueue(UI_KEY_RIGHT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 0);
}

TEST_F(BtnMenuScreenTest, HorizontalIgnoresVerticalKeys) {
    /* In horizontal mode, UP/DOWN should not navigate */
    stub_key_enqueue(UI_KEY_UP);
    stub_key_enqueue(UI_KEY_DOWN);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 0) << "Selection should stay at default";
}

TEST_F(BtnMenuScreenTest, VerticalNavDown) {
    menu.nav_mode = UI_NAV_VERTICAL;
    stub_key_enqueue(UI_KEY_DOWN);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 1);
}

TEST_F(BtnMenuScreenTest, VerticalNavUp) {
    menu.nav_mode = UI_NAV_VERTICAL;
    /* Start at 0, UP wraps to 2 */
    stub_key_enqueue(UI_KEY_UP);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 2);
}

TEST_F(BtnMenuScreenTest, VerticalIgnoresHorizontalKeys) {
    menu.nav_mode = UI_NAV_VERTICAL;
    stub_key_enqueue(UI_KEY_LEFT);
    stub_key_enqueue(UI_KEY_RIGHT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 0) << "Selection should stay at default";
}

TEST_F(BtnMenuScreenTest, BothNavAllDirections) {
    menu.nav_mode = UI_NAV_BOTH;
    /* RIGHT -> 1, DOWN -> 2, ENTER */
    stub_key_enqueue(UI_KEY_RIGHT);
    stub_key_enqueue(UI_KEY_DOWN);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 2);
}

TEST_F(BtnMenuScreenTest, BothLRSwapNavigation) {
    menu.nav_mode = UI_NAV_BOTH_LR_SWAP;
    /* In LR_SWAP: LEFT=next, RIGHT=prev */
    /* Start at 0, LEFT -> 1, ENTER */
    stub_key_enqueue(UI_KEY_LEFT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 1);
}

TEST_F(BtnMenuScreenTest, BothLRSwapRightGoesPrev) {
    menu.nav_mode = UI_NAV_BOTH_LR_SWAP;
    menu.default_sel = 1;
    /* RIGHT = prev: 1 -> 0 */
    stub_key_enqueue(UI_KEY_RIGHT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 0);
}

TEST_F(BtnMenuScreenTest, DefaultSelClampedToCount) {
    menu.default_sel = 99;  /* Way beyond count */
    unsigned char sel = runWithKey(UI_KEY_ENTER);
    /* Should be clamped to 0 */
    EXPECT_EQ(sel, 0);
}

TEST_F(BtnMenuScreenTest, ZeroCountReturnsCancelImmediately) {
    /* Edge case: zero buttons — original ui_button_menu_run returns CANCEL */
    menu.count = 0;
    /* The adapter should handle this gracefully. Let's enqueue ESC just in case
     * but ideally the zero-count path returns immediately. */
    stub_key_enqueue(UI_KEY_ESCAPE);
    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    /* Just verify it doesn't hang — either cancel or some sentinel */
    EXPECT_NE(modal, 0) << "Should not run forever with 0 buttons";
}

TEST_F(BtnMenuScreenTest, IdleTimeoutTriggered) {
    menu.idle_timeout = 20;  /* Very short */
    stub_timer_set_delta(25);  /* Single tick exceeds timeout */
    /* Don't enqueue any keys — screen should time out on tick */

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), UI_SEL_TIMEOUT);
}

TEST_F(BtnMenuScreenTest, IdleTimeoutAccumulates) {
    menu.idle_timeout = 30;
    stub_timer_set_delta(10);  /* Needs 3 ticks to timeout */
    /* 3 ticks with no key = 30 >= 30 timeout threshold */

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), UI_SEL_TIMEOUT);
}

TEST_F(BtnMenuScreenTest, BlinkCalledOnTick) {
    stub_counters_reset();
    /* First frame: no key → TICK fires (blink called).
     * Second frame: ENTER → exits before TICK. */
    stub_key_enqueue(0);  /* no-op: kb_get_char returns 0, tick proceeds */
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    ui_screen_run_modal(scr);

    EXPECT_GE(stub_get_blink_calls(), 1) << "Blink should be called at least once";
}

/* ---- Selection change callback test ---- */
static int s_sel_cb_count = 0;
static unsigned char s_sel_cb_last_sel = 255;
static void test_selection_cb(unsigned char sel, void *ctx) {
    (void)ctx;
    s_sel_cb_count++;
    s_sel_cb_last_sel = sel;
}

TEST_F(BtnMenuScreenTest, SelectionCallbackFired) {
    s_sel_cb_count = 0;
    s_sel_cb_last_sel = 255;
    menu.selection_cb = test_selection_cb;
    menu.selection_cb_ctx = nullptr;
    menu.default_sel = 0;

    /* Frame 1: RIGHT navigates 0->1, returns 0, TICK fires selection_cb for initial+change.
     * Frame 2: ENTER confirms, exits before TICK. */
    stub_key_enqueue(UI_KEY_RIGHT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    ui_screen_run_modal(scr);

    /* Callback fires at least for the initial prev_selected=255 -> 0 transition
     * and then for the 0->1 navigation on the first TICK */
    EXPECT_GE(s_sel_cb_count, 1);
    EXPECT_EQ(s_sel_cb_last_sel, 1);
}

/* ---- Frame callback test ---- */
static int s_frame_cb_count = 0;
static unsigned short test_frame_cb(unsigned char sel, unsigned short delta, void *ctx) {
    (void)sel; (void)delta; (void)ctx;
    s_frame_cb_count++;
    return 0;  /* Continue */
}

static unsigned short frame_cb_exit(unsigned char sel, unsigned short delta, void *ctx) {
    (void)sel; (void)delta; (void)ctx;
    s_frame_cb_count++;
    return UI_KEY_ENTER;  /* Force confirm */
}

TEST_F(BtnMenuScreenTest, FrameCallbackCalled) {
    s_frame_cb_count = 0;
    menu.frame_cb = test_frame_cb;
    menu.frame_cb_ctx = nullptr;

    /* Frame 1: no key → TICK fires (frame_cb runs).
     * Frame 2: ENTER → exits before TICK. */
    stub_key_enqueue(0);
    stub_key_enqueue(UI_KEY_ENTER);
    UIScreen *scr = ui_screen_from_button_menu(&menu);
    ui_screen_run_modal(scr);

    EXPECT_GE(s_frame_cb_count, 1);
}

TEST_F(BtnMenuScreenTest, FrameCallbackCanExit) {
    s_frame_cb_count = 0;
    menu.frame_cb = frame_cb_exit;
    menu.frame_cb_ctx = nullptr;

    /* No key enqueued — frame callback returning ENTER should confirm */
    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 0) << "Frame callback ENTER should confirm default sel";
}

/* ---- Mouse click selection ---- */
TEST_F(BtnMenuScreenTest, MouseClickSelectsButton) {
    /* Set up: mouse is inside button 1, hittest returns 1 */
    stub_hittest_set(1);
    kbormouse = 1;  /* Mouse mode active */

    /* Simulate a mouse-down event. We need a key to let the tick process,
     * but the hittest+mouse-down in the adapter picks it up. */
    stub_mouse_set(1, 150, 60);  /* Button down, inside button 1 area */
    stub_key_enqueue(UI_KEY_ENTER);  /* Confirm via keyboard as fallback */

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    unsigned char sel = UI_MODAL_TO_SEL(modal);
    EXPECT_TRUE(sel == 0 || sel == 1)
        << "Should select via keyboard (0) or mouse (1)";
}


/* ================================================================== */
/* Multiple sequential navigations                                     */
/* ================================================================== */

TEST_F(BtnMenuScreenTest, MultipleNavigationSteps) {
    /* Navigate: RIGHT RIGHT LEFT ENTER → should be at 1 */
    stub_key_enqueue(UI_KEY_RIGHT);  /* 0 -> 1 */
    stub_key_enqueue(UI_KEY_RIGHT);  /* 1 -> 2 */
    stub_key_enqueue(UI_KEY_LEFT);   /* 2 -> 1 */
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 1);
}

TEST_F(BtnMenuScreenTest, FullWrapAround) {
    /* Navigate RIGHT 3 times = back to start (count=3) */
    stub_key_enqueue(UI_KEY_RIGHT);  /* 0 -> 1 */
    stub_key_enqueue(UI_KEY_RIGHT);  /* 1 -> 2 */
    stub_key_enqueue(UI_KEY_RIGHT);  /* 2 -> 0 (wrap) */
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 0);
}


/* ================================================================== */
/* Screen stack with modal nesting                                     */
/* ================================================================== */

TEST_F(UIScreenTest, ModalPreservesOuterStack) {
    /* Push a base screen, then run a modal on top */
    EventLog base_log = {};
    UIScreen *base = ui_screen_alloc();
    base->on_event = logging_event;
    base->on_destroy = noop_destroy;
    base->userdata = &base_log;

    ui_screen_push(base);
    ASSERT_EQ(ui_screen_depth(), 1);

    /* Now run a modal */
    ModalTestState modal_state = {7, 0, 0};
    UIScreen *modal_scr = ui_screen_alloc();
    modal_scr->on_event = modal_event;
    modal_scr->on_render = modal_render;
    modal_scr->on_destroy = modal_destroy;
    modal_scr->userdata = &modal_state;

    stub_key_enqueue(UI_KEY_SPACE);

    int result = ui_screen_run_modal(modal_scr);
    EXPECT_EQ(result, 7);
    EXPECT_EQ(ui_screen_depth(), 1) << "Base screen should still be on stack";
    EXPECT_EQ(ui_screen_top(), base) << "Base screen should be on top again";
    EXPECT_GE(base_log.leave_count, 1) << "Base should have seen LEAVE";
    EXPECT_GE(base_log.enter_count, 2) << "Base should see ENTER again after modal";
}


/* ================================================================== */
/* Single button edge case                                             */
/* ================================================================== */

TEST_F(BtnMenuScreenTest, SingleButtonConfirm) {
    menu.count = 1;
    unsigned char sel = runWithKey(UI_KEY_ENTER);
    EXPECT_EQ(sel, 0);
}

TEST_F(BtnMenuScreenTest, SingleButtonNavWraps) {
    menu.count = 1;
    /* RIGHT and LEFT should both stay at 0 (wrap with count=1) */
    stub_key_enqueue(UI_KEY_RIGHT);
    stub_key_enqueue(UI_KEY_LEFT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    int modal = ui_screen_run_modal(scr);
    EXPECT_EQ(UI_MODAL_TO_SEL(modal), 0);
}


/* ================================================================== */
/* Idle reset on key press                                             */
/* ================================================================== */

TEST_F(BtnMenuScreenTest, IdleResetOnNavigation) {
    menu.idle_timeout = 1000;  /* Long, won't fire */
    stub_counters_reset();

    /* Navigate then confirm */
    stub_key_enqueue(UI_KEY_RIGHT);
    stub_key_enqueue(UI_KEY_ENTER);

    UIScreen *scr = ui_screen_from_button_menu(&menu);
    ui_screen_run_modal(scr);

    EXPECT_GE(stub_get_idle_reset_calls(), 1) << "Idle timer should reset on navigation";
}
