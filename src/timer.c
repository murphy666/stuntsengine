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

/* timer.c — Timer system extracted from stunts.c */
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "stunts.h"
#include "game_timing.h"
#include "shape2d.h"
#include "timer.h"

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC CLOCK_REALTIME
#endif

/* Forward declarations are now in data_game.h and other module headers, included via stunts.h. */
typedef void (*timer_callback_func_local)(void);

/* Timer-private state — moved from data_global.c */
static unsigned long  timer_callback_counter      = 0;
static unsigned long  last_timer_callback_counter  = 0;
static unsigned long  timer_copy_unk               = 0;
static unsigned short frame_timer_state            = 0;
static unsigned short frame_counter_state          = 0;
static unsigned char  audio_sfx_enabled_flag       = 0;
static unsigned char  audio_music_state            = 0;
static unsigned       sound_effects_queue          = 0;
static unsigned       music_tempo_state            = 0;
static unsigned       sound_volume_control         = 0;
static unsigned char  vibration_feedback_state     = 0;   /* was [2]={0,3}; byte 0 is the active value */
static unsigned int animation_sequence_state = 0;
static unsigned int animation_sequence_data = 0;
static timer_callback_func_local g_timer_callbacks[5] = { 0, 0, 0, 0, 0 };
static unsigned long g_timer_last_ms = 0;
static unsigned char g_timer_dispatching = 0;
static const unsigned long g_timer_counter_units_per_tick = 5UL;
static unsigned long timer_now_ms(void);
static void timer_dispatch_elapsed(void);

/* --- timer_get_counter_from_words --- */

/** @brief Timer get counter from words.
 * @return Function result.
 */
unsigned long timer_get_counter_from_words(void)
{
	unsigned short low;
	unsigned short high;

	low = frame_timer_state;
	high = frame_counter_state;

	return ((unsigned long)high << 16) | low;
}

/* Test-seam: lets unit-tests inject the hardware-timer word state.    */
/** @brief Timer test set frame words.
 * @param low Parameter `low`.
 * @param high Parameter `high`.
 */
void timer_test_set_frame_words(unsigned short low, unsigned short high)
{
	frame_timer_state   = low;
	frame_counter_state = high;
}

/* --- timer_get_counter --- */

/** @brief Timer get counter.
 * @return Function result.
 */
unsigned long timer_get_counter(void)
{
	unsigned long val;

	timer_dispatch_elapsed();
	val = timer_callback_counter;

	return val;
}

/** @brief Timer get dispatch hz.
 * @return Function result.
 */
unsigned long timer_get_dispatch_hz(void)
{
	unsigned long dispatch_hz;
	dispatch_hz = 1000UL / GAME_TIMER_MS_EFF;
	if (dispatch_hz == 0UL) {
		dispatch_hz = GAME_TIMER_HZ;
	}
	return dispatch_hz;
}

/** @brief Timer get display hz.
 * @return Function result.
 */
unsigned long timer_get_display_hz(void)
{
	static unsigned long cached_hz = 0UL;
	if (cached_hz == 0UL) {
		const char *env_hz = getenv("STUNTS_DISPLAY_HZ");
		unsigned long hz = GAME_DISPLAY_HZ;
		if (env_hz != 0 && *env_hz != '\0') {
			int parsed = atoi(env_hz);
			if (parsed < 10) {
				parsed = 10;
			}
			if (parsed > 120) {
				parsed = 120;
			}
			hz = (unsigned long)parsed;
		}
		cached_hz = hz;
	}
	return cached_hz;
}

/** @brief Timer get counter units per second.
 * @return Function result.
 */
unsigned long timer_get_counter_units_per_second(void)
{
	return timer_get_dispatch_hz() * g_timer_counter_units_per_tick;
}

/** @brief Timer get subticks for rate.
 * @param rate_hz Parameter `rate_hz`.
 * @param accum Parameter `accum`.
 * @return Function result.
 */
