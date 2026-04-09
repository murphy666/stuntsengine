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

/* highscore.c — Highscore system extracted from stunts.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stunts.h"
#include "ressources.h"
#include "memmgr.h"
#include "shape2d.h"
#include "shape3d.h"
#include "font.h"
#include "mouse.h"
#include "timer.h"
#include "highscore.h"
#include "ui_keys.h"
#include "ui_widgets.h"
#include "ui_screen.h"

enum {
    HIGHSCORE_ENTRY_COUNT = 7,
    HIGHSCORE_TEMP_ENTRY_INDEX = HIGHSCORE_ENTRY_COUNT - 1,
    HIGHSCORE_SECONDARY_INDEX_COUNT = HIGHSCORE_ENTRY_COUNT - 1,
    HIGHSCORE_ENTRY_SIZE = 52,
    HIGHSCORE_TABLE_SIZE = HIGHSCORE_ENTRY_COUNT * HIGHSCORE_ENTRY_SIZE,
    HIGHSCORE_ENTRY_NAME_CAPACITY = 17,
    HIGHSCORE_ENTRY_DETAIL_OFFSET = 17,
    HIGHSCORE_ENTRY_DETAIL_CAPACITY = 25,
    HIGHSCORE_ENTRY_RESULT_FLAG_OFFSET = 41,
    HIGHSCORE_ENTRY_OPPONENT_OFFSET = 42,
    HIGHSCORE_ENTRY_OPPONENT_PREFIX_CAPACITY = 3,
    HIGHSCORE_ENTRY_OPPONENT_SEPARATOR_OFFSET = 44,
    HIGHSCORE_ENTRY_OPPONENT_SUFFIX_OFFSET = 45,
    HIGHSCORE_ENTRY_OPPONENT_SUFFIX_CAPACITY = 5,
    HIGHSCORE_ENTRY_SCORE_OFFSET = 50,
    HIGHSCORE_EMPTY_SCORE = 65535,
    HIGHSCORE_UI_INDEX_NONE = 255,
    HIGHSCORE_FPS_DOUBLE_SCORE = 10,
    HIGHSCORE_FPS_REDUCED_SCORE = 30,
    HIGHSCORE_SCORE_REDUCED_NUMERATOR = 2,
    HIGHSCORE_SCORE_REDUCED_DENOMINATOR = 3,
    HIGHSCORE_ROW_Y_START = 25,
    HIGHSCORE_ROW_Y_STEP = 10,
    HIGHSCORE_RESULT_PLAYER_WIN = 0,
    HIGHSCORE_RESULT_OPPONENT_WIN = 1,
    HIGHSCORE_RESULT_DNF = 2,
    HIGHSCORE_MENU_BUTTON_COUNT = 4,
    HIGHSCORE_MENU_LAST_BUTTON = HIGHSCORE_MENU_BUTTON_COUNT - 1,
    HIGHSCORE_ANIM_FRAMES_PLAYER_WIN = 1,
    HIGHSCORE_ANIM_FRAMES_NON_WIN = 3,
    HIGHSCORE_ANIM_DEFAULT_SPEED = 30,
    HIGHSCORE_BUTTON_DRAW_WIDTH = 70,
    HIGHSCORE_BUTTON_SPAN = 71,
    HIGHSCORE_STAT_DIGITS = 3,
    HIGHSCORE_TRACK_BYTES_TO_COMPARE = 901,
};

static unsigned char *
highscore_entry_ptr(unsigned short index) {
    return (unsigned char *)highscore_data + (index * HIGHSCORE_ENTRY_SIZE);
}

static unsigned short
highscore_entry_score(const unsigned char *entry) {
    return *(const unsigned short *)(entry + HIGHSCORE_ENTRY_SCORE_OFFSET);
}

static void
highscore_set_entry_score(unsigned char *entry, unsigned short score) {
    *(unsigned short *)(entry + HIGHSCORE_ENTRY_SCORE_OFFSET) = score;
}

/* Variables moved from data_game.c */
static unsigned short highscore_secondary_indices[HIGHSCORE_SECONDARY_INDEX_COUNT]
    = { 0, 0, 0, 0, 0, 0 };

/* Variables moved from data_game.c (private to this translation unit) */
static unsigned short end_hiscore_random = 0;
static struct RECTANGLE hiscore_draw_text_rect = { 0, 0, 0, 0 };
static unsigned char input_button_repeat_state = 0;
static char input_replay_buffer[HIGHSCORE_ENTRY_NAME_CAPACITY]
    = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };


/* file-local data (moved from data_global.c) */
static unsigned short game_object_rotation_x = 4;
static unsigned short rotation_matrix_pitch = 84;
static unsigned short rotation_matrix_yaw = 164;
static unsigned char rotation_matrix_element_scale[4] = { 244, 0, 128, 0 };
static unsigned short hiscore_buttons_y1[] = { 174, 174, 174, 174, 174 };
static unsigned short hiscore_buttons_y2[] = { 197, 197, 197, 197, 197 };


