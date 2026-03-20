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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "stunts.h"
#include "memmgr.h"

typedef struct {
    char name[13];
    void* ptr;
    size_t size;
} MMGR_ENTRY;

static MMGR_ENTRY g_mmgr_entries[256];
static unsigned short g_mmgr_count;
static void* g_resmem_ptr;
static size_t g_resmem_size;
static unsigned short g_resmem_paras;

/** @brief Manage memory for store entry.
 * @param name Parameter `name`.
 * @param ptr Parameter `ptr`.
 * @param size Parameter `size`.
 */
static void mmgr_store_entry(const char* name, void* ptr, size_t size) {
    unsigned short i;
    const char* chunk_name;
    if (ptr == NULL) {
        return;
    }
    chunk_name = mmgr_path_to_name(name);
    for (i = 0; i < g_mmgr_count; i++) {
        if (g_mmgr_entries[i].ptr == ptr) {
            if (chunk_name != NULL) {
                memset(g_mmgr_entries[i].name, 0, sizeof(g_mmgr_entries[i].name));
                snprintf(g_mmgr_entries[i].name, sizeof(g_mmgr_entries[i].name), "%s", chunk_name);
            }
            g_mmgr_entries[i].size = size;
            return;
        }
    }
    if (chunk_name != NULL) {
        for (i = 0; i < g_mmgr_count; i++) {
            if (strncmp(g_mmgr_entries[i].name, chunk_name, 12) == 0) {
                g_mmgr_entries[i].ptr = ptr;
                g_mmgr_entries[i].size = size;
                return;
            }
        }
    }
    if (g_mmgr_count < (unsigned short)(sizeof(g_mmgr_entries) / sizeof(g_mmgr_entries[0]))) {
        MMGR_ENTRY* entry = &g_mmgr_entries[g_mmgr_count++];
        memset(entry, 0, sizeof(*entry));
        if (chunk_name != NULL) {
            snprintf(entry->name, sizeof(entry->name), "%s", chunk_name);
        }
        entry->ptr = ptr;
        entry->size = size;
    }
}

/** @brief Manage memory for find entry by ptr.
 * @param ptr Parameter `ptr`.
 * @return Function result.
 */
static MMGR_ENTRY* mmgr_find_entry_by_ptr(void* ptr) {

    unsigned short i;
    for (i = 0; i < g_mmgr_count; i++) {
        if (g_mmgr_entries[i].ptr == ptr) {
            return &g_mmgr_entries[i];
        }
    }
    return NULL;
}

/** @brief Manage memory for find entry by name.
 * @param name Parameter `name`.
 * @return Function result.
 */
static MMGR_ENTRY* mmgr_find_entry_by_name(const char* name) {

    unsigned short i;
    const char* chunk_name;
    if (name == NULL) {
        return NULL;
    }
    chunk_name = mmgr_path_to_name(name);
    for (i = 0; i < g_mmgr_count; i++) {
        if (g_mmgr_entries[i].ptr != NULL && strncmp(g_mmgr_entries[i].name, chunk_name, 12) == 0) {
            return &g_mmgr_entries[i];
        }
    }
    return NULL;
}

/** @brief Manage memory for path to name.
 * @param filename Parameter `filename`.
 * @return Function result.
 */
const char* mmgr_path_to_name(const char* filename) {

    const char* c;
    const char* result = filename;
    if (filename == NULL) {
        return "";
    }
    for (c = filename; *c != '\0'; c++) {
        if (*c == '/' || *c == '\\' || *c == ':') {
            result = c + 1;
        }
    }
    return result;
}

/** @brief Manage memory for alloc pages.
 * @param chunk_name Parameter `chunk_name`.
 * @param size_paras Parameter `size_paras`.
 */
