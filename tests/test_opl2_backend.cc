/*
 * Black-box tests for the OPL wrapper configuration.
 * These verify that the bundled Nuked OPL backend produces a sane pitch
 * through our wrapper at both the native chip rate and a resampled rate.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

extern "C" {
#include "opl2.h"
}

namespace {

constexpr int kChannel0 = 0;
constexpr int kA4Fnum = 580;
constexpr int kA4Block = 4;
constexpr double kExpectedHz = 440.0;

void program_simple_tone_voice() {
    /* Ch0 modulator slot = 0, carrier slot = 3. Use additive mode and silence
     * the modulator TL so the carrier dominates with a near-sine tone. */
    opl2_write(0x20, 0x21);
    opl2_write(0x23, 0x21);
    opl2_write(0x40, 0x3f);
    opl2_write(0x43, 0x00);
    opl2_write(0x60, 0xf0);
    opl2_write(0x63, 0xf0);
    opl2_write(0x80, 0x00);
    opl2_write(0x83, 0x00);
    opl2_write(0xc0, 0x01);
    opl2_write(0xe0, 0x00);
    opl2_write(0xe3, 0x00);
}

void key_on_a4() {
    opl2_write(0xa0 + kChannel0, kA4Fnum & 0xff);
    opl2_write(0xb0 + kChannel0,
               0x20 | ((kA4Block & 0x07) << 2) | ((kA4Fnum >> 8) & 0x03));
}

double estimate_frequency_hz(const std::vector<short> &samples, int sample_rate) {
    std::vector<int> crossing_indices;

    for (size_t i = 1; i < samples.size(); i++) {
        if (samples[i - 1] <= 0 && samples[i] > 0) {
            crossing_indices.push_back(static_cast<int>(i));
        }
    }
    if (crossing_indices.size() < 4u) {
        return 0.0;
    }

    double total_period = 0.0;
    int periods = 0;
    for (size_t i = 1; i < crossing_indices.size(); i++) {
        total_period += static_cast<double>(crossing_indices[i] - crossing_indices[i - 1]);
        periods++;
    }
    if (periods == 0 || total_period <= 0.0) {
        return 0.0;
    }
    return static_cast<double>(sample_rate) / (total_period / static_cast<double>(periods));
}

double render_and_measure_frequency(int sample_rate) {
    std::vector<short> samples(static_cast<size_t>(sample_rate / 5), 0);

    opl2_init(sample_rate);
    EXPECT_TRUE(opl2_is_ready());
    opl2_reset();
    program_simple_tone_voice();
    key_on_a4();
    opl2_generate(samples.data(), static_cast<int>(samples.size()));
    opl2_destroy();

    /* Skip the initial transient and estimate from the stable middle portion. */
    const size_t begin = samples.size() / 4u;
    const size_t end = samples.size() * 9u / 10u;
    std::vector<short> window(samples.begin() + static_cast<std::ptrdiff_t>(begin),
                              samples.begin() + static_cast<std::ptrdiff_t>(end));
    return estimate_frequency_hz(window, sample_rate);
}

}  // namespace

TEST(Opl2Backend, ProducesExpectedA4AtNativeRate) {
    double hz = render_and_measure_frequency(49716);
    ASSERT_GT(hz, 1.0);
    EXPECT_NEAR(hz, kExpectedHz, 12.0);
}

TEST(Opl2Backend, ProducesExpectedA4At22050) {
    double hz = render_and_measure_frequency(22050);
    ASSERT_GT(hz, 1.0);
    EXPECT_NEAR(hz, kExpectedHz, 12.0);
}

TEST(Opl2Backend, PitchIsStableAcrossConfiguredRates) {
    double native_hz = render_and_measure_frequency(49716);
    double resampled_hz = render_and_measure_frequency(22050);

    ASSERT_GT(native_hz, 1.0);
    ASSERT_GT(resampled_hz, 1.0);
    EXPECT_NEAR(native_hz, resampled_hz, 8.0);
}