/*
 * test_shape2d_pure_stubs.c — Minimal stubs for shape2d.c when building
 * test_shape2d_pure without the full game link.
 *
 * Only symbols referenced from shape2d.c that are NOT exercised by the tests
 * need to be stubbed here.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "shape2d.h"

/* ---- video / framebuffer ---- */
static unsigned char g_stub_framebuf[320 * 200];
unsigned char* video_get_framebuffer(void) { return g_stub_framebuf; }

/* ---- memory manager ---- */
void* mmgr_alloc_pages(unsigned short paras, const char* name) {
    (void)paras; (void)name;
    return NULL;
}
void* mmgr_get_chunk_by_name(const char* name) { (void)name; return NULL; }
long  mmgr_get_chunk_size(const char* name)     { (void)name; return 0; }
void* mmgr_normalize_ptr(void* p)               { return p; }
void  mmgr_path_to_name(const char* path, char* name, int sz) {
    (void)sz;
    if (name && path) strncpy(name, path, 8);
}
void  mmgr_release(const char* name) { (void)name; }

/* ---- file / resource helpers ---- */
void* file_decomp(void* p, long* sz)             { (void)p; (void)sz; return NULL; }
void* file_find(const char* n, long* sz)         { (void)n; (void)sz; return NULL; }
void* file_load_binary(const char* n, long* sz)  { (void)n; (void)sz; return NULL; }
void* locate_shape_nofatal(void* r, const char* n){ (void)r; (void)n; return NULL; }

/* ---- font ---- */
static unsigned char g_stub_fontdef[256] = {0};
unsigned char* g_fontdef_ptr   = g_stub_fontdef;
unsigned short fontdef_line_height  = 8;

/* ---- polygon rendering globals ---- */
unsigned char  polygon_pattern_type    = 0;
unsigned char  polygon_alternate_color = 0;

/* ---- sprite globals ---- */
static struct SPRITE g_stub_wndsprite;
static struct SPRITE g_stub_mcgawndsprite;
struct SPRITE* wndsprite      = &g_stub_wndsprite;
struct SPRITE* mcgawndsprite  = &g_stub_mcgawndsprite;

/* ---- palette map ---- */
static unsigned char g_stub_palmap[256] = {0};
unsigned char* palmap = g_stub_palmap;

/* ---- error handler ---- */
void fatal_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    abort();
}

/* ---- misc string tables (shape2d.c string constants from externs) ---- */
const char* aWindowReleased                = "wnd released";
const char* aWindowdefOutOfRowTableSpa     = "windowdef out of row table space";