void * mmgr_alloc_pages(const char* chunk_name, unsigned short size_paras) {

    size_t bytes = ((size_t)size_paras) << 4;
    void* ptr = malloc(bytes == 0 ? 1 : bytes);
    if (ptr == NULL) {
        fatal_error("mmgr_alloc_pages: out of memory for '%s'", chunk_name);
        return NULL;
    }
    mmgr_store_entry(chunk_name, ptr, bytes);
    return ptr;
}

/** @brief Manage memory for alloc resbytes.
 * @param name Parameter `name`.
 * @param size Parameter `size`.
 */
void * mmgr_alloc_resbytes(const char* name, long int size) {

    size_t bytes = (size <= 0) ? 1u : (size_t)size;
    void* ptr = malloc(bytes);
    if (ptr == NULL) {
        fatal_error("mmgr_alloc_resbytes: out of memory for '%s'", name);
        return NULL;
    }
    mmgr_store_entry(name, ptr, bytes);
    return ptr;
}

/** @brief Manage memory for alloc resmem.
 * @param size_paras Parameter `size_paras`.
 */
void mmgr_alloc_resmem(unsigned short size_paras) {

    size_t bytes = ((size_t)size_paras) << 4;

    if (g_resmem_ptr != NULL) {
        free(g_resmem_ptr);
        g_resmem_ptr = NULL;
        g_resmem_size = 0;
        g_resmem_paras = 0;
    }

    if (bytes == 0) {
        return;
    }

    g_resmem_ptr = malloc(bytes);
    if (g_resmem_ptr == NULL) {
        fatal_error("mmgr_alloc_resmem failed for %u paras", size_paras);
        return;
    }

    g_resmem_size = bytes;
    g_resmem_paras = size_paras;
}

/** @brief Manage memory for alloc resmem simple.
 * @param paras Parameter `paras`.
 */
void mmgr_alloc_resmem_simple(unsigned short paras) {

    mmgr_alloc_resmem(paras);
}

/** @brief Manage memory for alloc a000.
 */
void mmgr_alloc_a000(void) {

    mmgr_alloc_resmem(40960u);
}

/** @brief Manage memory for get ofs diff.
 * @return Function result.
 */
unsigned short mmgr_get_ofs_diff() {

    return g_resmem_paras;
}

/** @brief Manage memory for free.
 * @param ptr Parameter `ptr`.
 */
void * mmgr_free(char * ptr) {

    MMGR_ENTRY* entry = mmgr_find_entry_by_ptr(ptr);
    if (entry != NULL) {
        entry->ptr = NULL;
        entry->size = 0;
    }
    free(ptr);
    return NULL;
}

/** @brief Manage memory for free compact.
 * @param ret0_unused Parameter `ret0_unused`.
 * @param seg Parameter `seg`.
 */
void mmgr_free_compact(unsigned short ret0_unused, unsigned short seg) {

    (void)ret0_unused;
    (void)seg;
}

/** @brief Manage memory for copy paras.
 * @param srcseg Parameter `srcseg`.
 * @param destseg Parameter `destseg`.
 * @param paras Parameter `paras`.
 */
void mmgr_copy_paras(unsigned short srcseg, unsigned short destseg, short paras) {

    (void)srcseg;
    (void)destseg;
    (void)paras;
}

/** @brief Copy paras reverse.
 * @param srcseg Parameter `srcseg`.
 * @param destseg Parameter `destseg`.
 * @param paras Parameter `paras`.
 */
void copy_paras_reverse(unsigned short srcseg, unsigned short destseg, short paras) {
    (void)srcseg;
    (void)destseg;
    (void)paras;
}

/** @brief Manage memory for find free.
 */
void mmgr_find_free() {
}

/** @brief Manage memory for get chunk by name.
 * @param chunk_name Parameter `chunk_name`.
 */
void * mmgr_get_chunk_by_name(const char* chunk_name) {

    MMGR_ENTRY* entry = mmgr_find_entry_by_name(chunk_name);
    return entry ? entry->ptr : NULL;
}

/** @brief Manage memory for chunk exists.
 * @param name Parameter `name`.
 * @return Function result.
 */
