/*
 * Standalone unit tests for two pure functions extracted from src/carstate.c:
 *
 *   update_rpm_from_speed         (line 140)
 *   carState_update_wheel_suspension  (line 1582)
 *
 * Both functions are inlined as static copies so this test has NO link-time
 * dependency on any game source file.  The inline copies must remain in sync
 * with the originals; any divergence here is a maintenance signal.
 */

#include <gtest/gtest.h>
#include <cstdint>
#include <cstring>

/* ========================================================================
 * Minimal struct — only the three wheel-arrays used by the suspension fn.
 * The real CARSTATE has many more fields; we only need these for isolation.
 * ======================================================================== */
struct WheelState {
    short car_rc2[4]; /* suspension compression (current) */
    short car_rc4[4]; /* reset-to-zero on compression     */
    short car_rc5[4]; /* suspension target / rest level   */
};

/* ========================================================================
 * Inline copy of update_rpm_from_speed  (src/carstate.c line 140)
 * Three observable behaviours:
 *   1. changing_gear != 0  → skip calculation, keep currpm
 *   2. calculated rpm >= idle_rpm → return calculated rpm
 *   3. calculated rpm <  idle_rpm → clamp to idle_rpm
 * ======================================================================== */
static unsigned int rpm_from_speed(unsigned int currpm, unsigned int speed,
                                   unsigned int gearratio, int changing_gear,
                                   unsigned int idle_rpm)
{
    if (changing_gear == 0) {
        currpm = ((unsigned long)speed * gearratio) >> 16;
    }
    if (currpm >= idle_rpm) {
        return currpm;
    }
    return idle_rpm;
}

/* ========================================================================
 * Inline copy of carState_update_wheel_suspension (src/carstate.c line 1582)
 * Adapted to use WheelState instead of struct CARSTATE.
 * ======================================================================== */
static short susp_update(WheelState* pState, short suspension_delta, short wi)
{
    short old_rc2 = pState->car_rc2[wi];
    short delta   = 0;

    /* ---- rc5 decays toward 0 by 4 per frame ---- */
    if (pState->car_rc5[wi] != 0) {
        if (pState->car_rc5[wi] < 0) {
            pState->car_rc5[wi] += 4;
            if (pState->car_rc5[wi] > 0)
                pState->car_rc5[wi] = 0;
        } else {
            pState->car_rc5[wi] -= 4;
            if (pState->car_rc5[wi] < 0)
                pState->car_rc5[wi] = 0;
        }
    }

    /* ---- if suspension_delta < 0 and rc2 large enough, clamp to 0 ---- */
    if (suspension_delta < 0) {
        if (pState->car_rc2[wi] > -suspension_delta)
            suspension_delta = 0;
    }

    if (suspension_delta == 0) {
        /* No force: decay rc2 toward rc5 */
        if (pState->car_rc2[wi] > pState->car_rc5[wi]) {
            pState->car_rc2[wi] -= 128;
            if (pState->car_rc2[wi] < pState->car_rc5[wi])
                pState->car_rc2[wi] = pState->car_rc5[wi];
            delta = old_rc2 - pState->car_rc2[wi];
        } else if (pState->car_rc2[wi] < pState->car_rc5[wi]) {
            pState->car_rc2[wi] += 128;
            if (pState->car_rc2[wi] > pState->car_rc5[wi])
                pState->car_rc2[wi] = pState->car_rc5[wi];
            /* delta stays 0 */
        }
        /* else rc2 == rc5: no change */
    } else if (suspension_delta > 0) {
        /* Compression: cap step at 192, clamp rc2 at 384 */
        if (suspension_delta > 192)
            pState->car_rc2[wi] += 192;
        else
            pState->car_rc2[wi] += suspension_delta;
        if (pState->car_rc2[wi] > 384)
            pState->car_rc2[wi] = 384;
        pState->car_rc4[wi] = 0;
        /* delta stays 0 */
    } else {
        /* Extension (suspension_delta < 0): damp large forces */
        if (pState->car_rc2[wi] + suspension_delta <= (short)65248) {
            pState->car_rc2[wi] += (suspension_delta * 3) >> 2;
            if (pState->car_rc2[wi] < (short)65152)
                pState->car_rc2[wi] = (short)65152;
        } else {
            pState->car_rc2[wi] += suspension_delta;
        }
        delta = old_rc2 - pState->car_rc2[wi] + suspension_delta;
    }

    return old_rc2 + delta;
}

