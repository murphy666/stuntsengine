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

/*
 * opl2_backend.c - OPL backend using AdPlug Nuked OPL3 (YMF262-level emulation)
 *
 * Keeps the existing OPL2-style write/render API used by audio.c while running
 * on top of Nuked OPL3 for higher-accuracy chip behavior.
 */

#include "opl2.h"
#include <stdint.h>
#include <string.h>

#include "nukedopl.h"

static opl3_chip s_opl;
static int     s_ready = 0;
static int     s_sample_rate = 49716;

/** @brief Initialize the OPL backend at the requested output sample rate.
 * @param sample_rate Output sample rate in Hz.
 */
void opl2_init(int sample_rate) {
    if (sample_rate <= 0) {
        s_ready = 0;
        return;
    }

    s_sample_rate = sample_rate;

    OPL3_Reset(&s_opl, (uint32_t)sample_rate);

    /* Force OPL2-compatible mode (disable OPL3 mode bit). */
    OPL3_WriteReg(&s_opl, 261, 0);

    /* Enable waveform-select (bit 5 of reg 1).
     * Required for WS != 0 — ADENG1.VCE ENGI carrier uses WS=3. */
    OPL3_WriteReg(&s_opl, 1, 32);

    s_ready = 1;
}

/** @brief Opl2 reset.
 */
void opl2_reset(void) {
    if (!s_ready) {
        return;
    }

    OPL3_Reset(&s_opl, (uint32_t)s_sample_rate);

    /* Keep OPL2-compatible mode after reset. */
    OPL3_WriteReg(&s_opl, 261, 0);

    /* Re-enable waveform-select after reset */
    OPL3_WriteReg(&s_opl, 1, 32);
}

/** @brief Write one OPL register value.
 * @param reg OPL register index.
 * @param val Register value to write.
 */
void opl2_write(int reg, int val) {

    if (!s_ready) {
        return;
    }
    OPL3_WriteReg(&s_opl, (uint16_t)(reg & 511), (uint8_t)(val & 255));
}

/** @brief Generate mono PCM samples from the OPL core.
 * @param buf Destination sample buffer.
 * @param n Number of mono samples to generate.
 */
void opl2_generate(short *buf, int n) {
    int i;

    if (!s_ready || !buf || n <= 0) {
        if (buf && n > 0) {
            memset(buf, 0, (size_t)n * sizeof(short));
        }
        return;
    }

    for (i = 0; i < n; i++) {
        int16_t lr[2];
        int mixed;

        OPL3_GenerateResampled(&s_opl, lr);
        mixed = (int)lr[0] + (int)lr[1];
        buf[i] = (short)(mixed / 2);
    }
}

/** @brief Report whether the OPL backend has been initialized.
 * @return Non-zero when ready to render audio.
 */
int opl2_is_ready(void) {

    return s_ready;
}

/** @brief Opl2 destroy.
 */
void opl2_destroy(void) {

    memset(&s_opl, 0, sizeof(s_opl));
    s_ready = 0;
}
