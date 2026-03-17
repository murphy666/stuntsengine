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

/* menu.c — Menus & intro extracted from stunts.c */
#include <string.h>
#include <stdio.h>
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
#include "menu.h"
#include "ui_keys.h"
#include "ui_widgets.h"
#include "ui_screen.h"

/* Variables moved from data_game.c */
static void * tempdataptr = 0;

/* Variables moved from data_game.c (private to this translation unit) */
static struct RECTANGLE intro_draw_text_rect = {0, 0, 0, 0};
static char * miscptr = 0;
static char * oppresources[] = { 0, 0, 0, 0, 0, 0, 0 };
static unsigned short terrain_type_lookup = 9;


/* file-local data (moved from data_global.c) */
static struct RECTANGLE carmenu_cliprect = { 0, 320, 0, 95 };
static struct RECTANGLE menu_clip_rect = { 0, 320, 0, 0 };
static struct VECTOR carmenu_carpos = { 0, 64696, 2880 };

/* menu button arrays (moved from data_global.c) */
static unsigned short menu_buttons_x1[] = { 105, 66, 5, 190, 255 };
static unsigned short menu_buttons_x2[] = { 208, 107, 67, 253, 312 };
static unsigned short menu_buttons_y1[] = { 119, 77, 114, 76, 116 };
static unsigned short menu_buttons_y2[] = { 197, 120, 170, 122, 166 };
static unsigned short carmenu_buttons_y1[] = { 229, 229, 229, 229, 229 };
static unsigned short carmenu_buttons_y2[] = { 316, 316, 316, 316, 316 };
static unsigned short carmenu_buttons_x1[] = { 107, 125, 143, 161, 179 };
static unsigned short carmenu_buttons_x2[] = { 124, 142, 160, 178, 196 };

/** @brief Spin-wait until all keyboard and mouse buttons are released. */

static void wait_input_release_strict(void);
static unsigned short input_repeat_check_intro_stable(unsigned short timeout);

enum {
	MENU_MOUSE_BUTTON_PRIMARY_MASK = 3u,
	MENU_RELEASE_SPIN_GUARD_MAX = 6000u,
	MENU_REPEAT_SPIN_GUARD_MAX = 2048,
	MENU_SCREEN_WIDTH = 320,
	MENU_SCREEN_HEIGHT = 200,
	MENU_SCREEN_DEPTH = 15,
	MENU_TIMEOUT_INTRO_WAIT = 400,
	MENU_TIMEOUT_CREDITS_WAIT = 500,
	MENU_WAITFLAG_CREDITS = 150,
	MENU_WAITFLAG_TRACK_SETUP = 130,
	MENU_WAITFLAG_TRACK_PREVIEW = 155,
	MENU_WAITFLAG_CAR_MENU = 90,
	MENU_WAITFLAG_INTRO_PROD = 160,
	MENU_WAITFLAG_INTRO_DEFAULT = 180,
	MENU_IDLE_TIMEOUT_SHORT = 6000,
	MENU_IDLE_TIMEOUT_LONG = 12000,
	MENU_BLIT_MODE_FULL = 65535,
	MENU_BLIT_MODE_PARTIAL = 65534,
	MENU_SENTINEL_U8 = 255,
	MENU_SENTINEL_U16 = UI_DIALOG_AUTO_POS,
	MENU_KEY_ENTER = 13,
	MENU_KEY_SPACE = 32,
	MENU_KEY_ENTER_EXT = 7181,
	MENU_KEY_SPACE_EXT = 14624,
	MENU_KEY_ESCAPE = 27,
	MENU_KEY_LEFT = 19200,
	MENU_KEY_RIGHT = 19712,
	MENU_KEY_UP = 18432,
	MENU_KEY_DOWN = 20480,
	MENU_KEY_SMALL_STEP = 1,
	MENU_TEXT_GVER_Y = 16,
	MENU_TEXT_TRACK_TITLE_Y = 6,
	MENU_TEXT_HS_TITLE_Y = 18,
	MENU_TRACK_SKYBOX_TILE_INDEX = 900,
	MENU_TRACK_PROJ_ANGLE_X = 40,
	MENU_TRACK_PROJ_ANGLE_Y = 40,
	MENU_TRACK_BUTTON_COUNT = 3,
	MENU_TRACK_BTN_SELECT = 0,
	MENU_TRACK_BTN_RELOAD = 1,
	MENU_TRACK_BTN_DONE = 2,
	MENU_TRACK_NAME_BACKUP_SIZE = 82,
	MENU_TRACK_NAME_BACKUP_COPYLEN = 81,
	MENU_TRACK_PREVSEL_INVALIDATE = 255,
	MENU_HIGHSCORE_ENTRY_SIZE = 52,
	MENU_HIGHSCORE_TIME_OFFSET = 50,
	MENU_HIGHSCORE_INVALID_TIME = 65535,
	MENU_HIGHSCORE_ROW_Y = 30,
	MENU_HIGHSCORE_COL0_X = 16,
	MENU_HIGHSCORE_COL1_X = 120,
	MENU_HIGHSCORE_COL2_X = 224,
	MENU_HIGHSCORE_COL3_X = 272,
	MENU_TRACK_BTN_Y = 172,
	MENU_TRACK_BTN_W = 94,
	MENU_TRACK_BTN_H = 24,
	MENU_TRACK_BTN_SELECT_X = 17,
	MENU_TRACK_BTN_EDIT_X = 113,
	MENU_TRACK_BTN_MENU_X = 209,
	MENU_TEXT_OPP_DESC_X = 12,
	MENU_TEXT_OPP_DESC_Y = 33,
	MENU_OPP_DESC_SAFETY_MAX = 4096,
	MENU_OPP_BUTTON_COUNT = 5,
	MENU_OPP_BTN_PREV = 0,
	MENU_OPP_BTN_NEXT = 1,
	MENU_OPP_BTN_CLEAR = 2,
	MENU_OPP_BTN_CAR = 3,
	MENU_OPP_BTN_DONE = 4,
	MENU_OPP_BTN_W = 54,
	MENU_OPP_BTN_H = 18,
	MENU_OPP_BTN_PREV_X = 21,
	MENU_OPP_BTN_NEXT_X = 77,
	MENU_OPP_BTN_CLEAR_X = 133,
	MENU_OPP_BTN_CAR_X = 189,
	MENU_OPP_BTN_DONE_X = 245,
	MENU_OPP_CAR_PANEL_X = 240,
	MENU_TEXT_CAR_DESC_X = 88,
	MENU_CAR_TEXT_START_Y = 116,
	MENU_CAR_ID_CAPACITY = 32,
	MENU_CAR_ID_STRLEN = 5,
	MENU_CAR_NAME_LEN_MIN = 7,
	MENU_CAR_NAME_INDEX_0 = 3,
	MENU_CAR_NAME_INDEX_1 = 4,
	MENU_CAR_NAME_INDEX_2 = 5,
	MENU_CAR_NAME_INDEX_3 = 6,
	MENU_CAR_COUNT_MAX = 32,
	MENU_CAR_SHAPE_INDEX = 124,
	MENU_CAR_VISIBILITY_DEFAULT = 30000,
	MENU_CAR_TSFLAG_CLIPPED = 8,
	MENU_CAR_BACKLIGHTS_PAINT_ID = 45,
	MENU_CAR_MENU_PANEL_H = 103,
	MENU_CAR_INFO_PANEL_Y = 109,
	MENU_CAR_INFO_PANEL_L_X = 5,
	MENU_CAR_INFO_PANEL_M_X = 82,
	MENU_CAR_INFO_PANEL_W_L = 70,
	MENU_CAR_INFO_PANEL_W_M = 140,
	MENU_CAR_INFO_PANEL_H = 85,
	MENU_CAR_GRAPH_SPEED0_X = 115,
	MENU_CAR_GRAPH_SPEED1_X = 135,
	MENU_CAR_GRAPH_SPEED2_X = 155,
	MENU_CAR_GRAPH_SPEED3_X = 175,
	MENU_CAR_GRAPH_SPEED_Y = 9,
	MENU_CAR_GRAPH_AXIS_X = 185,
	MENU_CAR_GRAPH_AXIS_Y = 26,
	MENU_CAR_GRAPH_Y_BASE = 181,
	MENU_CAR_GRAPH_Y_MIN = 117,
	MENU_CAR_GRAPH_SCALE_SHIFT = 6,
	MENU_CAR_GRAPH_DIVISOR = 800,
	MENU_CAR_GRAPH_X_NUM = 38,
	MENU_CAR_GRAPH_X_BASE = 28,
	MENU_CAR_FRAMESPERSEC_GRAPH = 20,
	MENU_CAR_PROJ_ANGLE_X = 36,
	MENU_CAR_PROJ_ANGLE_Y = 17,
	MENU_CAR_PROJ_HEIGHT = 100,
	MENU_CAR_BUTTON_COUNT = 5,
	MENU_CAR_BTN_DONE = 0,
	MENU_CAR_BTN_NEXT = 1,
	MENU_CAR_BTN_PREV = 2,
	MENU_CAR_BTN_GEARBOX = 3,
	MENU_CAR_BTN_COLOR = 4,
	MENU_CAR_BTN_W = 86,
	MENU_CAR_BTN_H = 16,
	MENU_CAR_BTN_OFFSET = 1,
	MENU_CAR_PREVIEW_BOTTOM = 95,
	MENU_CAR_HOVER_OUTLINE_PAD_Y = 9,
	MENU_MAIN_BUTTON_COUNT = 5,
	MENU_MAIN_SELECT_CAR = 1,
	MENU_OPTION_CASE_OFFSET = 1,
	MENU_OPTION_CASE_CANCEL = 0,
	MENU_OPTION_CASE_EXIT = 7
};

