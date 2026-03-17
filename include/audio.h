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

#ifndef AUDIO_H
#define AUDIO_H

/* Audio engine init/teardown */
void * init_audio_resources(void * songptr, void * voiceptr, const char* name);
void   load_audio_finalize(void * audiores);
short  audio_load_driver(const char* driver, short a2, short a3);
void   audio_unload(void);
void   audiodrv_atexit(void);
short  audio_toggle_flag2(void);
void   audio_disable_flag2(void);
void   audio_enable_flag2(void);

/* Engine sound management */
char * pad_id(unsigned long * id_value);
short  audio_init_engine(short unused, void * data_ptr, void * song_res, void * voice_res);
void   audio_start_engine_note(short handle_id);
void   audio_stop_engine_note(short handle_id);
void   audio_driver_timer(void);
void   audio_update_engine_sound(short handle_id, short rpm, short distance_x, short distance_y,
                                  short distance_z, short unused_a, short unused_c, short unused_e, short distance_divisor);
void   audio_add_driver_timer(void);
void   audio_remove_driver_timer(void);
void   audio_driver_func3F(int mode);
unsigned short audio_set_menu_music_paused(unsigned short paused);

/* Car audio synchronisation */
void audio_sync_car_audio(void);
void audio_apply_crash_flags(unsigned char flags, unsigned short sound_id);
void audio_replay_update_engine_sounds(unsigned short* info, unsigned short sound_id);
void audio_on_replay_mode_changed(void);

/* Crash / skid FX selectors */
void   audio_select_crash2_fx_and_restart(short handle_id);
void   audio_select_crash1_fx(short handle_id);
void   audio_select_crash2_fx(short handle_id);
void   audio_select_skid1_fx(short handle_id);
void   audio_select_skid2_fx(short handle_id);
void   audio_clear_chunk_fx(short handle_id);

#endif /* AUDIO_H */
