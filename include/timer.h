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

#ifndef TIMER_H
#define TIMER_H

unsigned long timer_get_counter_from_words(void);
unsigned long timer_get_counter(void);
unsigned long timer_get_dispatch_hz(void);
unsigned long timer_get_display_hz(void);
unsigned long timer_get_counter_units_per_second(void);
unsigned long timer_get_subticks_for_rate(unsigned long rate_hz, unsigned long *accum);
unsigned long timer_get_counter_step_for_rate(unsigned long rate_hz, unsigned long *accum);
void timer_wait_for_counter_rate(unsigned long rate_hz, unsigned long *next_counter, unsigned long *accum);
unsigned long timer_get_delta(void);
unsigned long timer_get_delta_alt(void);
unsigned long timer_custom_delta(unsigned long ticks);
void timer_reset(void);
unsigned long timer_copy_counter(unsigned long ticks);
unsigned long timer_wait_for_dx(void);
int timer_compare_dx(void);
unsigned long timer_wait_ticks_and_get_counter(unsigned long ticks);
void timer_setup_interrupt(void);
void timer_stop_dispatch(void);
void timer_setup_interrupt_countdown(void);
void nopsub_30180(void);
void timer_reg_callback(void * callback);
void timer_remove_callback(void * callback);
void set_add_value(unsigned int delta_lo, unsigned int delta_hi);
int timer_deadline_reached(void);
void timer_wait_ticks(unsigned long delta);

/* Test-seam: inject simulated hardware-timer state words for unit tests. */
void timer_test_set_frame_words(unsigned short low, unsigned short high);

#endif /* TIMER_H */