int mmgr_chunk_exists(const char* name) {

    return mmgr_find_entry_by_name(name) != NULL;
}

/** @brief Manage memory for release.
 * @param ptr Parameter `ptr`.
 */
void mmgr_release(char * ptr) {

    (void)mmgr_free(ptr);
}

/** @brief Manage memory for get chunk size.
 * @param ptr Parameter `ptr`.
 * @return Function result.
 */
unsigned short mmgr_get_chunk_size(char * ptr) {

    MMGR_ENTRY* entry = mmgr_find_entry_by_ptr(ptr);
    if (entry == NULL) {
        return 0;
    }
    return (unsigned short)((entry->size + 15u) >> 4);
}

/** @brief Manage memory for normalize ptr.
 * @param ptr Parameter `ptr`.
 */
void * mmgr_normalize_ptr(char * ptr) {
    return ptr;
}

/** @brief Manage memory for get res ofs diff scaled.
 * @return Function result.
 */
unsigned long mmgr_get_res_ofs_diff_scaled(void) {

    if (g_resmem_size != 0) {
        return (unsigned long)g_resmem_size;
    }
    if (g_resmem_paras != 0) {
        return ((unsigned long)g_resmem_paras) << 4;
    }
    return 0;
}

/** @brief Manage memory for get chunk size bytes.
 * @param ptr Parameter `ptr`.
 * @return Function result.
 */
unsigned long mmgr_get_chunk_size_bytes(char * ptr) {

    MMGR_ENTRY* entry = mmgr_find_entry_by_ptr(ptr);
    return entry ? (unsigned long)entry->size : 0ul;
}

struct resheader {
    uint32_t size;
    unsigned short chunks;
};

/** @brief Memmgr layout valid.
 * @param data_size Parameter `data_size`.
 * @param chunks Parameter `chunks`.
 * @param header_mode Parameter `header_mode`.
 * @return Function result.
 */
static int memmgr_layout_valid(uint32_t data_size, unsigned short chunks, unsigned short header_mode) {

    uint32_t header_size;

    if (chunks == 0 || chunks > 4096u) {
        return 0;
    }

    header_size = (uint32_t)header_mode + ((uint32_t)chunks * 8u);
    if (data_size != 0 && (header_size >= data_size || header_size < (uint32_t)header_mode)) {
        return 0;
    }

    return 1;
}

/** @brief Memmgr layout has name.
 * @param names Parameter `names`.
 * @param chunks Parameter `chunks`.
 * @param lookup Parameter `lookup`.
 * @return Function result.
 */
static int memmgr_layout_has_name(const char *names, unsigned short chunks, const char lookup[4]) {
    unsigned short j;
    for (j = 0; j < chunks; j++) {
        const char *cur = names + ((unsigned long)j * 4ul);
        if (cur[0] == lookup[0] && cur[1] == lookup[1] && cur[2] == lookup[2] && cur[3] == lookup[3]) {
            return 1;
        }
    }
    return 0;
}

/** @brief Locate resource.
 * @param data Parameter `data`.
 * @param name Parameter `name`.
 * @param fatal Parameter `fatal`.
 * @return Function result.
 */
