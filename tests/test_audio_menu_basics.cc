/*
 * Focused menu-audio diagnostics:
 * - validate the shipped title music resource exposes the expected HDR1 layout
 * - validate a real tempo byte is present and derives a sane menu music rate
 */

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

uint32_t read_u32_le(const uint8_t *ptr) {
    return static_cast<uint32_t>(ptr[0]) |
           (static_cast<uint32_t>(ptr[1]) << 8u) |
           (static_cast<uint32_t>(ptr[2]) << 16u) |
           (static_cast<uint32_t>(ptr[3]) << 24u);
}

std::vector<uint8_t> read_file_all(const char *path) {
    std::vector<uint8_t> data;
    FILE *fp = std::fopen(path, "rb");
    if (!fp) {
        return data;
    }
    std::fseek(fp, 0, SEEK_END);
    long size = std::ftell(fp);
    std::fseek(fp, 0, SEEK_SET);
    if (size <= 0) {
        std::fclose(fp);
        return data;
    }
    data.resize(static_cast<size_t>(size));
    if (std::fread(data.data(), 1, data.size(), fp) != data.size()) {
        data.clear();
    }
    std::fclose(fp);
    return data;
}

bool chunk_name_eq4(const uint8_t *lhs, const char *rhs) {
    for (int i = 0; i < 4; i++) {
        char l = static_cast<char>(lhs[i]);
        char r = rhs[i];
        if (l >= 'a' && l <= 'z') {
            l = static_cast<char>(l - 'a' + 'A');
        }
        if (r >= 'a' && r <= 'z') {
            r = static_cast<char>(r - 'a' + 'A');
        }
        if (l != r) {
            return false;
        }
    }
    return true;
}

bool get_chunk_ci(const uint8_t *container, size_t container_size, const char *name4,
                  const uint8_t **out_chunk, unsigned int *out_size) {
    uint32_t total_size;
    unsigned int count;
    unsigned int names_base;
    unsigned int offs_base;
    unsigned int data_base;

    *out_chunk = nullptr;
    *out_size = 0u;
    if (container == nullptr || container_size < 14u) {
        return false;
    }

    total_size = read_u32_le(container);
    if (total_size > container_size) {
        total_size = static_cast<uint32_t>(container_size);
    }
    count = static_cast<unsigned int>(container[4]) |
            (static_cast<unsigned int>(container[5]) << 8u);
    names_base = 6u;
    offs_base = names_base + count * 4u;
    data_base = offs_base + count * 4u;
    if (count == 0u || data_base >= total_size) {
        return false;
    }

    for (unsigned int i = 0u; i < count; i++) {
        if (!chunk_name_eq4(container + names_base + i * 4u, name4)) {
            continue;
        }
        unsigned int rel = read_u32_le(container + offs_base + i * 4u);
        const uint8_t *chunk;
        unsigned int chunk_size;

        if (data_base + rel + 4u > total_size) {
            return false;
        }
        chunk = container + data_base + rel;
        chunk_size = read_u32_le(chunk);
        if (chunk_size < 4u || data_base + rel + chunk_size > total_size) {
            return false;
        }
        *out_chunk = chunk;
        *out_size = chunk_size;
        return true;
    }
    return false;
}

bool find_first_tempo_param(const uint8_t *track_chunk, unsigned int track_size,
                            unsigned int *out_tempo) {
    size_t pos;
    size_t end;

    if (track_chunk == nullptr || track_size <= 7u || track_chunk[5] != 231u) {
        return false;
    }
    pos = 7u + static_cast<size_t>(track_chunk[6]);
    end = track_size;

    while (pos < end) {
        uint32_t delta = 0u;
        while (pos < end) {
            uint8_t b = track_chunk[pos++];
            delta = (delta << 7u) | static_cast<uint32_t>(b & 127u);
            if ((b & 128u) == 0u) {
                break;
            }
        }
        (void)delta;
        if (pos >= end) {
            break;
        }

        uint8_t event = track_chunk[pos++];
        if (event == 221u && pos < end) {
            *out_tempo = static_cast<unsigned int>(track_chunk[pos]);
            return true;
        }
        if (event >= 217u && event <= 234u) {
            static constexpr std::array<signed char, 18> cmd_extra = {
                0, 0, 0, 1, 1, 1, 2, 1, 1, 1, 0, 1, 2, 5, -1, -1, 1, 1
            };
            signed char extra = cmd_extra[static_cast<size_t>(event - 217u)];
            if (extra < 0 || pos + static_cast<size_t>(extra) > end) {
                break;
            }
            pos += static_cast<size_t>(extra);
            continue;
        }
        if (event > 128u && pos < end) {
            pos++;
        }
        while (pos < end) {
            uint8_t b = track_chunk[pos++];
            if ((b & 128u) == 0u) {
                break;
            }
        }
    }
    return false;
}

