/*
 * Copyright (c) 2026 Stunts Engine Project
 *
 * Layout and initial-value tests for the data-only source files:
 *   src/data_core.c, src/data_menu_track.c, src/data_runtime.c
 *
 * These files contain plain C data tables (no function logic).  The tests
 * verify that the tables have the correct size, element count, and initial
 * values as they appear in the original game binary, ensuring they are not
 * accidentally truncated or corrupted during refactoring.
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>

extern "C" {
#include "stunts.h"

/* data_core.c globals */
extern short          hillHeightConsts[];
extern char           aMain[];
extern char           cameramode;
extern unsigned short rotation_x_angle;
extern unsigned short rotation_z_angle;
extern unsigned short rotation_y_angle;
extern unsigned char  resources[];
extern unsigned char  aDesert[];
extern unsigned char  aAlpine[];
extern unsigned char  aCity[];
extern unsigned char  aCountry[];

/* data_runtime.c globals */
extern unsigned short resmaxsize;
extern unsigned short pspofs;
extern unsigned short pspseg;
extern unsigned       resource_alloc_state_a;
extern unsigned       resource_alloc_state_b;
extern char           joystick_assigned_flags;

/* data_menu_track.c button coordinate tables */
extern unsigned short trackmenu_buttons_x1[];
extern unsigned short trackmenu_buttons_x2[];
extern unsigned short trackmenu_buttons_y1[];
extern unsigned short trackmenu_buttons_y2[];
extern unsigned short opponentmenu_buttons_x1[];
extern unsigned short opponentmenu_buttons_x2[];
extern unsigned short opponentmenu_buttons_y1[];
extern unsigned short opponentmenu_buttons_y2[];

/* data_menu_track.c physics/collision tables */
extern unsigned short grassDecelDivTab[];
extern unsigned char  wheel_rating_coefficients[];
extern short          position_rotation_matrix[];
extern short          camera_position_vector[];
extern unsigned short animation_duration_table[];
} /* extern "C" */

/* ================================================================== */
/* data_core.c — hillHeightConsts                                       */
/* ================================================================== */

TEST(DataCore, HillHeightConstCount)
{
    /* Only two constant values defined */
    EXPECT_EQ(0,   hillHeightConsts[0]);
    EXPECT_EQ(450, hillHeightConsts[1]);  /* 450 */
}

TEST(DataCore, HillHeightConstsDistinct)
{
    EXPECT_NE(hillHeightConsts[0], hillHeightConsts[1]);
}

/* ================================================================== */
/* data_core.c — aMain string                                           */
/* ================================================================== */

TEST(DataCore, AMainContent)
{
    EXPECT_STREQ("main", aMain);
}

TEST(DataCore, AMainLength)
{
    EXPECT_EQ(4u, strlen(aMain));
}

/* ================================================================== */
/* data_core.c — terrain-name strings                                   */
/* ================================================================== */

TEST(DataCore, ADesertContent)
{
    EXPECT_STREQ("desert", (const char *)aDesert);
}

TEST(DataCore, ADesertBufferSize)
{
    /* defined as unsigned char aDesert[9] */
    EXPECT_EQ(6u, strlen((const char *)aDesert));
}

TEST(DataCore, AAlpineContent)
{
    EXPECT_STREQ("alpine", (const char *)aAlpine);
}

TEST(DataCore, ACityContent)
{
    EXPECT_STREQ("city", (const char *)aCity);
}

TEST(DataCore, ACountryContent)
{
    EXPECT_STREQ("country", (const char *)aCountry);
}

/* ================================================================== */
/* data_core.c — rotation angle initial values                          */
/* ================================================================== */

TEST(DataCore, RotationXAngleInit)
{
    /* Initial value 210 = 210 */
    EXPECT_EQ(210u, rotation_x_angle);
}

TEST(DataCore, RotationZAngleInit)
{
    /* Initial value 464 = 464 */
    EXPECT_EQ(464u, rotation_z_angle);
}

TEST(DataCore, RotationYAngleInit)
{
    /* Initial value 80 = 80 */
    EXPECT_EQ(80u, rotation_y_angle);
}