/** @brief Data.
 * @param shadowColour Parameter `shadowColour`.
 * @return Function result.
 */
/* file-local data (moved from data_global.c) */


/* --- hiscore_draw_text --- */

// Draw text with 4-way shadow effect for highscore screens
// Returns pointer to bounding rectangle (hiscore_draw_text_rect_left-hiscore_draw_text_rect_bottom)
/** @brief Hiscore draw text.
 * @param str Parameter `str`.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @param colour Parameter `colour`.
 * @param shadowColour Parameter `shadowColour`.
 * @return Function result.
 */

struct RECTANGLE *
hiscore_draw_text(char *str, int x, int y, int colour, int shadowColour) {
    unsigned short textWidth;

    // Set up bounding rectangle (with -1 padding)
    hiscore_draw_text_rect.top = y - 1;
    hiscore_draw_text_rect.bottom = y + fontdef_line_height + 1;
    hiscore_draw_text_rect.left = x - 1;

    textWidth = font_get_text_width(str);
    hiscore_draw_text_rect.right = x + textWidth + 1;

    // Draw shadow text in 4 directions
    font_set_colors(shadowColour, 0);
    font_draw_text(str, x + 1, y + 1); // bottom-right
    font_draw_text(str, x - 1, y + 1); // bottom-left
    font_draw_text(str, x + 1, y - 1); // top-right
    font_draw_text(str, x - 1, y - 1); // top-left

    // Draw main text
    font_set_colors(colour, 0);
    font_draw_text(str, x, y);

    return &hiscore_draw_text_rect;
}

/* --- menu_reset_idle_timers --- */

/** @brief Menu reset idle timers.
 */
// Reset idle/menu timers used by main menu highlighting
/** @brief Menu reset idle timers.
 */
void
menu_reset_idle_timers(void) {
    distance_accumulator_counter = 0;
    game_timer_milliseconds = 0;
    idle_counter = 0;
}

/* --- highscore_write_a_ --- */

/** @brief Highscore write a.
 * @param write_defaults Parameter `write_defaults`.
 * @return Function result.
 */
// Load or initialize highscore table for the current track
/** @brief Highscore write a .
 * @param write_defaults Parameter `write_defaults`.
 * @return Function result.
 */
unsigned short
highscore_write_a_(unsigned short write_defaults) {
    unsigned short i;
    unsigned char entry[HIGHSCORE_ENTRY_SIZE];
    unsigned short j;

    input_button_repeat_state = HIGHSCORE_UI_INDEX_NONE;
    for (i = 0; i < HIGHSCORE_ENTRY_COUNT; ++i) {
        highscore_primary_index[i] = i;
    }

    file_build_path(track_highscore_path_buffer, gameconfig.game_trackname, ".hig", g_path_buf,
                    sizeof(g_path_buf));

    if (write_defaults == 0) {
        void *status;
        g_is_busy = true;
        status = file_read_with_mode(10, g_path_buf, highscore_data);
        g_is_busy = false;
        return (status == 0) ? 1 : 0;
    }

    for (j = 0; j < sizeof(entry); ++j) {
        entry[j] = 0;
    }
    memcpy(entry, "....................", sizeof("...................."));
    memcpy(entry + HIGHSCORE_ENTRY_DETAIL_OFFSET, ".......................",
           sizeof("......................."));
    entry[HIGHSCORE_ENTRY_RESULT_FLAG_OFFSET] = 0;
    memcpy(entry + HIGHSCORE_ENTRY_OPPONENT_OFFSET, "../....", sizeof("../...."));
    highscore_set_entry_score(entry, HIGHSCORE_EMPTY_SCORE);

    for (i = 0; i < HIGHSCORE_ENTRY_COUNT; ++i) {
        for (j = 0; j < sizeof(entry); ++j) {
            highscore_entry_ptr(i)[j] = entry[j];
        }
    }

    return file_write_fatal(g_path_buf, highscore_data, HIGHSCORE_TABLE_SIZE) ? 1 : 0;
}

/* --- highscore_write_b --- */

// Write the current highscore table (sorted by highscore_primary_index) to disk
/** @brief Highscore write b.
 * @return Function result.
 */

unsigned short
highscore_write_b(void) {
    unsigned char buffer[HIGHSCORE_TABLE_SIZE];
    unsigned short i;
    unsigned short j;

    for (i = 0; i < HIGHSCORE_ENTRY_COUNT; ++i) {
        unsigned short mapped_index = highscore_primary_index[i];
        unsigned char *src = highscore_entry_ptr(mapped_index);
        unsigned char *dst = buffer + (i * HIGHSCORE_ENTRY_SIZE);
        for (j = 0; j < HIGHSCORE_ENTRY_SIZE; ++j) {
            dst[j] = src[j];
        }
    }

    file_build_path(track_highscore_path_buffer, gameconfig.game_trackname, ".hig", g_path_buf,
                    sizeof(g_path_buf));
    g_is_busy = true;
    {
        short ok = file_write_fatal(g_path_buf, buffer, HIGHSCORE_TABLE_SIZE);
        g_is_busy = false;
        return ok ? 1 : 0;
    }
}

