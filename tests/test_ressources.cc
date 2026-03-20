/*
 * Copyright (c) 2026 Stunts Engine Project
 *
 * Comprehensive tests for ressources.c:
 *   file_build_path, file_find, file_find_next, file_find_next_alt,
 *   file_combine_and_find, file_paras, file_read, file_write,
 *   file_load_binary, file_decomp, file_load_resource,
 *   file_load_resfile, file_load_3dres, unload_resource,
 *   parse_filepath_separators_dosptr
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <string>

extern "C" {
#include "ressources.h"
#include "memmgr.h"
#include "stunts.h"

/* --- stubs for globals referenced by ressources.c --- */
struct GAMEINFO  gameconfig;
struct GAMEINFO  gameconfigcopy;
char            *replay_header     = nullptr;
char             g_path_buf[GAME_PATH_BUF_SIZE] = {0};
char             audiodriverstring[32] = "ad";
void            *songfileptr         = nullptr;
void            *voicefileptr        = nullptr;

/* memmgr stubs — use real malloc so file_decomp can succeed */
void *mmgr_alloc_resbytes(const char * /*n*/, long int size)
{
    if (size <= 0) return nullptr;
    return malloc((size_t)size);
}
void *mmgr_get_chunk_by_name(const char * /*n*/) { return nullptr; }
void  mmgr_release(char *ptr) { free(ptr); }

/* audio stubs */
void *init_audio_resources(void * /*s*/, void * /*v*/, const char * /*n*/) { return nullptr; }
void  load_audio_finalize(void * /*r*/) {}
} /* extern "C" */

/* ================================================================== */
/* Global temp-dir used by all file-system tests                        */
/* ================================================================== */
static char g_tmpdir[256] = {};

static std::string TmpPath(const char *name)
{
    return std::string(g_tmpdir) + "/" + name;
}

static void WriteBytes(const std::string &path, const void *data, size_t len)
{
    FILE *f = fopen(path.c_str(), "wb");
    ASSERT_NE(nullptr, f) << "WriteBytes: cannot create " << path;
    if (data && len) fwrite(data, 1, len, f);
    fclose(f);
}

static void WriteFill(const std::string &path, int fill, size_t len)
{
    std::string buf(len, (char)fill);
    WriteBytes(path, buf.data(), len);
}

/* Build a minimal single-pass RLE payload that decodes to body_len
   bytes verbatim (no escape sequences, skipseq flag set). */
static std::string MakeRLEPayload(const uint8_t *body, size_t body_len, size_t out_len)
{
    std::string v(9 + body_len, '\0');
    v[0] = 1;                          /* type = RLE */
    v[1] = (char)( out_len        & 255);
    v[2] = (char)((out_len >>  8) & 255);
    v[3] = (char)((out_len >> 16) & 255);
    v[4] = (char)( body_len       & 255);
    v[5] = (char)((body_len >>  8) & 255);
    v[6] = (char)((body_len >> 16) & 255);
    v[7] = 0;
    v[8] = (char)128;                    /* esclen=0, skipseq=1 */
    memcpy(&v[9], body, body_len);
    return v;
}

/* ================================================================== */
/* Fixture                                                               */
/* ================================================================== */
class ResourcesTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        strncpy(g_tmpdir, "/tmp/stuntstest_XXXXXX", sizeof(g_tmpdir) - 1);
        ASSERT_NE(nullptr, mkdtemp(g_tmpdir));

        WriteFill(TmpPath("file16.bin"), 'A', 16);
        WriteFill(TmpPath("file17.bin"), 'B', 17);
        WriteFill(TmpPath("file32.bin"), 'C', 32);
        WriteBytes(TmpPath("content.bin"), "Hello, World!", 13);

        /* minimal valid RLE: 3 bytes of 65 */
        uint8_t body[3] = {65, 65, 65};
        std::string rle = MakeRLEPayload(body, 3, 3);
        WriteBytes(TmpPath("rle_valid.bin"), rle.data(), rle.size());

        /* invalid: type byte 5 */
        uint8_t bad[4] = {5, 3, 0, 0};
        WriteBytes(TmpPath("rle_bad.bin"), bad, 4);

        /* empty file */
        WriteFill(TmpPath("empty.bin"), 0, 0);
    }

    void TearDown() override
    {
        DIR *d = opendir(g_tmpdir);
        if (d) {
            struct dirent *e;
            while ((e = readdir(d)) != nullptr) {
                if (e->d_name[0] == '.') continue;
                remove((std::string(g_tmpdir) + "/" + e->d_name).c_str());
            }
            closedir(d);
        }
        rmdir(g_tmpdir);
    }
};