/** @brief Spin-wait until all keyboard and mouse buttons are released. */
static void wait_input_release_strict(void)
{
	unsigned short buttons;
	unsigned short mouse_x;
	unsigned short mouse_y;
	unsigned short spin_guard;

	check_input();
	mousebutinputcode = 0;

	spin_guard = 0;
	do {
		(void)input_checking(1);
		mouse_get_state(&buttons, &mouse_x, &mouse_y);
		spin_guard++;
		if (spin_guard >= MENU_RELEASE_SPIN_GUARD_MAX) {
			break;
		}
	} while ((buttons & MENU_MOUSE_BUTTON_PRIMARY_MASK) != 0u || kb_checking() != 0);

	mousebutinputcode = 0;
}

/** @brief Wait for user input or a timeout, using stable timer deltas.
 *
 * @param timeout  Maximum wait time in timer ticks.
 * @return Non-zero input code if a key/button was pressed, 0 on timeout.
 */
static unsigned short input_repeat_check_intro_stable(unsigned short timeout)
{
	unsigned short elapsed = 0;
	unsigned short spin_guard = 0;

	/* Reset timer baseline so we measure from now */
	timer_get_delta();

	while (elapsed < timeout) {
		unsigned long delta = timer_get_delta();
		if (delta == 0) {
			/* No new tick yet — guard against infinite CPU spin */
			spin_guard++;
			if (spin_guard >= MENU_REPEAT_SPIN_GUARD_MAX) {
				delta = 1;
				spin_guard = 0;
			}
		} else {
			spin_guard = 0;
		}
		if (delta > 0) {
			unsigned short d = (unsigned short)((delta > MENU_SENTINEL_U16) ? MENU_SENTINEL_U16 : delta);
			unsigned short result = input_do_checking(d);
			if (result != 0) {
				return result;
			}
			elapsed += d;
		}
	}
	return 0;
}

/* --- load_intro_resources --- */

/** @brief Load and display intro credit resources with arrow animation.
 *
 * @return Non-zero if interrupted by user input.
 */

// Load intro credit resources and handle basic input/animation.
/** @brief Load intro resources.
 * @return Function result.
 */

unsigned short  load_intro_resources(void) {
	static const struct {
		const char* name;
		unsigned short x;
		unsigned short y;
		unsigned short color;
		unsigned short align;
		unsigned char is_text;
	} credits[] = {
		{ "cre", 120, 0, 11, 3, 1 },
		{ "gds0", 60, 12, 15, 8, 0 },
		{ "gds1", 104, 20, 15, 8, 0 },
		{ "des", 32, 20, 12, 4, 1 },
		{ "gdon", 44, 20, 15, 8, 0 },
		{ "gkev", 52, 20, 15, 8, 0 },
		{ "gbra", 60, 20, 15, 8, 0 },
		{ "grob", 68, 20, 15, 8, 0 },
		{ "gsta", 76, 20, 15, 8, 0 },
		{ "mus", 92, 20, 13, 5, 1 },
		{ "gmsy", 104, 20, 15, 8, 0 },
		{ "gkri", 112, 20, 15, 8, 0 },
		{ "gbri", 120, 20, 15, 8, 0 },
		{ "pro", 32, 172, 9, 1, 1 },
		{ "gkev", 44, 172, 15, 8, 0 },
		{ "opr", 56, 172, 9, 1, 1 },
		{ "gbra", 64, 172, 15, 8, 0 },
		{ "gric", 72, 172, 15, 8, 0 },
		{ "art", 84, 172, 10, 2, 1 },
		{ "gmsm", 96, 172, 15, 8, 0 },
		{ "gdav", 104, 172, 15, 8, 0 },
		{ "gnic", 112, 172, 15, 8, 0 },
		{ "gkev", 120, 172, 15, 8, 0 },
	};
	static const char arrow_names[] = "arowarrwarw1arw2arw3arw4arw5arw6arw7arw8type";
	char * arrows[11];
	void * cred_res;
	unsigned short i;
	unsigned short first_arrow_idx;

	cred_res = file_load_resfile("cred");
	if (tempdataptr == 0) {
		fatal_error("load_intro_resources: tempdataptr is null before arrow lookup");
	}
	for (i = 0; i < 11; ++i) {
		char arrow_id[5];
		arrow_id[0] = arrow_names[i * 4 + 0];
		arrow_id[1] = arrow_names[i * 4 + 1];
		arrow_id[2] = arrow_names[i * 4 + 2];
		arrow_id[3] = arrow_names[i * 4 + 3];
		arrow_id[4] = 0;
		arrows[i] = locate_shape_nofatal((char *)tempdataptr, arrow_id);
	}

	first_arrow_idx = MENU_SENTINEL_U16;
	for (i = 0; i < 10; ++i) {
		if (arrows[i] != 0) {
			first_arrow_idx = i;
			break;
		}
	}
	if (first_arrow_idx == MENU_SENTINEL_U16) {
		fatal_error("load_intro_resources: missing all intro arrows");
	}

	if (arrows[0] == 0) {
		arrows[0] = arrows[first_arrow_idx];
	}
	if (arrows[1] == 0) {
		arrows[1] = arrows[first_arrow_idx];
	}
	for (i = 2; i < 10; ++i) {
		if (arrows[i] == 0) {
			arrows[i] = arrows[first_arrow_idx];
		}
	}
	if (arrows[10] == 0) {
		arrows[10] = arrows[first_arrow_idx];
	}

	waitflag = MENU_WAITFLAG_CREDITS;
	sprite_select_wnd_as_sprite1_and_clear();

	for (i = 0; i < (sizeof(credits) / sizeof(credits[0])); ++i) {
		char * resptr;
		if (credits[i].is_text != 0) {
			resptr = locate_text_res(cred_res, (char*)credits[i].name);
		} else {
			resptr = (char *)locate_shape_alt(cred_res, (char*)credits[i].name);
		}
		copy_string(resID_byte1, resptr);
		intro_draw_text(resID_byte1, credits[i].x, credits[i].y, credits[i].color, credits[i].align);
	}

	unload_resource(cred_res);

	// Show one blended frame before animating.
	sprite_blit_shape_to_sprite1((struct SHAPE2D *)arrows[0]);
	sprite_blit_shape_to_sprite1((struct SHAPE2D *)arrows[10]);
	sprite_blit_to_video(wndsprite, MENU_BLIT_MODE_FULL);
	sprite_copy_2_to_1();

	for (i = 0; i < 10; ++i) {
		unsigned short delta = timer_get_delta_alt();

		sprite_select_wnd_as_sprite1();
		sprite_clear_sprite1_color(0);
		sprite_blit_shape_to_sprite1((struct SHAPE2D *)arrows[(i % 9) + 1]);
		sprite_blit_shape_to_sprite1((struct SHAPE2D *)arrows[10]);
		sprite_blit_to_video(wndsprite, 0);

		if (input_do_checking(delta) != 0) {
			return 1;
		}
	}

	if (input_repeat_check(MENU_TIMEOUT_CREDITS_WAIT) != 0) {
		return 1;
	}

	return 0;
}

/* --- intro_draw_text --- */

// Draw text with shadow effect for intro screens
// Returns pointer to bounding rectangle (intro_draw_text_rect_left-intro_draw_text_rect_bottom)
/** @brief Intro draw text.
 * @param str Parameter `str`.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @param colour Parameter `colour`.
 * @param shadowColour Parameter `shadowColour`.
 * @return Function result.
 */

struct RECTANGLE* intro_draw_text(char* str, int x, int y, int colour, int shadowColour) {
	unsigned short textWidth;
	
	// Set up bounding rectangle
	intro_draw_text_rect.top = y;
	intro_draw_text_rect.bottom = y + fontdef_line_height + 1;
	intro_draw_text_rect.left = x;
	
	textWidth = font_get_text_width(str);
	intro_draw_text_rect.right = x + textWidth + 1;
	
	// Draw shadow text (offset by +1,+1)
	font_set_colors(shadowColour, 0);
	font_draw_text(str, x + 1, y + 1);
	
	// Draw main text
	font_set_colors(colour, 0);
	font_draw_text(str, x, y);
	
	return &intro_draw_text_rect;
}

/* --- security_check --- */

/** @brief Simplified security check stub — always marks security as passed.
 *
/** @brief Index.
 * @param idx Parameter `idx`.
 * @return Function result.
 */

// Simplified security check: mark security as passed
/** @brief Security check.
 * @param idx Parameter `idx`.
 */

void security_check(unsigned short idx) {
	(void)idx;
	passed_security = 1;
}

/* --- run_option_menu_ --- */

// Options menu: input device/help/replay selection dialog.
/** @brief Run option menu.
 * @return Function result.
 */