/* ======================================================================== */
/* Helper: zero-initialise a WheelState                                      */
/* ======================================================================== */
static WheelState make_state(short rc2 = 0, short rc4 = 0, short rc5 = 0,
                             int wi = 0)
{
    WheelState s;
    memset(&s, 0, sizeof(s));
    s.car_rc2[wi] = rc2;
    s.car_rc4[wi] = rc4;
    s.car_rc5[wi] = rc5;
    return s;
}

/* ======================================================================== */
/* Tests: update_rpm_from_speed                                              */
/* ======================================================================== */

TEST(UpdateRpmFromSpeed, ChangingGearKeepsCurrentRpm)
{
    /* changing_gear != 0: skip the (speed * gearratio) calculation,
     * keep whatever currpm was. */
    EXPECT_EQ(2000u, rpm_from_speed(2000, 1000, 32768, /*cg=*/1, 100));
}

TEST(UpdateRpmFromSpeed, ChangingGearStillClampsToIdle)
{
    /* Even when changing gear we skip the speed calc, but the idle clamp
     * still applies afterward. */
    EXPECT_EQ(500u, rpm_from_speed(100, 1000, 32768, /*cg=*/1, 500));
}

TEST(UpdateRpmFromSpeed, NotChangingGear_ResultAboveIdle)
{
    /* speed=65536, ratio=65536 → (65536*65536)>>16 = 65536 = 65536.
     * idle=1000. 65536 >= 1000 → return 65536. */
    EXPECT_EQ(65536u, rpm_from_speed(0, 65536, 65536, /*cg=*/0, 1000));
}

TEST(UpdateRpmFromSpeed, NotChangingGear_ResultBelowIdle)
{
    /* speed=1, ratio=1 → 0 >> 16 = 0. idle=500 → clamp to 500. */
    EXPECT_EQ(500u, rpm_from_speed(9999, 1, 1, /*cg=*/0, 500));
}

TEST(UpdateRpmFromSpeed, NotChangingGear_ResultEqualsIdle)
{
    /* speed=256, ratio=256 → 256*256=65536 >> 16 = 1. idle=1 → equals → return 1.*/
    EXPECT_EQ(1u, rpm_from_speed(999, 256, 256, /*cg=*/0, 1));
}

TEST(UpdateRpmFromSpeed, ZeroSpeed_ClampsToIdle)
{
    /* speed=0 → calculated=0, any positive idle wins. */
    EXPECT_EQ(800u, rpm_from_speed(0, 0, 65535, /*cg=*/0, 800));
}

TEST(UpdateRpmFromSpeed, ZeroGearRatio_ClampsToIdle)
{
    /* ratio=0 → calculated=0 regardless of speed. */
    EXPECT_EQ(400u, rpm_from_speed(0, 1000, 0, /*cg=*/0, 400));
}

TEST(UpdateRpmFromSpeed, MaxValues_NoOverflow)
{
    /* speed=65535, ratio=65535: need unsigned long multiply to avoid
     * overflow on 16-bit-int platforms.
     * 65535 * 65535 = 4294836225 → >> 16 = 65534 = 65534. idle=0 → 65534. */
    EXPECT_EQ(65534u, rpm_from_speed(0, 65535, 65535, /*cg=*/0, 0));
}

/* ======================================================================== */
/* Tests: carState_update_wheel_suspension — rc5 decay                       */
/* ======================================================================== */

TEST(WheelSuspension, Rc5DecayFromPositive)
{
    WheelState s = make_state(/*rc2=*/0, /*rc4=*/0, /*rc5=*/20);
    susp_update(&s, 0, 0);
    /* rc5 was 20 → 20-4 = 16 */
    EXPECT_EQ(16, s.car_rc5[0]);
}

TEST(WheelSuspension, Rc5DecayFromNegative)
{
    WheelState s = make_state(/*rc2=*/0, /*rc4=*/0, /*rc5=*/-20);
    susp_update(&s, 0, 0);
    /* rc5 was -20 → -20+4 = -16 */
    EXPECT_EQ(-16, s.car_rc5[0]);
}

