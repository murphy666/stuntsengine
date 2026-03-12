/*
 * test_track_decode.cc — Tests for the track object binary decode functions.
 *
 * state_trackobject_raw_decode  — deserialises a 14-byte packed track-object
 *                                  entry from a flat byte table.
 * state_trkobjinfo_raw_decode   — deserialises a 14-byte packed track-object
 *                                  info entry from a flat byte table.
 *
 * The functions are pure: they do pointer arithmetic and field assignments
 * only, with no side effects or global state. We copy the three helpers
 * (read_u16, plus the two decode functions) as static helpers here so the
 * test pulls zero external dependencies from the rest of track.c.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>

/* ── minimal struct / constant definitions ──────────────────────────────── */

#define STATE_TRACKOBJECT_RAW_SIZE 14u
#define STATE_TRKOBJINFO_RAW_SIZE  14u

typedef struct {
    uint16_t trkobj_info_ofs;   /* bytes 0-1  little-endian */
    int16_t  rot_y;             /* bytes 2-3  little-endian (signed) */
    uint16_t shape_ofs;         /* bytes 4-5  little-endian */
    uint16_t lo_shape_ofs;      /* bytes 6-7  little-endian */
    uint8_t  overlay;           /* byte  8  */
    uint8_t  surface_type;      /* byte  9  */
    uint8_t  ignore_z_bias;     /* byte 10  */
    uint8_t  multi_tile_flag;   /* byte 11  */
    uint8_t  physical_model;    /* byte 12  */
    uint8_t  scene_reserved5;        /* byte 13  */
} TrackObjRaw;

typedef struct {
    uint8_t  no_of_blocks;      /* byte  0  */
    uint8_t  entry_point;       /* byte  1  */
    uint8_t  exit_point;        /* byte  2  */
    uint8_t  entry_type;        /* byte  3  */
    uint8_t  exit_type;         /* byte  4  */
    uint8_t  arrow_type;        /* byte  5  */
    int16_t  arrow_orient;      /* bytes 6-7  little-endian (signed) */
    uint16_t camera_data_ofs;   /* bytes 8-9  little-endian */
    uint8_t  opp1;              /* byte 10  */
    uint8_t  opp2;              /* byte 11  */
    uint8_t  opp3;              /* byte 12  */
    uint8_t  opp_sped_code;     /* byte 13  */
} TrkObjInfoRaw;

/* ── inline copies of the decode helpers ───────────────────────────────── */