unsigned long timer_get_subticks_for_rate(unsigned long rate_hz, unsigned long *accum)
{
	unsigned long dispatch_hz;
	unsigned long subticks;

	if (accum == 0 || rate_hz == 0UL) {
		return 0UL;
	}

	dispatch_hz = timer_get_dispatch_hz();
	*accum += rate_hz;
	subticks = *accum / dispatch_hz;
	*accum %= dispatch_hz;
	return subticks;
}

/** @brief Timer get counter step for rate.
 * @param rate_hz Parameter `rate_hz`.
 * @param accum Parameter `accum`.
 * @return Function result.
 */
unsigned long timer_get_counter_step_for_rate(unsigned long rate_hz, unsigned long *accum)
{
	unsigned long units_per_sec;
	unsigned long step;

	if (accum == 0 || rate_hz == 0UL) {
		return 1UL;
	}

	units_per_sec = timer_get_counter_units_per_second();
	*accum += units_per_sec;
	step = *accum / rate_hz;
	*accum %= rate_hz;
	if (step == 0UL) {
		step = 1UL;
	}
	return step;
}

/** @brief Timer wait for counter rate.
 * @param rate_hz Parameter `rate_hz`.
 * @param next_counter Parameter `next_counter`.
 * @param accum Parameter `accum`.
 */
void timer_wait_for_counter_rate(unsigned long rate_hz, unsigned long *next_counter, unsigned long *accum)
{
	unsigned long current;
	unsigned long step;

	if (next_counter == 0 || accum == 0 || rate_hz == 0UL) {
		return;
	}

	current = timer_get_counter();
	if (*next_counter == 0UL || current + 250UL < *next_counter || current > *next_counter + 250UL) {
		*next_counter = current;
		*accum = 0UL;
	}

	while ((long)(*next_counter - current) > 0) {
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = GAME_YIELD_NS;
		nanosleep(&ts, NULL);
		current = timer_get_counter();
	}

	step = timer_get_counter_step_for_rate(rate_hz, accum);
	*next_counter += step;
}

/* --- timer_get_delta --- */

/** @brief Timer get delta.
 * @return Function result.
 */
unsigned long timer_get_delta(void)
{
	/* Drive the timer dispatch first (mirrors DOS ISR firing before any read).
	 * Without this, menu loops that call timer_get_delta_alt() directly never
	 * advance timer_callback_counter and spin at CPU speed. */
	unsigned long curr = timer_get_counter();
	unsigned long result = curr - last_timer_callback_counter;
	last_timer_callback_counter = curr;

	/* If no new tick fired yet, yield for ~1 ms.  This prevents callers that
	 * loop on delta==0 from spinning at full CPU speed and triggering the
/** @brief Fallback.
 * @param iterations Parameter `iterations`.
 * @param result Parameter `result`.
 * @return Function result.
 */
	if (result == 0) {
		struct timespec ts;
		ts.tv_sec  = 0;
		ts.tv_nsec = GAME_YIELD_NS;
		nanosleep(&ts, NULL);
	}

	return result;
}

/* --- timer_get_delta_alt --- */

/** @brief Timer get delta alt.
 * @return Function result.
 */
unsigned long timer_get_delta_alt(void)
{
	return timer_get_delta();
}

/* --- timer_custom_delta --- */

/** @brief Timer custom delta.
 * @param ticks Parameter `ticks`.
 * @return Function result.
 */
unsigned long timer_custom_delta(unsigned long ticks)
{
	return timer_get_counter() - ticks;
}

/* --- timer_reset --- */

/** @brief Timer reset.
 */
void timer_reset()
{
	timer_callback_counter = 0;
	last_timer_callback_counter = 0;
	g_timer_last_ms = timer_now_ms();
}

/* --- timer_copy_counter --- */

/** @brief Timer copy counter.
 * @param ticks Parameter `ticks`.
 * @return Function result.
 */
unsigned long timer_copy_counter(unsigned long ticks)
{
	timer_copy_unk = timer_get_counter() + ticks;
	return timer_copy_unk;
}

/* --- timer_wait_for_dx --- */