TEST(WheelSuspension, Rc5DecayToZeroFromSmallPositive)
{
    /* rc5=2: 2-4 = -2 → clamps to 0 */
    WheelState s = make_state(/*rc2=*/0, /*rc4=*/0, /*rc5=*/2);
    susp_update(&s, 0, 0);
    EXPECT_EQ(0, s.car_rc5[0]);
}

TEST(WheelSuspension, Rc5AlreadyZero_NoChange)
{
    WheelState s = make_state(0, 0, 0);
    susp_update(&s, 0, 0);
    EXPECT_EQ(0, s.car_rc5[0]);
    EXPECT_EQ(0, s.car_rc2[0]);
}

/* ======================================================================== */
/* Tests: suspension_delta == 0 → decay rc2 toward rc5                                  */
/* ======================================================================== */

TEST(WheelSuspension, Arg2Zero_Rc2AboveRc5_DecaysBy0x80)
{
    /* rc2=512, rc5=0, suspension_delta=0 → rc2 decays to 384 */
    WheelState s = make_state(/*rc2=*/512, /*rc4=*/0, /*rc5=*/0);
    susp_update(&s, 0, 0);
    EXPECT_EQ(384, s.car_rc2[0]);
    /* delta = 512 - 384 = 128 → return = old+delta = 512+128 = 640 */
}

TEST(WheelSuspension, Arg2Zero_Rc2AboveRc5_ReturnValue)
{
    WheelState s = make_state(/*rc2=*/512, /*rc4=*/0, /*rc5=*/0);
    short ret = susp_update(&s, 0, 0);
    /* old=512, new=384, delta=128, return=640 */
    EXPECT_EQ((short)640, ret);
}

TEST(WheelSuspension, Arg2Zero_Rc2BelowRc5_RisesBy0x80_DeltaZero)
{
    /* rc2=0, rc5=512.  After rc5 decay: rc5=508.
     * rc2 < rc5: rc2 += 128 = 128. delta stays 0. return = 0. */
    WheelState s = make_state(/*rc2=*/0, /*rc4=*/0, /*rc5=*/512);
    short ret = susp_update(&s, 0, 0);
    EXPECT_EQ(128,    s.car_rc2[0]);
    EXPECT_EQ((short)508, s.car_rc5[0]);
    EXPECT_EQ(0, ret);
}

TEST(WheelSuspension, Arg2Zero_Rc2NearRc5_ClampsExactly)
{
    /* rc2=64, rc5=64 (both same). rc5 decays to 60 first.
     * Then rc2=64 > rc5=60: rc2 -= 128 = -64.
     * -64 < 60 → clamp to 60.
     * delta = old_rc2 - new_rc2 = 64 - 60 = 4.
     * return = old_rc2 + delta = 64 + 4 = 68 (same formula as Arg2Zero_Rc2AboveRc5_ReturnValue). */
    WheelState s = make_state(/*rc2=*/64, /*rc4=*/0, /*rc5=*/64);
    short ret = susp_update(&s, 0, 0);
    EXPECT_EQ(60, s.car_rc2[0]);
    EXPECT_EQ(68, ret);
}

/* ======================================================================== */
/* Tests: suspension_delta > 0 → compression                                            */
/* ======================================================================== */

TEST(WheelSuspension, Compression_SmallForce_AddsDirectly)
{
    /* rc2=0, suspension_delta=64 → rc2 += 64. rc4 reset to 0. return = old_rc2=0. */
    WheelState s = make_state(/*rc2=*/0, /*rc4=*/99, /*rc5=*/0);
    short ret = susp_update(&s, 64, 0);
    EXPECT_EQ(64, s.car_rc2[0]);
    EXPECT_EQ(0,    s.car_rc4[0]);
    EXPECT_EQ(0,    ret);
}

TEST(WheelSuspension, Compression_ForceAbove0xC0_CappedAt0xC0)
{
    /* suspension_delta=256 > 192: only 192 added */
    WheelState s = make_state(/*rc2=*/0, /*rc4=*/5, /*rc5=*/0);
    susp_update(&s, 256, 0);
    EXPECT_EQ(192, s.car_rc2[0]);
    EXPECT_EQ(0,    s.car_rc4[0]);
}

