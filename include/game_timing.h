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

/**
 *
 * ╔══════════════════════════════════════════════════════════════════════╗
 * ║  CALIBRATION KNOB — change GAME_SPEED_PERCENT to tune game speed   ║
 * ║                                                                      ║
 * ║   100  = original DOS 486 speed  (authentic)                        ║
 * ║    80  = 80 % speed  (slightly slower, good starting point)         ║
 * ║    60  = 60 % speed  (noticeably slower)                            ║
 * ║   120  = 20 % faster than DOS (not recommended)                     ║
 * ║                                                                      ║
 * ║  This single value scales ALL game logic uniformly:                 ║
 * ║    physics, AI, menus, timers, input repeat — everything.           ║
 * ║  The rendering refresh (GAME_VSYNC_HZ) is independent.             ║
 * ╚══════════════════════════════════════════════════════════════════════╝
 */
#ifndef GAME_TIMING_H
#define GAME_TIMING_H

/* -----------------------------------------------------------------------
 * SPEED CALIBRATION
 * 100 = authentic DOS speed.  Reduce if the game runs too fast on your
 * hardware; increase if it runs too slow.
 *
 * At 100 %:
 *   - DOS PIT fires at 100 Hz  (GAME_TIMER_MS_EFF = 10 ms per tick)
 *   - frame_callback divides by timer divisor = 100/framespersec = 5
 *   - elapsed_time2 (physics tick) advances at 100/5 = 20 Hz
 *   - update_car_speed() is calibrated for framespersec=20 (var_2=6)
 *   → physics rate matches calibration → authentic DOS acceleration
 *
 * At 50 %:  timer at 50 Hz → physics at 10 Hz → car accelerates at ½ DOS rate
 * ----------------------------------------------------------------------- */
#define GAME_SPEED_PERCENT   100UL

/* -----------------------------------------------------------------------
 * Game-logic timer  (DOS PIT, 100 Hz → 10 ms per tick at 100 % speed)
 *
 * GAME_TIMER_MS_EFF is the effective real-time milliseconds that must
 * elapse before one DOS timer tick is injected.  It is scaled by
 * GAME_SPEED_PERCENT:
 *   100 % → 10 ms/tick  (100 ticks/s  = authentic 486 PIT rate)
 *    80 % → 12 ms/tick  ( 83 ticks/s  = 20 % slower physics/AI)
 *    60 % → 16 ms/tick  ( 62 ticks/s  = 40 % slower)
 *
 * Formula: GAME_TIMER_MS_EFF = (1000 * 100) / (GAME_TIMER_HZ * GAME_SPEED_PERCENT)
 *                             = 100000 / (100 * GAME_SPEED_PERCENT)
 *                             = 1000 / GAME_SPEED_PERCENT
 * ----------------------------------------------------------------------- */
#define GAME_TIMER_HZ        100UL
#define GAME_TIMER_MS        (1000UL / GAME_TIMER_HZ)           /* 10 ms at 100 Hz */
#define GAME_TIMER_MS_EFF    (1000UL / GAME_SPEED_PERCENT)      /* scaled tick period */

/* -----------------------------------------------------------------------
 * VGA vertical retrace  (70 Hz → ≈14.3 ms per frame)
 * Independent of GAME_SPEED_PERCENT — screen always refreshes at 70 Hz.
 * ----------------------------------------------------------------------- */
#define GAME_VSYNC_HZ        70UL
#define GAME_VSYNC_MS        (1000UL / GAME_VSYNC_HZ)           /* 14 ms */
#define GAME_VSYNC_NS        (1000000000L / (long)GAME_VSYNC_HZ) /* 14 285 714 ns */

/* Unified display loop pacing target (menus + frame presents).
 * Physics runs at framespersec Hz (20) regardless of this value — only
 * the visual refresh rate changes.  Set STUNTS_DISPLAY_HZ env var at
 * runtime to override (e.g. STUNTS_DISPLAY_HZ=30 for slower machines). */
#define GAME_DISPLAY_HZ      60UL

/* -----------------------------------------------------------------------
 * Spin-yield duration
 * When a loop is waiting for the next timer tick or game frame it sleeps
 * this long before re-checking, preventing 100 % CPU burn.
 * ----------------------------------------------------------------------- */
#define GAME_YIELD_MS        1UL
#define GAME_YIELD_NS        (GAME_YIELD_MS * 1000000L)          /* 1 000 000 ns */

/* -----------------------------------------------------------------------
 * Menu/input pacing scale
 * 100 = original UI input speed, lower values make submenus less twitchy.
 * 45 matches a slower 486-like menu feel on modern SDL polling loops.
 * ----------------------------------------------------------------------- */
#define GAME_MENU_DELTA_PERCENT 45UL

#endif /* GAME_TIMING_H */
