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

#ifndef OPL2_BACKEND_H
#define OPL2_BACKEND_H

/*
 * opl2_backend.h - Pure-C OPL2 (YM3812) emulation interface
 *
 * Thin wrapper around the MAME/adplug fmopl.c emulator.
 * All audio calls that need OPL2 go through this interface.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialise the OPL2 emulator at the given sample rate (Hz).
 * Must be called before any other opl2_* function.
 * Safe to call multiple times (re-creates the chip).
 */
void opl2_init(int sample_rate);

/*
 * Silence and reset all OPL2 registers.
 * Re-enables the waveform-select bit (reg 1 bit5) which is required
 * for WS != 0 (sine variants used by ADENG1.VCE).
 */
void opl2_reset(void);

/*
 * Write one OPL2 register.
 * Equivalent to writing address 'reg' to the OPL2 address port and
 * 'val' to the data port.
 */
void opl2_write(int reg, int val);

/*
 * Render 'n' mono 16-bit samples into buf[0..n-1].
 * Zeros the buffer if the backend is not ready.
 */
void opl2_generate(short *buf, int n);

/* Returns 1 if the backend has been successfully initialised. */
int opl2_is_ready(void);

/* Free all resources (call on exit). */
void opl2_destroy(void);

#ifdef __cplusplus
}
#endif

#endif /* OPL2_BACKEND_H */
