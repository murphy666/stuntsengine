/*
 * test_memmgr_stub.cc — Google Test version of test_memmgr_stub.c
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

extern "C" {
#include "memmgr.h"

/* stubs required by memmgr.c */
char textresprefix = 'e';

void fatal_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
}
} // extern "C"

TEST(Memmgr, PathToName) {
    const char* name = mmgr_path_to_name("assets/track/MAIN.RES");
    EXPECT_STREQ(name, "MAIN.RES") << "strips directory path";

    name = mmgr_path_to_name("DIRECT.RES");
    EXPECT_STREQ(name, "DIRECT.RES") << "no directory → unchanged";

    name = mmgr_path_to_name("a/b/c/DEEP.RES");
    EXPECT_STREQ(name, "DEEP.RES") << "deep path";
}

TEST(Memmgr, AllocChunk) {
    char* p = static_cast<char*>(mmgr_alloc_pages("assets/MAIN.RES", 2));
    ASSERT_NE(p, nullptr) << "mmgr_alloc_pages allocates memory";
    EXPECT_TRUE(mmgr_chunk_exists("MAIN.RES")) << "chunk_exists resolves base filename";
    EXPECT_EQ(mmgr_get_chunk_by_name("MAIN.RES"), p) << "get_chunk_by_name returns stored pointer";
    EXPECT_EQ(mmgr_get_chunk_size(p), (unsigned)2)    << "get_chunk_size returns paras";
    EXPECT_EQ(mmgr_get_chunk_size_bytes(p), 32ul)     << "get_chunk_size_bytes returns bytes";
    mmgr_release(p);
    EXPECT_FALSE(mmgr_chunk_exists("MAIN.RES")) << "release removes chunk presence";
}

TEST(Memmgr, AllocResbytes) {
    char* p = static_cast<char*>(mmgr_alloc_resbytes("misc.bin", 123));
    ASSERT_NE(p, nullptr) << "mmgr_alloc_resbytes allocates memory";
    EXPECT_EQ(mmgr_get_chunk_size_bytes(p), 123ul) << "tracks exact size";
    mmgr_free(p);
    EXPECT_FALSE(mmgr_chunk_exists("misc.bin")) << "free removes chunk presence";
}

TEST(Memmgr, AllocA000) {
    mmgr_alloc_a000();
    EXPECT_EQ(mmgr_get_ofs_diff(), 40960u) << "mmgr_alloc_a000 reserves expected paras";
}

/* -----------------------------------------------------------------------
 * locate_resource  (header-mode 6)
 * Layout: [0-3]=size  [4-5]=N  [6..6+N*4-1]=names  [6+N*4..6+N*8-1]=offsets
 *         [6+N*8..]  = payload
 * ---------------------------------------------------------------------- */
static void build_res_chunk(char* buf, long buf_size) {
    /* N=3 entries: "ABCD", "WXYZ", "eBC " (prefix for locate_text_res) */
    memset(buf, 0, (size_t)buf_size);
    /* total size */
    buf[0] = (char)buf_size; buf[1] = 0; buf[2] = 0; buf[3] = 0;
    /* chunks = 3 */
    buf[4] = 3; buf[5] = 0;
    /* name table */
    buf[6]  = 'A'; buf[7]  = 'B'; buf[8]  = 'C'; buf[9]  = 'D';
    buf[10] = 'W'; buf[11] = 'X'; buf[12] = 'Y'; buf[13] = 'Z';
    buf[14] = 'e'; buf[15] = 'B'; buf[16] = 'C'; buf[17] = ' ';
    /* offset table: offsets 0, 8, 16 into payload */
    buf[18] =  0; buf[19] = 0; buf[20] = 0; buf[21] = 0;
    buf[22] =  8; buf[23] = 0; buf[24] = 0; buf[25] = 0;
    buf[26] = 16; buf[27] = 0; buf[28] = 0; buf[29] = 0;
    /* payload */
    buf[30] = 1; buf[31] = 2; buf[32] = 3; buf[33] = 4;  /* "ABCD" data */
    buf[38] = 9; buf[39] = 10;                            /* "WXYZ" data */
    buf[46] = 17; buf[47] = 18;                           /* "eBC " data */
}