/* ================================================================== */
/* file_build_path                                                       */
/* ================================================================== */

TEST(FileBuildPath, FullPath)
{
    char buf[260] = {};
    file_build_path("assets", "MAIN", ".RES", buf, sizeof(buf));
    EXPECT_STREQ("assets/MAIN.RES", buf);
}
TEST(FileBuildPath, NullDir)
{
    char buf[260] = {};
    file_build_path(nullptr, "MAIN", ".RES", buf, sizeof(buf));
    EXPECT_STREQ("MAIN.RES", buf);
}
TEST(FileBuildPath, NullExt)
{
    char buf[260] = {};
    file_build_path("assets", "MAIN", nullptr, buf, sizeof(buf));
    EXPECT_STREQ("assets/MAIN", buf);
}
TEST(FileBuildPath, NullDst)
{
    EXPECT_NO_FATAL_FAILURE(file_build_path("assets", "MAIN", ".RES", nullptr, 0));
}
TEST(FileBuildPath, TrailingSlash)
{
    char buf[260] = {};
    file_build_path("assets/", "MAIN", ".RES", buf, sizeof(buf));
    EXPECT_STREQ("assets/MAIN.RES", buf);
}
TEST(FileBuildPath, TrailingBackslash)
{
    char buf[260] = {};
    file_build_path("assets\\", "MAIN", ".RES", buf, sizeof(buf));
    EXPECT_STREQ("assets\\MAIN.RES", buf);
}
TEST(FileBuildPath, TrailingColon)
{
    char buf[260] = {};
    file_build_path("C:", "MAIN", ".RES", buf, sizeof(buf));
    EXPECT_STREQ("C:MAIN.RES", buf);
}
TEST(FileBuildPath, EmptyDir)
{
    char buf[260] = {};
    file_build_path("", "MAIN", ".RES", buf, sizeof(buf));
    EXPECT_STREQ("MAIN.RES", buf);
}
TEST(FileBuildPath, AllNullComponents)
{
    char buf[260] = {};
    file_build_path(nullptr, nullptr, nullptr, buf, sizeof(buf));
    EXPECT_STREQ("", buf);
}
TEST(FileBuildPath, EmptyExt)
{
    char buf[260] = {};
    file_build_path("dir", "file", "", buf, sizeof(buf));
    EXPECT_STREQ("dir/file", buf);
}
TEST(FileBuildPath, NullDstNullAll)
{
    EXPECT_NO_FATAL_FAILURE(file_build_path(nullptr, nullptr, nullptr, nullptr, 0));
}
TEST(FileBuildPath, NameOnly)
{
    char buf[260] = {};
    file_build_path(nullptr, "GAME", nullptr, buf, sizeof(buf));
    EXPECT_STREQ("GAME", buf);
}

/* ================================================================== */
/* file_find / file_find_next / file_find_next_alt                       */
/* ================================================================== */