/** @brief Timer wait for dx.
 * @return Function result.
 */
unsigned long timer_wait_for_dx(void)
{
	unsigned long res;
	unsigned long spin_guard = 0;
	do {
		res = timer_get_counter();
		spin_guard++;
		if (spin_guard >= 8388608UL) {
			break;
		}
	} while (res < timer_copy_unk);
	
	return res;
}

/* --- timer_compare_dx --- */

/** @brief Timer compare dx.
 * @return Function result.
 */
int timer_compare_dx(void)
{
	return timer_get_counter() >= timer_copy_unk;
}

/* --- timer_wait_ticks_and_get_counter --- */

/** @brief Timer wait ticks and get counter.
 * @param ticks Parameter `ticks`.
 * @return Function result.
 */
unsigned long timer_wait_ticks_and_get_counter(unsigned long ticks)
{
	unsigned long target, res;
	target = timer_get_counter() + ticks;
	
	do {
		res = timer_get_counter();
	} while (res < target);
	
	return res;
}

/* --- timer_now_ms --- */

/** @brief Timer now ms.
 * @return Function result.
 */
static unsigned long timer_now_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (unsigned long)(ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL);
}

/* --- timer_dispatch_elapsed --- */

/** @brief Timer dispatch elapsed.
 * @return Function result.
 */
static void timer_dispatch_elapsed(void)
{
	unsigned long now_ms;
	unsigned long elapsed_ms;
	unsigned long ticks_to_dispatch;
	unsigned long tick_index;
	int callback_index;

	if (g_timer_dispatching != 0) {
		return;
	}

	now_ms = timer_now_ms();
	if (g_timer_last_ms == 0) {
		g_timer_last_ms = now_ms;
		return;
	}

	if (now_ms <= g_timer_last_ms) {
		return;
	}

	elapsed_ms = now_ms - g_timer_last_ms;
	/* DOS PIT was reprogrammed to GAME_TIMER_HZ (100 Hz = GAME_TIMER_MS ms/tick).
	 * GAME_TIMER_MS_EFF stretches that period by GAME_SPEED_PERCENT so the
	 * entire game logic (physics, AI, input, menus) slows/speeds uniformly. */
	ticks_to_dispatch = elapsed_ms / GAME_TIMER_MS_EFF;
	if (ticks_to_dispatch == 0) {
		return;
	}

	if (ticks_to_dispatch > 64UL) {
		ticks_to_dispatch = 64UL;
	}

	g_timer_last_ms += ticks_to_dispatch * GAME_TIMER_MS_EFF;

	g_timer_dispatching = 1;
	for (tick_index = 0; tick_index < ticks_to_dispatch; tick_index++) {
		timer_callback_counter += g_timer_counter_units_per_tick;
		for (callback_index = 0; callback_index < 5; callback_index++) {
			if (g_timer_callbacks[callback_index] != (timer_callback_func_local)0) {
				g_timer_callbacks[callback_index]();
			}
		}
	}
	g_timer_dispatching = 0;
}

/* --- timer_setup_interrupt --- */

/** @brief Timer setup interrupt.
 */
void timer_setup_interrupt(void)
{
	int i;

	music_tempo_state = 5;
	sound_volume_control = 5;
	audio_sfx_enabled_flag = 0;
	audio_music_state = 1;

	vibration_feedback_state = 0;
	for (i = 0; i < 5; i++) {
		g_timer_callbacks[i] = (timer_callback_func_local)0;
	}

	g_timer_last_ms = timer_now_ms();
	timer_callback_counter = 0;
	last_timer_callback_counter = 0;

	add_exit_handler(timer_stop_dispatch);
}

/* --- timer_stop_dispatch --- */

/** @brief Timer stop dispatch.
 */
void timer_stop_dispatch(void)
{
	g_timer_dispatching = 0;
}

/* --- timer_setup_interrupt_countdown --- */

/* Port of nopsub_30180: enable countdown while keeping timer ISR installed. */
/** @brief Timer setup interrupt countdown.
 */
