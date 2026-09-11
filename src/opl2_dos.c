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
 * opl2_dos.c - OPL2 backend driving the real AdLib-compatible chip on DOS.
 *
 * Same interface as the Nuked-OPL3 software backend in opl2.c, but instead of
 * synthesizing the register stream it writes it to the OPL at port 388h that
 * every AdLib and Sound Blaster card decodes. The chip then makes the sound
 * itself, so opl2_generate() returns silence and nothing is mixed into the PCM
 * stream (SDL's Sound Blaster output still carries digitized speech/effects).
 *
 * This exists because software FM synthesis is far too expensive for the
 * hardware this build targets: a 486-class machine spends roughly 20 ms per
 * 100 Hz game tick inside the emulator, which drags the whole game to well
 * under 1 fps. Writing the chip costs a few port accesses instead.
 *
 * Build the emulated backend on DOS instead with -DSTUNTS_FM_EMULATION=ON,
 * for a Sound Blaster-compatible card whose FM part is missing.
 */

#include "opl2.h"

#include <stdbool.h>
#include <string.h>

/* pc.h supplies inportb/outportb as inlines. */
#include <inlines/pc.h>

enum {
    /* AdLib-compatible OPL2 registers, also decoded by Sound Blaster cards. */
    OPL2_ADDRESS_PORT = 0x388,
    OPL2_DATA_PORT = 0x389,

    /* The OPL2 bus needs about 3.3 us after an address write and 23 us after a
     * data write. Reading the status port supplies that delay without yielding
     * partway through a register write. */
    OPL2_ADDRESS_DELAY_READS = 6,
    OPL2_DATA_DELAY_READS = 35,

    /* Highest register the OPL2 decodes; the chip has none above this. */
    OPL2_REGISTER_COUNT = 0xF6,

    OPL2_REG_TEST_WSE = 0x01,  /* bit 5 enables waveform select */
    OPL2_REG_TIMER1 = 0x02,
    OPL2_REG_TIMER_CTRL = 0x04,
    OPL2_REG_KEYON_BASE = 0xB0, /* 0xB0..0xB8, bit 5 is key-on */

    OPL2_WAVEFORM_SELECT_ENABLE = 0x20
};

static bool s_ready = false;

/* Last value written to each register. Port writes are comparatively
 * expensive — 41 port accesses each, and every one traps under emulation — and
 * the driver rewrites unchanged registers every tick, so skip the no-ops. */
static unsigned char s_shadow[OPL2_REGISTER_COUNT];
static bool s_shadow_valid = false;

/** @brief Write one register to the OPL chip, unconditionally.
 * @param reg OPL register index.
 * @param val Register value to write.
 */
static void
opl2_port_write(unsigned char reg, unsigned char val) {
    int i;

    outportb(OPL2_ADDRESS_PORT, reg);
    for (i = 0; i < OPL2_ADDRESS_DELAY_READS; i++) {
        (void)inportb(OPL2_ADDRESS_PORT);
    }
    outportb(OPL2_DATA_PORT, val);
    for (i = 0; i < OPL2_DATA_DELAY_READS; i++) {
        (void)inportb(OPL2_ADDRESS_PORT);
    }
}

/** @brief Zero every OPL register and invalidate the shadow.
 */
static void
opl2_silence_chip(void) {
    int reg;

    for (reg = 0; reg < OPL2_REGISTER_COUNT; reg++) {
        opl2_port_write((unsigned char)reg, 0);
    }
    memset(s_shadow, 0, sizeof(s_shadow));
    s_shadow_valid = true;

    /* Waveform select must be on for WS != 0, which the engine instruments
     * use. Mirrors the software backend's init. */
    opl2_port_write(OPL2_REG_TEST_WSE, OPL2_WAVEFORM_SELECT_ENABLE);
    s_shadow[OPL2_REG_TEST_WSE] = OPL2_WAVEFORM_SELECT_ENABLE;
}

/** @brief Probe for an OPL2 by timing its timer-1 overflow flags.
 *
 * The standard AdLib detection: reset both timers, start timer 1, wait for it
 * to overflow, and check the status-port flags. Only a real (or emulated) OPL
 * sets them.
 *
 * @return true when a chip responds.
 */
static bool
opl2_detect(void) {
    unsigned char status1;
    unsigned char status2;
    int i;

    opl2_port_write(OPL2_REG_TIMER_CTRL, 0x60); /* mask both timers */
    opl2_port_write(OPL2_REG_TIMER_CTRL, 0x80); /* reset IRQ + flags */
    status1 = inportb(OPL2_ADDRESS_PORT);

    opl2_port_write(OPL2_REG_TIMER1, 0xFF);     /* timer 1 = 1 step (80 us) */
    opl2_port_write(OPL2_REG_TIMER_CTRL, 0x21); /* unmask + start timer 1 */

    /* ~100 us of status reads; each is a bus cycle, so this does not depend on
     * a calibrated delay loop. */
    for (i = 0; i < 400; i++) {
        (void)inportb(OPL2_ADDRESS_PORT);
    }
    status2 = inportb(OPL2_ADDRESS_PORT);

    opl2_port_write(OPL2_REG_TIMER_CTRL, 0x60);
    opl2_port_write(OPL2_REG_TIMER_CTRL, 0x80);

    /* Before: both overflow flags clear. After: timer 1 overflowed. */
    return (status1 & 0xE0) == 0x00 && (status2 & 0xE0) == 0xC0;
}

/** @brief Initialize the OPL backend.
 * @param sample_rate Output sample rate in Hz; unused, the chip clocks itself.
 */
void
opl2_init(int sample_rate) {
    (void)sample_rate;

    s_ready = false;
    s_shadow_valid = false;

    if (!opl2_detect()) {
        return;
    }

    opl2_silence_chip();
    s_ready = true;
}

/** @brief Silence and reset all OPL registers.
 */
void
opl2_reset(void) {
    if (!s_ready) {
        return;
    }
    opl2_silence_chip();
}

/** @brief Write one OPL register value.
 * @param reg OPL register index.
 * @param val Register value to write.
 */
void
opl2_write(int reg, int val) {
    unsigned char r;
    unsigned char v;

    if (!s_ready) {
        return;
    }

    /* The software backend accepts the OPL3 register range; a real OPL2
     * decodes only the low bank, so drop anything above it (the OPL3-mode
     * register the emulated backend clears has no counterpart here). */
    if (reg < 0 || reg >= OPL2_REGISTER_COUNT) {
        return;
    }

    r = (unsigned char)reg;
    v = (unsigned char)(val & 255);

    if (s_shadow_valid && s_shadow[r] == v) {
        return;
    }

    opl2_port_write(r, v);
    s_shadow[r] = v;
}

/** @brief Produce PCM for the mixer — silence, since the chip is audible itself.
 * @param buf Destination sample buffer.
 * @param n Number of mono samples.
 */
void
opl2_generate(short *buf, int n) {
    if (buf && n > 0) {
        memset(buf, 0, (size_t)n * sizeof(short));
    }
}

/** @brief Report whether an OPL chip was found.
 * @return Non-zero when a chip is being driven.
 */
int
opl2_is_ready(void) {
    return s_ready ? 1 : 0;
}

/** @brief Release the chip, leaving it silent rather than droning after exit.
 */
void
opl2_destroy(void) {
    if (s_ready) {
        opl2_silence_chip();
    }
    s_ready = false;
    s_shadow_valid = false;
}