TEST_F(ResourcesTest, FindNull)
{
    EXPECT_EQ(nullptr, file_find(nullptr));
}
TEST_F(ResourcesTest, FindNonExistent)
{
    EXPECT_EQ(nullptr, file_find(TmpPath("does_not_exist_xxxx.bin").c_str()));
}
TEST_F(ResourcesTest, FindExistingAbsolutePath)
{
    const char *r = file_find(TmpPath("file16.bin").c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_NE(std::string::npos, std::string(r).find("file16.bin"));
}
TEST_F(ResourcesTest, FindNextAfterExactMatch)
{
    file_find(TmpPath("file16.bin").c_str());
    EXPECT_EQ(nullptr, file_find_next());
}
TEST_F(ResourcesTest, FindNextAltEqualsNext)
{
    file_find(nullptr);
    EXPECT_EQ(nullptr, file_find_next_alt());
}
TEST_F(ResourcesTest, FindWildcardMatchesMultiple)
{
    std::string pat = std::string(g_tmpdir) + "/file*.bin";
    const char *first = file_find(pat.c_str());
    EXPECT_NE(nullptr, first);
    int count = 1;
    while (file_find_next() != nullptr) count++;
    EXPECT_GE(count, 3);
}
TEST_F(ResourcesTest, FindWildcardQuestionMark)
{
    std::string pat = std::string(g_tmpdir) + "/file??.bin";
    EXPECT_NE(nullptr, file_find(pat.c_str()));
}
TEST_F(ResourcesTest, FindWildcardNoMatch)
{
    std::string pat = std::string(g_tmpdir) + "/ZZZZZ*.bin";
    EXPECT_EQ(nullptr, file_find(pat.c_str()));
}
TEST_F(ResourcesTest, FindNextAltAfterWildcard)
{
    std::string pat = std::string(g_tmpdir) + "/file*.bin";
    file_find(pat.c_str());
    /* file_find_next_alt is identical to file_find_next */
    const char *n = file_find_next_alt();
    /* may or may not be nullptr depending on match count, just must not crash */
    (void)n;
}

/* ================================================================== */
/* file_combine_and_find                                                  */
/* ================================================================== */

TEST_F(ResourcesTest, CombineAndFindHit)
{
    const char *r = file_combine_and_find(g_tmpdir, "file16", ".bin");
    ASSERT_NE(nullptr, r);
    EXPECT_NE(std::string::npos, std::string(r).find("file16.bin"));
}
TEST_F(ResourcesTest, CombineAndFindMiss)
{
    EXPECT_EQ(nullptr, file_combine_and_find(g_tmpdir, "ZZNOFILE", ".bin"));
}
TEST_F(ResourcesTest, CombineAndFindNullDirNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(file_combine_and_find(nullptr, "file16", ".bin"));
}
TEST_F(ResourcesTest, CombineAndFindNullName)
{
    EXPECT_NO_FATAL_FAILURE(file_combine_and_find(g_tmpdir, nullptr, ".bin"));
}

/* ================================================================== */
/* file_paras_nofatal / file_decomp_paras_nofatal                        */
/* ================================================================== */

TEST_F(ResourcesTest, ParasNonExistent)
{
    EXPECT_EQ(0, file_paras_nofatal(TmpPath("no_such.bin").c_str()));
}
TEST_F(ResourcesTest, ParasNullFilename)
{
    EXPECT_EQ(0, file_paras_nofatal(nullptr));
}
TEST_F(ResourcesTest, Paras16Bytes)
{
    EXPECT_EQ(1, file_paras_nofatal(TmpPath("file16.bin").c_str()));
}
TEST_F(ResourcesTest, Paras17Bytes)
{
    EXPECT_EQ(2, file_paras_nofatal(TmpPath("file17.bin").c_str()));
}
TEST_F(ResourcesTest, Paras32Bytes)
{
    EXPECT_EQ(2, file_paras_nofatal(TmpPath("file32.bin").c_str()));
}
TEST_F(ResourcesTest, ParasEmptyFile)
{
    EXPECT_EQ(0, file_paras_nofatal(TmpPath("empty.bin").c_str()));
}
TEST_F(ResourcesTest, DecompParasDelegatesToParas16)
{
    EXPECT_EQ(file_paras_nofatal(TmpPath("file16.bin").c_str()),
              file_decomp_paras_nofatal(TmpPath("file16.bin").c_str()));
}
TEST_F(ResourcesTest, DecompParasDelegatesToParas17)
{
    EXPECT_EQ(file_paras_nofatal(TmpPath("file17.bin").c_str()),
              file_decomp_paras_nofatal(TmpPath("file17.bin").c_str()));
}
TEST_F(ResourcesTest, DecompParasNufatalNonExistent)
{
    EXPECT_EQ(0, file_decomp_paras_nofatal(TmpPath("no.bin").c_str()));
}
TEST_F(ResourcesTest, ParasFatalWrapperSameAsNofatal)
{
    EXPECT_EQ(file_paras_nofatal(TmpPath("file16.bin").c_str()),
              file_paras_fatal(TmpPath("file16.bin").c_str()));
}
TEST_F(ResourcesTest, DecompParasFatalWrapperSameAsNofatal)
{
    EXPECT_EQ(file_decomp_paras_nofatal(TmpPath("file16.bin").c_str()),
              file_decomp_paras_fatal(TmpPath("file16.bin").c_str()));
}

/* ================================================================== */
/* file_read_nofatal                                                      */
/* ================================================================== */

TEST_F(ResourcesTest, ReadNofatalMissingFile)
{
    char buf[64];
    EXPECT_EQ(nullptr, file_read_nofatal(TmpPath("missing.bin").c_str(), buf));
}
TEST_F(ResourcesTest, ReadNofatalNullFilename)
{
    char buf[64];
    EXPECT_EQ(nullptr, file_read_nofatal(nullptr, buf));
}
TEST_F(ResourcesTest, ReadNofatalIntoBuffer)
{
    char buf[64] = {};
    void *r = file_read_nofatal(TmpPath("content.bin").c_str(), buf);
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(buf, (char *)r);
    EXPECT_EQ(0, memcmp(buf, "Hello, World!", 13));
}
TEST_F(ResourcesTest, ReadNofatalAllocatesWhenDstNull)
{
    void *r = file_read_nofatal(TmpPath("content.bin").c_str(), nullptr);
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, "Hello, World!", 13));
    free(r);
}
TEST_F(ResourcesTest, ReadNofatalEmptyFileReturnsNull)
{
    char buf[64];
    EXPECT_EQ(nullptr, file_read_nofatal(TmpPath("empty.bin").c_str(), buf));
}
TEST_F(ResourcesTest, ReadNofatal16ByteContent)
{
    char buf[64] = {};
    void *r = file_read_nofatal(TmpPath("file16.bin").c_str(), buf);
    ASSERT_NE(nullptr, r);
    for (int i = 0; i < 16; i++) EXPECT_EQ('A', buf[i]);
}
TEST_F(ResourcesTest, ReadAllocPaddedBeyondEnd)
{
    /* file_read allocates rounded-up + 16 extra zero bytes */
    void *r = file_read_nofatal(TmpPath("content.bin").c_str(), nullptr);
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, ((char *)r)[13]); /* byte after 13-byte content must be zero */
    free(r);
}
TEST_F(ResourcesTest, ReadFatalWrapperHit)
{
    char buf[64] = {};
    void *r = file_read_fatal(TmpPath("content.bin").c_str(), buf);
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(buf, "Hello, World!", 13));
}