unsigned short  run_option_menu_(void) {
	unsigned char should_repeat;
	unsigned char dialog_choice;
	unsigned char dialog_case;
	unsigned char input_default;
	void * textresptr;
	short dialog_result;

	miscptr = file_load_resfile("misc");
	sprite_copy_2_to_1();
	sprite_clear_sprite1_color((unsigned char)terrain_type_lookup);

	copy_string(resID_byte1, locate_shape_alt(miscptr, "gstu"));
	intro_draw_text(resID_byte1, font_get_centered_x(resID_byte1), 6, dialog_fnt_colour, 0);
	should_repeat = 1;

	while (should_repeat != 0) {
		copy_string(resID_byte1, locate_shape_alt(miscptr, "gver"));
		intro_draw_text(resID_byte1, font_get_centered_x(resID_byte1), MENU_TEXT_GVER_Y, dialog_fnt_colour, 0);

		textresptr = locate_text_res(miscptr, "mop");
		wait_input_release_strict();
		dialog_result = ui_dialog_confirm_restext(textresptr, 0);
		if (dialog_result < 0) {
			dialog_result = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0, textresptr, UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, dialogarg2, 0, 0);
		}
		dialog_choice = (unsigned char)dialog_result;

		dialog_case = (unsigned char)(dialog_choice + MENU_OPTION_CASE_OFFSET);
		switch (dialog_case) {
		case MENU_OPTION_CASE_CANCEL:
		case MENU_OPTION_CASE_EXIT:
			should_repeat = 0;
			break;
		case 1:
			if (mouse_motion_state_flag != 0) {
				input_default = 2;
			} else if (joystick_assigned_flags != 0) {
				input_default = 1;
			} else {
				input_default = 0;
			}
			wait_input_release_strict();
			dialog_result = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 1, locate_text_res(miscptr, "mid"), UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, performGraphColor, 0, input_default);
			if (dialog_result < 0) {
				dialog_result = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 0, locate_text_res(miscptr, "mid"), UI_DIALOG_AUTO_POS, UI_DIALOG_AUTO_POS, performGraphColor, 0, input_default);
			}
			dialog_choice = (unsigned char)dialog_result;
			if (dialog_choice == 0) {
				do_key_restext();
			} else if (dialog_choice == 1) {
				do_joy_restext();
			} else if (dialog_choice == 2) {
				do_mou_restext();
			}
			break;
		case 2:
			do_mof_restext();
			break;
		case 3:
			do_sonsof_restext();
			break;
		case 4: {
			unsigned short selected = do_fileselect_dialog(replay_file_path_buffer, (char*)aDefault_1, ".rpl", locate_text_res(mainresptr, "rep"));
			if (selected != 0) {
				waitflag = MENU_WAITFLAG_CREDITS;
				show_waiting();
				file_load_replay(replay_file_path_buffer, (const char*)aDefault_1);
				should_repeat = 1;
				goto exit_loop;
			}
			break;
		}
		case 5:
			do_mrl_textres();
			break;
		case 6:
			do_dos_restext();
			unload_resource(miscptr);
			call_exitlist2();
			break;
		default:
			break;
		}
	}

exit_loop:
	unload_resource(miscptr);
	return should_repeat;
}

/* --- run_tracks_menu --- */

/* Per-frame callback for track menu: composites wndsprite content each frame */
static unsigned short trackmenu_frame_cb(unsigned char sel, unsigned short delta, void *ctx) {
	(void)sel;
	(void)delta;
	(void)ctx;
	sprite_copy_2_to_1();
	mouse_draw_opaque_check();
	sprite_putimage(wndsprite->sprite_bitmapptr);
	mouse_draw_transparent_check();
	return 0;
}

/* Selection-change callback for track menu */
struct trackmenu_ctx {
	unsigned short blit_mode;
};

static void trackmenu_on_selection(unsigned char sel, void *ctx_ptr) {
	struct trackmenu_ctx *ctx = (struct trackmenu_ctx *)ctx_ptr;
	(void)sel;
	sprite_blit_to_video(wndsprite, ctx->blit_mode);
	ctx->blit_mode = MENU_BLIT_MODE_PARTIAL;
	sprite_copy_2_to_1();
}

/** @brief Run the track-selection menu and return once a choice is made.
 * @param unused Reserved parameter kept for compatibility with the original call sites.
 */
void run_tracks_menu(unsigned short unused) {
	unsigned char lengths[4];
	unsigned char selection;
	unsigned char * best_entry;
	unsigned short chosen;
	UIButtonMenu btnmenu;
	struct trackmenu_ctx cb_ctx;

	if (unused == 0) {
		check_input();
		goto rebuild_ui;
	}

reload_setup:
	check_input();
	show_waiting();
	waitflag = MENU_WAITFLAG_TRACK_SETUP;
	track_setup();
	load_tracks_menu_shapes();

rebuild_ui:

	show_waiting();
	waitflag = MENU_WAITFLAG_TRACK_PREVIEW;

	// Build preview window and render current track snapshot.
	wndsprite = sprite_make_wnd(MENU_SCREEN_WIDTH, MENU_SCREEN_HEIGHT, MENU_SCREEN_DEPTH);

	load_skybox(track_elem_map[MENU_TRACK_SKYBOX_TILE_INDEX]);
	shape3d_load_all();
	set_projection(MENU_TRACK_PROJ_ANGLE_X, MENU_TRACK_PROJ_ANGLE_Y, MENU_SCREEN_WIDTH, MENU_SCREEN_HEIGHT);
	init_game_state(-2);
	sprite_select_wnd_as_sprite1();
	sprite_clear_sprite1_color((unsigned char)skybox_grd_color);
	sprite_set_1_size(0, MENU_SCREEN_WIDTH, 0, MENU_SCREEN_HEIGHT);
	draw_track_preview();
	shape3d_free_all();
	unload_skybox();
	sprite_select_wnd_as_sprite1();

	copy_string(resID_byte1, "'");
	strcat(resID_byte1, gameconfig.game_trackname);
	strcat(resID_byte1, "'");
	intro_draw_text(resID_byte1, font_get_centered_x(resID_byte1), MENU_TEXT_TRACK_TITLE_Y, dialog_fnt_colour, 0);

	if (highscore_write_a_(0) == 0) {
		best_entry = (unsigned char *)highscore_data + (highscore_primary_index[0] * MENU_HIGHSCORE_ENTRY_SIZE);
		if (*(unsigned short *)(best_entry + MENU_HIGHSCORE_TIME_OFFSET) != MENU_HIGHSCORE_INVALID_TIME) {
			copy_string(resID_byte1, locate_text_res(mainresptr, "hs0"));
			intro_draw_text(resID_byte1, font_get_centered_x(resID_byte1), MENU_TEXT_HS_TITLE_Y, dialog_fnt_colour, 0);

			font_set_fontdef2(fontnptr);
			print_highscore_entry_(0, lengths);
			font_set_colors(0, 0);
			font_draw_text(resID_byte1 + lengths[0], MENU_HIGHSCORE_COL0_X, MENU_HIGHSCORE_ROW_Y);
			font_draw_text(resID_byte1 + lengths[1], MENU_HIGHSCORE_COL1_X, MENU_HIGHSCORE_ROW_Y);
			font_draw_text(resID_byte1 + lengths[2], MENU_HIGHSCORE_COL2_X, MENU_HIGHSCORE_ROW_Y);
			font_draw_text(resID_byte1 + lengths[3], MENU_HIGHSCORE_COL3_X, MENU_HIGHSCORE_ROW_Y);
			font_set_fontdef();
		}
	}

	{
		void * tedit_res = file_load_resfile("tedit");

		draw_button(
			locate_text_res(tedit_res, "bmt"),
			MENU_TRACK_BTN_SELECT_X,
			MENU_TRACK_BTN_Y,
			MENU_TRACK_BTN_W,
			MENU_TRACK_BTN_H,
			button_text_color,
			button_shadow_color,
			button_highlight_color,
			0
		);

		/* ASM: "bet" button at y=113 (New/Edit Tracks) — was missing */
		draw_button(
			locate_text_res(tedit_res, "bet"),
			MENU_TRACK_BTN_EDIT_X,
			MENU_TRACK_BTN_Y,
			MENU_TRACK_BTN_W,
			MENU_TRACK_BTN_H,
			button_text_color,
			button_shadow_color,
			button_highlight_color,
			0
		);

		draw_button(
			locate_text_res(tedit_res, "bmm"),
			MENU_TRACK_BTN_MENU_X,
			MENU_TRACK_BTN_Y,
			MENU_TRACK_BTN_W,
			MENU_TRACK_BTN_H,
			button_text_color,
			button_shadow_color,
			button_highlight_color,
			0
		);

		unload_resource(tedit_res);
	}

	/* Set up button menu for the 3-button track selection bar */
	memset(&btnmenu, 0, sizeof(btnmenu));
	btnmenu.count = MENU_TRACK_BUTTON_COUNT;
	memcpy(btnmenu.x1, trackmenu_buttons_x1, sizeof(unsigned short) * btnmenu.count);
	memcpy(btnmenu.x2, trackmenu_buttons_x2, sizeof(unsigned short) * btnmenu.count);
	memcpy(btnmenu.y1, trackmenu_buttons_y1, sizeof(unsigned short) * btnmenu.count);
	memcpy(btnmenu.y2, trackmenu_buttons_y2, sizeof(unsigned short) * btnmenu.count);
	btnmenu.sprite_hi = camera_view_matrix;
	btnmenu.sprite_lo = object_visibility_state;
	btnmenu.default_sel = 0;
	btnmenu.idle_timeout = MENU_IDLE_TIMEOUT_SHORT;
	btnmenu.nav_mode = UI_NAV_HORIZONTAL;
	btnmenu.frame_cb = trackmenu_frame_cb;
	btnmenu.frame_cb_ctx = NULL;
	cb_ctx.blit_mode = MENU_BLIT_MODE_FULL;
	btnmenu.selection_cb = trackmenu_on_selection;
	btnmenu.selection_cb_ctx = &cb_ctx;

	while (1) {
		int modal;
		UIScreen *scr = ui_screen_from_button_menu(&btnmenu);
		modal = ui_screen_run_modal(scr);
		selection = UI_MODAL_TO_SEL(modal);

		if (selection == UI_SEL_CANCEL || selection == UI_SEL_TIMEOUT ||
		    selection == MENU_TRACK_BTN_DONE) {
			sprite_free_wnd(wndsprite);
			return;
		}

		if (selection == MENU_TRACK_BTN_SELECT) {
			char track_dir_backup[MENU_TRACK_NAME_BACKUP_SIZE];
			strncpy(track_dir_backup, track_highscore_path_buffer, sizeof(track_dir_backup) - 1);
			track_dir_backup[sizeof(track_dir_backup) - 1] = '\0';
			chosen = do_fileselect_dialog(track_highscore_path_buffer, gameconfig.game_trackname, ".trk", locate_text_res(mainresptr, "trk"));
			strncpy(track_highscore_path_buffer, track_dir_backup, MENU_TRACK_NAME_BACKUP_COPYLEN);
			track_highscore_path_buffer[MENU_TRACK_NAME_BACKUP_COPYLEN] = '\0';
			file_build_path(track_highscore_path_buffer, gameconfig.game_trackname, ".trk", g_path_buf);
			if (chosen != 0) {
				file_read_fatal(g_path_buf, track_elem_map);
				sprite_free_wnd(wndsprite);
				check_input();
				goto rebuild_ui;
			}
			/* Re-enter the button loop with selection reset */
			cb_ctx.blit_mode = MENU_BLIT_MODE_PARTIAL;
			btnmenu.default_sel = MENU_TRACK_BTN_SELECT;
			continue;
		}

		if (selection == MENU_TRACK_BTN_RELOAD) {
			sprite_free_wnd(wndsprite);
			goto reload_setup;
		}
	}
}

