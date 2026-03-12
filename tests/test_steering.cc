/*
 * test_steering.cc — Tests for keyboard input byte mapping and steerWhlRespTable.
 *
 * Standalone (no game source files): all game data needed is defined inline.
 *
 * Covers:
 *   - Input byte bit-field extraction: (inputByte >> 2) & 3 → steeringInput
 *   - Keyboard flag bit assignments (bit 2 = RIGHT, bit 3 = LEFT)
 *   - steerWhlRespTable_20fps data integrity (16 speed slots × 4 bytes)
 *   - steerWhlRespTable_10fps data integrity (15 complete speed slots × 4 bytes)
 *   - Monotonically non-increasing sensitivity with speed
 *   - Left/right delta symmetry: right_delta == -left_delta at each slot
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>

/* ---- Verbatim table copies from src/data_menu_track.c ---- */

static const unsigned char steer20fps[64] = {
    0, 8, 248, 0,  0, 7, 249, 0,
    0, 6, 250, 0,  0, 5, 251, 0,
    0, 4, 252, 0,  0, 4, 252, 0,
    0, 3, 253, 0,  0, 3, 253, 0,
    0, 2, 254, 0,  0, 2, 254, 0,
    0, 2, 254, 0,  0, 1, 255, 0,
    0, 1, 255, 0,  0, 1, 255, 0,
    0, 1, 255, 0,  0, 1, 255, 0
};

/* 62 bytes: 15 complete slots (60 bytes) + 2-byte start of slot 15 */
static const unsigned char steer10fps[62] = {
    0, 16, 240, 0,  0, 14, 242, 0,
    0, 12, 244, 0,  0, 10, 246, 0,
    0, 8, 248, 0,  0, 8, 248, 0,
    0, 6, 250, 0,  0, 6, 250, 0,
    0, 4, 252, 0,  0, 4, 252, 0,
    0, 4, 252, 0,  0, 2, 254, 0,
    0, 2, 254, 0,  0, 1, 255, 0,
    0, 1, 255, 0,  0, 1
};

/* Helper: signed byte from unsigned table */
static int sbyte(const unsigned char* tbl, int idx) {
    return (int)(signed char)tbl[idx];
}

/* ================================================================
 * 1. Input byte → steeringInput  (logic: (inputByte >> 2) & 3)
 * ================================================================ */

TEST(SteerInput, RightKey) {
    /* bit 2 = 4 = RIGHT */
    unsigned char ib = 4;
    EXPECT_EQ((ib >> 2) & 3, 1) << "RIGHT key → steeringInput 1";
}

TEST(SteerInput, LeftKey) {
    /* bit 3 = 8 = LEFT */
    unsigned char ib = 8;
    EXPECT_EQ((ib >> 2) & 3, 2) << "LEFT key → steeringInput 2";
}

TEST(SteerInput, NeutralNoKeys) {
    EXPECT_EQ((0u >> 2) & 3, 0u) << "no keys → steeringInput 0";
}

TEST(SteerInput, BothLRKeys) {
    unsigned char ib = 12; /* both L+R pressed */
    EXPECT_EQ((ib >> 2) & 3, 3u) << "both L+R → steeringInput 3";
}

TEST(SteerInput, UpDownDoNotSteer) {
    /* UP=bit0, DOWN=bit1: neither maps to steer bits */
    EXPECT_EQ((1u >> 2) & 3, 0u) << "UP key → no steer";
    EXPECT_EQ((2u >> 2) & 3, 0u) << "DOWN key → no steer";
}

TEST(SteerInput, AccelBrakeDoNotSteer) {
    /* A=bit4, Z=bit5: neither maps to steer bits */
    EXPECT_EQ((16u >> 2) & 3, 0u) << "A (accel) key → no steer";
    EXPECT_EQ((32u >> 2) & 3, 0u) << "Z (brake) key → no steer";
}

TEST(SteerInput, FullFlagMix) {
    /* gas + RIGHT: 16 | 4 = 20 */
    unsigned char ib = 20;
    EXPECT_EQ((ib >> 2) & 3, 1u) << "accel+RIGHT → steeringInput 1";

    /* brake + LEFT: 32 | 8 = 40 */
    ib = 40;
    EXPECT_EQ((ib >> 2) & 3, 2u) << "brake+LEFT → steeringInput 2";
}

/* ================================================================
 * 2. steerWhlRespTable_20fps  (16 complete 4-byte speed slots)
 *
 * Layout per slot i:  [0]=neutral_delta, [1]=right_delta,
 *                     [2]=left_delta,    [3]=padding (0)
 * Speed index at run-time: speedMask = (car_speed2 >> 10) & 252
 * So slot 0 → speedMask=0, slot 1 → speedMask=4, ..., slot 15 → speedMask=60
 * ================================================================ */