/* ================================================================== */
/* file_load_binary_nofatal                                               */
/* ================================================================== */

TEST_F(ResourcesTest, LoadBinaryMissing)
{
    EXPECT_EQ(nullptr, file_load_binary_nofatal(TmpPath("nope.bin").c_str()));
}
TEST_F(ResourcesTest, LoadBinaryReadsContent)
{
    void *r = file_load_binary_nofatal(TmpPath("content.bin").c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, "Hello, World!", 13));
    free(r);
}
TEST_F(ResourcesTest, LoadBinaryFatalWrapper)
{
    void *r = file_load_binary_fatal(TmpPath("content.bin").c_str());
    ASSERT_NE(nullptr, r);
    free(r);
}

/* ================================================================== */
/* file_write_nofatal                                                     */
/* ================================================================== */

TEST_F(ResourcesTest, WriteNofatalSuccess)
{
    std::string p = TmpPath("write_test.bin");
    char data[] = "test data";
    EXPECT_EQ(0, file_write_nofatal(p.c_str(), data, 9));
    void *r = file_load_binary_nofatal(p.c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, "test data", 9));
    free(r);
    remove(p.c_str());
}
TEST_F(ResourcesTest, WriteNofatalNullFilename)
{
    char d[] = "x";
    EXPECT_EQ(-1, file_write_nofatal(nullptr, d, 1));
}
TEST_F(ResourcesTest, WriteNofatalBadPath)
{
    EXPECT_EQ(-1, file_write_nofatal("/nonexistent_dir_zzz/file.bin", nullptr, 0));
}
TEST_F(ResourcesTest, WriteNofatalNullSrcZeroLen)
{
    std::string p = TmpPath("empty_write.bin");
    EXPECT_EQ(0, file_write_nofatal(p.c_str(), nullptr, 0));
    remove(p.c_str());
}
TEST_F(ResourcesTest, WriteRoundTrip)
{
    std::string p = TmpPath("roundtrip.bin");
    uint8_t out[8] = {222, 173, 190, 239, 1, 2, 3, 4};
    file_write_nofatal(p.c_str(), out, 8);
    void *r = file_load_binary_nofatal(p.c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, out, 8));
    free(r);
    remove(p.c_str());
}
TEST_F(ResourcesTest, WriteFatalWrapperSuccess)
{
    std::string p = TmpPath("write_fatal.bin");
    char d[] = "ok";
    EXPECT_EQ(0, file_write_fatal(p.c_str(), d, 2));
    remove(p.c_str());
}
TEST_F(ResourcesTest, WriteNofatalLargerPayload)
{
    std::string p = TmpPath("large.bin");
    std::string payload(1024, 'Z');
    EXPECT_EQ(0, file_write_nofatal(p.c_str(), payload.data(), 1024));
    EXPECT_EQ(64, file_paras_nofatal(p.c_str()));  /* 1024/16 = 64 paras */
    remove(p.c_str());
}

