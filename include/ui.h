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

#ifndef UI_H
#define UI_H

/* Text input */
unsigned short read_line(char* buffer, unsigned char flags, unsigned short max_chars,
                          unsigned short width, unsigned short height, void * fontptr,
                          unsigned short x, unsigned short y, void * callback_ptr, unsigned short timeout);
unsigned short call_read_line(char* dest, unsigned short maxlen,
                               unsigned short x, unsigned short y, void * fontptr);
#define call_read_line call_read_line

/* Dialogs */
short show_dialog(unsigned short dialog_type, unsigned short create_window,
                  void * text_res_ptr,
                  unsigned short dialog_x, unsigned short dialog_y,
                  unsigned short border_color, unsigned short * coords_array,
                  unsigned char default_button);
#define show_dialog show_dialog

unsigned short do_fileselect_dialog(char* pathbuf, char* defaultName,
                                     const char* ext, void * textresptr);
#define do_fileselect_dialog do_fileselect_dialog

/* Button drawing */
void draw_button(char * text, unsigned short x, unsigned short y,
                 unsigned short width, unsigned short height,
                 unsigned short topcolor, unsigned short bottomcolor,
                 unsigned short fillcolor, unsigned short textcolor);
#define draw_button draw_button

/* Mouse position helper */
void mouse_minmax_position(int);

/* Text resource handlers (called to populate in-game text resources) */
void do_mrl_textres(void);
void do_joy_restext(void);
void do_key_restext(void);
void do_mof_restext(void);
void do_pau_restext(void);
void do_dos_restext(void);
void do_sonsof_restext(void);
void do_mou_restext(void);
short do_dea_textres(void);

/* Utility string / number formatting */
void    copy_string(char* dest, char * src);
char *  locate_text_res(char * data, char* name);
char *  stunts_itoa(int value, char* str, int radix);
void    print_int_as_string_maybe(char* dest, int value, int zeroPadFlag, int width);
void    format_frame_as_string(char* dest, unsigned short frames, unsigned short showFractions);
void    nopsub_28F26(void);

/* System-level helpers */
void    fatal_error(const char*, ...);
void    add_exit_handler(void (*handler)(void));
short   set_criterr_handler(short (* callback)(void));
void    libsub_quit_to_dos_alt(short a1);

/* DOS interrupt 0 handler stubs */
void    intr0_handler(void);
void    init_div0_(void);

/* Misc file/resource helpers not covered by ressources.h */
void    ensure_file_exists(int unk);
void *  file_read_with_mode(unsigned short mode, const char* filename, void * dest);
void *  file_load_shape2d_fatal_thunk(const char* filename);
void *  file_load_shape2d_nofatal_thunk(const char* filename);

/* Security / copy protection */
void    security_check(unsigned short idx);

/* Wait helper */
int     random_wait(void);

/* Timer stop (moved from timer.c but referenced broadly) */
void    timer_stop_dispatch(void);

/* Memory-manager scaled offset helper (underscore suffix = legacy alias) */
unsigned long mmgr_get_res_ofs_diff_scaled_(void);

/* Save file dialog */
short do_savefile_dialog(char* filename, char* extension, void* template_ptr);

/* Track-area scrollbar / slider tracking */
unsigned short mouse_track_op(unsigned short mode, unsigned short x1, unsigned short x2,
    unsigned short y1, unsigned short y2, unsigned short value,
    unsigned short range_offset, unsigned short max_value);

#endif /* UI_H */
