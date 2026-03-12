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

#ifndef GAME_H
#define GAME_H

void loop_game(int command, int context_value, int frame_value);
void run_game(void);

/* run_game_session - play/hiscore/replay loop (called from main).
 * Runs run_game() in a loop until the player returns to the main menu.
 * Handles: race finish → end_hiscore() → view-replay or drive-again. */
void run_game_session(void);

/* run_main_session - top-level intro → menu → race lifecycle loop.
 * Extracted from main() (stunts.c). Contains the outer intro/menu while(1)
 * and the inner menu-dispatch while(1). Returns when the player quits. */
void run_main_session(void);

/* Intro sequence (moved from menu.c).
 * run_intro_        - show "prod"+"titl" screens, wait for input/timeout.
 * run_intro_looped_ - load audio/resources, run intro, credit screen. */
unsigned short run_intro_(void);
unsigned short run_intro_looped_(void);

/* Game state management (moved from stunts.c).
 * These implement the core game/physics update cycle and save-state. */
void           reset_replay_runtime_state(void);
void           init_carstate_from_simd(struct CARSTATE* playerstate, struct SIMD* simd,
                       char transmission, long posX, long posY, long posZ, short track_angle);
void           init_game_state(short arg);
void           restore_gamestate(unsigned short frame);
void           update_gamestate(void);

/* Configurable render refresh rate.
 * game_set_refresh_rate(hz) overrides GAME_REFRESH_HZ at runtime (0 = reset to default).
 * game_get_refresh_rate()   returns the currently active rate in Hz. */
void         game_set_refresh_rate(unsigned int hz);
unsigned int game_get_refresh_rate(void);

/* ------------------------------------------------------------------ */
/*  Generic game loop infrastructure                                    */
/*                                                                      */
/*  All game loops run through game_loop_run() for uniform pacing,     */
/*  physics isolation, and FPS telemetry.                              */
/*                                                                      */
/*  Callback return values:                                             */
/*    GAME_LOOP_CONTINUE – iteration done; call render, then loop      */
/*    GAME_LOOP_SKIP     – physics-only step; skip render, loop again  */
/*    GAME_LOOP_BREAK    – exit the loop                               */
/* ------------------------------------------------------------------ */
typedef enum {
	GAME_LOOP_CONTINUE = 0,
	GAME_LOOP_SKIP     = 2,
	GAME_LOOP_BREAK    = 1
} game_loop_status_t;

typedef game_loop_status_t (*game_loop_tick_cb)(void *ctx);
typedef game_loop_status_t (*game_loop_render_cb)(void *ctx);

typedef struct {
	game_loop_tick_cb   tick;       /* physics/logic step; NULL = no tick   */
	game_loop_render_cb render;     /* frame present;  NULL = no pacing     */
	void               *ctx;       /* opaque context passed to callbacks    */
	int                 fps_debug; /* 1 = log FPS to stdout once per second */
} game_loop_t;

/* Run loop until a callback returns GAME_LOOP_BREAK.
 * Paces render calls at game_get_refresh_rate() Hz. */
void game_loop_run(const game_loop_t *loop);

/* ------------------------------------------------------------------
 * Game setup / lifecycle helpers (moved from stunts.c)
 * ------------------------------------------------------------------ */
/* DOS DS==SS segment check — always 1 in flat memory model */
int    compare_ds_ss(void);
void   show_waiting(void);
void   call_exitlist2(void);
unsigned short setup_intro(void);
void   audio_unload(void);
void   input_push_status(void);
void   input_pop_status(void);
void   setup_aero_trackdata(void *carresptr, int is_opponent);
void   init_plantrak_(void);
int    setup_player_cars(void);
void   free_player_cars(void);
void   load_palandcursor(void);

/* Kevin random number generator (defined in stunts.c) */
void   init_kevinrandom(const char* seed);
void   get_kevinrandom_seed(char* seed);
int    get_kevinrandom(void);
int    get_super_random(void);

/* Global state — defined in src/gamemech.c */

#ifdef GAME_IMPL
#  define _GAME_
#else
#  define _GAME_ extern
#endif

_GAME_ void *g_dast_shape_ptr;

#undef _GAME_

#endif /* GAME_H */