/* ================================================================== */
/* file_decomp_nofatal / file_decomp_fatal                               */
/* ================================================================== */

TEST_F(ResourcesTest, DecompNofatalNullFilename)
{
    EXPECT_EQ(nullptr, file_decomp_nofatal(nullptr));
}
TEST_F(ResourcesTest, DecompNofatalMissingFile)
{
    EXPECT_EQ(nullptr, file_decomp_nofatal(TmpPath("no.bin").c_str()));
}
TEST_F(ResourcesTest, DecompNofatalInvalidTypeByte)
{
    EXPECT_EQ(nullptr, file_decomp_nofatal(TmpPath("rle_bad.bin").c_str()));
}
TEST_F(ResourcesTest, DecompNofatalTooShort)
{
    std::string p = TmpPath("tiny.bin");
    uint8_t tiny[3] = {1, 0, 0};
    WriteBytes(p, tiny, 3);
    EXPECT_EQ(nullptr, file_decomp_nofatal(p.c_str()));
    remove(p.c_str());
}
TEST_F(ResourcesTest, DecompNofatalValidRLE)
{
    void *r = file_decomp_nofatal(TmpPath("rle_valid.bin").c_str());
    ASSERT_NE(nullptr, r) << "file_decomp returned NULL for valid RLE data";
    EXPECT_EQ(65, ((uint8_t *)r)[0]);
    EXPECT_EQ(65, ((uint8_t *)r)[1]);
    EXPECT_EQ(65, ((uint8_t *)r)[2]);
    free(r);
}
TEST_F(ResourcesTest, DecompFatalNullReturnsNull)
{
    EXPECT_EQ(nullptr, file_decomp_fatal(nullptr));
}
TEST_F(ResourcesTest, DecompFatalInvalidData)
{
    EXPECT_EQ(nullptr, file_decomp_fatal(TmpPath("rle_bad.bin").c_str()));
}
TEST_F(ResourcesTest, DecompNofatalMultipleCallsSameFile)
{
    /* mmgr_get_chunk_by_name returns nullptr (no cache), so each call re-reads */
    void *r1 = file_decomp_nofatal(TmpPath("rle_valid.bin").c_str());
    void *r2 = file_decomp_nofatal(TmpPath("rle_valid.bin").c_str());
    ASSERT_NE(nullptr, r1);
    ASSERT_NE(nullptr, r2);
    EXPECT_EQ(0, memcmp(r1, r2, 3));
    free(r1);
    free(r2);
}

/* ================================================================== */
/* file_load_resource                                                     */
/* ================================================================== */

TEST_F(ResourcesTest, LoadResourceType0)
{
    void *r = file_load_resource(0, TmpPath("content.bin").c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, "Hello, World!", 13));
    free(r);
}
TEST_F(ResourcesTest, LoadResourceType1)
{
    void *r = file_load_resource(1, TmpPath("content.bin").c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, "Hello, World!", 13));
    free(r);
}
TEST_F(ResourcesTest, LoadResourceMissingPath)
{
    EXPECT_EQ(nullptr, file_load_resource(0, TmpPath("no_such.bin").c_str()));
}
TEST_F(ResourcesTest, LoadResourceNullFilename)
{
    EXPECT_EQ(nullptr, file_load_resource(0, nullptr));
}
TEST_F(ResourcesTest, LoadResourceType7ValidRLE)
{
    void *r = file_load_resource(7, TmpPath("rle_valid.bin").c_str());
    ASSERT_NE(nullptr, r);
    free(r);
}
TEST_F(ResourcesTest, LoadResourceType7BinaryFallback)
{
    /* content.bin has first byte 'H' which is not a valid decomp type,
       so file_decomp returns NULL and type-7 falls back to binary read */
    void *r = file_load_resource(7, TmpPath("content.bin").c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, "Hello, World!", 13));
    free(r);
}
TEST_F(ResourcesTest, LoadResourceUnknownTypeReturnsNull)
{
    EXPECT_EQ(nullptr, file_load_resource(99, TmpPath("content.bin").c_str()));
}

/* ================================================================== */
/* file_load_resfile                                                      */
/* ================================================================== */