/* --- enter_hiscore --- */

/** @brief Enter hiscore.
 * @param score Parameter `score`.
 * @param textres_ptr Parameter `textres_ptr`.
 * @param raceResultFlag Parameter `raceResultFlag`.
 * @return Function result.
 */
// Insert a new highscore entry and prompt for player name
/** @brief Enter hiscore.
 * @param score Parameter `score`.
 * @param textres_ptr Parameter `textres_ptr`.
 * @param raceResultFlag Parameter `raceResultFlag`.
 * @return Function result.
 */
unsigned short
enter_hiscore(unsigned short score, void *textres_ptr, unsigned char raceResultFlag) {
    unsigned char entry[HIGHSCORE_ENTRY_SIZE];
    unsigned short insertion = 0;
    unsigned short i;
    unsigned short dialog_coords[2] = { 0, 0 };

    if (framespersec == HIGHSCORE_FPS_DOUBLE_SCORE) {
        score <<= 1;
    }
    else if (framespersec >= HIGHSCORE_FPS_REDUCED_SCORE) {
        score = (unsigned short)(score * HIGHSCORE_SCORE_REDUCED_NUMERATOR
                                 / HIGHSCORE_SCORE_REDUCED_DENOMINATOR);
    }

    if (score >= highscore_entry_score(highscore_entry_ptr(HIGHSCORE_TEMP_ENTRY_INDEX))) {
        highscore_draw_screen();
        return 0;
    }

    for (i = 0; i < HIGHSCORE_ENTRY_COUNT; ++i) {
        highscore_primary_index[i] = i;
    }

    while (insertion < HIGHSCORE_ENTRY_COUNT) {
        unsigned short existing = highscore_entry_score(highscore_entry_ptr(insertion));
        if (score < existing) {
            break;
        }
        ++insertion;
    }

    input_button_repeat_state = (unsigned char)insertion;

    for (i = insertion; i < HIGHSCORE_TEMP_ENTRY_INDEX; ++i) {
        highscore_secondary_indices[i] = i;
    }

    highscore_primary_index[insertion] = HIGHSCORE_TEMP_ENTRY_INDEX;

    {
        unsigned i;
        for (i = 0; i < sizeof(entry); ++i) {
            entry[i] = 0;
        }
    }
    snprintf((char *)(entry + HIGHSCORE_ENTRY_DETAIL_OFFSET), HIGHSCORE_ENTRY_DETAIL_CAPACITY,
             "%s", gnam_string);
    entry[HIGHSCORE_ENTRY_RESULT_FLAG_OFFSET] = raceResultFlag;

    if (gameconfig.game_opponenttype != 0) {
        snprintf((char *)(entry + HIGHSCORE_ENTRY_OPPONENT_OFFSET),
                 HIGHSCORE_ENTRY_OPPONENT_PREFIX_CAPACITY, "%s", player_name_buffer);
        entry[HIGHSCORE_ENTRY_OPPONENT_SEPARATOR_OFFSET] = '/';
        snprintf((char *)(entry + HIGHSCORE_ENTRY_OPPONENT_SUFFIX_OFFSET),
                 HIGHSCORE_ENTRY_OPPONENT_SUFFIX_CAPACITY, "%s", gsna_string);
    }
    else {
        memcpy(entry + HIGHSCORE_ENTRY_OPPONENT_OFFSET, " ", 2);
    }

    highscore_set_entry_score(entry, score);

    memcpy((void *)highscore_entry_ptr(HIGHSCORE_TEMP_ENTRY_INDEX), entry, sizeof(entry));

    sprite_select_wnd_as_sprite1();
    highscore_draw_screen();
    sprite_blit_to_video(wndsprite, 65535);

    ui_dialog_show_restext(3, 0, textres_ptr, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, dialogarg2,
                           &dialog_coords[0], 0);

    check_input();
    {
        call_read_line(input_replay_buffer, HIGHSCORE_ENTRY_NAME_CAPACITY - 1, dialog_coords[0],
                       dialog_coords[1], terraincenterpos + HIGHSCORE_ENTRY_DETAIL_OFFSET);
    }

    snprintf((char *)entry, HIGHSCORE_ENTRY_NAME_CAPACITY, "%s", input_replay_buffer);

    memcpy((void *)highscore_entry_ptr(HIGHSCORE_TEMP_ENTRY_INDEX), entry, sizeof(entry));

    sprite_select_wnd_as_sprite1();
    highscore_draw_screen();
    sprite_blit_to_video(wndsprite, 65535);

    highscore_write_b();
    highscore_draw_screen();

    return 0;
}

/* --- print_highscore_entry_ --- */

