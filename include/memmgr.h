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

#ifndef MEMMGR_H
#define MEMMGR_H

#pragma pack (push, 1)
struct MEMCHUNK {
	char resname[12];
	unsigned ressize;
	unsigned resofs;
	unsigned resunk;
};
#pragma pack (pop)

const char* mmgr_path_to_name(const char* filename);
void * mmgr_alloc_pages(const char* path_name, unsigned short page_count);
void mmgr_alloc_resmem(unsigned short paras);
void mmgr_alloc_resmem_simple(unsigned short paras);
void mmgr_free_compact(unsigned short ret0_unused, unsigned short seg);
void mmgr_alloc_a000(void);
unsigned short mmgr_get_ofs_diff();
void * mmgr_free(char * ptr);
void mmgr_copy_paras(unsigned short srcseg, unsigned short destseg, short paras);
void copy_paras_reverse(unsigned short srcseg, unsigned short destseg, short paras);
void mmgr_find_free();
void * mmgr_get_chunk_by_name(const char* path_name);
int mmgr_chunk_exists(const char* name);
void mmgr_release(char * ptr);
unsigned short mmgr_get_chunk_size(char * ptr);
unsigned short mmgr_resize_memory(unsigned short old_pages, unsigned short new_pages, unsigned short reserved);
void * mmgr_normalize_ptr(char * ptr);
void * mmgr_alloc_resbytes(const char* name, long int size);
unsigned long mmgr_get_res_ofs_diff_scaled(void);
unsigned long mmgr_get_chunk_size_bytes(char * ptr);

char * locate_resource(char * data, char* name, unsigned short fatal);
char * locate_shape_nofatal(char * data, char* name);
char * locate_shape_fatal(char * data, char* name);
void * locate_shape_alt(void * data, char* name);
char * locate_sound_fatal(char * data, char* name);
void locate_many_resources(char * data, char* names, char ** result);
char * locate_text_res(char * data, char* name);

#endif
