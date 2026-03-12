/*
 * test_hill_terrain_render_stubs.c — Minimal stubs for shape3d.c + math.c
 * when building test_hill_terrain_render without the full game link.
 *
 * Provides definitions for external functions that shape3d.c references
 * but that are not exercised by the terrain culling tests.
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "shape2d.h"

/* ---- sprite globals (shape3d.c render functions use sprite1) ---- */
struct SPRITE sprite1;
struct SPRITE sprite2;

/* ---- memory manager stubs ---- */
void* mmgr_alloc_resbytes(const char* name, long int size) {
    (void)name;
    size_t bytes = (size <= 0) ? 1u : (size_t)size;
    return malloc(bytes);
}

void* mmgr_free(char* ptr) {
    free(ptr);
    return NULL;
}

void mmgr_release(char* ptr) {
    free(ptr);
}

unsigned long mmgr_get_res_ofs_diff_scaled(void) {
    return 0;
}

unsigned long mmgr_get_chunk_size_bytes(char* ptr) {
    (void)ptr;
    return 0;
}

/* ---- resource stubs ---- */
void* file_load_3dres(const char* filename) {
    (void)filename;
    return NULL;
}

char* locate_shape_nofatal(char* data, char* name) {
    (void)data; (void)name;
    return NULL;
}

char* locate_shape_fatal(char* data, char* name) {
    (void)data; (void)name;
    return NULL;
}

/* ---- drawing stubs (not reached during culling tests) ---- */
void draw_filled_lines(unsigned short color, unsigned short numlines,
                       unsigned short y, unsigned short* x_left,
                       unsigned short* x_right) {
    (void)color; (void)numlines; (void)y; (void)x_left; (void)x_right;
}

void draw_patterned_lines(unsigned char color, unsigned short numlines,
                          unsigned short y, unsigned short* x_left,
                          unsigned short* x_right) {
    (void)color; (void)numlines; (void)y; (void)x_left; (void)x_right;
}

void draw_two_color_patterned_lines(unsigned char color, unsigned short numlines,
                                    unsigned short y, unsigned short* x_left,
                                    unsigned short* x_right) {
    (void)color; (void)numlines; (void)y; (void)x_left; (void)x_right;
}

void putpixel_single_clipped(int color, unsigned short y, unsigned short x) {
    (void)color; (void)y; (void)x;
}

void build_sphere_vertex_buffer(unsigned src_ptr, unsigned* dst_ptr) {
    (void)src_ptr; (void)dst_ptr;
}

/* ---- error handler ---- */
void fatal_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[FATAL] ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    abort();
}

/* ---- video stubs ---- */
static unsigned char g_stub_framebuf[320 * 200];
unsigned char* video_get_framebuffer(void) { return g_stub_framebuf; }