TEST(Memmgr, LocateResourceMode6) {
    const long kSize = 64;
    char* chunk = static_cast<char*>(mmgr_alloc_resbytes("RESTEST", kSize));
    ASSERT_NE(chunk, nullptr);
    build_res_chunk(chunk, kSize);

    /* null guards */
    EXPECT_EQ(locate_shape_nofatal(nullptr, (char*)"ABCD"), nullptr) << "null data";
    EXPECT_EQ(locate_shape_nofatal(chunk,   nullptr),        nullptr) << "null name";

    /* found entries */
    char* p0 = locate_shape_nofatal(chunk, (char*)"ABCD");
    char* p1 = locate_shape_nofatal(chunk, (char*)"WXYZ");
    ASSERT_NE(p0, nullptr) << "ABCD found";
    ASSERT_NE(p1, nullptr) << "WXYZ found";
    EXPECT_EQ((unsigned char)p0[0], 1u) << "ABCD payload byte 0";
    EXPECT_EQ((unsigned char)p0[1], 2u) << "ABCD payload byte 1";
    EXPECT_EQ((unsigned char)p1[0], 9u) << "WXYZ payload byte 0";
    EXPECT_EQ(p1 - p0, 8)               << "offset gap between entries";

    /* not found (non-fatal) */
    EXPECT_EQ(locate_shape_nofatal(chunk, (char*)"MISS"), nullptr) << "missing entry";

    mmgr_free(chunk);
}

TEST(Memmgr, LocateTextRes) {
    /* textresprefix='e' (defined in stubs), name "BC" → looks for "eBC " */
    const long kSize = 64;
    char* chunk = static_cast<char*>(mmgr_alloc_resbytes("TXTTEST", kSize));
    ASSERT_NE(chunk, nullptr);
    build_res_chunk(chunk, kSize);

    char name[] = { 'B', 'C', '\0' };
    char* t = locate_text_res(chunk, name);
    ASSERT_NE(t, nullptr) << "text resource found";
    EXPECT_EQ((unsigned char)t[0], 17u) << "text payload byte 0";
    EXPECT_EQ((unsigned char)t[1], 18u) << "text payload byte 1";

    mmgr_free(chunk);
}

TEST(Memmgr, LocateManyResources) {
    const long kSize = 64;
    char* chunk = static_cast<char*>(mmgr_alloc_resbytes("MANYTEST", kSize));
    ASSERT_NE(chunk, nullptr);
    build_res_chunk(chunk, kSize);

    /* Two names terminated by '\0' in next slot */
    char names[] = { 'A','B','C','D',  'W','X','Y','Z',  '\0' };
    char* results[2] = { nullptr, nullptr };
    locate_many_resources(chunk, names, results);

    ASSERT_NE(results[0], nullptr) << "ABCD located";
    ASSERT_NE(results[1], nullptr) << "WXYZ located";
    EXPECT_EQ((unsigned char)results[0][0], 1u)  << "ABCD data";
    EXPECT_EQ((unsigned char)results[1][0], 9u)  << "WXYZ data";

    mmgr_free(chunk);
}

TEST(Memmgr, NormalizePtr) {
    char buf[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    EXPECT_EQ(mmgr_normalize_ptr(buf),     static_cast<void*>(buf)) << "identity";
    EXPECT_EQ(mmgr_normalize_ptr(nullptr), static_cast<void*>(nullptr)) << "null stays null";
}

TEST(Memmgr, AllocResmem) {
    mmgr_alloc_resmem(16);
    EXPECT_EQ(mmgr_get_ofs_diff(),            16u)   << "16 paras stored";
    EXPECT_EQ(mmgr_get_res_ofs_diff_scaled(), 256ul) << "16*16=256 bytes";

    /* reallocate frees old and sets new */
    mmgr_alloc_resmem(8);
    EXPECT_EQ(mmgr_get_ofs_diff(),            8u)    << "after realloc 8 paras";
    EXPECT_EQ(mmgr_get_res_ofs_diff_scaled(), 128ul) << "8*16=128 bytes";

    mmgr_alloc_resmem(0);  /* release */
    EXPECT_EQ(mmgr_get_ofs_diff(),            0u)    << "zero after free";
    EXPECT_EQ(mmgr_get_res_ofs_diff_scaled(), 0ul)   << "zero bytes after free";
}

TEST(Memmgr, AllocResmemSimple) {
    mmgr_alloc_resmem_simple(4);
    EXPECT_EQ(mmgr_get_ofs_diff(), 4u) << "simple wrapper stores paras";
    mmgr_alloc_resmem_simple(0);        /* cleanup */
}

TEST(Memmgr, GetChunkSizeUnknownPtr) {
    char buf[16];
    EXPECT_EQ(mmgr_get_chunk_size(buf),       0u)  << "unknown ptr → 0 paras";
    EXPECT_EQ(mmgr_get_chunk_size_bytes(buf), 0ul) << "unknown ptr → 0 bytes";
}
