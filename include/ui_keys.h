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
 * ui_keys.h - Keyboard / joystick / mouse input constants
 *
 * These match the DOS BIOS key codes used throughout the original Stunts
 * engine.  The values MUST stay identical so that resource-text hotkey
 * logic, joystick mapping, and SDL-to-DOS translation remain correct.
 */

#ifndef UI_KEYS_H
#define UI_KEYS_H

/* ---- Navigation ---------------------------------------------------- */
#define UI_KEY_UP           18432   /* 0x4800 — Up arrow                */
#define UI_KEY_DOWN         20480   /* 0x5000 — Down arrow              */
#define UI_KEY_LEFT         19200   /* 0x4B00 — Left arrow              */
#define UI_KEY_RIGHT        19712   /* 0x4D00 — Right arrow             */
#define UI_KEY_HOME         18176   /* 0x4700 — Home                    */
#define UI_KEY_END          20224   /* 0x4F00 — End                     */

/* ---- Action keys --------------------------------------------------- */
#define UI_KEY_ENTER        13
#define UI_KEY_ESCAPE       27
#define UI_KEY_SPACE        32
#define UI_KEY_TAB          9
#define UI_KEY_BACKSPACE    8

/* ---- Editing ------------------------------------------------------- */
#define UI_KEY_INSERT       20992   /* 0x5200 — Insert                  */
#define UI_KEY_DELETE       21248   /* 0x5300 — Delete                  */

/* ---- Extended (numpad / alternate scan codes) ---------------------- */
#define UI_KEY_ENTER_EXT    7181    /* 0x1C0D — Numpad Enter            */
#define UI_KEY_SPACE_EXT    14624   /* 0x3920 — Alternate Space         */
#define UI_KEY_ESCAPE_EXT   283     /* 0x011B — Alternate Escape code   */

/* ---- Timing -------------------------------------------------------- */
#define UI_INPUT_REPEAT_MS         100   /* Auto-repeat interval (ms)   */
#define UI_MOUSE_AUTOHIDE_MS      2500  /* Mouse cursor auto-hide (ms) */
#define UI_FRAMECOUNTER_WRAP      20000 /* Frame counter wrap threshold */
#define UI_FRAMECOUNTER_SUBTRACT  10000 /* Subtracted on counter wrap   */

/* ---- Return value sentinels ---------------------------------------- */
#define UI_SEL_CANCEL       255   /* Escape / cancel from button menu   */
#define UI_SEL_TIMEOUT      254   /* Idle timeout from button menu      */

/* ---- Convenience predicates ---------------------------------------- */
#define UI_IS_CONFIRM(k) \
    ((k) == UI_KEY_ENTER || (k) == UI_KEY_SPACE || \
     (k) == UI_KEY_ENTER_EXT || (k) == UI_KEY_SPACE_EXT)

#define UI_IS_CANCEL(k) \
    ((k) == UI_KEY_ESCAPE || (k) == UI_KEY_ESCAPE_EXT)

#define UI_IS_NAV_PREV(k) \
    ((k) == UI_KEY_LEFT || (k) == UI_KEY_UP)

#define UI_IS_NAV_NEXT(k) \
    ((k) == UI_KEY_RIGHT || (k) == UI_KEY_DOWN)

#endif /* UI_KEYS_H */