// Format a single highscore entry into the shared text buffer.
/** @brief Print highscore entry.
 * @param index Parameter `index`.
 * @param lengths Parameter `lengths`.
 */

void
print_highscore_entry_(unsigned short index, unsigned char *lengths) {
    unsigned char entry[HIGHSCORE_ENTRY_SIZE];
    unsigned short mapped_index;
    unsigned short offset;
    unsigned short i;
    unsigned short time_value;
    unsigned short old_framespersec;
    char frame_buf[16];
    unsigned char *src;
    char *dst;

    mapped_index = highscore_primary_index[index];
    src = highscore_entry_ptr(mapped_index);
    for (i = 0; i < sizeof(entry); ++i) {
        entry[i] = src[i];
    }

    if (lengths != NULL) {
        lengths[0] = 0;
    }

    dst = resID_byte1;
    snprintf(dst, 2048, "%s", (char *)entry);
    offset = (unsigned short)(strlen(dst) + 1);
    if (lengths != NULL) {
        lengths[1] = (unsigned char)offset;
    }

    snprintf(dst + offset, 2048 - offset, "%s", (char *)entry + HIGHSCORE_ENTRY_DETAIL_OFFSET);
    offset = (unsigned short)(offset + strlen(dst + offset) + 1);
    if (lengths != NULL) {
        lengths[2] = (unsigned char)offset;
    }
    dst[offset] = '\0';

    if (entry[HIGHSCORE_ENTRY_RESULT_FLAG_OFFSET] == 1) {
        size_t cur_len = strlen(dst + offset);
        snprintf(dst + offset + cur_len, 2048 - offset - cur_len, "(");
    }

    {
        size_t cur_len = strlen(dst + offset);
        snprintf(dst + offset + cur_len, 2048 - offset - cur_len, "%s",
                 (char *)entry + HIGHSCORE_ENTRY_OPPONENT_OFFSET);
    }
    if (entry[HIGHSCORE_ENTRY_RESULT_FLAG_OFFSET] == 1) {
        size_t cur_len = strlen(dst + offset);
        snprintf(dst + offset + cur_len, 2048 - offset - cur_len, ")");
    }

    offset = (unsigned short)(offset + strlen(dst + offset) + 1);
    if (lengths != NULL) {
        lengths[3] = (unsigned char)offset;
    }

    time_value = highscore_entry_score(entry);
    old_framespersec = framespersec;
    framespersec = 20;
    format_frame_as_string(frame_buf, time_value == HIGHSCORE_EMPTY_SCORE ? 0 : time_value, 1);
    framespersec = old_framespersec;

    snprintf(dst + offset, 2048 - offset, "%s", frame_buf);
}

/* --- highscore_draw_screen --- */

// Draw the highscore screen header and entries.
/** @brief Highscore draw screen.
 */

void
highscore_draw_screen(void) {
    unsigned char lengths[4];
    unsigned short i;
    unsigned short y;
    unsigned short color;
    unsigned short centered_x;

    sprite_select_wnd_as_sprite1();

    copy_string(resID_byte1, locate_text_res(mainresptr, "hs1"));
    {
        size_t len = strlen(resID_byte1);
        snprintf(resID_byte1 + len, sizeof(resID_byte1) - len, " '%s'", gameconfig.game_trackname);
    }
    centered_x = font_get_centered_x(resID_byte1);
    hiscore_draw_text(resID_byte1, centered_x, 5, dialog_fnt_colour, 0);

    copy_string(resID_byte1, locate_text_res(mainresptr, "hs2"));
    hiscore_draw_text(resID_byte1, 16, 15, dialog_fnt_colour, 0);
    copy_string(resID_byte1, locate_text_res(mainresptr, "hs3"));
    hiscore_draw_text(resID_byte1, 120, 15, dialog_fnt_colour, 0);
    copy_string(resID_byte1, locate_text_res(mainresptr, "hs5"));
    hiscore_draw_text(resID_byte1, 224, 15, dialog_fnt_colour, 0);
    copy_string(resID_byte1, locate_text_res(mainresptr, "hs4"));
    hiscore_draw_text(resID_byte1, 272, 15, dialog_fnt_colour, 0);

    font_set_fontdef2(fontnptr);

    for (i = 0; i < HIGHSCORE_ENTRY_COUNT; ++i) {
        print_highscore_entry_(i, lengths);
        color = (i == input_button_repeat_state) ? dialogarg2 : 0;
        y = (unsigned short)(HIGHSCORE_ROW_Y_START + (i * HIGHSCORE_ROW_Y_STEP));
        font_set_colors(0, color);
        font_draw_text(resID_byte1 + lengths[0], 16, y);
        font_draw_text(resID_byte1 + lengths[1], 120, y);
        font_draw_text(resID_byte1 + lengths[2], 224, y);
        font_draw_text(resID_byte1 + lengths[3], 272, y);
    }

    font_set_fontdef();
}

/* ---- Hiscore menu: event-driven UIScreen implementation ----------- */

