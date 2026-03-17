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

#ifndef RESSOURCES_H
#define RESSOURCES_H

#ifdef __cplusplus
extern "C" {
#endif

const char* file_find(const char* query);
const char* file_find_next(void);
const char* file_find_next_alt(void);

void file_build_path(const char* dir, const char* name, const char* ext, char* dst);
const char* file_combine_and_find(const char* dir, const char* name, const char* ext);

unsigned short file_paras(const char* filename, int fatal);
unsigned short file_paras_fatal(const char* filename);
unsigned short file_paras_nofatal(const char* filename);

unsigned short file_decomp_paras(const char* filename, int fatal);
unsigned short file_decomp_paras_fatal(const char* filename);
unsigned short file_decomp_paras_nofatal(const char* filename);

void * file_read(const char* filename, void * dst, int fatal);
void * file_read_fatal(const char* filename, void * dst);
void * file_read_nofatal(const char* filename, void * dst);

/* Global state — defined in src/data_runtime.c */

#ifdef DATA_RUNTIME_IMPL
#  define _RT_
#else
#  define _RT_ extern
#endif

_RT_ char             joystick_assigned_flags;
_RT_ unsigned         resource_alloc_state_a;
_RT_ unsigned         resource_alloc_state_b;

#undef _RT_

extern short is_audioloaded;
extern char  g_is_busy;

/* Debug helpers */
void debug_track_terrain_map(const char * tag);

short file_write(const char* filename, void * src, unsigned long length, int fatal);
short file_write_fatal(const char* filename, void * src, unsigned long length);
short file_write_nofatal(const char* filename, void * src, unsigned long length);

void * file_decomp(const char* filename, int fatal);
void * file_decomp_fatal(const char* filename);
void * file_decomp_nofatal(const char* filename);

void * file_load_binary(const char* filename, int fatal);
void * file_load_binary_nofatal(const char* filename);
void * file_load_binary_fatal(const char* filename);

void * file_load_resfile(const char* filename);
void * file_load_resource(int type, const char* filename);
void unload_resource(void * resptr);
void file_load_audiores(const char* songfile, const char* voicefile, const char* name);
void * file_load_3dres(const char* filename);

short file_load_replay(const char* dir, const char* name);
short file_write_replay(const char* filename);

void parse_filepath_separators_dosptr(const char* src, void* dst);

#ifdef __cplusplus
}
#endif

#endif