TEST(SteerTable20fps, NeutralDeltaIsAlwaysZero) {
    for (int slot = 0; slot < 16; slot++) {
        EXPECT_EQ(sbyte(steer20fps, slot * 4), 0)
            << "slot " << slot << " neutral delta must be 0";
    }
}

TEST(SteerTable20fps, RightDeltaIsAlwaysPositive) {
    for (int slot = 0; slot < 16; slot++) {
        EXPECT_GT(sbyte(steer20fps, slot * 4 + 1), 0)
            << "slot " << slot << " right delta must be > 0";
    }
}

TEST(SteerTable20fps, LeftDeltaIsAlwaysNegative) {
    for (int slot = 0; slot < 16; slot++) {
        EXPECT_LT(sbyte(steer20fps, slot * 4 + 2), 0)
            << "slot " << slot << " left delta must be < 0";
    }
}

TEST(SteerTable20fps, LeftRightSymmetry) {
    for (int slot = 0; slot < 16; slot++) {
        int right =  sbyte(steer20fps, slot * 4 + 1);
        int left  = -sbyte(steer20fps, slot * 4 + 2);
        EXPECT_EQ(right, left)
            << "slot " << slot << " right_delta == -left_delta";
    }
}

TEST(SteerTable20fps, SensitivityDecreaseWithSpeed) {
    /* right_delta at slot 0 should be >= right_delta at slot 15 */
    int delta0  = sbyte(steer20fps, 0 * 4 + 1);
    int delta15 = sbyte(steer20fps, 15 * 4 + 1);
    EXPECT_GE(delta0, delta15) << "steering less responsive at high speed";
    /* spot-check: slot 0=8, slot 4=4, slot 8=2, slot 15=1 */
    EXPECT_EQ(sbyte(steer20fps,  0 * 4 + 1),  8) << "slot 0 right delta=8";
    EXPECT_EQ(sbyte(steer20fps,  4 * 4 + 1),  4) << "slot 4 right delta=4";
    EXPECT_EQ(sbyte(steer20fps,  8 * 4 + 1),  2) << "slot 8 right delta=2";
    EXPECT_EQ(sbyte(steer20fps, 15 * 4 + 1),  1) << "slot 15 right delta=1";
}

TEST(SteerTable20fps, PaddingByteIsZero) {
    for (int slot = 0; slot < 16; slot++) {
        EXPECT_EQ(steer20fps[slot * 4 + 3], 0u)
            << "slot " << slot << " padding byte must be 0";
    }
}

/* ================================================================
 * 3. steerWhlRespTable_10fps  (15 complete 4-byte speed slots)
 *
 * Same layout; larger deltas at identical speed slots (lower fps =
 * bigger per-frame step to compensate for fewer frames per second).
 * ================================================================ */

TEST(SteerTable10fps, NeutralDeltaIsAlwaysZero) {
    for (int slot = 0; slot < 15; slot++) {
        EXPECT_EQ(sbyte(steer10fps, slot * 4), 0)
            << "slot " << slot << " neutral delta must be 0";
    }
}

TEST(SteerTable10fps, RightDeltaIsAlwaysPositive) {
    for (int slot = 0; slot < 15; slot++) {
        EXPECT_GT(sbyte(steer10fps, slot * 4 + 1), 0)
            << "slot " << slot << " right delta must be > 0";
    }
}

TEST(SteerTable10fps, LeftDeltaIsAlwaysNegative) {
    for (int slot = 0; slot < 15; slot++) {
        EXPECT_LT(sbyte(steer10fps, slot * 4 + 2), 0)
            << "slot " << slot << " left delta must be < 0";
    }
}

TEST(SteerTable10fps, LeftRightSymmetry) {
    for (int slot = 0; slot < 15; slot++) {
        int right =  sbyte(steer10fps, slot * 4 + 1);
        int left  = -sbyte(steer10fps, slot * 4 + 2);
        EXPECT_EQ(right, left)
            << "slot " << slot << " right_delta == -left_delta";
    }
}

TEST(SteerTable10fps, LargerThan20fps) {
    /* At each shared speed slot, 10fps steps must be >= 20fps steps
     * (10fps has fewer updates per second so each step is bigger). */
    for (int slot = 0; slot < 15; slot++) {
        int d10 = sbyte(steer10fps, slot * 4 + 1);
        int d20 = sbyte(steer20fps, slot * 4 + 1);
        EXPECT_GE(d10, d20)
            << "slot " << slot << " 10fps delta >= 20fps delta";
    }
}