void timer_setup_interrupt_countdown(void)
{
	timer_setup_interrupt();
	sound_effects_queue = 100;
	audio_sfx_enabled_flag = 1;
	audio_music_state = 1;
}

/* --- timer_enable_countdown --- */

/** @brief Timer enable countdown.
 */
void timer_enable_countdown(void)
{
	timer_setup_interrupt_countdown();
}

/* --- timer_reg_callback --- */

/**
 * timer_reg_callback - Register a timer interrupt callback
 * 
 * @param callback Far pointer to the callback function
 * 
 * Searches the timerintr array for an empty slot and adds the callback.
 * Calls fatal_error if no free slots are available.
 */
void timer_reg_callback(void * callback) {
	int i;
	timer_callback_func_local cb = (timer_callback_func_local)callback;

	/* Avoid duplicate registrations. */
	for (i = 0; i < 5; i++) {
		if (g_timer_callbacks[i] == cb) {
			return;
		}
	}

	for (i = 0; i < 5; i++) {
		if (g_timer_callbacks[i] == (timer_callback_func_local)0) {
			g_timer_callbacks[i] = cb;
			return;
		}
	}
	fatal_error("NO ROOM LEFT ON TIMER INTERRUPT ROUTINE LIST\r\n");
}

/* --- timer_remove_callback --- */

/**
 * timer_remove_callback - Remove a timer interrupt callback
 * 
 * @param callback Far pointer to the callback function to remove
 * 
 * Searches the timerintr array for the callback and removes it,
 * shifting subsequent entries down to fill the gap.
 */
void timer_remove_callback(void * callback) {
	int i, j;
	timer_callback_func_local cb = (timer_callback_func_local)callback;

	for (i = 0; i < 5; i++) {
		if (g_timer_callbacks[i] == cb) {
			for (j = i; j < 4; j++) {
				g_timer_callbacks[j] = g_timer_callbacks[j + 1];
			}
			g_timer_callbacks[4] = (timer_callback_func_local)0;
			return;
		}
	}
}

/* --- set_add_value --- */

/**
 * set_add_value - Set a timer deadline
 * 
 * @param delta_lo Low word of ticks to add
 * @param delta_hi High word of ticks to add
 * 
/** @brief Set add value.
 * @param delta_lo Parameter `delta_lo`.
 * @param delta_hi Parameter `delta_hi`.
 */
void set_add_value(unsigned int delta_lo, unsigned int delta_hi) {
	unsigned long curr, deadline;
	
	curr = timer_get_counter();
	deadline = curr + ((unsigned long)delta_hi << 16) + delta_lo;
	animation_sequence_state = (unsigned int)(deadline & 65535);
	animation_sequence_data = (unsigned int)(deadline >> 16);
/** @brief Wait.
 * @param animation_sequence_data Parameter `animation_sequence_data`.
 * @param timer_deadline_reached Parameter `timer_deadline_reached`.
 * @return Function result.
 */
	/* Return immediately — no wait (matches original ASM) */
}

/* --- timer_deadline_reached --- */

/**
 * timer_deadline_reached - Check if timer has reached deadline
 * 
 * @return 1 if timer_get_counter() >= animation_sequence_state:animation_sequence_data, 0 otherwise
 */
int timer_deadline_reached(void) {
	unsigned long curr, deadline;
	
	curr = timer_get_counter();
	deadline = ((unsigned long)animation_sequence_data << 16) + animation_sequence_state;
	
	return (curr >= deadline) ? 1 : 0;
}

/* --- timer_wait_ticks --- */

/**
 * timer_wait_ticks - Wait for a specified number of ticks
 * 
 * @param delta Ticks to wait
 * 
/** @brief Timer get counter.
 * @param delta Parameter `delta`.
 * @return Function result.
 */
void timer_wait_ticks(unsigned long delta) {
	unsigned long curr, deadline;
	
	curr = timer_get_counter();
	deadline = curr + delta;
	
	do {
		curr = timer_get_counter();
	} while (curr < deadline);
}