/** @brief Draw a multi-line opponent description from a ']'-delimited string.
 *
 * @param textresptr  Pointer to the description resource text.
 */

/* --- opponent_menu_draw_description --- */

/** @brief Opponent menu draw description.
 * @param textresptr Parameter `textresptr`.
 */
static void opponent_menu_draw_description(char * textresptr) {
	unsigned short text_len = 0;
	unsigned short y_offset = 0;
	char* buffer = resID_byte1;
	unsigned int safety = 0;

	if (textresptr == 0) {
		return;
	}

	while (1) {
		if (safety++ > MENU_OPP_DESC_SAFETY_MAX) {
			break;
		}

		unsigned char ch = (unsigned char)(*textresptr++);

		if (ch == 0) {
			break;
		}

		if (ch == ']') {
			if (text_len != 0) {
				buffer[text_len] = 0;
				font_draw_text(buffer, MENU_TEXT_OPP_DESC_X, (unsigned short)(MENU_TEXT_OPP_DESC_Y + y_offset));
			}

			text_len = 0;
			y_offset = (unsigned short)(y_offset + fontdef_line_height);
			continue;
		}

		buffer[text_len++] = (char)ch;
	}

	if (text_len != 0) {
		buffer[text_len] = 0;
		font_draw_text(buffer, MENU_TEXT_OPP_DESC_X, (unsigned short)(MENU_TEXT_OPP_DESC_Y + y_offset));
	}
}

/** @brief Internal opponent-selection menu: browse, clear, pick car. */

/* --- run_opponent_menu --- */

/* ---- Opponent menu: event-driven UIScreen implementation ----------- */

typedef struct {
	unsigned char selected;
	unsigned char prev_selected;
	unsigned char prev_opponent;
	unsigned char has_opponent_res;
	void *opponent_resptr;
} OppMenuState;

/** Update selection from mouse position via hittest. */
static void opp_update_hittest(OppMenuState *st)
{
	short hit = mouse_multi_hittest(
		MENU_OPP_BUTTON_COUNT,
		opponentmenu_buttons_x1,
		opponentmenu_buttons_x2,
		opponentmenu_buttons_y1,
		opponentmenu_buttons_y2
	);
	if (hit >= 0 && hit < MENU_OPP_BUTTON_COUNT) {
		if (kbormouse != 0
			&& !(gameconfig.game_opponenttype == 0
				 && hit == MENU_OPP_BTN_CAR)) {
			st->selected = (unsigned char)hit;
		}
	}
}

/** Activate the currently selected button.  Returns non-zero to pop. */
static int opp_activate(OppMenuState *st)
{
	if (st->selected == MENU_OPP_BTN_PREV) {
		gameconfig.game_opponenttype--;
		if (gameconfig.game_opponenttype < 1)
			gameconfig.game_opponenttype = 6;
		return 0;
	}
	if (st->selected == MENU_OPP_BTN_NEXT) {
		gameconfig.game_opponenttype++;
		if (gameconfig.game_opponenttype == 7)
			gameconfig.game_opponenttype = 1;
		return 0;
	}
	if (st->selected == MENU_OPP_BTN_CLEAR) {
		gameconfig.game_opponenttype = 0;
		return 0;
	}
	if (st->selected == MENU_OPP_BTN_CAR) {
		if (gameconfig.game_opponenttype == 0)
			return 0;
		check_input();
		mouse_draw_opaque_check();
		sprite_free_wnd(wndsprite);
		if (st->has_opponent_res != 0 && st->opponent_resptr != 0) {
			unload_resource(st->opponent_resptr);
			st->has_opponent_res = 0;
			st->opponent_resptr = 0;
		}
		show_waiting();
		run_car_menu(
			&gameconfig.game_opponentcarid[0],
			(unsigned char *)&gameconfig.game_opponentmaterial,
			(unsigned char *)&gameconfig.game_opponenttransmission,
			(unsigned int)gameconfig.game_opponenttype
		);
		st->prev_opponent = MENU_SENTINEL_U8;
		return 0;
	}
	if (st->selected == MENU_OPP_BTN_DONE) {
		if (gameconfig.game_opponenttype != 0) {
			if ((unsigned char)gameconfig.game_opponentcarid[0] == MENU_SENTINEL_U8) {
				memcpy(gameconfig.game_opponentcarid,
					   gameconfig.game_playercarid,
					   sizeof(gameconfig.game_playercarid));
				gameconfig.game_opponentmaterial =
					(char)((gameconfig.game_playermaterial & 1) ^ 1);
				gameconfig.game_opponenttransmission = 0;
			}
		} else {
			gameconfig.game_opponentcarid[0] = (char)MENU_SENTINEL_U8;
		}
		return 1; /* pop */
	}
	return 0;
}

static int opp_on_event(UIScreen *self, const UIEvent *ev)
{
	OppMenuState *st = (OppMenuState *)self->userdata;

	if (ev->type == UI_EVENT_KEY_DOWN) {
		unsigned short key = ev->key;

		menu_reset_idle_timers();

		if (key == MENU_KEY_ENTER || key == MENU_KEY_ESCAPE
			|| key == MENU_KEY_SPACE) {
			return opp_activate(st);
		}

		if (key == MENU_KEY_LEFT) {
			st->selected = (st->selected == MENU_OPP_BTN_PREV)
				? MENU_OPP_BTN_DONE
				: (unsigned char)(st->selected - 1);
			if (gameconfig.game_opponenttype == 0
				&& st->selected == MENU_OPP_BTN_CAR) {
				st->selected = (st->selected == MENU_OPP_BTN_PREV)
					? MENU_OPP_BTN_DONE
					: (unsigned char)(st->selected - 1);
			}
			return 0;
		}

		if (key == MENU_KEY_RIGHT) {
			st->selected = (st->selected >= MENU_OPP_BTN_DONE)
				? MENU_OPP_BTN_PREV
				: (unsigned char)(st->selected + 1);
			if (gameconfig.game_opponenttype == 0
				&& st->selected == MENU_OPP_BTN_CAR) {
				st->selected = (st->selected >= MENU_OPP_BTN_DONE)
					? MENU_OPP_BTN_PREV
					: (unsigned char)(st->selected + 1);
			}
			return 0;
		}

		return 0;
	}

	if (ev->type == UI_EVENT_MOUSE_MOVE) {
		opp_update_hittest(st);
		return 0;
	}

	if (ev->type == UI_EVENT_MOUSE_DOWN) {
		opp_update_hittest(st);
		if (kbormouse != 0) {
			return opp_activate(st);
		}
		return 0;
	}

	return 0;
}