typedef struct {
    void *misc_res;
    void *opp_res;
    void *opp_text_res;
    struct SHAPE2D *opp_anim_shape;
    struct SPRITE *small_wnd;
    unsigned short sel_button;
    unsigned short prev_button;
    unsigned short anim_timer;
    unsigned short anim_speed;
    unsigned char allow_animation;
    unsigned char race_result;
    unsigned char anim_frame;
    unsigned char anim_letter;
    unsigned short button_x1[4];
    unsigned short button_x2[4];
    unsigned short button_y1[4];
    unsigned short button_y2[4];
} HiscoreMenuState;

static int
hiscore_on_event(UIScreen *self, const UIEvent *ev) {
    HiscoreMenuState *st = (HiscoreMenuState *)self->userdata;

    if (ev->type == UI_EVENT_KEY_DOWN) {
        unsigned short key = ev->key;

        if (key == UI_KEY_ENTER || key == UI_KEY_SPACE) {
            return (int)st->sel_button + 1;
        }
        if (key == UI_KEY_LEFT) {
            st->sel_button = (st->sel_button == 0) ? HIGHSCORE_MENU_LAST_BUTTON
                                                   : (unsigned short)(st->sel_button - 1);
        }
        if (key == UI_KEY_RIGHT) {
            st->sel_button = (st->sel_button >= HIGHSCORE_MENU_LAST_BUTTON)
                                 ? 0
                                 : (unsigned short)(st->sel_button + 1);
        }
        return 0;
    }

    if (ev->type == UI_EVENT_MOUSE_MOVE || ev->type == UI_EVENT_MOUSE_DOWN) {
        unsigned char hit = mouse_multi_hittest(HIGHSCORE_MENU_BUTTON_COUNT, st->button_x1,
                                                st->button_x2, st->button_y1, st->button_y2);
        if (hit != HIGHSCORE_UI_INDEX_NONE) {
            st->sel_button = hit;
            if (ev->type == UI_EVENT_MOUSE_DOWN && kbormouse) {
                return (int)st->sel_button + 1;
            }
        }
        return 0;
    }

    return 0;
}

static void
hiscore_on_render(UIScreen *self) {
    HiscoreMenuState *st = (HiscoreMenuState *)self->userdata;
    unsigned short replay_delta;

    if (st->prev_button != st->sel_button) {
        st->prev_button = st->sel_button;
        /* ASM (loc_14188): restore clean sprite from sprite2, redraw button
		 * row from wndsprite, then let the blink routine draw the outline. */
        sprite_copy_2_to_1();
        mouse_draw_opaque_check();
        sprite_putimage(wndsprite->sprite_bitmapptr);
        mouse_draw_transparent_check();
    }

    replay_delta = mouse_update_menu_blink((unsigned char)st->sel_button, st->button_x1,
                                           st->button_x2, st->button_y1, st->button_y2,
                                           camera_view_matrix, object_visibility_state);

    st->anim_timer += replay_delta;
    if (st->allow_animation && st->anim_timer >= st->anim_speed && st->opp_anim_shape != 0
        && st->opp_text_res != 0) {
        unsigned char loop_frames = (st->race_result == HIGHSCORE_RESULT_PLAYER_WIN)
                                        ? HIGHSCORE_ANIM_FRAMES_PLAYER_WIN
                                        : HIGHSCORE_ANIM_FRAMES_NON_WIN;
        char anim_name[4];
        char *anim_txt;
        st->anim_frame = (unsigned char)((st->anim_frame + 1) % loop_frames);
        anim_name[0] = (char)st->anim_letter;
        anim_name[1] = (char)('1' + st->anim_frame);
        anim_name[2] = 'a';
        anim_name[3] = 0;
        anim_txt = locate_text_res(st->opp_text_res, anim_name);
        st->anim_timer = 0;
        mouse_draw_opaque_check();
        font_set_fontdef2(fontnptr);
        hiscore_draw_text(anim_txt, 16, 8, dialog_fnt_colour, 0);
        font_set_fontdef();
        mouse_draw_transparent_check();
    }
}

static void
hiscore_on_destroy(UIScreen *self) {
    HiscoreMenuState *st = (HiscoreMenuState *)self->userdata;
    if (!st)
        return;

    audio_unload();
    if (st->small_wnd != 0) {
        sprite_free_wnd(st->small_wnd);
    }
    if (st->opp_res != 0) {
        unload_resource(st->opp_res);
    }
    unload_resource(st->misc_res);
    sprite_free_wnd(wndsprite);

    free(st);
    self->userdata = NULL;
}

/* --- end_hiscore --- */

// End-of-race high score and results screen.
// This is a direct C port of the original 16-bit assembly implementation.
// It keeps the same flow (load misc/opponent resources, render stats, run opponent animation,
// handle high-score writing and the post-race menu).
/** @brief End hiscore.
 * @return Function result.
 */