TEST(WheelSuspension, Compression_ClampAt0x180)
{
    /* rc2=256, suspension_delta=256 → rc2 += 192 = 448 → clamp to 384 */
    WheelState s = make_state(/*rc2=*/256, /*rc4=*/5, /*rc5=*/0);
    short ret = susp_update(&s, 256, 0);
    EXPECT_EQ(384, s.car_rc2[0]);
    EXPECT_EQ(0,     s.car_rc4[0]);
    /* delta=0, return = old_rc2 = 256 */
    EXPECT_EQ(256, ret);
}

/* ======================================================================== */
/* Tests: suspension_delta < 0 → extension                                              */
/* ======================================================================== */

TEST(WheelSuspension, Extension_Rc2LargerThanAbsArg2_ClampedToZero)
{
    /* rc2=256, suspension_delta=-80. -suspension_delta=80. rc2=256 > 80 → clamp suspension_delta=0.
     * Falls into suspension_delta==0 path: rc2 > rc5=0 → rc2 -= 128 → 128. */
    WheelState s = make_state(/*rc2=*/256, /*rc4=*/0, /*rc5=*/0);
    susp_update(&s, -80, 0);
    EXPECT_EQ(128, s.car_rc2[0]);
}

TEST(WheelSuspension, ExtensionSmall_FullForceApplied)
{
    /* rc2=16, suspension_delta=-80. -suspension_delta=80. rc2=16 <= 80 → suspension_delta stays.
     * rc2+suspension_delta = 16-80 = -64. (short)65248 = -288. -64 > -288 → full.
     * new_rc2 = -64.  delta = old-new+suspension_delta = 16-(-64)+(-80) = 0. */
    WheelState s = make_state(/*rc2=*/16, /*rc4=*/0, /*rc5=*/0);
    short ret = susp_update(&s, -80, 0);
    EXPECT_EQ((short)-64, s.car_rc2[0]);
    EXPECT_EQ(16, ret); /* old_rc2 + 0 */
}

TEST(WheelSuspension, ExtensionLarge_ForceIsDamped)
{
    /* rc2=0, suspension_delta=-400. rc2+suspension_delta=-400. -400 <= -288 → damped.
     * rc2 += (-400*3)>>2 = -300. -300 > -384 (65152) → no clamp.
     * delta = 0 - (-300) + (-400) = -100. return = 0 + (-100) = -100. */
    WheelState s = make_state(/*rc2=*/0, /*rc4=*/0, /*rc5=*/0);
    short ret = susp_update(&s, -400, 0);
    EXPECT_EQ(-300, s.car_rc2[0]);
    EXPECT_EQ(-100, ret);
}

TEST(WheelSuspension, ExtensionVeryLarge_ClampedAtFE80)
{
    /* rc2=0, suspension_delta=-700.  damped: rc2 += -525.  -525 < -384 → clamp to -384.
     * delta = 0 - (-384) + (-700) = 384 - 700 = -316. return = -316. */
    WheelState s = make_state(/*rc2=*/0, /*rc4=*/0, /*rc5=*/0);
    short ret = susp_update(&s, -700, 0);
    EXPECT_EQ((short)65152, s.car_rc2[0]); /* -384 == 65152 as signed short */
    EXPECT_EQ(-316, ret);
}

TEST(WheelSuspension, WheelIndexIsolation)
{
    /* Verify that wheel index 2 is updated independently; wheels 0,1,3 untouched. */
    WheelState s;
    memset(&s, 0, sizeof(s));
    s.car_rc2[2] = 128;
    s.car_rc4[2] = 7;
    susp_update(&s, 64, 2); /* compression into wheel 2 */
    EXPECT_EQ(0, s.car_rc2[0]);
    EXPECT_EQ(0, s.car_rc2[1]);
    EXPECT_EQ(192, s.car_rc2[2]); /* 128 + 64 */
    EXPECT_EQ(0, s.car_rc2[3]);
    EXPECT_EQ(0, s.car_rc4[2]); /* rc4 reset on compression */
}