static void opp_on_render(UIScreen *self)
{
	OppMenuState *st = (OppMenuState *)self->userdata;
	char *opponent_text;

	mouse_draw_transparent_check();

	/* ---- full redraw when opponent type changes ---- */
	if (st->prev_opponent != (unsigned char)gameconfig.game_opponenttype) {
		if (st->prev_opponent != MENU_SENTINEL_U8) {
			sprite_free_wnd(wndsprite);
			if (st->has_opponent_res != 0 && st->opponent_resptr != 0) {
				unload_resource(st->opponent_resptr);
			}
		}

		ensure_file_exists(4);
		if (gameconfig.game_opponenttype != 0) {
			char oppname[5] = "opp1";
			oppname[3] = (char)('0' + gameconfig.game_opponenttype);
			st->opponent_resptr = file_load_resfile(oppname);
			st->has_opponent_res = 1;
		} else {
			st->opponent_resptr = 0;
			st->has_opponent_res = 0;
		}

		wndsprite = sprite_make_wnd(MENU_SCREEN_WIDTH, MENU_SCREEN_HEIGHT,
									MENU_SCREEN_DEPTH);
		st->prev_opponent = (unsigned char)gameconfig.game_opponenttype;
		st->prev_selected = MENU_SENTINEL_U8;

		if (video_flag5_is0 == 0) {
			sprite_select_wnd_as_sprite1();
		} else {
			setup_mcgawnd2();
		}

		sprite_clear_sprite1_color(0);
		sprite_draw_shape_with_palmap(locate_shape_fatal(opp_res, "scrn"));

		draw_button(locate_text_res(miscptr, "bla"),
			MENU_OPP_BTN_PREV_X,
			(unsigned short)(opponentmenu_buttons_y1[0] + 1),
			MENU_OPP_BTN_W, MENU_OPP_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);

		draw_button(locate_text_res(miscptr, "bnx"),
			MENU_OPP_BTN_NEXT_X,
			(unsigned short)(opponentmenu_buttons_y1[0] + 1),
			MENU_OPP_BTN_W, MENU_OPP_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);

		draw_button(locate_text_res(miscptr, "bcl"),
			MENU_OPP_BTN_CLEAR_X,
			(unsigned short)(opponentmenu_buttons_y1[0] + 1),
			MENU_OPP_BTN_W, MENU_OPP_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);

		draw_button(locate_text_res(miscptr, "bca"),
			MENU_OPP_BTN_CAR_X,
			(unsigned short)(opponentmenu_buttons_y1[0] + 1),
			MENU_OPP_BTN_W, MENU_OPP_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);

		draw_button(locate_text_res(miscptr, "bdo"),
			MENU_OPP_BTN_DONE_X,
			(unsigned short)(opponentmenu_buttons_y1[0] + 1),
			MENU_OPP_BTN_W, MENU_OPP_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);

		sprite_draw_shape_with_palmap(
			oppresources[(unsigned char)gameconfig.game_opponenttype]);
		sprite_draw_shape_with_palmap(locate_shape_fatal(opp_res, "clip"));

		sprite_clear_shape_alt(wndsprite->sprite_bitmapptr, 0, 0);

		if (video_flag5_is0 != 0) {
			sprite_select_wnd_as_sprite1();
		}

		opponent_text = (gameconfig.game_opponenttype != 0)
			? locate_text_res(st->opponent_resptr, "des")
			: locate_text_res(miscptr, "rac");

		font_set_fontdef2(fontnptr);
		font_set_colors(0, 0);
		opponent_menu_draw_description(opponent_text);
		font_set_fontdef();
	}

	/* ---- selection change ---- */
	if (st->prev_selected != st->selected) {
		st->prev_selected = st->selected;
		sprite_copy_2_to_1();
		menu_reset_idle_timers();
	}

	/* ---- per-frame blit + blink ---- */
	sprite_copy_2_to_1();
	mouse_draw_opaque_check();
	sprite_putimage(wndsprite->sprite_bitmapptr);
	mouse_draw_transparent_check();

	mouse_update_menu_blink(
		st->selected,
		opponentmenu_buttons_x1,
		opponentmenu_buttons_x2,
		opponentmenu_buttons_y1,
		opponentmenu_buttons_y2,
		camera_view_matrix,
		object_visibility_state
	);
}

static void opp_on_destroy(UIScreen *self)
{
	OppMenuState *st = (OppMenuState *)self->userdata;
	if (!st) return;

	sprite_free_wnd(wndsprite);
	if (st->has_opponent_res != 0 && st->opponent_resptr != 0) {
		unload_resource(st->opponent_resptr);
	}
	mmgr_free(opp_res);
	unload_resource(miscptr);
	mouse_draw_opaque_check();

	free(st);
	self->userdata = NULL;
}

static UIScreen *make_opponent_screen(void)
{
	UIScreen *scr;
	OppMenuState *st;

	ensure_file_exists(4);
	miscptr = file_load_resfile("misc");
	opp_res = file_load_resource(8, "sdosel");
	locate_many_resources(opp_res, "opp0opp1opp2opp3opp4opp5opp6",
						  oppresources);
	menu_reset_idle_timers();

	st = (OppMenuState *)calloc(1, sizeof(OppMenuState));
	st->prev_opponent = MENU_SENTINEL_U8;
	st->prev_selected = MENU_SENTINEL_U8;

	scr = ui_screen_alloc();
	scr->on_event   = opp_on_event;
	scr->on_render  = opp_on_render;
	scr->on_destroy = opp_on_destroy;
	scr->userdata   = st;

	return scr;
}

/** @brief Run the opponent-selection menu and apply the chosen opponent settings.
 */
void run_opponent_menu(void) {
	ui_screen_run_modal(make_opponent_screen());
}

/** @brief Draw a multi-line car description from a ']'-delimited string.
 *
 * @param textresptr  Pointer to the description resource text.
 * @param y_start     Starting Y coordinate for the first line.
 */

/* --- car_menu_draw_description --- */

/** @brief Car menu draw description.
 * @param textresptr Parameter `textresptr`.
 * @param y_start Parameter `y_start`.
 */
static void car_menu_draw_description(char * textresptr, unsigned short y_start) {

	unsigned short text_len = 0;
	unsigned short y = y_start;
	char* buffer = resID_byte1;

	if (textresptr == 0) {
		return;
	}

	while (1) {
		unsigned char ch = (unsigned char)(*textresptr++);

		if (ch == 0) {
			break;
		}

		if (ch == ']') {
			if (text_len != 0) {
				buffer[text_len] = 0;
				font_draw_text(buffer, MENU_TEXT_CAR_DESC_X, y);
			}

			text_len = 0;
			y = (unsigned short)(y + fontdef_line_height);
			continue;
		}

		buffer[text_len++] = (char)ch;
	}

	if (text_len != 0) {
		buffer[text_len] = 0;
		font_draw_text(buffer, MENU_TEXT_CAR_DESC_X, y);
	}
}

/** @brief Internal car-selection menu: browse cars, set material/transmission.
 *
 * @param caridptr         Pointer to the 4-char car ID buffer to fill.
 * @param materialofs      Pointer to the selected material/colour index.
 * @param transmissionofs  Pointer to the selected transmission type.
 * @param opponenttype     0 if player, 1-6 for opponent.
 */

/* ---- Car menu: event-driven UIScreen implementation --------------- */

typedef struct {
	/* Parameters */
	char *caridptr;
	unsigned char *materialofs;
	unsigned char *transmissionofs;
	unsigned int opponenttype;

	/* 3D rendering */
	struct TRANSFORMEDSHAPE3D local_transshape;
	struct RECTANGLE rect_clip;
	struct RECTANGLE rect_union_rc;
	struct RECTANGLE update_rc;

	/* Car list */
	char carids[MENU_CAR_ID_CAPACITY][MENU_CAR_ID_STRLEN];
	unsigned char car_count;
	unsigned char car_index;
	unsigned char prev_car_index;

	/* UI state */
	unsigned char hover_index;
	unsigned char prev_hover;
	unsigned char redraw_state;
	unsigned char rendered_flag;
	unsigned short rotation;
	unsigned short rotation_delta;
	unsigned short button_x1[MENU_CAR_BUTTON_COUNT];
	unsigned short button_x2[MENU_CAR_BUTTON_COUNT];
	unsigned short button_y1[MENU_CAR_BUTTON_COUNT];
	unsigned short button_y2[MENU_CAR_BUTTON_COUNT];

	/* Resources */
	void *carres;
	void *sdcsel_shape;
	struct SPRITE *opp_wnd;
} CarMenuState;

/** Write selected car ID to output buffer (shared by Done + idle). */
static int car_do_done(CarMenuState *st)
{
	if (!st->rendered_flag) return 0;
	st->caridptr[0] = st->carids[st->car_index][0];
	st->caridptr[1] = st->carids[st->car_index][1];
	st->caridptr[2] = st->carids[st->car_index][2];
	st->caridptr[3] = st->carids[st->car_index][3];
	return 1; /* pop */
}