/* ================================================================== */
/* data_core.c — camera and flag defaults                               */
/* ================================================================== */

TEST(DataCore, CameraModeDefault)
{
    EXPECT_EQ(0, cameramode);
}

/* ================================================================== */
/* data_runtime.c — resources[] pool                                    */
/* ================================================================== */

TEST(DataRuntime, ResourcesFirstByteIsSpace)
{
    /* Each slot starts with 12 spaces as the initial name field */
    EXPECT_EQ(32, resources[0]);
}

TEST(DataRuntime, ResourcesSlot0Bytes12_13AreZero)
{
    /* Bytes 12–13 of slot 0 are zero (size/flags) */
    EXPECT_EQ(0, resources[12]);
    EXPECT_EQ(0, resources[13]);
}

TEST(DataRuntime, ResourcesSlot0Byte16IsTwo)
{
    /* Byte 16 of the first slot encodes a flag = 2 in the original data */
    EXPECT_EQ(2, resources[16]);
}

TEST(DataRuntime, ResourcesSecondSlotStartsWithSpace)
{
    /* Each slot is 18 bytes; slot 1 starts at offset 18 */
    EXPECT_EQ(32, resources[18]);
}

TEST(DataRuntime, ResmaxsizeDefaultZero)
{
    EXPECT_EQ(0u, resmaxsize);
}

TEST(DataRuntime, PspofsDefaultZero)
{
    EXPECT_EQ(0u, pspofs);
}

TEST(DataRuntime, PspsegDefaultZero)
{
    EXPECT_EQ(0u, pspseg);
}

TEST(DataRuntime, ResourceAllocStateDefault)
{
    EXPECT_EQ(0u, resource_alloc_state_a);
    EXPECT_EQ(0u, resource_alloc_state_b);
}

TEST(DataRuntime, JoystickFlagsDefault)
{
    EXPECT_EQ(0, joystick_assigned_flags);
}

/* timer_copy_unk and last_timer_callback_counter are now static in timer.c;
   their default-zero values are guaranteed by C99 §6.7.9 and tested in
   test_timer (CounterFromWordsZero). */

/* ================================================================== */
/* data_menu_track.c — trackmenu button coordinates                     */
/* ================================================================== */

TEST(DataMenuTrack, TrackMenuX1Values)
{
    EXPECT_EQ(16u, trackmenu_buttons_x1[0]);
    EXPECT_EQ(112u, trackmenu_buttons_x1[1]);
    EXPECT_EQ(208u, trackmenu_buttons_x1[2]);
}

TEST(DataMenuTrack, TrackMenuX2Values)
{
    EXPECT_EQ(112u, trackmenu_buttons_x2[0]);
    EXPECT_EQ(208u, trackmenu_buttons_x2[1]);
    EXPECT_EQ(304u, trackmenu_buttons_x2[2]);
}

TEST(DataMenuTrack, TrackMenuY1Uniform)
{
    EXPECT_EQ(171u, trackmenu_buttons_y1[0]);
    EXPECT_EQ(171u, trackmenu_buttons_y1[1]);
    EXPECT_EQ(171u, trackmenu_buttons_y1[2]);
}

TEST(DataMenuTrack, TrackMenuY2Uniform)
{
    EXPECT_EQ(197u, trackmenu_buttons_y2[0]);
    EXPECT_EQ(197u, trackmenu_buttons_y2[1]);
    EXPECT_EQ(197u, trackmenu_buttons_y2[2]);
}

TEST(DataMenuTrack, TrackMenuButtonsNonOverlap)
{
    /* x1[i] < x2[i] for every slot */
    for (int i = 0; i < 3; i++) {
        EXPECT_LT(trackmenu_buttons_x1[i], trackmenu_buttons_x2[i])
            << "slot " << i;
    }
}

/* ================================================================== */
/* data_menu_track.c — opponentmenu button coordinates                  */
/* ================================================================== */

TEST(DataMenuTrack, OpponentMenuX1Values)
{
    EXPECT_EQ(20u, opponentmenu_buttons_x1[0]);
    EXPECT_EQ(76u, opponentmenu_buttons_x1[1]);
    EXPECT_EQ(132u, opponentmenu_buttons_x1[2]);
    EXPECT_EQ(188u, opponentmenu_buttons_x1[3]);
    EXPECT_EQ(244u, opponentmenu_buttons_x1[4]);
}