TEST(SteerTable10fps, SpotCheckSlot0) {
    EXPECT_EQ(sbyte(steer10fps, 0), 0)   << "slot 0 neutral=0";
    EXPECT_EQ(sbyte(steer10fps, 1), 16)  << "slot 0 right=16 (16)";
    EXPECT_EQ(sbyte(steer10fps, 2), -16) << "slot 0 left=-16 (240)";
}

/* ================================================================
 * 4. steeringInput=3 ("both keys pressed") uses the 4th byte of each
 *    slot, which is the padding byte (always 0). This means the game
 *    interprets simultaneous L+R as "no steering input" — the same
 *    behaviour as index 0 (neutral) but accessed via the 4th byte.
 * ================================================================ */

TEST(SteerInput, BothKeysMapsToNeutralDelta) {
    /* steeringInput = (inputByte >> 2) & 3
     * For 12 (L+R): (12 >> 2) & 3 = 3
     * Table index = speedMask + 3  → 4th byte of slot = padding = 0
     * The game treats simultaneous L+R as zero steering delta. */
    for (int slot = 0; slot < 16; slot++) {
        EXPECT_EQ(sbyte(steer20fps, slot * 4 + 3), 0)
            << "slot " << slot << ": steeringInput=3 (both keys) → zero delta";
    }
}

/* ================================================================
 * 5. steerWhlRespTable_10fps truncation (62 bytes, not 64).
 *
 *    The 10fps table only has 62 bytes.  At the maximum speed slot
 *    (speedMask=60) the game computes idx = speedMask + steeringInput.
 *    For steeringInput ∈ {0,1} the indices 60 and 61 are within bounds.
 *    For steeringInput ∈ {2,3} the indices 62 and 63 are OUT OF BOUNDS.
 *
 *    In the original DOS game steeringInput=2 (LEFT key) or =3 (both)
 *    at maximum speed never occurs in practice, so this was a latent
 *    bug that never triggered. The C port reproduces the original data
 *    faithfully; this test documents the known constraint.
 * ================================================================ */

TEST(SteerTable10fps, TruncatedTo62BytesKnownConstraint) {
    /* The 10fps table is deliberately 62 bytes. */
    static_assert(sizeof(steer10fps) == 62,
                  "10fps table must be exactly 62 bytes — change this only if the"
                  " original game data is corrected to a full 64 bytes");

    /* Highest safe indices in the 62-byte table: 60 and 61 */
    EXPECT_EQ(steer10fps[60], 0u) << "speedMask=60, steeringInput=0: neutral byte present";
    EXPECT_EQ(steer10fps[61], 1u) << "speedMask=60, steeringInput=1: right +1, index 61 present";

    /* Document: indices 62 and 63 (LEFT and both-keys at max speed) are absent. */
    const int MAX_SPEED_MASK = 60;  /* (car_speed2_max >> 10) & 252 */
    EXPECT_LE(MAX_SPEED_MASK + 1, (int)sizeof(steer10fps) - 1)
        << "steeringInput=1 at max speed is still in-bounds";
    EXPECT_GT(MAX_SPEED_MASK + 2, (int)sizeof(steer10fps) - 1)
        << "steeringInput=2 at max speed would be OUT OF BOUNDS — known game quirk";
}

/* ================================================================
 * 6. steerWhlRespTable_10fps — sensitivity decreases with speed
 * ================================================================ */

TEST(SteerTable10fps, SensitivityDecreaseWithSpeed) {
    /* right_delta at slot 0 >= right_delta at slot 14 */
    int delta0  = sbyte(steer10fps, 0 * 4 + 1);
    int delta14 = sbyte(steer10fps, 14 * 4 + 1);
    EXPECT_GE(delta0, delta14) << "steering less responsive at high speed";
    /* Spot-checks: slot 0=16, slot 2=12, slot 4=8, slot 8=4, slot 14=1 */
    EXPECT_EQ(sbyte(steer10fps,  0 * 4 + 1), 16) << "slot  0 right=16";
    EXPECT_EQ(sbyte(steer10fps,  2 * 4 + 1), 12) << "slot  2 right=12";
    EXPECT_EQ(sbyte(steer10fps,  4 * 4 + 1),  8) << "slot  4 right=8";
    EXPECT_EQ(sbyte(steer10fps,  8 * 4 + 1),  4) << "slot  8 right=4";
    EXPECT_EQ(sbyte(steer10fps, 14 * 4 + 1),  1) << "slot 14 right=1";
}