TEST_F(ResourcesTest, LoadResfileNoFile)
{
    EXPECT_EQ(nullptr, file_load_resfile(TmpPath("NOSUCHNAME").c_str()));
}
TEST_F(ResourcesTest, LoadResfileNullFilename)
{
    EXPECT_EQ(nullptr, file_load_resfile(nullptr));
}
TEST_F(ResourcesTest, LoadResfileFindsResExtension)
{
    std::string rp = TmpPath("MYRES.res");
    WriteBytes(rp, "resdata", 7);
    void *r = file_load_resfile(TmpPath("MYRES").c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, "resdata", 7));
    free(r);
    remove(rp.c_str());
}

/* ================================================================== */
/* file_load_3dres                                                        */
/* ================================================================== */

TEST_F(ResourcesTest, Load3dresNullFilename)
{
    EXPECT_EQ(nullptr, file_load_3dres(nullptr));
}
TEST_F(ResourcesTest, Load3dresNoFile)
{
    EXPECT_EQ(nullptr, file_load_3dres(TmpPath("NO3D").c_str()));
}
TEST_F(ResourcesTest, Load3dresFinds3shFallback)
{
    std::string sp = TmpPath("MY3D.3sh");
    WriteBytes(sp, "3ddata", 6);
    void *r = file_load_3dres(TmpPath("MY3D").c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_EQ(0, memcmp(r, "3ddata", 6));
    free(r);
    remove(sp.c_str());
}

/* ================================================================== */
/* unload_resource                                                        */
/* ================================================================== */

TEST_F(ResourcesTest, UnloadResourceNull)
{
    EXPECT_NO_FATAL_FAILURE(unload_resource(nullptr));
}
TEST_F(ResourcesTest, UnloadResourceAllocated)
{
    void *r = file_load_binary_nofatal(TmpPath("content.bin").c_str());
    ASSERT_NE(nullptr, r);
    EXPECT_NO_FATAL_FAILURE(unload_resource(r));
}

/* ================================================================== */
/* parse_filepath_separators_dosptr                                       */
/* ================================================================== */

TEST(ParseFilepath, SimpleBasename)
{
    char out[32] = {};
    parse_filepath_separators_dosptr("MAIN.RES", out);
    EXPECT_STREQ("MAIN", out);
}
TEST(ParseFilepath, PathWithForwardSlash)
{
    char out[32] = {};
    parse_filepath_separators_dosptr("assets/MAIN.RES", out);
    EXPECT_STREQ("MAIN", out);
}
TEST(ParseFilepath, PathWithBackslash)
{
    char out[32] = {};
    parse_filepath_separators_dosptr("C:\\GAME\\MAIN.RES", out);
    EXPECT_STREQ("MAIN", out);
}
TEST(ParseFilepath, PathWithColon)
{
    char out[32] = {};
    parse_filepath_separators_dosptr("C:MAIN.RES", out);
    EXPECT_STREQ("MAIN", out);
}
TEST(ParseFilepath, NoExtension)
{
    char out[32] = {};
    parse_filepath_separators_dosptr("FILENAME", out);
    EXPECT_STREQ("FILENAME", out);
}
TEST(ParseFilepath, UppercasesOutput)
{
    char out[32] = {};
    parse_filepath_separators_dosptr("dir/lower.res", out);
    EXPECT_STREQ("LOWER", out);
}
TEST(ParseFilepath, EmptyInput)
{
    char out[32] = {'X', 0};
    parse_filepath_separators_dosptr("", out);
    EXPECT_STREQ("", out);
}
TEST(ParseFilepath, NullDstNoCrash)
{
    EXPECT_NO_FATAL_FAILURE(parse_filepath_separators_dosptr("test.res", nullptr));
}
TEST(ParseFilepath, NullSrcNoCrash)
{
    char out[32] = {};
    EXPECT_NO_FATAL_FAILURE(parse_filepath_separators_dosptr(nullptr, out));
}
TEST(ParseFilepath, TruncatesAt12Chars)
{
    char out[32] = {};
    parse_filepath_separators_dosptr("THISFILENAMEISVERYLONG", out);
    EXPECT_LE(strlen(out), 12u);
    EXPECT_EQ(0, strncmp(out, "THISFILENAM", 11));
}
TEST(ParseFilepath, DotFirstChar)
{
    /* ".hidden" — stops at dot immediately, output is empty */
    char out[32] = {};
    parse_filepath_separators_dosptr(".hidden", out);
    EXPECT_STREQ("", out);
}
TEST(ParseFilepath, DeepPath)
{
    char out[32] = {};
    parse_filepath_separators_dosptr("a/b/c/d/FILE.RES", out);
    EXPECT_STREQ("FILE", out);
}