/** Activate the currently hovered car-menu button.  Returns non-zero to pop. */
static int car_activate(CarMenuState *st)
{
	char *descptr;

	switch (st->hover_index) {
	case MENU_CAR_BTN_DONE:
		return car_do_done(st);
	case MENU_CAR_BTN_NEXT:
		st->car_index = (unsigned char)((st->car_index + 1 >= st->car_count)
			? 0 : (st->car_index + 1));
		return 0;
	case MENU_CAR_BTN_PREV:
		st->car_index = (st->car_index == 0)
			? (unsigned char)(st->car_count - 1)
			: (unsigned char)(st->car_index - 1);
		return 0;
	case MENU_CAR_BTN_GEARBOX:
		*st->transmissionofs ^= 1;
		descptr = (*st->transmissionofs != 0)
			? locate_text_res(miscptr, "bau")
			: locate_text_res(miscptr, "bma");
		sprite_select_wnd_as_sprite1();
		draw_button(descptr,
			(unsigned short)(st->button_x1[MENU_CAR_BTN_GEARBOX] + MENU_CAR_BTN_OFFSET),
			(unsigned short)(st->button_y1[MENU_CAR_BTN_GEARBOX] + MENU_CAR_BTN_OFFSET),
			MENU_CAR_BTN_W, MENU_CAR_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);
		sprite_clear_shape_alt(wndsprite->sprite_bitmapptr, 0, 0);
		st->redraw_state = 3;
		return 0;
	case MENU_CAR_BTN_COLOR:
		(*st->materialofs)++;
		st->redraw_state = 3;
		return 0;
	default:
		return 0;
	}
}

static int car_on_event(UIScreen *self, const UIEvent *ev)
{
	CarMenuState *st = (CarMenuState *)self->userdata;

	if (ev->type == UI_EVENT_KEY_DOWN) {
		unsigned short key = ev->key;

		menu_reset_idle_timers();

		if (key == MENU_KEY_ENTER || key == MENU_KEY_ESCAPE
			|| key == MENU_KEY_SPACE
			|| key == MENU_KEY_ENTER_EXT || key == MENU_KEY_SPACE_EXT) {
			return car_activate(st);
		}

		if (key == MENU_KEY_LEFT || key == MENU_KEY_UP) {
			st->hover_index = (st->hover_index == MENU_CAR_BTN_DONE)
				? MENU_CAR_BTN_COLOR
				: (unsigned char)(st->hover_index - 1);
			return 0;
		}

		if (key == MENU_KEY_RIGHT || key == MENU_KEY_DOWN) {
			st->hover_index = (st->hover_index >= MENU_CAR_BTN_COLOR)
				? MENU_CAR_BTN_DONE
				: (unsigned char)(st->hover_index + 1);
			return 0;
		}

		return 0;
	}

	if (ev->type == UI_EVENT_MOUSE_MOVE) {
		unsigned char hit = mouse_multi_hittest(
			MENU_CAR_BUTTON_COUNT,
			st->button_x1, st->button_x2,
			st->button_y1, st->button_y2);
		if (hit != MENU_SENTINEL_U8 && kbormouse != 0) {
			st->hover_index = hit;
		}
		return 0;
	}

	if (ev->type == UI_EVENT_MOUSE_DOWN) {
		unsigned char hit = mouse_multi_hittest(
			MENU_CAR_BUTTON_COUNT,
			st->button_x1, st->button_x2,
			st->button_y1, st->button_y2);
		if (hit != MENU_SENTINEL_U8 && kbormouse != 0) {
			st->hover_index = hit;
			return car_activate(st);
		}
		return 0;
	}

	return 0;
}