std::vector<uint8_t> load_title_song() {
    static const char *candidates[] = {
        "build_cmake/SKIDTITL.KMS",
        "ressources/SKIDTITL.KMS",
        "SKIDTITL.KMS",
        nullptr
    };

    for (int i = 0; candidates[i] != nullptr; i++) {
        std::vector<uint8_t> data = read_file_all(candidates[i]);
        if (!data.empty()) {
            return data;
        }
    }
    return {};
}

}  // namespace

TEST(AudioMenuBasics, TitleHdr1LayoutLooksSane) {
    std::vector<uint8_t> song = load_title_song();
    const uint8_t *title_chunk = nullptr;
    const uint8_t *hdr1_chunk = nullptr;
    unsigned int title_size = 0u;
    unsigned int hdr1_size = 0u;

    if (song.empty()) {
        GTEST_SKIP() << "SKIDTITL.KMS not available";
    }

    ASSERT_TRUE(get_chunk_ci(song.data(), song.size(), "TITL", &title_chunk, &title_size));
    ASSERT_TRUE(get_chunk_ci(title_chunk, title_size, "HDR1", &hdr1_chunk, &hdr1_size));
    ASSERT_GE(hdr1_size, 16u);

    unsigned int instrument_count = static_cast<unsigned int>(hdr1_chunk[6]);
    unsigned int track_count = static_cast<unsigned int>(hdr1_chunk[7u + instrument_count * 4u]);
    EXPECT_EQ(5u, instrument_count);
    EXPECT_EQ(6u, track_count);
}

TEST(AudioMenuBasics, TitleTempoDerivesExpectedMenuRate) {
    std::vector<uint8_t> song = load_title_song();
    const uint8_t *title_chunk = nullptr;
    const uint8_t *hdr1_chunk = nullptr;
    unsigned int title_size = 0u;
    unsigned int hdr1_size = 0u;
    unsigned int instrument_count;
    unsigned int track_count;
    unsigned int track_pos;
    bool found_tempo = false;
    unsigned int tempo = 0u;

    if (song.empty()) {
        GTEST_SKIP() << "SKIDTITL.KMS not available";
    }

    ASSERT_TRUE(get_chunk_ci(song.data(), song.size(), "TITL", &title_chunk, &title_size));
    ASSERT_TRUE(get_chunk_ci(title_chunk, title_size, "HDR1", &hdr1_chunk, &hdr1_size));

    instrument_count = static_cast<unsigned int>(hdr1_chunk[6]);
    track_pos = 7u + instrument_count * 4u;
    track_count = static_cast<unsigned int>(hdr1_chunk[track_pos++]);

    for (unsigned int i = 0u; i < track_count && !found_tempo; i++, track_pos += 5u) {
        char track_name[5];
        const uint8_t *track_chunk = nullptr;
        unsigned int track_size = 0u;

        std::memcpy(track_name, hdr1_chunk + track_pos, 4u);
        track_name[4] = '\0';
        if (!get_chunk_ci(title_chunk, title_size, track_name, &track_chunk, &track_size)) {
            continue;
        }
        found_tempo = find_first_tempo_param(track_chunk, track_size, &tempo);
    }

    ASSERT_TRUE(found_tempo);
    EXPECT_EQ(120u, tempo);
    EXPECT_EQ(48u, tempo * 2u / 5u);
}