char * locate_resource(char * data, char* name, unsigned short fatal) {

    unsigned short j;
    char lookup[4];
    struct resheader * hdr;
    unsigned short chunks;
    unsigned short chunks6;
    unsigned short alt_chunks;
    unsigned short chunks10;
    char * names;
    uint32_t * offsets;
    uint32_t header_size;
    uint32_t data_size;
    unsigned long base;
    unsigned short header_mode;
    int valid6;
    int valid10;

    if (data == NULL || name == NULL) {
        return NULL;
    }

    lookup[0] = name[0];
    lookup[1] = name[1];
    lookup[2] = name[2];
    lookup[3] = name[3];
    if (lookup[1] == 0) lookup[1] = ' ';
    if (lookup[2] == 0) lookup[2] = ' ';
    if (lookup[3] == 0) lookup[3] = ' ';

    hdr = (struct resheader*)data;
    data_size = (uint32_t)mmgr_get_chunk_size_bytes(data);
    if (data_size == 0) {
        data_size = hdr->size;
    }

    chunks6 = hdr->chunks;
    alt_chunks = (unsigned short)((unsigned short)(unsigned char)data[8] |
        ((unsigned short)(unsigned char)data[9] << 8));
    chunks10 = alt_chunks;

    valid6 = memmgr_layout_valid(data_size, chunks6, 6);
    valid10 = memmgr_layout_valid(data_size, chunks10, 10);

    if (valid6 && valid10) {
        if (memmgr_layout_has_name(data + 6, chunks6, lookup)) {
            header_mode = 6;
            chunks = chunks6;
        } else if (memmgr_layout_has_name(data + 10, chunks10, lookup)) {
            header_mode = 10;
            chunks = chunks10;
        } else {
            header_mode = 6;
            chunks = chunks6;
        }
    } else if (valid10) {
        header_mode = 10;
        chunks = chunks10;
    } else if (valid6) {
        header_mode = 6;
        chunks = chunks6;
    } else {
        if (fatal != 0) {
            fatal_error("locate_resource invalid header for %.4s", lookup);
        }
        return NULL;
    }

    if (header_mode == 10) {
        names = data + 10;
        offsets = (uint32_t*)(names + ((unsigned long)chunks * 4ul));
        header_size = (uint32_t)(10u + ((uint32_t)chunks * 8u));
    } else {
        names = data + 6;
        offsets = (uint32_t*)(names + ((unsigned long)chunks * 4ul));
        header_size = (uint32_t)(6u + ((uint32_t)chunks * 8u));
    }

    base = (unsigned long)header_size;

    for (j = 0; j < chunks; j++) {
        char *cur = names + ((unsigned long)j * 4ul);
        if (cur[0] == lookup[0] && cur[1] == lookup[1] && cur[2] == lookup[2] && cur[3] == lookup[3]) {
            return data + base + offsets[j];
        }
    }

    if (fatal != 0) {
        fatal_error("locate_resource failed for %.4s", lookup);
    }
    return NULL;
}

/** @brief Locate shape nofatal.
 * @param data Parameter `data`.
 * @param name Parameter `name`.
 * @return Function result.
 */
char * locate_shape_nofatal(char * data, char* name) {

    return locate_resource(data, name, 0);
}

/** @brief Locate shape fatal.
 * @param data Parameter `data`.
 * @param name Parameter `name`.
 * @return Function result.
 */
char * locate_shape_fatal(char * data, char* name) {

    return locate_resource(data, name, 1);
}

/** @brief Locate shape alt.
 * @param data Parameter `data`.
 * @param name Parameter `name`.
 */
void * locate_shape_alt(void * data, char* name) {

    return locate_shape_fatal((char*)data, name);
}

/** @brief Locate sound fatal.
 * @param data Parameter `data`.
 * @param name Parameter `name`.
 * @return Function result.
 */
char * locate_sound_fatal(char * data, char* name) {
    return locate_resource(data, name, 1);
}

/** @brief Locate many resources.
 * @param data Parameter `data`.
 * @param names Parameter `names`.
 * @param result Parameter `result`.
 */
void locate_many_resources(char * data, char* names, char ** result) {

    while (names != NULL && result != NULL && *names != 0) {
        *result++ = locate_shape_fatal(data, names);
        names += 4;
    }
}

/** @brief Locate text res.
 * @param data Parameter `data`.
 * @param name Parameter `name`.
 * @return Function result.
 */
char * locate_text_res(char * data, char* name) {

    char textname[4];
    textname[0] = (char)textresprefix;
    textname[1] = name[0];
    textname[2] = name[1];
    textname[3] = name[2];
    return locate_shape_fatal(data, textname);
}