static void car_on_render(UIScreen *self)
{
	CarMenuState *st = (CarMenuState *)self->userdata;
	unsigned short need_blit;
	unsigned short carpos_polar;
	unsigned short old_frames;
	unsigned short graph_x, graph_y;
	char *descptr;

	/* ---- car change: reload resources + draw UI ---- */
	if (st->prev_car_index != st->car_index) {
		if (st->prev_car_index != MENU_SENTINEL_U8) {
			unload_resource(st->carres);
			shape3d_free_car_shapes();
		}

		shape3d_load_car_shapes(st->carids[st->car_index],
								gameconfig.game_opponentcarid);

		{
			char carname[8] = "carcoun";
			carname[3] = st->carids[st->car_index][0];
			carname[4] = st->carids[st->car_index][1];
			carname[5] = st->carids[st->car_index][2];
			carname[6] = st->carids[st->car_index][3];
			st->carres = file_load_resfile(carname);
		}
		setup_aero_trackdata(st->carres, 0);

		sprite_select_wnd_as_sprite1_and_clear();

		draw_button(0, 0, MENU_CAR_MENU_PANEL_H, MENU_SCREEN_WIDTH,
					MENU_CAR_INFO_PANEL_H, button_text_color,
					button_shadow_color, button_highlight_color, 0);
		draw_button(0, MENU_CAR_INFO_PANEL_L_X, MENU_CAR_INFO_PANEL_Y,
					MENU_CAR_INFO_PANEL_W_L, MENU_CAR_INFO_PANEL_H,
					button_text_color, button_shadow_color,
					button_highlight_color, 0);
		draw_button(0, MENU_CAR_INFO_PANEL_M_X, MENU_CAR_INFO_PANEL_Y,
					MENU_CAR_INFO_PANEL_W_M, MENU_CAR_INFO_PANEL_H,
					button_text_color, button_shadow_color,
					button_highlight_color, 0);

		sprite_blit_shape_to_sprite1(
			locate_shape_fatal(st->sdcsel_shape, "grap"));
		font_set_fontdef2(fontnptr);
		font_set_colors(dialog_fnt_colour, 0);
		font_draw_text("150", MENU_CAR_GRAPH_SPEED0_X, MENU_CAR_GRAPH_SPEED_Y);
		font_draw_text("100", MENU_CAR_GRAPH_SPEED1_X, MENU_CAR_GRAPH_SPEED_Y);
		font_draw_text(" 50", MENU_CAR_GRAPH_SPEED2_X, MENU_CAR_GRAPH_SPEED_Y);
		font_draw_text("  0", MENU_CAR_GRAPH_SPEED3_X, MENU_CAR_GRAPH_SPEED_Y);
		font_draw_text("0  20  40", MENU_CAR_GRAPH_AXIS_X, MENU_CAR_GRAPH_AXIS_Y);
		font_set_fontdef();

		draw_button(locate_text_res(miscptr, "bdo"),
			(unsigned short)(st->button_x1[MENU_CAR_BTN_DONE] + MENU_CAR_BTN_OFFSET),
			(unsigned short)(st->button_y1[MENU_CAR_BTN_DONE] + MENU_CAR_BTN_OFFSET),
			MENU_CAR_BTN_W, MENU_CAR_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);
		draw_button(locate_text_res(miscptr, "bnx"),
			(unsigned short)(st->button_x1[MENU_CAR_BTN_NEXT] + MENU_CAR_BTN_OFFSET),
			(unsigned short)(st->button_y1[MENU_CAR_BTN_NEXT] + MENU_CAR_BTN_OFFSET),
			MENU_CAR_BTN_W, MENU_CAR_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);
		draw_button(locate_text_res(miscptr, "bla"),
			(unsigned short)(st->button_x1[MENU_CAR_BTN_PREV] + MENU_CAR_BTN_OFFSET),
			(unsigned short)(st->button_y1[MENU_CAR_BTN_PREV] + MENU_CAR_BTN_OFFSET),
			MENU_CAR_BTN_W, MENU_CAR_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);

		descptr = (*st->transmissionofs != 0)
			? locate_text_res(miscptr, "bau")
			: locate_text_res(miscptr, "bma");
		draw_button(descptr,
			(unsigned short)(st->button_x1[MENU_CAR_BTN_GEARBOX] + MENU_CAR_BTN_OFFSET),
			(unsigned short)(st->button_y1[MENU_CAR_BTN_GEARBOX] + MENU_CAR_BTN_OFFSET),
			MENU_CAR_BTN_W, MENU_CAR_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);
		draw_button(locate_text_res(miscptr, "bco"),
			(unsigned short)(st->button_x1[MENU_CAR_BTN_COLOR] + MENU_CAR_BTN_OFFSET),
			(unsigned short)(st->button_y1[MENU_CAR_BTN_COLOR] + MENU_CAR_BTN_OFFSET),
			MENU_CAR_BTN_W, MENU_CAR_BTN_H,
			button_text_color, button_shadow_color,
			button_highlight_color, 0);

		old_frames = framespersec;
		framespersec = MENU_CAR_FRAMESPERSEC_GRAPH;
		init_game_state(-2);
		state.playerstate.car_transmission = 1;
		for (graph_x = 0; graph_x < MENU_CAR_GRAPH_DIVISOR; ++graph_x) {
			update_car_speed(1, 0, &state.playerstate, &simd_player);
			graph_y = (unsigned short)(MENU_CAR_GRAPH_Y_BASE
				- (((unsigned long)(state.playerstate.car_speed >> 8)
					<< MENU_CAR_GRAPH_SCALE_SHIFT)
				   / MENU_WAITFLAG_CREDITS));
			if (graph_y < MENU_CAR_GRAPH_Y_MIN) break;
			putpixel_single_clipped(performGraphColor, graph_y,
				(unsigned short)((MENU_CAR_GRAPH_X_NUM * graph_x)
					/ MENU_CAR_GRAPH_DIVISOR + MENU_CAR_GRAPH_X_BASE));
		}
		framespersec = old_frames;

		font_set_fontdef2(fontnptr);
		descptr = locate_text_res(st->carres, "des");
		car_menu_draw_description(descptr, MENU_CAR_TEXT_START_Y);
		font_set_fontdef();

		st->update_rc.left = 0;
		st->update_rc.right = MENU_SCREEN_WIDTH;
		st->update_rc.top = 0;
		st->update_rc.bottom = MENU_SCREEN_HEIGHT;
		st->prev_hover = MENU_SENTINEL_U8;
		st->redraw_state = 3;
		st->rendered_flag = 0;
	}

	/* ---- rotation animation ---- */
	st->rotation = (unsigned short)(st->rotation + st->rotation_delta);

	need_blit = (st->redraw_state == 1) ? 1 : 0;

	/* ---- 3D render ---- */
	if (st->redraw_state == 0 || st->redraw_state == 3) {
		carpos_polar = (unsigned short)polarAngle(carmenu_carpos.y,
												  carmenu_carpos.z);
		if (timertestflag_copy != 0) {
			st->rect_clip = rect_invalid;
		} else {
			st->rect_clip = carmenu_cliprect;
		}

		select_cliprect_rotate(0, carpos_polar, 0, &carmenu_cliprect, 0);
		if (*st->materialofs >= game3dshapes[124].shape3d_numpaints) {
			*st->materialofs = 0;
		}

		st->local_transshape.rotvec.z = st->rotation;
		st->local_transshape.material = *st->materialofs;
		(void)shape3d_render_transformed(&st->local_transshape);

		menu_clip_rect.bottom = (st->prev_car_index == st->car_index)
			? MENU_CAR_PREVIEW_BOTTOM : MENU_SCREEN_HEIGHT;
		rect_intersect(&menu_clip_rect, &st->rect_clip);
		rect_union(&st->update_rc, &st->rect_clip, &st->rect_union_rc);

		if (st->redraw_state == 3) {
			need_blit = 1;
		} else {
			st->redraw_state = 1;
		}
	}

	/* ---- blit to screen ---- */
	if (need_blit) {
		st->redraw_state = 0;
		st->rendered_flag = 1;
		sprite_select_wnd_as_sprite1();
		sprite_set_1_size(st->update_rc.left, st->update_rc.right,
						  st->update_rc.top, st->update_rc.bottom);
		sprite_putimage(
			(struct SHAPE2D *)locate_shape_fatal(st->sdcsel_shape, "stop"));
		get_a_poly_info();
		sprite_select_wnd_as_sprite1();
		sprite_set_1_size(st->update_rc.left, st->update_rc.right,
						  st->update_rc.top, st->update_rc.bottom);
		st->rect_union_rc = st->rect_clip;
		if (st->opponenttype != 0
			&& st->prev_car_index != st->car_index) {
			sprite_select_wnd_as_sprite1();
			if (video_flag5_is0 == 0) {
				nullsub_2(opp_res,
						  (unsigned short)(st->opponenttype + '0'));
				sprite_putimage_transparent(
					oppresources[st->opponenttype],
					MENU_OPP_CAR_PANEL_X, 0);
			} else {
				sprite_putimage_and_alt(st->opp_wnd->sprite_bitmapptr,
										MENU_OPP_CAR_PANEL_X, 0);
			}
		}
		sprite_copy_2_to_1();
		sprite_set_1_size(st->update_rc.left, st->update_rc.right,
						  st->update_rc.top, st->update_rc.bottom);
		mouse_draw_opaque_check();
		sprite_putimage(wndsprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		st->prev_car_index = st->car_index;
	}

	/* ---- hover change: redraw outline ---- */
	if (st->hover_index != st->prev_hover) {
		if (st->prev_hover != MENU_SENTINEL_U8) {
			sprite_copy_2_to_1();
			sprite_set_1_size(
				st->button_x1[st->prev_hover],
				(unsigned short)((st->button_x2[st->prev_hover]
								  + video_flag2_is1)
								 & video_flag3_isFFFF),
				st->button_y1[st->prev_hover],
				(unsigned short)(st->button_y2[st->prev_hover]
								 + MENU_CAR_HOVER_OUTLINE_PAD_Y));
			mouse_draw_opaque_check();
			sprite_putimage(wndsprite->sprite_bitmapptr);
			mouse_draw_transparent_check();
			video_refresh();
			sprite_copy_2_to_1();
		}
		menu_reset_idle_timers();
		st->prev_hover = st->hover_index;
	}

	/* Only copy background if the 3D frame was not already built */
	if (!need_blit) {
		sprite_copy_2_to_1();
	}

	/* ---- blink + present ---- */
	st->rotation_delta = mouse_update_menu_blink(
		st->hover_index,
		st->button_x1, st->button_x2,
		st->button_y1, st->button_y2,
		camera_view_matrix, object_visibility_state);
	if (st->rotation_delta == 0) {
		st->rotation_delta = MENU_KEY_SMALL_STEP;
	}

	/* ---- idle timeout ---- */
	idle_counter += st->rotation_delta;
	if (idle_counter > MENU_IDLE_TIMEOUT_LONG) {
		idle_counter = 0;
		idle_expired++;
	}

	if (idle_expired != 0) {
		st->hover_index = MENU_CAR_BTN_DONE;
		if (st->rendered_flag) {
			car_do_done(st);
			self->_wants_pop = 1;
			self->_modal_result = 1;
		}
	}
}

static void car_on_destroy(UIScreen *self)
{
	CarMenuState *st = (CarMenuState *)self->userdata;
	if (!st) return;

	sprite_free_wnd(wndsprite);
	unload_resource(st->carres);
	shape3d_free_car_shapes();
	if (st->opponenttype != 0 && video_flag5_is0 != 0
		&& st->opp_wnd != 0) {
		sprite_free_wnd(st->opp_wnd);
	}
	if (st->opponenttype == 0) {
		unload_resource(miscptr);
	}
	mmgr_free(st->sdcsel_shape);
	mouse_draw_opaque_check();
	idle_expired = 0;

	free(st);
	self->userdata = NULL;
}

static UIScreen *make_car_screen(
	char *caridptr,
	unsigned char *materialofs,
	unsigned char *transmissionofs,
	unsigned int opponenttype)
{
	UIScreen *scr;
	CarMenuState *st;
	const char *findfile;
	int i;

	st = (CarMenuState *)calloc(1, sizeof(CarMenuState));
	st->caridptr = caridptr;
	st->materialofs = materialofs;
	st->transmissionofs = transmissionofs;
	st->opponenttype = opponenttype;

	st->local_transshape.pos = carmenu_carpos;
	st->local_transshape.shapeptr = &game3dshapes[MENU_CAR_SHAPE_INDEX];
	st->local_transshape.rotvec.x = 0;
	st->local_transshape.rotvec.y = 0;
	st->local_transshape.shape_visibility_threshold =
		MENU_CAR_VISIBILITY_DEFAULT;

	timertestflag_copy = timertestflag;
	if (timertestflag_copy != 0) {
		st->local_transshape.rectptr = &st->rect_clip;
		st->local_transshape.ts_flags = MENU_CAR_TSFLAG_CLIPPED;
	} else {
		st->local_transshape.rectptr = 0;
		st->local_transshape.ts_flags = 0;
	}

	ensure_file_exists(2);
	findfile = file_combine_and_find(0, "car*", ".res");
	if (findfile == 0) {
		nullsub_1();
		free(st);
		return NULL;
	}

	do {
		const char *base = strrchr(findfile, '/');
		if (base != 0) base++;
		else base = findfile;

		if (strlen(base) >= MENU_CAR_NAME_LEN_MIN) {
			st->carids[st->car_count][0] = base[MENU_CAR_NAME_INDEX_0];
			st->carids[st->car_count][1] = base[MENU_CAR_NAME_INDEX_1];
			st->carids[st->car_count][2] = base[MENU_CAR_NAME_INDEX_2];
			st->carids[st->car_count][3] = base[MENU_CAR_NAME_INDEX_3];
			st->carids[st->car_count][4] = 0;
			st->car_count++;
			if (st->car_count >= MENU_CAR_COUNT_MAX) break;
		}
		findfile = file_find_next_alt();
	} while (findfile != 0);

	nullsub_1();

	if (st->car_count > 1) {
		unsigned char ii, jj;
		for (ii = 0; ii + 1 < st->car_count; ++ii) {
			for (jj = (unsigned char)(ii + 1); jj < st->car_count; ++jj) {
				if (strcmp(st->carids[ii], st->carids[jj]) > 0) {
					char tmp[5];
					strcpy(tmp, st->carids[ii]);
					strcpy(st->carids[ii], st->carids[jj]);
					strcpy(st->carids[jj], tmp);
				}
			}
		}
	}

	for (st->car_index = 0; st->car_index < st->car_count;
		 ++st->car_index) {
		if (st->carids[st->car_index][0] == caridptr[0]
			&& st->carids[st->car_index][1] == caridptr[1]
			&& st->carids[st->car_index][2] == caridptr[2]
			&& st->carids[st->car_index][3] == caridptr[3])
			break;
	}
	if (st->car_index >= st->car_count) st->car_index = 0;

	waitflag = MENU_WAITFLAG_CAR_MENU;
	backlights_paint_override = MENU_CAR_BACKLIGHTS_PAINT_ID;

	st->sdcsel_shape = file_load_shape2d_fatal_thunk("sdcsel");

	if (opponenttype == 0) {
		miscptr = file_load_resfile("misc");
	}

	if (opponenttype != 0) {
		unsigned short opp_w = 0;
		unsigned short opp_h = 0;
		menu_clip_rect.right = MENU_OPP_CAR_PANEL_X;
		if (video_flag5_is0 != 0) {
			unsigned short *dims =
				(unsigned short *)oppresources[opponenttype];
			opp_w = dims[0];
			opp_h = dims[1];
			st->opp_wnd = sprite_make_wnd(opp_w, opp_h,
										   MENU_SCREEN_DEPTH);
			setup_mcgawnd2();
			sprite_clear_sprite1_color(0);
			nullsub_2(opp_res, (unsigned short)(opponenttype + '0'));
			sprite_putimage_transparent(oppresources[opponenttype], 0, 0);
			sprite_clear_shape_alt(st->opp_wnd->sprite_bitmapptr, 0, 0);
		}
	} else {
		menu_clip_rect.right = MENU_SCREEN_WIDTH;
	}

	st->prev_car_index = MENU_SENTINEL_U8;
	st->prev_hover = MENU_SENTINEL_U8;
	st->redraw_state = 3;
	for (i = 0; i < MENU_CAR_BUTTON_COUNT; i++) {
		st->button_x1[i] = carmenu_buttons_y1[i];
		st->button_x2[i] = carmenu_buttons_y2[i];
		st->button_y1[i] = carmenu_buttons_x1[i];
		st->button_y2[i] = carmenu_buttons_x2[i];
	}
	menu_reset_idle_timers();
	set_projection(MENU_CAR_PROJ_ANGLE_X, MENU_CAR_PROJ_ANGLE_Y,
				   MENU_SCREEN_WIDTH, MENU_CAR_PROJ_HEIGHT);
	timer_get_delta_alt();
	init_polyinfo();

	wndsprite = sprite_make_wnd(MENU_SCREEN_WIDTH, MENU_SCREEN_HEIGHT,
								MENU_SCREEN_DEPTH);

	scr = ui_screen_alloc();
	scr->on_event   = car_on_event;
	scr->on_render  = car_on_render;
	scr->on_destroy = car_on_destroy;
	scr->userdata   = st;

	return scr;
}

/* --- run_car_menu --- */

void run_car_menu(
	char* caridptr,
	unsigned char* materialofs,
	unsigned char* transmissionofs,
	unsigned int opponenttype
)
{
	UIScreen *scr = make_car_screen(caridptr, materialofs,
									transmissionofs, opponenttype);
	if (scr) {
		ui_screen_run_modal(scr);
	}
}

/* --- run_intro_ --- */

/** @brief Show the "prod" and "titl" intro screens with input/timeout.
 *
 * @return Non-zero if interrupted by user input.
 */

// Intro sequence: show "prod" and "titl" screens and wait for input/timeout
/** @brief Run intro.
 * @return Function result.
 */
unsigned short run_intro_(void) {

	unsigned short result;
	struct SHAPE2D * shape;

	mouse_draw_opaque_check();
	sprite_copy_2_to_1_clear();
	mouse_draw_transparent_check();
	sprite_select_wnd_as_sprite1_and_clear();

	shape = (struct SHAPE2D *)locate_shape_fatal(tempdataptr, "prod");
	if (shape->s2d_pos_y != 0) {
		waitflag = MENU_WAITFLAG_INTRO_PROD;
	} else {
		waitflag = MENU_WAITFLAG_INTRO_DEFAULT;
	}

	sprite_blit_shape_to_sprite1(shape);
	result = sprite_blit_to_video(wndsprite, MENU_BLIT_MODE_FULL);
	if (result != 0) {
		return result;
	}

	result = input_repeat_check_intro_stable(MENU_TIMEOUT_INTRO_WAIT);
	if (result != 0) {
		return result;
	}

	sprite_select_wnd_as_sprite1_and_clear();
	waitflag = MENU_WAITFLAG_INTRO_DEFAULT;

	shape = (struct SHAPE2D *)locate_shape_fatal(tempdataptr, "titl");
	sprite_blit_shape_to_sprite1(shape);
	result = sprite_blit_to_video(wndsprite, MENU_BLIT_MODE_FULL);
	if (result != 0) {
		return result;
	}

	result = input_repeat_check_intro_stable(MENU_TIMEOUT_INTRO_WAIT);
	return result;
}

/* --- run_intro_looped_ --- */

/** @brief Run the full intro loop: title screens, 3-D flyover, credits.
 *
 * @return Non-zero if interrupted by user input.
 */

// Run the intro sequence, loading resources and credit screen as needed
/** @brief Run intro looped.
 * @return Function result.
 */
unsigned short  run_intro_looped_(void) {

	unsigned short result = 0;

	file_load_audiores("skidtitl", "skidms", "TITL");

	tempdataptr = file_load_resource(2, "sdtitl");
	wndsprite = sprite_make_wnd(MENU_SCREEN_WIDTH, MENU_SCREEN_HEIGHT, MENU_SCREEN_DEPTH);

	result = run_intro_();

	sprite_free_wnd(wndsprite);
	mmgr_free(tempdataptr);

	if (result == 0) {
		result = setup_intro();
		if (result == 0) {
			tempdataptr = file_load_resource(2, "sdcred");
			wndsprite = sprite_make_wnd(MENU_SCREEN_WIDTH, MENU_SCREEN_HEIGHT, MENU_SCREEN_DEPTH);
			sprite_select_wnd_as_sprite1_and_clear();
			sprite_blit_to_video(wndsprite, 0);
			result = load_intro_resources();
			sprite_free_wnd(wndsprite);
			mmgr_free(tempdataptr);
		}
	}

	audio_unload();
	return result;
}

/* --- menu_navigate --- */

/** @brief Choose the nearest menu button in the requested direction.
 *
 * @param current  Index of the currently selected button.
/** @brief Direction.
 * @param count Parameter `count`.
 * @return Function result.
 */

// Choose the nearest button in the requested direction using button geometry.
static unsigned char __attribute__((unused)) menu_navigate(
	unsigned char current,
	int dir_x,
	int dir_y,
	const unsigned short * x1,
	const unsigned short * x2,
	const unsigned short * y1,
	const unsigned short * y2,
	unsigned short count
)
{
	unsigned short best = current;
	unsigned short best_primary = 65535;
	unsigned short best_secondary = 65535;
	int cur_x;
	int cur_y;
	unsigned short i;

	cur_x = (int)(x1[current] + x2[current]) / 2;
	cur_y = (int)(y1[current] + y2[current]) / 2;

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
			best = (unsigned char)i;
			best_primary = primary;
			best_secondary = secondary;
		}
	}

	return best;
}

