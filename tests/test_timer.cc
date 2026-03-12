/*
 * Copyright (c) 2026 Stunts Engine Project
 *
 * Tests for timer.c pure/stateless functions:
 *   timer_get_dispatch_hz, timer_get_subticks_for_rate,
 *   timer_get_counter_step_for_rate, timer_get_counter_from_words
 */

#include <gtest/gtest.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include "game_timing.h"
#include "timer.h"

/* --- stubs for externs.h globals still needed by the test binary --- */
/* (all timer-private statics are now inside timer.c itself)          */

/* --- stub for add_exit_handler --- */
void add_exit_handler(void (*handler)(void)) { (void)handler; }

/* --- stub for fatal_error --- */
void fatal_error(const char *fmt, ...) { (void)fmt; abort(); }

} /* extern "C" */

/* ------------------------------------------------------------------ */
/* Timer.DispatchHz                                                     */
/* ------------------------------------------------------------------ */

TEST(Timer, DispatchHz)
{
    /* GAME_TIMER_MS_EFF = 10  =>  1000/10 = 100 */
    EXPECT_EQ(100UL, timer_get_dispatch_hz());
}

/* ------------------------------------------------------------------ */
/* Timer.CounterUnitsPerSecond                                          */
/* ------------------------------------------------------------------ */

TEST(Timer, CounterUnitsPerSecond)
{
    /* dispatch_hz=100, g_timer_counter_units_per_tick=5 => 500 */
    EXPECT_EQ(500UL, timer_get_counter_units_per_second());
}

/* ------------------------------------------------------------------ */
/* Timer.SubticksZeroAccum — first call with rate==dispatch_hz         */
/* ------------------------------------------------------------------ */

TEST(Timer, SubticksZeroAccum)
{
    unsigned long accum = 0;
    /* rate==100, dispatch_hz==100: accum becomes 100 => subticks=1, accum=0 */
    unsigned long subticks = timer_get_subticks_for_rate(100UL, &accum);
    EXPECT_EQ(1UL, subticks);
    EXPECT_EQ(0UL, accum);
}

/* ------------------------------------------------------------------ */
/* Timer.SubticksHalfRate — accumulation over two dispatch ticks        */
/* ------------------------------------------------------------------ */

TEST(Timer, SubticksHalfRate)
{
    unsigned long accum = 0;
    /* rate=50, dispatch_hz=100: each call adds 50; two calls needed for 1 subtick */
    unsigned long t1 = timer_get_subticks_for_rate(50UL, &accum);
    EXPECT_EQ(0UL, t1);
    EXPECT_EQ(50UL, accum);

    unsigned long t2 = timer_get_subticks_for_rate(50UL, &accum);
    EXPECT_EQ(1UL, t2);
    EXPECT_EQ(0UL, accum);
}

/* ------------------------------------------------------------------ */
/* Timer.SubticksDoubleRate — rate faster than dispatch                 */
/* ------------------------------------------------------------------ */

TEST(Timer, SubticksDoubleRate)
{
    unsigned long accum = 0;
    /* rate=200, dispatch_hz=100: each call yields 2 subticks */
    unsigned long t = timer_get_subticks_for_rate(200UL, &accum);
    EXPECT_EQ(2UL, t);
    EXPECT_EQ(0UL, accum);
}

/* ------------------------------------------------------------------ */
/* Timer.SubticksNullGuard                                              */
/* ------------------------------------------------------------------ */

TEST(Timer, SubticksNullGuard)
{
    EXPECT_EQ(0UL, timer_get_subticks_for_rate(100UL, nullptr));
}

TEST(Timer, SubticksZeroRateGuard)
{
    unsigned long accum = 0;
    EXPECT_EQ(0UL, timer_get_subticks_for_rate(0UL, &accum));
    EXPECT_EQ(0UL, accum); /* accum must be untouched */
}

/* ------------------------------------------------------------------ */
/* Timer.CounterStepNormalRate                                          */
/* ------------------------------------------------------------------ */

TEST(Timer, CounterStepAtDispatchHz)
{
    unsigned long accum = 0;
    /* rate=100: units_per_sec=500, step=500/100=5, accum=0 */
    unsigned long step = timer_get_counter_step_for_rate(100UL, &accum);
    EXPECT_EQ(5UL, step);
    EXPECT_EQ(0UL, accum);
}

TEST(Timer, CounterStepAtCounterHz)
{
    unsigned long accum = 0;
    /* rate=500 (== units_per_sec): step=500/500=1, accum=0 */
    unsigned long step = timer_get_counter_step_for_rate(500UL, &accum);
    EXPECT_EQ(1UL, step);
    EXPECT_EQ(0UL, accum);
}

TEST(Timer, CounterStepAccumulation)
{
    unsigned long accum = 0;
    /* rate=1000, units_per_sec=500: step=500/1000=0 -> clamped to 1; accum=500 */
    unsigned long step = timer_get_counter_step_for_rate(1000UL, &accum);
    EXPECT_EQ(1UL, step); /* minimum 1 */
    EXPECT_EQ(500UL, accum);

    /* Second call: accum=1000, step=1000/1000=1, accum=0 */
    step = timer_get_counter_step_for_rate(1000UL, &accum);
    EXPECT_EQ(1UL, step);
    EXPECT_EQ(0UL, accum);
}

/* ------------------------------------------------------------------ */
/* Timer.CounterStepMinimumOne                                          */
/* ------------------------------------------------------------------ */

TEST(Timer, CounterStepMinimumOne)
{
    unsigned long accum = 0;
    /* rate=9999 >> units_per_sec=500: step=500/9999=0 -> clamped to 1 */
    unsigned long step = timer_get_counter_step_for_rate(9999UL, &accum);
    EXPECT_GE(step, 1UL);
}

/* ------------------------------------------------------------------ */
/* Timer.CounterStepNullGuard                                           */
/* ------------------------------------------------------------------ */

TEST(Timer, CounterStepNullGuard)
{
    EXPECT_EQ(1UL, timer_get_counter_step_for_rate(100UL, nullptr));
}

TEST(Timer, CounterStepZeroRateGuard)
{
    unsigned long accum = 0;
    EXPECT_EQ(1UL, timer_get_counter_step_for_rate(0UL, &accum));
}

/* ------------------------------------------------------------------ */
/* Timer.CounterFromWords                                               */
/* ------------------------------------------------------------------ */

TEST(Timer, CounterFromWordsZero)
{
    timer_test_set_frame_words(0, 0);
    EXPECT_EQ(0UL, timer_get_counter_from_words());
}

TEST(Timer, CounterFromWordsLowOnly)
{
    timer_test_set_frame_words(4660, 0);
    EXPECT_EQ(4660UL, timer_get_counter_from_words());
}

TEST(Timer, CounterFromWordsHighOnly)
{
    timer_test_set_frame_words(0, 1);
    EXPECT_EQ(65536UL, timer_get_counter_from_words());
}

TEST(Timer, CounterFromWordsBoth)
{
    timer_test_set_frame_words(22136, 2);
    EXPECT_EQ(153208UL, timer_get_counter_from_words());
}