static uint16_t ru16(const uint8_t* p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static int decode_trkobj(const uint8_t* table, unsigned index, TrackObjRaw* out)
{
    if (!table || !out) return 0;
    const uint8_t* p = table + (STATE_TRACKOBJECT_RAW_SIZE * index);
    out->trkobj_info_ofs = ru16(p + 0);
    out->rot_y           = (int16_t)ru16(p + 2);
    out->shape_ofs       = ru16(p + 4);
    out->lo_shape_ofs    = ru16(p + 6);
    out->overlay         = p[8];
    out->surface_type    = p[9];
    out->ignore_z_bias   = p[10];
    out->multi_tile_flag = p[11];
    out->physical_model  = p[12];
    out->scene_reserved5      = p[13];
    return 1;
}

static int decode_trkobjinfo(const uint8_t* table, unsigned index, TrkObjInfoRaw* out)
{
    if (!table || !out) return 0;
    const uint8_t* p = table + (STATE_TRKOBJINFO_RAW_SIZE * index);
    out->no_of_blocks    = p[0];
    out->entry_point     = p[1];
    out->exit_point      = p[2];
    out->entry_type      = p[3];
    out->exit_type       = p[4];
    out->arrow_type      = p[5];
    out->arrow_orient    = (int16_t)ru16(p + 6);
    out->camera_data_ofs = ru16(p + 8);
    out->opp1            = p[10];
    out->opp2            = p[11];
    out->opp3            = p[12];
    out->opp_sped_code   = p[13];
    return 1;
}

/* ══════════════════════════════════════════════════════════════════════════
 * TrackObjRaw decode — helpers
 * ══════════════════════════════════════════════════════════════════════════ */

/*
 * Pack a TrackObjRaw descriptor into 14 raw bytes (little-endian u16s).
 */
static void pack_trkobj(uint8_t* out, uint16_t info_ofs, int16_t rot_y,
                        uint16_t shape, uint16_t lo_shape,
                        uint8_t overlay, uint8_t surface, uint8_t z_bias,
                        uint8_t multi, uint8_t phys, uint8_t unk5)
{
    out[0] = (uint8_t)(info_ofs & 255);   out[1] = (uint8_t)(info_ofs >> 8);
    out[2] = (uint8_t)((uint16_t)rot_y & 255); out[3] = (uint8_t)((uint16_t)rot_y >> 8);
    out[4] = (uint8_t)(shape & 255);     out[5] = (uint8_t)(shape >> 8);
    out[6] = (uint8_t)(lo_shape & 255);  out[7] = (uint8_t)(lo_shape >> 8);
    out[8]  = overlay;  out[9]  = surface; out[10] = z_bias;
    out[11] = multi;    out[12] = phys;    out[13] = unk5;
}

/* ── Null-pointer guards ─────────────────────────────────────────────────── */

TEST(TrackObjDecode, NullTableReturnsZero) {
    TrackObjRaw out{};
    EXPECT_EQ(decode_trkobj(nullptr, 0, &out), 0);
}

TEST(TrackObjDecode, NullOutReturnsZero) {
    const uint8_t table[14] = {};
    EXPECT_EQ(decode_trkobj(table, 0, nullptr), 0);
}

TEST(TrackObjDecode, BothNullReturnsZero) {
    EXPECT_EQ(decode_trkobj(nullptr, 0, nullptr), 0);
}

TEST(TrackObjDecode, ValidInputReturnsOne) {
    uint8_t table[14] = {};
    TrackObjRaw out{};
    EXPECT_EQ(decode_trkobj(table, 0, &out), 1);
}

/* ── Field decoding at index 0 ──────────────────────────────────────────── */

TEST(TrackObjDecode, AllFieldsAtIndex0) {
    uint8_t table[STATE_TRACKOBJECT_RAW_SIZE];
    pack_trkobj(table,
        /*info_ofs*/   4660,
        /*rot_y*/      22136,
        /*shape*/      43981,
        /*lo_shape*/   61185,
        /*overlay*/    17,
        /*surface*/    34,
        /*z_bias*/     51,
        /*multi*/      68,
        /*phys*/       85,
        /*unk5*/       102);

    TrackObjRaw out{};
    decode_trkobj(table, 0, &out);

    EXPECT_EQ(out.trkobj_info_ofs, 4660u);
    EXPECT_EQ((uint16_t)out.rot_y, 22136u);
    EXPECT_EQ(out.shape_ofs,       43981u);
    EXPECT_EQ(out.lo_shape_ofs,    61185u);
    EXPECT_EQ(out.overlay,         17u);
    EXPECT_EQ(out.surface_type,    34u);
    EXPECT_EQ(out.ignore_z_bias,   51u);
    EXPECT_EQ(out.multi_tile_flag, 68u);
    EXPECT_EQ(out.physical_model,  85u);
    EXPECT_EQ(out.scene_reserved5,      102u);
}

/* ── Little-endian u16 decoding ─────────────────────────────────────────── */

TEST(TrackObjDecode, LittleEndianU16) {
    uint8_t table[14] = {};
    /* Set bytes 0-1 to {255, 1} → should decode as 511 */
    table[0] = 255; table[1] = 1;
    TrackObjRaw out{};
    decode_trkobj(table, 0, &out);
    EXPECT_EQ(out.trkobj_info_ofs, 511u) << "low byte first, high byte second";
}

/* ── Signed rot_y (negative value) ─────────────────────────────────────── */

TEST(TrackObjDecode, NegativeRotY) {
    uint8_t table[14] = {};
    /* -1 in little-endian int16: bytes {255, 255} */
    table[2] = 255; table[3] = 255;
    TrackObjRaw out{};
    decode_trkobj(table, 0, &out);
    EXPECT_EQ(out.rot_y, (int16_t)-1);
}

TEST(TrackObjDecode, RotYMinusHalf) {
    uint8_t table[14] = {};
    /* -1024 = 64512 → bytes {0, 252} */
    table[2] = 0; table[3] = 252;
    TrackObjRaw out{};
    decode_trkobj(table, 0, &out);
    EXPECT_EQ(out.rot_y, (int16_t)-1024);
}

/* ── Index striding: each entry is 14 bytes apart ──────────────────────── */

TEST(TrackObjDecode, IndexOneReadsSecondEntry) {
    uint8_t table[2 * STATE_TRACKOBJECT_RAW_SIZE] = {};
    /* Entry 0: info_ofs = 1 */
    table[0] = 1; table[1] = 0;
    /* Entry 1: info_ofs = 2 */
    table[STATE_TRACKOBJECT_RAW_SIZE + 0] = 2;
    table[STATE_TRACKOBJECT_RAW_SIZE + 1] = 0;

    TrackObjRaw out{};
    decode_trkobj(table, 1, &out);
    EXPECT_EQ(out.trkobj_info_ofs, 2u) << "index 1 must stride by 14 bytes";
}

TEST(TrackObjDecode, IndexZeroUnaffectedBySecondEntry) {
    uint8_t table[2 * STATE_TRACKOBJECT_RAW_SIZE] = {};
    table[0] = 170; /* entry 0, byte 0 */
    table[STATE_TRACKOBJECT_RAW_SIZE] = 187; /* entry 1, byte 0 */

    TrackObjRaw out{};
    decode_trkobj(table, 0, &out);
    EXPECT_EQ(out.trkobj_info_ofs & 255, 170u);
}

/* ── All-zero table decodes cleanly ──────────────────────────────────────── */

TEST(TrackObjDecode, AllZeroTable) {
    const uint8_t table[14] = {};
    TrackObjRaw out;
    memset(&out, 255, sizeof(out));
    decode_trkobj(table, 0, &out);
    EXPECT_EQ(out.trkobj_info_ofs, 0u);
    EXPECT_EQ(out.rot_y,           0);
    EXPECT_EQ(out.shape_ofs,       0u);
    EXPECT_EQ(out.overlay,         0u);
    EXPECT_EQ(out.surface_type,    0u);
    EXPECT_EQ(out.scene_reserved5,      0u);
}

/* ══════════════════════════════════════════════════════════════════════════
 * TrkObjInfoRaw decode
 * ══════════════════════════════════════════════════════════════════════════ */

static void pack_trkobjinfo(uint8_t* out,
                            uint8_t blocks, uint8_t entry, uint8_t expt,
                            uint8_t etype, uint8_t xtype, uint8_t arrow,
                            int16_t orient, uint16_t cam_ofs,
                            uint8_t o1, uint8_t o2, uint8_t o3, uint8_t spd)
{
    out[0] = blocks; out[1] = entry; out[2] = expt;
    out[3] = etype;  out[4] = xtype; out[5] = arrow;
    out[6] = (uint8_t)((uint16_t)orient & 255);
    out[7] = (uint8_t)((uint16_t)orient >> 8);
    out[8] = (uint8_t)(cam_ofs & 255);
    out[9] = (uint8_t)(cam_ofs >> 8);
    out[10] = o1; out[11] = o2; out[12] = o3; out[13] = spd;
}

/* ── Null guards ─────────────────────────────────────────────────────────── */

TEST(TrkObjInfoDecode, NullTableReturnsZero) {
    TrkObjInfoRaw out{};
    EXPECT_EQ(decode_trkobjinfo(nullptr, 0, &out), 0);
}

TEST(TrkObjInfoDecode, NullOutReturnsZero) {
    const uint8_t table[14] = {};
    EXPECT_EQ(decode_trkobjinfo(table, 0, nullptr), 0);
}

TEST(TrkObjInfoDecode, ValidInputReturnsOne) {
    const uint8_t table[14] = {};
    TrkObjInfoRaw out{};
    EXPECT_EQ(decode_trkobjinfo(table, 0, &out), 1);
}

/* ── All fields at index 0 ───────────────────────────────────────────────── */

TEST(TrkObjInfoDecode, AllFieldsAtIndex0) {
    uint8_t table[STATE_TRKOBJINFO_RAW_SIZE];
    pack_trkobjinfo(table,
        /*blocks*/ 3, /*entry*/ 1, /*exit*/ 2,
        /*etype*/  4, /*xtype*/ 5, /*arrow*/ 6,
        /*orient*/ 4660, /*cam_ofs*/ 43981,
        /*opp*/ 7, 8, 9, /*spd*/ 15);

    TrkObjInfoRaw out{};
    decode_trkobjinfo(table, 0, &out);

    EXPECT_EQ(out.no_of_blocks,    3u);
    EXPECT_EQ(out.entry_point,     1u);
    EXPECT_EQ(out.exit_point,      2u);
    EXPECT_EQ(out.entry_type,      4u);
    EXPECT_EQ(out.exit_type,       5u);
    EXPECT_EQ(out.arrow_type,      6u);
    EXPECT_EQ((uint16_t)out.arrow_orient, 4660u);
    EXPECT_EQ(out.camera_data_ofs, 43981u);
    EXPECT_EQ(out.opp1,            7u);
    EXPECT_EQ(out.opp2,            8u);
    EXPECT_EQ(out.opp3,            9u);
    EXPECT_EQ(out.opp_sped_code,   15u);
}

/* ── Negative arrow_orient ──────────────────────────────────────────────── */

TEST(TrkObjInfoDecode, NegativeArrowOrient) {
    uint8_t table[14] = {};
    /* -2 = 65534 → bytes {254, 255} at offset 6 */
    table[6] = 254; table[7] = 255;
    TrkObjInfoRaw out{};
    decode_trkobjinfo(table, 0, &out);
    EXPECT_EQ(out.arrow_orient, (int16_t)-2);
}

/* ── Index striding ──────────────────────────────────────────────────────── */

TEST(TrkObjInfoDecode, Index1ReadsSecondEntry) {
    uint8_t table[2 * STATE_TRKOBJINFO_RAW_SIZE] = {};
    table[0] = 170; /* entry 0 byte 0 */
    table[STATE_TRKOBJINFO_RAW_SIZE + 0] = 187; /* entry 1 byte 0 */

    TrkObjInfoRaw out{};
    decode_trkobjinfo(table, 1, &out);
    EXPECT_EQ(out.no_of_blocks, 187u) << "index 1 must stride by 14 bytes";
}

/* ── camera_data_ofs little-endian ──────────────────────────────────────── */

TEST(TrkObjInfoDecode, CameraOfsLittleEndian) {
    uint8_t table[14] = {};
    table[8] = 52; table[9] = 18; /* 4660 in LE */
    TrkObjInfoRaw out{};
    decode_trkobjinfo(table, 0, &out);
    EXPECT_EQ(out.camera_data_ofs, 4660u);
}

/* ── All-zero decodes cleanly ────────────────────────────────────────────── */

TEST(TrkObjInfoDecode, AllZeroTable) {
    const uint8_t table[14] = {};
    TrkObjInfoRaw out;
    memset(&out, 255, sizeof(out));
    decode_trkobjinfo(table, 0, &out);
    EXPECT_EQ(out.no_of_blocks,    0u);
    EXPECT_EQ(out.entry_point,     0u);
    EXPECT_EQ(out.arrow_orient,    0);
    EXPECT_EQ(out.camera_data_ofs, 0u);
    EXPECT_EQ(out.opp_sped_code,   0u);
}
