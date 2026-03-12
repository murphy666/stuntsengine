/*
 * test_sdl_audio_tone.cc — GTest version of test_sdl_audio_tone.c
 *
 * Synthesises a short 440 Hz sine tone and queues it to SDL audio.
 * GTEST_SKIP()s if SDL2 is unavailable.
 */

#include <gtest/gtest.h>
#include <SDL2/SDL.h>
#include <cmath>
#include <cstdint>
#include <vector>
#include <cstring>

static constexpr int    SAMPLE_RATE  = 44100;
static constexpr double FREQ_HZ      = 440.0;
static constexpr double DURATION_SEC = 0.05;
static constexpr int    NUM_SAMPLES  = static_cast<int>(SAMPLE_RATE * DURATION_SEC);

class SdlAudioTest : public ::testing::Test {
protected:
    SDL_AudioDeviceID dev = 0;
    SDL_AudioSpec obtained = {};

    void SetUp() override {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            GTEST_SKIP() << "SDL_Init(AUDIO) failed: " << SDL_GetError();
        }
        SDL_AudioSpec want = {};
        want.freq     = SAMPLE_RATE;
        want.format   = AUDIO_S16SYS;
        want.channels = 1;
        want.samples  = 512;
        want.callback = nullptr;
        dev = SDL_OpenAudioDevice(nullptr, 0, &want, &obtained, 0);
        if (!dev) {
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
            GTEST_SKIP() << "SDL_OpenAudioDevice failed: " << SDL_GetError();
        }
    }
    void TearDown() override {
        if (dev) {
            SDL_ClearQueuedAudio(dev);
            SDL_CloseAudioDevice(dev);
            dev = 0;
        }
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
};

TEST_F(SdlAudioTest, OpenDevice) {
    EXPECT_NE(dev, (SDL_AudioDeviceID)0)           << "audio device opened";
    EXPECT_EQ(obtained.channels, (uint8_t)1)       << "mono";
    EXPECT_EQ(obtained.freq, SAMPLE_RATE)          << "sample rate";
    EXPECT_EQ(obtained.format, (SDL_AudioFormat)AUDIO_S16SYS) << "16-bit";
}

TEST_F(SdlAudioTest, QueueAndPlayTone) {
    // Synthesise a short 440 Hz tone
    std::vector<int16_t> buf(NUM_SAMPLES);
    for (int i = 0; i < NUM_SAMPLES; i++) {
        double t   = static_cast<double>(i) / SAMPLE_RATE;
        double val = sin(2.0 * M_PI * FREQ_HZ * t);
        buf[i] = static_cast<int16_t>(val * 16384);
    }

    int rc = SDL_QueueAudio(dev, buf.data(), static_cast<uint32_t>(buf.size() * sizeof(int16_t)));
    EXPECT_EQ(rc, 0) << "SDL_QueueAudio: " << SDL_GetError();

    uint32_t queued = SDL_GetQueuedAudioSize(dev);
    EXPECT_GT(queued, 0u) << "queued bytes > 0 after queue";

    SDL_PauseAudioDevice(dev, 0);

    // Drain the queue
    int wait_ms = 0;
    while (SDL_GetQueuedAudioSize(dev) > 0 && wait_ms < 300) {
        SDL_Delay(5);
        wait_ms += 5;
    }
    // We don't assert on drain (CI may have null audio driver)
    SUCCEED();
}