unsigned short
end_hiscore(void) {
    void *misc_res;
    void *opp_res = 0;
    void *opp_text_res = 0;
    struct SHAPE2D *opp_anim_shape = 0;
    struct SPRITE *small_wnd = 0;
    unsigned short dialog_y = 107;
    unsigned short avg_speed = 0;
    unsigned short anim_speed = HIGHSCORE_ANIM_DEFAULT_SPEED;
    unsigned short button_x1[HIGHSCORE_MENU_BUTTON_COUNT];
    unsigned short button_x2[HIGHSCORE_MENU_BUTTON_COUNT];
    unsigned short button_y1[HIGHSCORE_MENU_BUTTON_COUNT];
    unsigned short button_y2[HIGHSCORE_MENU_BUTTON_COUNT];
    unsigned i;
    unsigned char race_result = HIGHSCORE_RESULT_DNF;
    unsigned char opponent_type = (unsigned char)gameconfig.game_opponenttype;
    unsigned char allow_animation = opponent_type;
    unsigned char anim_frame = 0;
    unsigned char anim_letter = 'd';
    unsigned char need_highscore_entry = 0;
    bool show_eval_only = true; // Matches var_14 in the original code
    bool has_track_match = true;

    ensure_file_exists(4);
    misc_res = file_load_resfile("misc");
    if (opponent_type != 0) {
        char opp_name[5] = "opp0";
        opp_name[3] = (char)('0' + opponent_type);
        opp_res = file_load_resfile(opp_name);
    }

    // Build primary window(s).
    wndsprite = sprite_make_wnd(320, 200, 15);
    if (video_flag5_is0) {
        small_wnd = sprite_make_wnd(200, 100, 15);
    }

    // Clear background and lay down simple panels.
    sprite_select_wnd_as_sprite1_and_clear();
    draw_button(0, 0, 0, 320, 100, button_text_color, button_shadow_color, button_highlight_color,
                0);
    draw_button(0, 0, 101, 320, 99, button_text_color, button_shadow_color, button_highlight_color,
                0);

// Helper to draw a line of text centered and advance Y.
#define DRAW_LINE(text_ptr)                                              \
    do {                                                                 \
        unsigned short x_ = font_get_centered_x(text_ptr);               \
        hiscore_draw_text(text_ptr, x_, dialog_y, dialog_fnt_colour, 0); \
        dialog_y += 10;                                                  \
    } while (0)

    // Player time line (original ASM skips entirely on DNF)
    if (gState_total_finish_time != 0) {
        copy_string(resID_byte1, locate_text_res(misc_res, "elt"));
        format_frame_as_string(resID_byte1 + strlen(resID_byte1),
                               (unsigned short)(gState_total_finish_time - gState_penalty), 1);
        if (replay_mode_state_flag & 2) {
            char *con_txt = locate_text_res(misc_res, "con");
            copy_string(resID_byte1 + strlen(resID_byte1), con_txt);
        }
        DRAW_LINE(resID_byte1);
        if (gState_penalty != 0) {
            copy_string(resID_byte1, locate_text_res(misc_res, "ppt"));
            format_frame_as_string(resID_byte1 + strlen(resID_byte1), gState_penalty, 1);
            DRAW_LINE(resID_byte1);
        }
    }

    // Opponent result / winner determination.
    race_result = HIGHSCORE_RESULT_DNF;
    if (opponent_type != 0) {
        if (gState_144 == 0) {
            copy_string(resID_byte1, locate_text_res(misc_res, "olt"));
            copy_string(resID_byte1 + strlen(resID_byte1), locate_text_res(misc_res, "dnf"));
        }
        else if (gState_total_finish_time == 0 || gState_144 < gState_total_finish_time) {
            copy_string(resID_byte1, locate_text_res(misc_res, "owt"));
            format_frame_as_string(resID_byte1 + strlen(resID_byte1), gState_144, 1);
            race_result = HIGHSCORE_RESULT_OPPONENT_WIN;
        }
        else {
            copy_string(resID_byte1, locate_text_res(misc_res, "olt"));
            format_frame_as_string(resID_byte1 + strlen(resID_byte1), gState_144, 1);
        }
        if (gState_total_finish_time != 0 && race_result != HIGHSCORE_RESULT_OPPONENT_WIN) {
            race_result = HIGHSCORE_RESULT_PLAYER_WIN;
        }
        DRAW_LINE(resID_byte1);
    }

    // Pick and load victory/defeat audio.
    if (race_result == HIGHSCORE_RESULT_PLAYER_WIN) {
        file_load_audiores("skidvict", "skidms", "VICT");
    }
    else {
        file_load_audiores("skidover", "skidms", "OVER");
    }

    // Average speed line.
    if ((gState_pEndFrame + elapsed_time1) != 0) {
        unsigned long distance = (unsigned long)gState_travDist;
        unsigned long time_ticks = (unsigned long)gState_pEndFrame + (unsigned long)elapsed_time1;
        avg_speed = (unsigned short)((time_ticks != 0) ? (distance / time_ticks) : 0);
    }
    copy_string(resID_byte1, locate_text_res(misc_res, "avs"));
    print_int_as_string_maybe(resID_byte1 + strlen(resID_byte1), avg_speed, 0,
                  HIGHSCORE_STAT_DIGITS);
    copy_string(resID_byte1 + strlen(resID_byte1), locate_text_res(misc_res, "mph"));
    DRAW_LINE(resID_byte1);

    // Impact speed (if any)
    if (gState_impactSpeed != 0) {
        copy_string(resID_byte1, locate_text_res(misc_res, "imp"));
        print_int_as_string_maybe(resID_byte1 + strlen(resID_byte1),
                                  (unsigned short)(gState_impactSpeed >> 8), 0,
                                  HIGHSCORE_STAT_DIGITS);
        copy_string(resID_byte1 + strlen(resID_byte1), locate_text_res(misc_res, "mph"));
        DRAW_LINE(resID_byte1);
    }

    // Top speed.
    copy_string(resID_byte1, locate_text_res(misc_res, "top"));
    print_int_as_string_maybe(resID_byte1 + strlen(resID_byte1),
                              (unsigned short)(gState_topSpeed >> 8), 0,
                              HIGHSCORE_STAT_DIGITS);
    copy_string(resID_byte1 + strlen(resID_byte1), locate_text_res(misc_res, "mph"));
    DRAW_LINE(resID_byte1);

    // Jumps.
    if (gState_jumpCount != 0) {
        copy_string(resID_byte1, locate_text_res(misc_res, "jum"));
        print_int_as_string_maybe(resID_byte1 + strlen(resID_byte1), gState_jumpCount, 0,
                                  HIGHSCORE_STAT_DIGITS);
        DRAW_LINE(resID_byte1);
    }

    // Opponent animation/text setup (simplified).
    if (allow_animation != 0 && (replay_mode_state_flag & 4) == 0) {
        opp_text_res = opp_res;
        opp_anim_shape = (struct SHAPE2D *)locate_shape_alt(opp_res,
                                                            (race_result == HIGHSCORE_RESULT_PLAYER_WIN)
                                                                ? "winn"
                                                                : "lose");
        anim_letter = (race_result == HIGHSCORE_RESULT_PLAYER_WIN) ? 'v' : 'd';
        anim_frame = end_hiscore_random
            = (unsigned char)((get_kevinrandom() + gState_frame)
                              & ((race_result == HIGHSCORE_RESULT_PLAYER_WIN)
                                     ? HIGHSCORE_ANIM_FRAMES_PLAYER_WIN
                                     : HIGHSCORE_ANIM_FRAMES_NON_WIN));
    }

    // Verify current track matches loaded data; determine if we can update highscores.
    has_track_match = true;
    {
        file_build_path(track_highscore_path_buffer, gameconfig.game_trackname, ".trk", g_path_buf,
                        sizeof(g_path_buf));
        {
            void *track_res = file_load_resource(1, g_path_buf);
            if (track_res != 0) {
                unsigned short i;
                for (i = 0; i < HIGHSCORE_TRACK_BYTES_TO_COMPARE; ++i) {
                    if (*((unsigned char *)track_res + i)
                        != *((unsigned char *)track_elem_map + i)) {
                        has_track_match = false;
                        break;
                    }
                }
                mmgr_release(track_res);
            }
            else {
                has_track_match = false;
            }
        }
    }

    if (has_track_match) {
        /* ASM (loc_139BA): try read first; only write defaults if read fails.
		 * highscore_write_a_ returns 0 on success, nonzero on failure. */
        if (highscore_write_a_(0) != 0) {
            if (highscore_write_a_(1) != 0) {
                need_highscore_entry = HIGHSCORE_UI_INDEX_NONE;
            }
        }
        if (need_highscore_entry != HIGHSCORE_UI_INDEX_NONE) {
            if (gState_total_finish_time != 0 && (replay_mode_state_flag & 6) == 0) {
                unsigned short best_time
                    = highscore_entry_score(highscore_entry_ptr(HIGHSCORE_TEMP_ENTRY_INDEX));
                if (best_time == HIGHSCORE_EMPTY_SCORE || gState_total_finish_time < best_time) {
                    need_highscore_entry = 1;
                }
            }
        }
    }
    else {
        need_highscore_entry = HIGHSCORE_UI_INDEX_NONE;
    }

    /* NOTE: The ASM evaluation phase (opponent animation + text in the top
	 * panel) is a separate interactive loop (loc_13A42-loc_13ECD) that runs
	 * before the highscore table/menu.  It redraws the top panel background
	 * on exit.  Without the full evaluation phase ported, skip the initial
	 * animation render to avoid overlapping the highscore title. */

    // Draw highscores if available.
    if (need_highscore_entry != HIGHSCORE_UI_INDEX_NONE) {
        highscore_draw_screen();
    }

    // If a new highscore should be entered, do it now.
    if (need_highscore_entry == 1) {
        check_input();
        mouse_draw_opaque_check();
        show_eval_only = true;
        {
            char *inh_txt = locate_text_res(misc_res, "inh");
            enter_hiscore(gState_total_finish_time, inh_txt, race_result);
        }
        need_highscore_entry = 0;
        highscore_write_b();
        highscore_draw_screen();
    }

    // Button labels and positions derived from original data tables.
    button_x1[0] = game_object_rotation_x;
    button_x1[1] = rotation_matrix_pitch;
    button_x1[2] = rotation_matrix_yaw;
    button_x1[3] = (unsigned short)rotation_matrix_element_scale[0]
                   | ((unsigned short)rotation_matrix_element_scale[1] << 8);
    button_y1[0] = hiscore_buttons_y1[0];
    button_y1[1] = hiscore_buttons_y1[1];
    button_y1[2] = hiscore_buttons_y1[2];
    button_y1[3] = hiscore_buttons_y1[3];
    button_y2[0] = hiscore_buttons_y2[0];
    button_y2[1] = hiscore_buttons_y2[1];
    button_y2[2] = hiscore_buttons_y2[2];
    button_y2[3] = hiscore_buttons_y2[3];

    for (i = 0; i < HIGHSCORE_MENU_BUTTON_COUNT; ++i) {
        button_x2[i] = (unsigned short)(button_x1[i] + HIGHSCORE_BUTTON_SPAN);
    }

    {
        char *label0 = locate_text_res(misc_res, show_eval_only ? "bev" : "bhi");
        char *label1 = locate_text_res(misc_res, "brp");
        char *label2 = locate_text_res(misc_res, (opponent_type != 0) ? "bra" : "bdr");
        char *label3 = locate_text_res(misc_res, "bmm");

        draw_button(label0, (unsigned short)(button_x1[0] + 1), button_y1[0],
                HIGHSCORE_BUTTON_DRAW_WIDTH,
                    (unsigned short)(button_y2[0] - button_y1[0]), button_text_color,
                    button_shadow_color, button_highlight_color, 0);
        draw_button(label1, (unsigned short)(button_x1[1] + 1), button_y1[1],
                HIGHSCORE_BUTTON_DRAW_WIDTH,
                    (unsigned short)(button_y2[1] - button_y1[1]), button_text_color,
                    button_shadow_color, button_highlight_color, 0);
        draw_button(label2, (unsigned short)(button_x1[2] + 1), button_y1[2],
                HIGHSCORE_BUTTON_DRAW_WIDTH,
                    (unsigned short)(button_y2[2] - button_y1[2]), button_text_color,
                    button_shadow_color, button_highlight_color, 0);
        draw_button(label3, (unsigned short)(button_x1[3] + 1), button_y1[3],
                HIGHSCORE_BUTTON_DRAW_WIDTH,
                    (unsigned short)(button_y2[3] - button_y1[3]), button_text_color,
                    button_shadow_color, button_highlight_color, 0);
    }

    // Menu loop — event-driven via UIScreen.
    mouse_draw_opaque_check();
    sprite_blit_to_video(wndsprite, 65535);
    /* ASM: sprite_copy_2_to_1 after initial blit saves clean state for
	 * restoring during the menu loop (button highlight needs a clean backup). */
    sprite_copy_2_to_1();
    {
        HiscoreMenuState *hst = (HiscoreMenuState *)calloc(1, sizeof(HiscoreMenuState));
        UIScreen *scr;
        int modal;

        hst->misc_res = misc_res;
        hst->opp_res = opp_res;
        hst->opp_text_res = opp_text_res;
        hst->opp_anim_shape = opp_anim_shape;
        hst->small_wnd = small_wnd;
        /* ASM (loc_1420F): animation in menu loop is gated by var_14
		 * (show_eval_only). Without the evaluation phase loop, var_14
		 * stays 1, so menu-loop animation never runs in the original. */
        hst->allow_animation = 0;
        hst->race_result = race_result;
        hst->anim_letter = anim_letter;
        hst->anim_frame = anim_frame;
        hst->anim_speed = anim_speed;
        memcpy(hst->button_x1, button_x1, sizeof(button_x1));
        memcpy(hst->button_x2, button_x2, sizeof(button_x2));
        memcpy(hst->button_y1, button_y1, sizeof(button_y1));
        memcpy(hst->button_y2, button_y2, sizeof(button_y2));
        /* ASM: var_selectedmenu=1 (View Replay), var_78=1 initially. */
        hst->sel_button = 1;
        hst->prev_button = HIGHSCORE_UI_INDEX_NONE; /* force first-frame redraw */

        scr = ui_screen_alloc();
        scr->on_event = hiscore_on_event;
        scr->on_render = hiscore_on_render;
        scr->on_destroy = hiscore_on_destroy;
        scr->userdata = hst;

        modal = ui_screen_run_modal(scr);
        return (unsigned short)(modal - 1);
    }
#undef DRAW_LINE
}