/* --- run_menu_ --- */

/* Context for main menu selection-change callback */
struct main_menu_ctx {
	struct SPRITE *sprite;
	unsigned short blit_mode;
};

static void main_menu_on_selection(unsigned char sel, void *ctx_ptr) {
	struct main_menu_ctx *ctx = (struct main_menu_ctx *)ctx_ptr;
	(void)sel;
	sprite_select_wnd_as_sprite1();
	sprite_blit_to_video(ctx->sprite, ctx->blit_mode);
	ctx->blit_mode = MENU_BLIT_MODE_PARTIAL;
	sprite_copy_2_to_1();
}

/** @brief Main menu selection loop — returns the chosen menu item.
 *
 * @return Selected menu index, or sentinel on escape.
 */

// Main menu selection loop
/** @brief Run menu.
 * @return Function result.
 */
char run_menu_(void) {

	UIButtonMenu btnmenu;
	struct main_menu_ctx cb_ctx;
	unsigned char result;
	void * menu_resource;
	void * shape_resource;
	struct SPRITE * sprite_ptr;

	show_waiting();
	waitflag = MENU_WAITFLAG_INTRO_DEFAULT;

	sprite_ptr = sprite_make_wnd(MENU_SCREEN_WIDTH, MENU_SCREEN_HEIGHT, MENU_SCREEN_DEPTH);
	wndsprite = sprite_ptr;

	menu_resource = file_load_resource(2, "sdmsel");
	sprite_select_wnd_as_sprite1();
	shape_resource = locate_shape_fatal(menu_resource, "scrn");
	sprite_blit_shape_to_sprite1(shape_resource);
	mmgr_free(menu_resource);

	memset(&btnmenu, 0, sizeof(btnmenu));
	btnmenu.count = MENU_MAIN_BUTTON_COUNT;
	memcpy(btnmenu.x1, menu_buttons_x1, sizeof(unsigned short) * btnmenu.count);
	memcpy(btnmenu.x2, menu_buttons_x2, sizeof(unsigned short) * btnmenu.count);
	memcpy(btnmenu.y1, menu_buttons_y1, sizeof(unsigned short) * btnmenu.count);
	memcpy(btnmenu.y2, menu_buttons_y2, sizeof(unsigned short) * btnmenu.count);
	btnmenu.sprite_hi = camera_view_matrix;
	btnmenu.sprite_lo = object_visibility_state;
	btnmenu.default_sel = 0;
	btnmenu.idle_timeout = MENU_IDLE_TIMEOUT_SHORT;
	btnmenu.nav_mode = UI_NAV_BOTH_LR_SWAP;

	cb_ctx.sprite = sprite_ptr;
	cb_ctx.blit_mode = MENU_BLIT_MODE_FULL;
	btnmenu.selection_cb = main_menu_on_selection;
	btnmenu.selection_cb_ctx = &cb_ctx;

	{
		UIScreen *scr = ui_screen_from_button_menu(&btnmenu);
		int modal = ui_screen_run_modal(scr);
		result = UI_MODAL_TO_SEL(modal);
	}

	sprite_free_wnd(sprite_ptr);

	if (result == UI_SEL_TIMEOUT) {
		return 0;
	}
	return (char)result;
}