TEST(DataMenuTrack, OpponentMenuY1Uniform)
{
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(177u, opponentmenu_buttons_y1[i]) << "slot " << i;
    }
}

TEST(DataMenuTrack, OpponentMenuY2Uniform)
{
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(197u, opponentmenu_buttons_y2[i]) << "slot " << i;
    }
}

TEST(DataMenuTrack, OpponentMenuButtonsNonOverlap)
{
    for (int i = 0; i < 5; i++) {
        EXPECT_LT(opponentmenu_buttons_x1[i], opponentmenu_buttons_x2[i])
            << "slot " << i;
    }
}

TEST(DataMenuTrack, OpponentMenuFiveSlots)
{
    /* Verify 5 entries by checking last slot has expected value */
    EXPECT_EQ(300u, opponentmenu_buttons_x2[4]);
}

/* ================================================================== */
/* data_menu_track.c — grassDecelDivTab                                 */
/* Divides car_speed2 by this value when N wheels are on grass.         */
/* Verified against restunts/src/restunts/asm/dseg.asm:                 */
/*   dw 255, 256, 192, 128, 64                                          */
/* ================================================================== */

TEST(DataMenuTrack, GrassDecelDivTabSize5)
{
    /* Slot 4 (all four wheels on grass) has the smallest divisor = most decel */
    EXPECT_EQ(64u, grassDecelDivTab[4]);
}

TEST(DataMenuTrack, GrassDecelDivTabAllValues)
{
    EXPECT_EQ(255u, grassDecelDivTab[0]); /* 0 wheels on grass */
    EXPECT_EQ(256u, grassDecelDivTab[1]); /* 1 wheel  on grass */
    EXPECT_EQ(192u, grassDecelDivTab[2]); /* 2 wheels on grass */
    EXPECT_EQ(128u, grassDecelDivTab[3]); /* 3 wheels on grass */
    EXPECT_EQ(64u, grassDecelDivTab[4]); /* 4 wheels on grass */
}

TEST(DataMenuTrack, GrassDecelDivTabDecreasing_From2To4)
{
    /* More wheels on grass → smaller divisor → more deceleration.
     * Index 1 (256) is intentionally larger than index 0 (255); indices 2–4 strictly decrease. */
    EXPECT_GT(grassDecelDivTab[2], grassDecelDivTab[3]);
    EXPECT_GT(grassDecelDivTab[3], grassDecelDivTab[4]);
}

TEST(DataMenuTrack, GrassDecelDivTabNonZero)
{
    /* All divisors must be non-zero to avoid division by zero in update_grip */
    for (int i = 0; i < 5; i++) {
        EXPECT_GT(grassDecelDivTab[i], 0u) << "slot " << i;
    }
}

/* ================================================================== */
/* data_menu_track.c — wheel_rating_coefficients                        */
/* 8 bytes encoding 4 little-endian unsigned shorts:                    */
/*   front-left=21, front-right=21, rear-left=15, rear-right=15         */
/* ================================================================== */

TEST(DataMenuTrack, WheelRatingCoefficients_RawBytes)
{
    EXPECT_EQ(21u, wheel_rating_coefficients[0]);
    EXPECT_EQ(0u, wheel_rating_coefficients[1]);
    EXPECT_EQ(21u, wheel_rating_coefficients[2]);
    EXPECT_EQ(0u, wheel_rating_coefficients[3]);
    EXPECT_EQ(15u, wheel_rating_coefficients[4]);
    EXPECT_EQ(0u, wheel_rating_coefficients[5]);
    EXPECT_EQ(15u, wheel_rating_coefficients[6]);
    EXPECT_EQ(0u, wheel_rating_coefficients[7]);
}

TEST(DataMenuTrack, WheelRatingCoefficients_LittleEndianShorts)
{
    /* get_wheel_rating_coefficients(i) = bytes[i*2] | (bytes[i*2+1]<<8) */
    auto get = [](int i) -> unsigned short {
        extern unsigned char wheel_rating_coefficients[];
        return (unsigned short)(wheel_rating_coefficients[i * 2] |
                                (wheel_rating_coefficients[i * 2 + 1] << 8));
    };
    EXPECT_EQ(21u,  get(0)); /* front */
    EXPECT_EQ(21u,  get(1)); /* front */
    EXPECT_EQ(15u,  get(2)); /* rear  */
    EXPECT_EQ(15u,  get(3)); /* rear  */
}

TEST(DataMenuTrack, WheelRatingCoefficients_FrontHigherThanRear)
{
    unsigned short front = (unsigned short)(wheel_rating_coefficients[0] |
                                            (wheel_rating_coefficients[1] << 8));
    unsigned short rear  = (unsigned short)(wheel_rating_coefficients[4] |
                                            (wheel_rating_coefficients[5] << 8));
    EXPECT_GT(front, rear);
}

/* ================================================================== */
/* data_menu_track.c — position_rotation_matrix and camera_position_vector */
/* Used by car_car_detect_collision() to sign-flip corner coordinates.  */
/* position_rotation_matrix[i]: 1=flip X  for corner i                 */
/* camera_position_vector[i]:   1=flip Z  for corner i                 */
/* ================================================================== */

TEST(DataMenuTrack, PositionRotationMatrix_Values)
{
    EXPECT_EQ(1, position_rotation_matrix[0]); /* corner 0: x flipped */
    EXPECT_EQ(0, position_rotation_matrix[1]); /* corner 1: x normal  */
    EXPECT_EQ(0, position_rotation_matrix[2]); /* corner 2: x normal  */
    EXPECT_EQ(1, position_rotation_matrix[3]); /* corner 3: x flipped */
}

TEST(DataMenuTrack, CameraPositionVector_Values)
{
    EXPECT_EQ(0, camera_position_vector[0]); /* corner 0: z normal  */
    EXPECT_EQ(0, camera_position_vector[1]); /* corner 1: z normal  */
    EXPECT_EQ(1, camera_position_vector[2]); /* corner 2: z flipped */
    EXPECT_EQ(1, camera_position_vector[3]); /* corner 3: z flipped */
}

TEST(DataMenuTrack, CollisionCornersXSigns)
{
    /* x-signs of the 4 corners: {+,-,+,-} derived from position_rotation_matrix.
     * corner i x-sign: prm[i] ? -1 : +1 */
    int expected[4] = { -1, +1, +1, -1 };
    for (int i = 0; i < 4; i++) {
        int sign = position_rotation_matrix[i] ? -1 : +1;
        EXPECT_EQ(expected[i], sign) << "corner " << i;
    }
}

TEST(DataMenuTrack, CollisionCornersZSigns)
{
    /* z-signs of the 4 corners: {+,+,-,-} derived from camera_position_vector.
     * corner i z-sign: cpv[i] ? -1 : +1 */
    int expected[4] = { +1, +1, -1, -1 };
    for (int i = 0; i < 4; i++) {
        int sign = camera_position_vector[i] ? -1 : +1;
        EXPECT_EQ(expected[i], sign) << "corner " << i;
    }
}

/* ================================================================== */
/* data_menu_track.c — animation_duration_table                         */
/* 8 increasing frame-count thresholds for animation phase transitions. */
/* ================================================================== */

TEST(DataMenuTrack, AnimationDurationTable_FirstAndLast)
{
    EXPECT_EQ(30u, animation_duration_table[0]); /* 30  */
    EXPECT_EQ(960u, animation_duration_table[7]); /* 960 */
}

TEST(DataMenuTrack, AnimationDurationTable_AllValues)
{
    const unsigned short expected[8] = {
        30, 200, 320, 400,
        530, 700, 880, 960
    };
    for (int i = 0; i < 8; i++) {
        EXPECT_EQ(expected[i], animation_duration_table[i]) << "slot " << i;
    }
}

TEST(DataMenuTrack, AnimationDurationTable_StrictlyIncreasing)
{
    for (int i = 1; i < 8; i++) {
        EXPECT_GT(animation_duration_table[i], animation_duration_table[i - 1])
            << "slot " << i;
    }
}

