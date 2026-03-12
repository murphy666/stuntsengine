/*
 * Copyright (c) 2026 Stunts Engine Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "stunts.h"
#include "ressources.h"
#include "timer.h"
#include "game_timing.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <SDL2/SDL.h>
#include "opl2.h"

#define AUDIO_MAX_HANDLES 25
#define AUDIO_INVALID_CHUNK ((short)-1)
#define AUDIO_FX_TICKS_PERSIST 255u
#define AUDIO_CMD_QUEUE_SIZE 256
#define AUDIO_DMA_RING_SAMPLES 16384
#define AUDIO_MENU_RESOURCE_NOTES_MAX 512
#define AUDIO_VCE_MAX_INSTRUMENTS 64

typedef enum AUDIO_SB_CMD {
    AUDIO_SB_CMD_NONE = 0,
    AUDIO_SB_CMD_SPEAKER_ON,
    AUDIO_SB_CMD_SPEAKER_OFF,
    AUDIO_SB_CMD_SET_TIME_CONSTANT,
    AUDIO_SB_CMD_START_DMA,
    AUDIO_SB_CMD_STOP_DMA,
    AUDIO_SB_CMD_NOTE_ON,
    AUDIO_SB_CMD_NOTE_OFF,
    AUDIO_SB_CMD_SET_VOLUME,
    AUDIO_SB_CMD_SET_PITCH,
    AUDIO_SB_CMD_SELECT_CHUNK,
    AUDIO_SB_CMD_SELECT_SKID_CHUNK
} AUDIO_SB_CMD;

typedef struct AUDIO_SB_COMMAND {
    unsigned char cmd;
    short handle_id;
    short value0;
    short value1;
} AUDIO_SB_COMMAND;

typedef struct AUDIO_SB_DSP_STATE {
    unsigned char speaker_on;
    unsigned char dma_running;
    unsigned char pending_cmd;
    unsigned char pending_bytes_expected;
    unsigned char pending_bytes_count;
    unsigned char pending_data[2];
    unsigned short dma_block_len;
    unsigned short sample_rate_hz;
    unsigned short time_constant;
    unsigned char dma_auto_init;
    unsigned int dma_bytes_remaining;
    unsigned int sample_gen_accum;
} AUDIO_SB_DSP_STATE;

typedef struct AUDIO_VCE_INSTRUMENT {
    unsigned char valid;
    unsigned char has_fm;          /* 1 = full OPL2 FM params valid (ADENG1.VCE) */
    char name[5];
    unsigned short record_size;
    /* Frequency mapping (rec[14/15]): Fnum = RPM / freq_div + freq_base * 16 */
    unsigned char freq_div;
    unsigned char freq_base;
    /* OPL2 channel regs (rec[68..69]) — from AD15.DRV disassembly */
    unsigned char opl_con;         /* connection: 0=FM, 1=AM (additive) */
    unsigned char opl_fb;          /* operator feedback: 0-7 */
    /* Op0 (modulator) 12 fields at rec[70]: AR,DR,SL,RR,TL,KSL,MULT,KSR,EGT,VIB,AM,WS */
    unsigned char op0[12];
    /* Op1 (carrier)   12 fields at rec[82]: same layout */
    unsigned char op1[12];
    /* Legacy fields */
    unsigned char brightness;
} AUDIO_VCE_INSTRUMENT;

typedef struct AUDIO_HANDLE_STUB {
    unsigned char allocated;
    unsigned char playing;
    unsigned char dirty;
    unsigned char pending_restart;
    short init_mode;
    short volume;
    short last_volume;
    short rpm;
    short current_pitch;
    short target_pitch;
    short active_chunk;       /* crash SFX slot — original [si+14h] */
    short active_skid_chunk;    /* skid SFX slot  — original [si+16h] */
    short chunk_alt1;
    short chunk_alt2;
    short chunk_crash1;
    short chunk_crash2;
    unsigned short engine_active_ticks;
    uint32_t phase_accum;
    uint32_t skid_phase_accum; /* independent PRNG state for skid slot */
    void * data_ptr;
    void * song_res;
    void * voice_res;
    /* Frequency mapping from VCE instrument rec[14]/rec[15].
     * Fnum = rpm / freq_div + freq_base * 16  (default: div=6 for PCENG1 ENGI) */
    unsigned char freq_div;
    unsigned char freq_base;
    /* OPL2 FM synthesis state */
    int            opl2_channel;    /* -1 = not assigned, 0-8 = OPL2 channel */
    unsigned char  opl2_keyon;      /* current key-on state on this OPL2 channel */
    unsigned char  opl2_programmed; /* 1 = FM registers written at least once */
    unsigned char  has_engi_inst;   /* 1 = engi_inst below is valid */
    AUDIO_VCE_INSTRUMENT engi_inst; /* cached ENGI instrument from this handle's VCE */
    /* OPL2 SFX channels for tire squeal and crash (driven by VCE instruments, not noise) */
    int            opl2_skid_channel;   /* OPL2 channel for tire squeal, -1 = none */
    int            opl2_crash_channel;  /* OPL2 channel for crash SFX, -1 = none */
    unsigned char  opl2_skid_keyon;
    unsigned char  opl2_crash_keyon;
    unsigned char  has_skid_inst;   /* 1 = skid_inst valid (VCE "SKID") */
    unsigned char  has_scra_inst;   /* 1 = scra_inst valid (VCE "SCRA") */
    unsigned char  has_cras_inst;   /* 1 = cras_inst valid (VCE "CRAS") */
    unsigned char  has_blow_inst;   /* 1 = blow_inst valid (VCE "BLOW") */
    unsigned char  has_bump_inst;   /* 1 = bump_inst valid (VCE "BUMP") */
    AUDIO_VCE_INSTRUMENT skid_inst; /* tire squeal */
    AUDIO_VCE_INSTRUMENT scra_inst; /* tire scrape */
    AUDIO_VCE_INSTRUMENT cras_inst; /* crash boom */
    AUDIO_VCE_INSTRUMENT blow_inst; /* blowout */
    AUDIO_VCE_INSTRUMENT bump_inst; /* bump */
} AUDIO_HANDLE_STUB;

static AUDIO_HANDLE_STUB g_audio_handles[AUDIO_MAX_HANDLES];
static unsigned char g_audio_driver_loaded = 0;
static unsigned char g_audio_driver_timer_registered = 0;
static unsigned char g_audio_flag2_enabled = 1;
static unsigned char g_audio_flag6_enabled = 1;
static AUDIO_SB_DSP_STATE g_audio_dsp;
static AUDIO_SB_COMMAND g_audio_cmd_queue[AUDIO_CMD_QUEUE_SIZE];
static unsigned short g_audio_cmd_write_idx = 0;
static unsigned short g_audio_cmd_read_idx = 0;
static unsigned char g_audio_chunk_fx_ticks[AUDIO_MAX_HANDLES];
static unsigned char g_audio_crash_fx_ticks[AUDIO_MAX_HANDLES];
static unsigned char g_audio_crash_fx_kind[AUDIO_MAX_HANDLES];
static int g_audio_chunk_noise_state[AUDIO_MAX_HANDLES];
static int g_audio_chunk_noise_target[AUDIO_MAX_HANDLES];
static unsigned short g_audio_chunk_noise_hold[AUDIO_MAX_HANDLES];
/* Skid-specific noise state (separate from crash noise) */
static int g_audio_skid_noise_state[AUDIO_MAX_HANDLES];
static int g_audio_skid_noise_target[AUDIO_MAX_HANDLES];
static unsigned short g_audio_skid_noise_hold[AUDIO_MAX_HANDLES];
static unsigned char g_audio_skid_fx_ticks[AUDIO_MAX_HANDLES];
static int16_t g_audio_dma_ring[AUDIO_DMA_RING_SAMPLES];
static unsigned int g_audio_dma_ring_write = 0;
static unsigned int g_audio_dma_ring_read = 0;
static unsigned int g_audio_phase_accum = 0;
static SDL_AudioDeviceID g_audio_sdl_dev = 0;
/* Per-channel OPL2 ownership: g_opl2_ch_owner[ch] = handle_id, or -1 if free */
static int g_opl2_ch_owner[9] = { -1, -1, -1, -1, -1, -1, -1, -1, -1 };
/* Shared render buffer for OPL2 engine samples (all channels mixed by the chip) */
static short g_opl2_sample_buf[4096];
static unsigned int g_audio_output_rate_hz = 22050u;
static unsigned int g_audio_samples_generated = 0;
static unsigned int g_audio_samples_consumed = 0;
static unsigned int g_audio_samples_emitted = 0;
static unsigned int g_audio_refresh_hz = (unsigned int)GAME_DISPLAY_HZ;
static unsigned long g_audio_refresh_accum = 0u;
static uint32_t g_audio_music_phase = 0;
static unsigned int g_audio_music_tick_counter = 0;
static unsigned int g_audio_music_note_index = 0;
static unsigned char g_audio_menu_music_enabled = 0;
static unsigned char g_audio_menu_music_paused = 0;
static char g_audio_menu_music_name[5] = { 0, 0, 0, 0, 0 };
static int g_audio_menu_note_transpose = -12;
static unsigned int g_audio_menu_duration_scale = 1u;
static int g_audio_menu_gain = 9000;
static unsigned int g_audio_menu_note_ticks = 24u;
static unsigned char g_audio_menu_resource_notes[AUDIO_MENU_RESOURCE_NOTES_MAX];
static unsigned short g_audio_menu_resource_durations[AUDIO_MENU_RESOURCE_NOTES_MAX];
static unsigned char g_audio_menu_resource_instruments[AUDIO_MENU_RESOURCE_NOTES_MAX];
static unsigned char g_audio_menu_resource_velocities[AUDIO_MENU_RESOURCE_NOTES_MAX];
static unsigned short g_audio_menu_resource_count = 0;
static unsigned short g_audio_menu_resource_index = 0;
static unsigned short g_audio_menu_resource_ticks_left = 0;
static unsigned char g_audio_menu_resource_current_note = 60;
static unsigned char g_audio_menu_resource_current_instrument = 0;
static unsigned char g_audio_menu_resource_current_velocity = 96;
static unsigned char g_audio_menu_use_resource = 0;
static uint32_t g_audio_menu_phase2 = 0;
static int g_audio_menu_lp_state = 0;
static uint32_t g_audio_menu_mod_phase = 0;
static unsigned short g_audio_menu_resource_ticks_total = 0;
static unsigned short g_audio_menu_resource_tick_pos = 0;
static AUDIO_VCE_INSTRUMENT g_audio_vce_instruments[AUDIO_VCE_MAX_INSTRUMENTS];
static unsigned short g_audio_vce_instrument_count = 0;
static unsigned char g_audio_last_init_is_menu = 0;

#define AUDIO_SFX_MAX_CHUNKS 32
static unsigned short g_audio_sfx_chunk_count = 0;
static char g_audio_sfx_chunk_name[AUDIO_SFX_MAX_CHUNKS][5];
static unsigned short g_audio_sfx_chunk_ofs[AUDIO_SFX_MAX_CHUNKS];
static unsigned short g_audio_sfx_chunk_end[AUDIO_SFX_MAX_CHUNKS];
static unsigned char g_audio_sfx_chunk_duration[AUDIO_SFX_MAX_CHUNKS]; /* KMS bytecode ticks */

static int audio_is_valid_handle(short handle_id);
static int audio_str_ieq(const char *a, const char *b);
static unsigned int audio_note_to_hz(unsigned char note);
static unsigned int audio_read_u32_le(const unsigned char *ptr);
static unsigned short audio_extract_menu_resource_notes(void *songptr);
static unsigned short audio_parse_vce_instruments(void *voiceptr);
/** @brief Program an OPL2 channel with instrument operator settings. */
static void audio_opl2_program_channel(int ch, const AUDIO_VCE_INSTRUMENT *ins);
/** @brief Apply channel volume to OPL2 carrier/modulator operators. */
static void audio_opl2_set_volume_ch(int ch, int volume, const AUDIO_VCE_INSTRUMENT *ins);
static void audio_opl2_set_freq(int ch, unsigned int rpm, const AUDIO_VCE_INSTRUMENT *ins, int keyon);
/** @brief Start a one-shot OPL2 SFX note on the selected channel. */
static void audio_opl2_sfx_keyon(int ch, const AUDIO_VCE_INSTRUMENT *ins, int vol);
static const AUDIO_VCE_INSTRUMENT *audio_get_sfx_inst_for_chunk(const AUDIO_HANDLE_STUB *h, int chunk);
static const signed char audio_kms_cmd_extra[18]; /* forward decl */
/**
 * @brief Silence an OPL2 channel by asserting key-off.
 *
 * @param ch  OPL2 channel index (0-8).
 */

static void audio_opl2_silence_channel(int ch);   /* forward decl */
static void audio_sfx_parse_directory(void *sfxres);
static short audio_sfx_find_chunk(const char *name4);
static unsigned char audio_sfx_duration_ticks(short chunk_id);


/**
 * @brief Compute the SoundBlaster DSP time-constant byte for a given sample rate.
 *
* The SB time constant is 256 - (1000000 / rate_hz), clamped to [1..255].
 * Input rate is clamped to [4000..48000] Hz.
 *
 * @param rate_hz  Desired playback sample rate in Hz.
 * @return SB DSP time-constant byte.
 */

static unsigned short audio_sb_time_constant_for_rate(unsigned int rate_hz) {
    unsigned int div;
    unsigned int tc;
    if (rate_hz < 4000u) {
        rate_hz = 4000u;
    }
    if (rate_hz > 48000u) {
        rate_hz = 48000u;
    }
    div = 1000000u / rate_hz;
    if (div >= 255u) {
        div = 255u;
    }
    if (div < 1u) {
        div = 1u;
    }
    tc = 256u - div;
    if (tc > 255u) {
        tc = 255u;
    }
    return (unsigned short)tc;
}

/**
 * @brief Query the timer dispatch rate clamped to [30..200] Hz.
 *
 * @return Effective audio refresh rate in Hz.
 */

static unsigned int audio_default_refresh_hz(void) {
    unsigned long hz = timer_get_dispatch_hz();
    if (hz < 30UL) {
        hz = 30UL;
    }
    if (hz > 200UL) {
        hz = 200UL;
    }
    return (unsigned int)hz;
}

/**
 * @brief Case-insensitive 4-character tag comparison.
 *
 * @param name  String to test (NULL-safe; returns 0 if NULL).
 * @param tag   4-character reference tag.
* @return 1 if the first 4 characters match (case-insensitive), 0 otherwise.
 */

static int audio_name_tag_is(const char *name, const char tag[4]) {
    int i;
    if (name == 0) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        unsigned char a = (unsigned char)name[i];
        unsigned char b = (unsigned char)tag[i];
        if (a >= 'a' && a <= 'z') {
            a = (unsigned char)(a - 'a' + 'A');
        }
        if (b >= 'a' && b <= 'z') {
            b = (unsigned char)(b - 'a' + 'A');
        }
        if (a != b) {
            return 0;
        }
    }
    return 1;
}

/**
 * @brief Test whether a 4-char resource name belongs to a menu music track.
 *
 * Recognised menu track names: TITL, SLCT, VICT, OVER, DEMO.
 *
 * @param name  4-character resource tag to test.
 * @return 1 if it is a menu-music track name, 0 otherwise.
 */

static int audio_is_menu_track_name(const char *name) {
    if (audio_name_tag_is(name, "TITL") ||
        audio_name_tag_is(name, "SLCT") ||
        audio_name_tag_is(name, "VICT") ||
        audio_name_tag_is(name, "OVER") ||
        audio_name_tag_is(name, "DEMO")) {
        return 1;
    }
    return 0;
}

/**
 * @brief Case-insensitive equality test for two 4-character names.
 *
 * @param a  First 4-character name.
 * @param b  Second 4-character name.
* @return 1 if equal (ignoring ASCII case), 0 otherwise.
 */

static int audio_name4_eq(const char *a, const char *b) {
    int i;
    for (i = 0; i < 4; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca >= 'a' && ca <= 'z') ca = (unsigned char)(ca - 'a' + 'A');
        if (cb >= 'a' && cb <= 'z') cb = (unsigned char)(cb - 'a' + 'A');
        if (ca != cb) return 0;
    }
    return 1;
}

/*
 * load_sfx_file - load driver-prefixed SFX files
* Called by file_load_resource(type=6, "eng") to load e.g. "geeng.sfx" or "adeng.sfx"
 */
void* load_sfx_file(const char* filename)
{
    char prefix[3];
    char path[260];
    void* ptr;

    if (filename == NULL) {
        return NULL;
    }

    /* Extract driver prefix from audiodriverstring (e.g. "sb15" -> "ad" for AdLib) */
    prefix[0] = (char)tolower((unsigned char)audiodriverstring[0]);
    prefix[1] = (char)tolower((unsigned char)audiodriverstring[1]);
    prefix[2] = '\0';

    /* Map sb/pc drivers to actual prefixes used in file names */
    if (prefix[0] == 's' && prefix[1] == 'b') {
        /* sb15 -> adeng.sfx (AdLib/SoundBlaster uses AD prefix) */
        prefix[0] = 'a';
        prefix[1] = 'd';
    } else if (prefix[0] == 'p' && prefix[1] == 'c') {
        /* pc15 -> pceng.sfx */
        prefix[0] = 'p';
        prefix[1] = 'c';
    } else if (prefix[0] == 'm' && prefix[1] == 't') {
        /* mt32 -> mteng.sfx */
        prefix[0] = 'm';
        prefix[1] = 't';
    } else if (prefix[0] == 'a' && prefix[1] == 'd') {
        /* ad15 -> adeng.sfx */
        prefix[0] = 'a';
        prefix[1] = 'd';
    }

    /* Build path: prefix + filename + ".sfx" */
    snprintf(path, sizeof(path), "%s%s.sfx", prefix, filename);

    /* Try loading with case-insensitive file lookup */
    ptr = file_load_binary_nofatal(path);
    if (ptr != NULL) {
        return ptr;
    }

    /* Fallback to generic "geeng.sfx" if driver-specific file not found */
    snprintf(path, sizeof(path), "ge%s.sfx", filename);
    ptr = file_load_binary_nofatal(path);
    if (ptr != NULL) {
        return ptr;
    }

    return NULL;
}

/*
 * Scan KMS bytecodes to compute total duration in ticks.
* Returns the maximum of (last_note_abs_tick + last_note_duration) over all
 * note events, which gives the total playback length before the D9 END opcode.
 * For indefinite-sustain sounds (SKID/SKI2 with dur=32767) returns 254 so the
 * caller can use AUDIO_FX_TICKS_PERSIST (255) instead.
 */
static unsigned char audio_sfx_compute_kms_duration(
        const unsigned char *bc, unsigned int bc_len) {
    unsigned int pos = 0;
    uint32_t abs_tick = 0;
    uint32_t max_end = 0;

    while (pos < bc_len) {
        /* Read VLQ delta */
        uint32_t delta = 0;
        while (pos < bc_len) {
            unsigned char b = bc[pos++];
            delta = (delta << 7u) | (uint32_t)(b & 127u);
            if (!(b & 128u)) break;
        }
        abs_tick += delta;
        if (pos >= bc_len) break;

        unsigned char ev = bc[pos++];
        if (ev == 217u) break; /* END — stop scanning */

        if (ev >= 217u && ev <= 234u) {
            /* Command opcode — skip its extra bytes */
            signed char extra = audio_kms_cmd_extra[ev - 217u];
            if (extra < 0) break; /* E7/E8 variable-length; bail */
            if (pos + (unsigned int)extra > bc_len) break;
            pos += (unsigned int)extra;
            continue;
        }

        /* Note event: pitch byte already consumed as 'ev' */
        if (ev >= 128u && pos < bc_len) pos++; /* skip velocity */

        /* VLQ duration */
        uint32_t dur = 0;
        while (pos < bc_len) {
            unsigned char b = bc[pos++];
            dur = (dur << 7u) | (uint32_t)(b & 127u);
            if (!(b & 128u)) break;
        }
        {
            uint32_t note_end = abs_tick + dur;
            if (note_end > max_end) max_end = note_end;
        }
    }

    if (max_end == 0u) return 6u; /* fallback */
    if (max_end >= 254u) return 254u; /* cap below AUDIO_FX_TICKS_PERSIST */
    return (unsigned char)max_end;
}

/*
 * Navigate a nested KMS container to find the T0S0 sub-chunk that holds
 * the bytecode track.  Returns pointer to bytecodes and sets *out_len.
 * Format: each chunk is itself a KMS directory {u16 size, u16 pad, u16 count,
 *         count*4 names, count*4 offsets, data_area...}.
 *         The first 4 bytes of each sub-chunk record is u32 LE record_size.
 */
static const unsigned char *audio_sfx_find_t0s0(
        const unsigned char *chunk, unsigned int chunk_len,
        unsigned int *out_len) {
    unsigned int sub_count, sub_names, sub_offs, sub_data;
    unsigned int j;

    *out_len = 0;
    if (chunk_len < 10u) return 0;

    /* chunk[0..3] = u32 LE total size (includes these 4 bytes)
     * chunk[4..5] = u16 LE sub-entry count */
    sub_count = (unsigned int)((unsigned short)chunk[4] | ((unsigned short)chunk[5] << 8));
    if (sub_count == 0u || sub_count > 16u) return 0;

    sub_names = 6u;
    sub_offs  = sub_names + sub_count * 4u;
    sub_data  = sub_offs  + sub_count * 4u;
    if (sub_data >= chunk_len) return 0;

    for (j = 0; j < sub_count; j++) {
        /* Check if this sub-chunk is named "T0S0" */
        unsigned int ni = sub_names + j * 4u;
        if (ni + 4u > chunk_len) break;
        if (chunk[ni] == 'T' && chunk[ni+1] == '0' &&
            chunk[ni+2] == 'S' && chunk[ni+3] == '0') {
            unsigned int oi = sub_offs + j * 4u;
            unsigned int rel_off;
            unsigned int rec_abs;
            unsigned int rec_size;
            if (oi + 4u > chunk_len) break;
            rel_off = (unsigned int)(chunk[oi] | (chunk[oi+1]<<8) |
                                     (chunk[oi+2]<<16) | (chunk[oi+3]<<24));
            rec_abs = sub_data + rel_off;
            if (rec_abs + 4u > chunk_len) break;
            rec_size = (unsigned int)(chunk[rec_abs] | (chunk[rec_abs+1]<<8) |
                                      (chunk[rec_abs+2]<<16) | (chunk[rec_abs+3]<<24));
            if (rec_size < 5u || rec_abs + rec_size > chunk_len) break;
            *out_len = rec_size - 4u;
            return chunk + rec_abs + 4u;
        }
    }
    return 0;
}

/**
 * @brief Parse a KMS SFX resource directory into the chunk lookup tables.
 *
 * Reads the top-level KMS container header, extracts each named chunk,
 * navigates each nested T0S0 sub-chunk to compute its playback duration,
 * and populates g_audio_sfx_chunk_name, g_audio_sfx_chunk_ofs,
 * g_audio_sfx_chunk_end, and g_audio_sfx_chunk_duration.
 *
 * @param sfxres  Pointer to the loaded SFX resource data.
 */

static void audio_sfx_parse_directory(void *sfxres) {
    const unsigned char *data = (const unsigned char *)sfxres;
    unsigned int file_size;
    unsigned int count;
    unsigned int i;
    unsigned int names_base;
    unsigned int offs_base;
    unsigned int data_area;

    g_audio_sfx_chunk_count = 0;
    memset(g_audio_sfx_chunk_name, 0, sizeof(g_audio_sfx_chunk_name));
    memset(g_audio_sfx_chunk_ofs, 0, sizeof(g_audio_sfx_chunk_ofs));
    memset(g_audio_sfx_chunk_end, 0, sizeof(g_audio_sfx_chunk_end));
    memset(g_audio_sfx_chunk_duration, 0, sizeof(g_audio_sfx_chunk_duration));

    if (data == 0) return;

    /* KMS container header: u16 total_size, u16 reserved, u16 count */
    file_size = (unsigned int)((unsigned short)data[0] | ((unsigned short)data[1] << 8));
    if (file_size < 64u) return;
    count = (unsigned int)((unsigned short)data[4] | ((unsigned short)data[5] << 8));
    if (count == 0u) return;
    if (count > AUDIO_SFX_MAX_CHUNKS) count = AUDIO_SFX_MAX_CHUNKS;

    names_base = 6u;
    offs_base  = names_base + count * 4u;  /* offset table: count x u32 LE */
    data_area  = offs_base  + count * 4u;  /* start of chunk data records */
    if (data_area > file_size) return;

    for (i = 0; i < count; i++) {
        unsigned int nofs = names_base + i * 4u;
        unsigned int pofs = offs_base  + i * 4u;
        unsigned int rel_ofs;
        unsigned int abs_ofs;
        unsigned int chunk_end;

        /* Read 4-char name */
        g_audio_sfx_chunk_name[i][0] = (char)data[nofs + 0u];
        g_audio_sfx_chunk_name[i][1] = (char)data[nofs + 1u];
        g_audio_sfx_chunk_name[i][2] = (char)data[nofs + 2u];
        g_audio_sfx_chunk_name[i][3] = (char)data[nofs + 3u];
        g_audio_sfx_chunk_name[i][4] = '\0';

        /* Read 32-bit LE offset relative to data_area */
        rel_ofs = (unsigned int)(data[pofs] | (data[pofs+1]<<8) |
                                 (data[pofs+2]<<16) | (data[pofs+3]<<24));
        abs_ofs = data_area + rel_ofs;
        if (abs_ofs >= file_size) continue;

        /* First 4 bytes of the record are u32 LE chunk_size */
        {
            unsigned int rec_size = (unsigned int)(data[abs_ofs] | (data[abs_ofs+1]<<8) |
                                                   (data[abs_ofs+2]<<16) | (data[abs_ofs+3]<<24));
            chunk_end = abs_ofs + rec_size;
            if (chunk_end > file_size) chunk_end = file_size;
        }

        g_audio_sfx_chunk_ofs[i] = (unsigned short)abs_ofs;
        g_audio_sfx_chunk_end[i] = (unsigned short)chunk_end;

        /* Navigate nested container to find T0S0 bytecodes and compute duration */
        {
            unsigned int bc_len = 0;
            const unsigned char *bc = audio_sfx_find_t0s0(
                    data + abs_ofs, chunk_end - abs_ofs, &bc_len);
            if (bc != 0 && bc_len > 0) {
                g_audio_sfx_chunk_duration[i] = audio_sfx_compute_kms_duration(bc, bc_len);
            } else {
                g_audio_sfx_chunk_duration[i] = 6u; /* fallback */
            }
        }
    }

    g_audio_sfx_chunk_count = (unsigned short)count;
}

/**
 * @brief Find a chunk index by its 4-character name.
 *
 * @param name4  4-character chunk name to search for.
* @return Chunk index, or AUDIO_INVALID_CHUNK (-1) if not found.
 */
static short audio_sfx_find_chunk(const char *name4) {
    unsigned int i;
    if (name4 == 0) return AUDIO_INVALID_CHUNK;
    for (i = 0; i < (unsigned int)g_audio_sfx_chunk_count; i++) {
        if (audio_name4_eq(g_audio_sfx_chunk_name[i], name4)) {
            return (short)i;
        }
    }
    return AUDIO_INVALID_CHUNK;
}

/**
 * @brief Return the pre-computed playback duration in ticks for a chunk.
 *
* @param chunk_id  Chunk index (from audio_sfx_find_chunk()).
 * @return Duration in ticks, or 6 as a safe fallback.
 */

static unsigned char audio_sfx_duration_ticks(short chunk_id) {
    if (chunk_id < 0 || (unsigned int)chunk_id >= (unsigned int)g_audio_sfx_chunk_count) {
        return 6u;
    }
    if (g_audio_sfx_chunk_duration[chunk_id] == 0u) {
        return 6u;
    }
    return g_audio_sfx_chunk_duration[chunk_id];
}

/**
 * @brief Store the 4-character menu music track name.
 *
 * @param name  4-character track name string.
 */

static void audio_menu_music_set_name(const char *name) {
    int i;
    for (i = 0; i < 4; i++) {
        if (name != 0 && name[i] != '\0') {
            g_audio_menu_music_name[i] = name[i];
        } else {
            g_audio_menu_music_name[i] = '\0';
        }
    }
    g_audio_menu_music_name[4] = '\0';
}

/**
 * @brief Case-insensitive full string equality test.
 *
 * @param a  First string.
 * @param b  Second string.
 * @return 1 if strings are equal ignoring ASCII case, 0 otherwise.
 */

static int audio_str_ieq(const char *a, const char *b) {
    while (*a != '\0' && *b != '\0') {
        unsigned char ca = (unsigned char)*a;
        unsigned char cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') {
            ca = (unsigned char)(ca - 'A' + 'a');
        }
        if (cb >= 'A' && cb <= 'Z') {
            cb = (unsigned char)(cb - 'A' + 'a');
        }
        if (ca != cb) {
            return 0;
        }
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

/**
 * @brief Read a 32-bit unsigned little-endian integer from a byte buffer.
 *
 * @param ptr  Pointer to at least 4 bytes of data.
 * @return The 32-bit value.
 */

static unsigned int audio_read_u32_le(const unsigned char *ptr) {
    return (unsigned int)ptr[0] |
           ((unsigned int)ptr[1] << 8u) |
           ((unsigned int)ptr[2] << 16u) |
           ((unsigned int)ptr[3] << 24u);
}

/**
 * @brief Convert a MIDI note number to a frequency in Hz.
 *
 * Applies g_audio_menu_note_transpose before the lookup, then clamps
 * the transposed note to [0..127] before indexing the LUT.
 *
* @param note  Raw MIDI note (0-127).
 * @return Frequency in Hz.
 */

static unsigned int audio_note_to_hz(unsigned char note) {
    static const unsigned short midi_hz_lut[128] = {
        8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 15, 15,
        16, 17, 18, 19, 21, 22, 23, 25, 26, 28, 29, 31,
        33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62,
        65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123,
        131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247,
        262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494,
        523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988,
        1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
        2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
        4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
        8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544
    };
    int midi_note = (int)note + g_audio_menu_note_transpose;

    if (midi_note < 0) {
        midi_note = 0;
    }
    if (midi_note > 127) {
        midi_note = 127;
    }
    return (unsigned int)midi_hz_lut[midi_note];
}

/**
 * @brief Fast integer parabolic sine approximation.
 *
* Uses x*(32768-|x|)>>13 on a 16-bit phase (wraps at 65536).
 *
 * @param phase  Phase counter (0-65535 maps to 0-2pi).
 * @return Signed sine value.
 */

static int audio_parabolic_sine(uint32_t phase) {
    int x = (int)(phase & 65535u);
    if (x >= 32768) x -= 65536;
    int ax = (x < 0) ? -x : x;
    return (x * (32768 - ax)) >> 13;
}

/**
* @brief Bright multi-harmonic waveform (sawtooth approximation).
 *
 * Sums harmonics 1-4 of a parabolic sine to produce an OPL2 FM-like timbre.
 *
 * @param phase  Phase counter (0-65535).
 * @return Mixed signed audio sample.
 */

static int audio_soft_wave(uint32_t phase) {
    /* Sawtooth approximation: all harmonics (1 - 1/2 + 1/3 - 1/4).
     * Produces the bright OPL2 FM timbre vs a hollow pure sine. */
    int h1 = audio_parabolic_sine(phase);
    int h2 = audio_parabolic_sine(phase * 2u) / 2;
    int h3 = audio_parabolic_sine(phase * 3u) / 3;
    int h4 = audio_parabolic_sine(phase * 4u) / 4;
    return (h1 - h2 + h3 - h4) * 4 / 5;
}

/**
 * @brief Parse the VCE voice/instrument resource into the global table.
 *
 * Reads the KMS container directory, extracts each instrument record, and
 * populates g_audio_vce_instruments with name, freq mapping, and optional
* OPL2 FM parameters (ADENG1.VCE format).
 *
 * @param voiceptr  Pointer to the loaded VCE resource data.
 * @return Number of instruments parsed, or 0 on error.
 */

static unsigned short audio_parse_vce_instruments(void *voiceptr) {
    const unsigned char *vce;
    unsigned int total_size;
    unsigned short count;
    unsigned int names_base;
    unsigned int offs_base;
    unsigned int records_base;
    unsigned short i;

    g_audio_vce_instrument_count = 0;
    memset(g_audio_vce_instruments, 0, sizeof(g_audio_vce_instruments));

    if (voiceptr == 0) {
        return 0;
    }

    vce = (const unsigned char *)voiceptr;
    total_size = audio_read_u32_le(vce) + 8u;
    if (total_size < 64u || total_size > (256u * 1024u)) {
        return 0;
    }

    count = (unsigned short)((unsigned short)vce[4] | ((unsigned short)vce[5] << 8u));
    if (count == 0u || count > AUDIO_VCE_MAX_INSTRUMENTS) {
        return 0;
    }

    names_base = 6u;
    offs_base = names_base + (unsigned int)count * 4u;
    records_base = offs_base + (unsigned int)count * 4u;
    if (records_base >= total_size) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        unsigned int record_rel;
        unsigned int record_rel_next;
        unsigned int record_off;
        unsigned int record_size;
        const unsigned char *rec;
        AUDIO_VCE_INSTRUMENT *inst = &g_audio_vce_instruments[i];

        record_rel = audio_read_u32_le(vce + offs_base + (unsigned int)i * 4u);
        if (i + 1u < count) {
            record_rel_next = audio_read_u32_le(vce + offs_base + (unsigned int)(i + 1u) * 4u);
        } else {
            record_rel_next = total_size - records_base;
        }
        if (record_rel >= record_rel_next) {
            continue;
        }
        record_off = records_base + record_rel;
        record_size = record_rel_next - record_rel;
        if (record_off + record_size > total_size || record_size < 24u) {
            continue;
        }

        rec = vce + record_off;
        inst->valid = 1;
        inst->name[0] = (char)vce[names_base + (unsigned int)i * 4u + 0u];
        inst->name[1] = (char)vce[names_base + (unsigned int)i * 4u + 1u];
        inst->name[2] = (char)vce[names_base + (unsigned int)i * 4u + 2u];
        inst->name[3] = (char)vce[names_base + (unsigned int)i * 4u + 3u];
        inst->name[4] = '\0';
        inst->record_size = (unsigned short)record_size;
        inst->brightness = (unsigned char)((rec[34] >> 4u) & 15u);
        /** rec[14]/rec[15]: RPM-to-Fnum mapping (audio_update_engine_sound seg007.asm) */
        inst->freq_div  = rec[14u];
        inst->freq_base = rec[15u];
        /* OPL2 FM params: present when record_size >= 94 (ADENG1.VCE format) */
        if (record_size >= 94u) {
            inst->opl_con = rec[68];
            inst->opl_fb  = rec[69];
            /* Op0 (modulator) 12 bytes at rec+70: AR,DR,SL,RR,TL,KSL,MULT,KSR,EGT,VIB,AM,WS */
            memcpy(inst->op0, rec + 70u, 12u);
            /* Op1 (carrier)   12 bytes at rec+82: same layout */
            memcpy(inst->op1, rec + 82u, 12u);
            inst->has_fm = 1;
        }
    }

    g_audio_vce_instrument_count = count;
    return count;
}

/* -----------------------------------------------------------------------
 * KMS VLQ-based stream parser helpers
 * (From restunts seg028.asm sub_3945A analysis)
 *
 * Format: [VLQ_delta][event_byte][opt_velocity][VLQ_duration]
 *   - event 0-216: note; pitch = event & 127 (MIDI note).
 *     If event > 128: 1 extra velocity byte follows.
 *   - event 217-234: command; extra bytes from jump table:
 *     D9-DB=0, DC-DE=1, DF=2, E0-E2=1, E3=0, E4=1, E5=2, E6=5
 *     E7/E8 = track separator (stop parsing this track)
 *   - delta = music ticks from previous event dispatch
 *   - duration = music ticks the note sounds
 *   - music tick rate: 48 Hz for DD param=120 (100*128*120/32000)
 * --------------------------------------------------------------------- */
#define AUDIO_KMS_MUSIC_HZ  48u  /* music ticks/sec for DD=120 */

static const signed char audio_kms_cmd_extra[18] = {
/* D9  DA  DB  DC  DD  DE  DF  E0  E1  E2  E3  E4  E5  E6  E7  E8  E9  EA */
    0,  0,  0,  1,  1,  1,  2,  1,  1,  1,  0,  1,  2,  5, -1, -1,  1,  1
};

/**
 * @brief Decode a KMS variable-length quantity (VLQ) from a byte stream.
 *
 * Reads 7-bit groups (MSB = continuation) until a byte with MSB clear.
 *
 * @param d    Byte array containing the stream.
 * @param pos  Current read position; updated past the VLQ on return.
 * @param end  Exclusive end index of the stream.
 * @return Decoded 32-bit value.
 */

static uint32_t audio_kms_read_vlq(const unsigned char *d, size_t *pos, size_t end) {
    uint32_t v = 0u;
    while (*pos < end) {
        unsigned char b = d[(*pos)++];
        v = (v << 7u) | (uint32_t)(b & 127u);
        if (!(b & 128u)) break;
    }
    return v;
}

/* Temporary note event for KMS parsing */
typedef struct {
    unsigned char pitch;
    unsigned short dur;  /* music ticks */
    uint32_t abs_tick;
} AUDIO_KMS_NOTE;

#define AUDIO_KMS_TMP_MAX 512

/* -----------------------------------------------------------------------
 * Multi-track KMS pool (all tracks in one flat array, like test file)
 * --------------------------------------------------------------------- */
#define AUDIO_KMS_MAX_TRACKS    8
#define AUDIO_KMS_EVENTS_POOL   4096

typedef struct {
    unsigned int start_idx;
    unsigned int count;
    uint32_t     loop_ticks;
    char         name[20];
    int          synth_type; /* 0=tone, 1=perc, 2=hat */
} AUDIO_KMS_TRACK_INFO;

typedef struct {
    unsigned int ei;
    uint32_t     frame_in_gap;
    uint32_t     phase;
    uint32_t     gap_frames;
    uint32_t     snd_frames;
    uint32_t     lfsr;
} AUDIO_KMS_VOICE;

static AUDIO_KMS_NOTE       g_audio_kms_evt_pool[AUDIO_KMS_EVENTS_POOL];
static AUDIO_KMS_TRACK_INFO g_audio_kms_tracks[AUDIO_KMS_MAX_TRACKS];
static unsigned int         g_audio_kms_n_tracks = 0u;
static AUDIO_KMS_VOICE      g_audio_kms_voices[AUDIO_KMS_MAX_TRACKS];

/**
 * @brief Parse a single KMS track into an array of note events.
 *
 * Scans a KMS byte stream between @p start and @p end, collecting note
 * events into @p tmp. Command opcodes are skipped per audio_kms_cmd_extra.
 *
 * @param data           Raw KMS data buffer.
 * @param start          Start offset within @p data.
 * @param end            Exclusive end offset.
 * @param tmp            Output array for parsed note events.
 * @param max_tmp        Capacity of @p tmp.
 * @param out_loop_ticks Set to the total tick length of the loop.
 * @return Number of note events written to @p tmp.
 */

static unsigned int audio_kms_parse_track(
        const unsigned char *data, size_t start, size_t end,
        AUDIO_KMS_NOTE *tmp, unsigned int max_tmp,
        uint32_t *out_loop_ticks) {
    size_t pos = start;
    uint32_t abs_tick = 0u;
    uint32_t loop_end = 0u;
    unsigned int count = 0u;

    while (pos < end && count < max_tmp) {
        unsigned char peek = data[pos];
        if (peek == 231u) break;

        uint32_t delta = audio_kms_read_vlq(data, &pos, end);
        abs_tick += delta;
        if (pos >= end) break;

        unsigned char event = data[pos++];
        if (event == 231u) { pos--; break; }

        if (event >= 217u && event <= 234u) {
            signed char extra = audio_kms_cmd_extra[event - 217u];
            if (extra < 0) break;
            if (pos + (size_t)extra > end) break;
            pos += (size_t)extra;
            continue;
        }

        unsigned char pitch = event & 127u;
        if (event > 128u && pos < end) pos++;  /* skip velocity */

        uint32_t dur = audio_kms_read_vlq(data, &pos, end);
        if (dur == 0u || dur > 10000u) continue;

        tmp[count].pitch    = pitch;
        tmp[count].dur      = (unsigned short)(dur > 65535u ? 65535u : dur);
        tmp[count].abs_tick = abs_tick;
        count++;

        uint32_t et = abs_tick + dur;
        if (et > loop_end) loop_end = et;
    }

    if (out_loop_ticks) *out_loop_ticks = loop_end;
    return count;
}

/**
 * @brief Compute gap and sound frame counts for a KMS track event.
 *
* @param tr              Track info (event pool slice, loop_ticks).
 * @param ei              Event index within the track.
 * @param sample_rate     Output sample rate in Hz.
 * @param out_gap_frames  Output: total frames until next event.
 * @param out_snd_frames  Output: frames the note should sound.
 */

static void audio_kms_gap_for_event(
        const AUDIO_KMS_TRACK_INFO *tr, unsigned int ei,
        unsigned int sample_rate,
        uint32_t *out_gap_frames, uint32_t *out_snd_frames) {
    const AUDIO_KMS_NOTE *ev = &g_audio_kms_evt_pool[tr->start_idx + ei];
    uint32_t next_abs = (ei + 1u < tr->count)
        ? g_audio_kms_evt_pool[tr->start_idx + ei + 1u].abs_tick
        : tr->loop_ticks;
    uint32_t gap_ticks = (next_abs > ev->abs_tick)
        ? (next_abs - ev->abs_tick) : (uint32_t)ev->dur;
    uint32_t snd_ticks = ((uint32_t)ev->dur < gap_ticks) ? (uint32_t)ev->dur : gap_ticks;
    uint32_t gap_f = gap_ticks * sample_rate / AUDIO_KMS_MUSIC_HZ;
    uint32_t snd_f = snd_ticks * sample_rate / AUDIO_KMS_MUSIC_HZ;
    if (gap_f < 1u) gap_f = 1u;
    *out_gap_frames = gap_f;
    *out_snd_frames = snd_f;
}

/**
* @brief Classify a KMS track as tonal (0), percussive (1), or hi-hat (2).
 *
 * Classification is based on the track name string.
 *
 * @param name  Track name string.
 * @return 0=tone, 1=perc (kick/snare/tom), 2=hat/cymbal.
 */

static int audio_kms_synth_type(const char *name) {
    if (strstr(name, "Kick")  || strstr(name, "kick")  || strstr(name, "KICK")  ||
        strstr(name, "Snare") || strstr(name, "snare") || strstr(name, "SNARE") ||
        strstr(name, "Tom")   || strstr(name, "tom")   || strstr(name, "TOM"))   return 1;
    if (strstr(name, "Hat")   || strstr(name, "hat")   || strstr(name, "HAT")   ||
        strstr(name, "Cym")   || strstr(name, "cym")   || strstr(name, "CYM"))   return 2;
    return 0;
}

/**
 * @brief Extract and compile note events from a KMS song resource.
 *
 * Scans the song buffer for E7-prefixed track headers, parses each track
 * into g_audio_kms_evt_pool, initialises per-voice playback state, and
 * primes the legacy sequencer counters.
 *
 * @param songptr  Pointer to the loaded KMS song resource.
 * @return Total note events loaded across all tracks, or 0 on error.
 */

static unsigned short audio_extract_menu_resource_notes(void *songptr) {
    const unsigned char *song;
    unsigned int size;
    unsigned int pool_used;
    unsigned int t;
    static AUDIO_KMS_NOTE kms_tmp[AUDIO_KMS_TMP_MAX];

    /* Reset multi-track pool */
    g_audio_kms_n_tracks = 0u;
    memset(g_audio_kms_voices, 0, sizeof(g_audio_kms_voices));
    memset(g_audio_kms_tracks, 0, sizeof(g_audio_kms_tracks));

    /* Reset legacy sequencer state */
    g_audio_menu_resource_count = 0;
    g_audio_menu_resource_index = 0;
    g_audio_menu_resource_ticks_left = 0;
    g_audio_menu_resource_ticks_total = 0;
    g_audio_menu_resource_tick_pos = 0;
    g_audio_menu_resource_current_note = 60;
    g_audio_menu_resource_current_instrument = 0;
    g_audio_menu_resource_current_velocity = 96;
    g_audio_menu_phase2 = 0;
    g_audio_menu_lp_state = 0;
    g_audio_menu_mod_phase = 0;

    if (songptr == 0) return 0;

    song = (const unsigned char *)songptr;
    size = audio_read_u32_le(song);
    if (size < 32u || size > (256u * 1024u)) size = 16384u;

    /* Scan for E7 track headers, parse each track into flat pool */
    pool_used = 0u;
    size_t si = 0u;
    while (si < (size_t)size && g_audio_kms_n_tracks < AUDIO_KMS_MAX_TRACKS) {
        if (song[si] != 231u) { si++; continue; }
        size_t nlen = (si + 1u < (size_t)size) ? (size_t)song[si + 1u] : 0u;
        size_t ts   = si + 2u + nlen;

        AUDIO_KMS_TRACK_INFO *tr = &g_audio_kms_tracks[g_audio_kms_n_tracks];
        unsigned int nc = (nlen < 19u) ? (unsigned int)nlen : 19u;
        unsigned int ni;
        for (ni = 0u; ni < nc; ni++) tr->name[ni] = (char)song[si + 2u + ni];
        tr->name[nc] = '\0';

        /* Find where this track's stream ends (next E7 or EOF) */
        size_t te = (size_t)size;
        size_t sj = ts;
        while (sj < (size_t)size) { if (song[sj] == 231u) { te = sj; break; } sj++; }

        /* Parse into kms_tmp then copy into pool */
        unsigned int avail = (AUDIO_KMS_EVENTS_POOL > pool_used)
            ? (AUDIO_KMS_EVENTS_POOL - pool_used) : 0u;
        unsigned int maxn = (avail < AUDIO_KMS_TMP_MAX) ? avail : AUDIO_KMS_TMP_MAX;
        uint32_t lt = 0u;
        unsigned int cnt = audio_kms_parse_track(song, ts, te, kms_tmp, maxn, &lt);

        unsigned int slot = pool_used;
        for (ni = 0u; ni < cnt && pool_used < AUDIO_KMS_EVENTS_POOL; ni++, pool_used++)
            g_audio_kms_evt_pool[pool_used] = kms_tmp[ni];

        tr->start_idx  = slot;
        tr->count      = pool_used - slot;
        tr->loop_ticks = lt;
        tr->synth_type = audio_kms_synth_type(tr->name);

        if (tr->count > 0u) g_audio_kms_n_tracks++;
        si = ts;
    }

    if (g_audio_kms_n_tracks == 0u) return 0u;

    /* Initialise per-voice playback state */
    unsigned int rate = g_audio_output_rate_hz;
    if (rate == 0u) rate = 22050u;
    for (t = 0u; t < g_audio_kms_n_tracks; t++) {
        g_audio_kms_voices[t].lfsr = 2900361217u + t * 2654435769u;
        audio_kms_gap_for_event(&g_audio_kms_tracks[t], 0u, rate,
                                &g_audio_kms_voices[t].gap_frames,
                                &g_audio_kms_voices[t].snd_frames);
    }

    /* Set g_audio_menu_resource_count so the >= 8 guard in the caller passes */
    g_audio_menu_resource_count = (unsigned short)pool_used;
    return (unsigned short)pool_used;
}
/**
 * @brief Acquire the SDL audio device lock.
 *
 * No-op when no SDL device is open.
 */

static void audio_sdl_lock(void) {
    if (g_audio_sdl_dev != 0) {
        SDL_LockAudioDevice(g_audio_sdl_dev);
    }
}

/**
 * @brief Release the SDL audio device lock.
 *
 * No-op when no SDL device is open.
 */

static void audio_sdl_unlock(void) {
    if (g_audio_sdl_dev != 0) {
        SDL_UnlockAudioDevice(g_audio_sdl_dev);
    }
}

/**
* @brief Open the SDL audio output device (22050 Hz, S16, stereo, queued).
 *
 * Initialises the SDL audio subsystem if needed, opens the default output
 * device, records the obtained sample rate, and unpauses the device.
 *
 * @return 1 on success, 0 on failure.
 */

static int audio_sdl_open_device(void) {
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    const char * driver_name;

    if (g_audio_sdl_dev != 0) {
        return 1;
    }

    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            return 0;
        }
    }

    SDL_zero(desired);
    SDL_zero(obtained);
    desired.freq = 22050;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = 0;
    desired.userdata = 0;

    g_audio_sdl_dev = SDL_OpenAudioDevice(0, 0, &desired, &obtained, 0);
    if (g_audio_sdl_dev == 0) {
        return 0;
    }

    driver_name = SDL_GetCurrentAudioDriver();
    (void)driver_name;

    if (obtained.freq > 0) {
        g_audio_output_rate_hz = (unsigned int)obtained.freq;
    } else {
        g_audio_output_rate_hz = 22050u;
    }

    SDL_PauseAudioDevice(g_audio_sdl_dev, 0);
    return 1;
}

/**
 * @brief Close the SDL audio output device and reset the sample rate.
 */

static void audio_sdl_close_device(void) {
    if (g_audio_sdl_dev != 0) {
        SDL_CloseAudioDevice(g_audio_sdl_dev);
        g_audio_sdl_dev = 0;
    }
    g_audio_output_rate_hz = 22050u;
}

/**
 * @brief Clamp a value to the 7-bit unsigned range [0..127].
 *
 * @param value  Input value.
 * @return Value clamped to [0, 127].
 */

static unsigned short audio_clamp_u7(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 127) {
        return 127;
    }
    return (unsigned short)value;
}

/**
 * @brief Enqueue a command into the lock-free SB DSP command ring.
 *
 * If the ring is full the oldest unread command is silently dropped.
 *
* @param cmd        Command opcode (AUDIO_SB_CMD_* value).
 * @param handle_id  Target audio handle, or -1 for global commands.
 * @param value0     First command parameter.
 * @param value1     Second command parameter.
 */

static void audio_sb_queue_command(unsigned char cmd, short handle_id, short value0, short value1) {
    unsigned short next_idx = (unsigned short)((g_audio_cmd_write_idx + 1) % AUDIO_CMD_QUEUE_SIZE);
    if (next_idx == g_audio_cmd_read_idx) {
        g_audio_cmd_read_idx = (unsigned short)((g_audio_cmd_read_idx + 1) % AUDIO_CMD_QUEUE_SIZE);
    }
    g_audio_cmd_queue[g_audio_cmd_write_idx].cmd = cmd;
    g_audio_cmd_queue[g_audio_cmd_write_idx].handle_id = handle_id;
    g_audio_cmd_queue[g_audio_cmd_write_idx].value0 = value0;
    g_audio_cmd_queue[g_audio_cmd_write_idx].value1 = value1;
    g_audio_cmd_write_idx = next_idx;
}

/**
 * @brief Write one mono sample into the DMA ring buffer.
 *
 * If the buffer is full the oldest sample is overwritten.
 *
 * @param sample  16-bit signed PCM sample.
 */

static void audio_dma_ring_write_sample(int16_t sample) {
    unsigned int next_write = (g_audio_dma_ring_write + 1u) % AUDIO_DMA_RING_SAMPLES;
    if (next_write == g_audio_dma_ring_read) {
        g_audio_dma_ring_read = (g_audio_dma_ring_read + 1u) % AUDIO_DMA_RING_SAMPLES;
    }
    g_audio_dma_ring[g_audio_dma_ring_write] = sample;
    g_audio_dma_ring_write = next_write;
    g_audio_samples_emitted++;
}

/**
 * @brief Read one mono sample from the DMA ring buffer.
 *
 * @param out_sample  Receives the sample if one is available.
 * @return 1 if a sample was read, 0 if the buffer was empty.
 */

static int audio_dma_ring_read_sample(int16_t *out_sample) {
    if (g_audio_dma_ring_read == g_audio_dma_ring_write) {
        return 0;
    }
    *out_sample = g_audio_dma_ring[g_audio_dma_ring_read];
    g_audio_dma_ring_read = (g_audio_dma_ring_read + 1u) % AUDIO_DMA_RING_SAMPLES;
    g_audio_samples_consumed++;
    return 1;
}

/**
 * @brief Drain the DMA ring buffer into the SDL audio queue.
 *
 * Reads stereo-interleaved frames from the ring and submits them to SDL,
 * maintaining a target queue depth to prevent underruns.
 */

static void audio_sdl_queue_from_ring(void) {
    int16_t interleaved[8192];
    int frame_count;
    int frame_budget;
    Uint32 queued_bytes;
    Uint32 target_bytes;

    if (g_audio_sdl_dev == 0) {
        return;
    }

    queued_bytes = SDL_GetQueuedAudioSize(g_audio_sdl_dev);
    if (queued_bytes > (Uint32)(g_audio_output_rate_hz * 4u)) {
        return;
    }

    target_bytes = (Uint32)((g_audio_output_rate_hz * 4u) / 5u);
    if (target_bytes < 4096u) {
        target_bytes = 4096u;
    }
    if (queued_bytes >= target_bytes) {
        return;
    }

    frame_budget = (int)((target_bytes - queued_bytes) / 4u);
    if (frame_budget < 256) {
        frame_budget = 256;
    }
    if (frame_budget > 4096) {
        frame_budget = 4096;
    }

    frame_count = 0;
    while (frame_count < frame_budget) {
        int16_t mono;
        if (!audio_dma_ring_read_sample(&mono)) {
            break;
        }
        interleaved[frame_count * 2] = mono;
        interleaved[frame_count * 2 + 1] = mono;
        frame_count++;
    }

    if (frame_count > 0) {
        SDL_QueueAudio(g_audio_sdl_dev, interleaved, (Uint32)(frame_count * 2 * (int)sizeof(int16_t)));
    }
}

/**
 * @brief Emulate a SoundBlaster DSP port write to I/O port 556.
 *
* Processes single-byte DSP commands (D1 speaker-on, D3 speaker-off,
 * DA stop DMA, 1C start auto-init DMA, 40/14/48 with pending data bytes).
 *
 * @param port   I/O port address (only 556 is handled).
 * @param value  Byte written to the port.
 */

static void audio_sb_write_port(unsigned short port, unsigned char value) {
    if (port != 556u) {
        return;
    }

    if (g_audio_dsp.pending_bytes_expected != 0u) {
        g_audio_dsp.pending_data[g_audio_dsp.pending_bytes_count] = value;
        g_audio_dsp.pending_bytes_count++;
        if (g_audio_dsp.pending_bytes_count >= g_audio_dsp.pending_bytes_expected) {
            if (g_audio_dsp.pending_cmd == 64u) {
                g_audio_dsp.time_constant = (unsigned short)value;
                g_audio_dsp.sample_rate_hz = (unsigned short)(1000000u / (256u - (unsigned short)value));
                audio_sb_queue_command(AUDIO_SB_CMD_SET_TIME_CONSTANT, -1, (short)g_audio_dsp.time_constant, (short)g_audio_dsp.sample_rate_hz);
            } else if (g_audio_dsp.pending_cmd == 20u || g_audio_dsp.pending_cmd == 72u) {
                g_audio_dsp.dma_block_len = (unsigned short)g_audio_dsp.pending_data[0] | ((unsigned short)g_audio_dsp.pending_data[1] << 8u);
                if (g_audio_dsp.pending_cmd == 20u) {
                    audio_sb_queue_command(AUDIO_SB_CMD_START_DMA, -1, (short)g_audio_dsp.dma_block_len, 0);
                }
            }
            g_audio_dsp.pending_cmd = 0;
            g_audio_dsp.pending_bytes_expected = 0;
            g_audio_dsp.pending_bytes_count = 0;
        }
        return;
    }

    switch (value) {
        case 209u:
            audio_sb_queue_command(AUDIO_SB_CMD_SPEAKER_ON, -1, 0, 0);
            break;
        case 211u:
            audio_sb_queue_command(AUDIO_SB_CMD_SPEAKER_OFF, -1, 0, 0);
            break;
        case 218u:
            audio_sb_queue_command(AUDIO_SB_CMD_STOP_DMA, -1, 0, 0);
            break;
        case 64u:
            g_audio_dsp.pending_cmd = value;
            g_audio_dsp.pending_bytes_expected = 1;
            g_audio_dsp.pending_bytes_count = 0;
            break;
        case 20u:
        case 72u:
            g_audio_dsp.pending_cmd = value;
            g_audio_dsp.pending_bytes_expected = 2;
            g_audio_dsp.pending_bytes_count = 0;
            break;
        case 28u:
            audio_sb_queue_command(AUDIO_SB_CMD_START_DMA, -1, (short)g_audio_dsp.dma_block_len, 1);
            break;
        default:
            break;
    }
}

/**
 * @brief Drain and dispatch all pending commands from the SB command ring.
 *
 * Processes SPEAKER_ON/OFF, START/STOP_DMA, NOTE_ON/OFF, SET_VOLUME,
 * SET_PITCH, SELECT_CHUNK, and SELECT_SKID_CHUNK commands.
 */

static void audio_sb_process_commands(void) {
    while (g_audio_cmd_read_idx != g_audio_cmd_write_idx) {
        AUDIO_SB_COMMAND cmd = g_audio_cmd_queue[g_audio_cmd_read_idx];
        g_audio_cmd_read_idx = (unsigned short)((g_audio_cmd_read_idx + 1) % AUDIO_CMD_QUEUE_SIZE);

        switch (cmd.cmd) {
            case AUDIO_SB_CMD_SPEAKER_ON:
                g_audio_dsp.speaker_on = 1;
                break;
            case AUDIO_SB_CMD_SPEAKER_OFF:
                g_audio_dsp.speaker_on = 0;
                break;
            case AUDIO_SB_CMD_START_DMA:
                g_audio_dsp.dma_running = 1;
                g_audio_dsp.dma_auto_init = (cmd.value1 != 0) ? 1u : 0u;
                g_audio_dsp.dma_bytes_remaining = (unsigned int)((unsigned short)cmd.value0 + 1u);
                break;
            case AUDIO_SB_CMD_STOP_DMA:
                g_audio_dsp.dma_running = 0;
                g_audio_dsp.dma_auto_init = 0;
                g_audio_dsp.dma_bytes_remaining = 0;
                break;
            case AUDIO_SB_CMD_NOTE_ON:
                if (audio_is_valid_handle(cmd.handle_id)) {
                    AUDIO_HANDLE_STUB *h = &g_audio_handles[cmd.handle_id];
                    h->playing = 1;
                    h->dirty = 1;
                    h->pending_restart = 0;
                    if (h->opl2_channel >= 0 && h->has_engi_inst && opl2_is_ready()) {
                        unsigned int rpmv = (unsigned int)(h->current_pitch > 0 ? h->current_pitch : 0);
                        h->opl2_keyon = 1;
                        audio_opl2_set_freq(h->opl2_channel, rpmv, &h->engi_inst, 1);
                    }
                }
                break;
            case AUDIO_SB_CMD_NOTE_OFF:
                if (audio_is_valid_handle(cmd.handle_id)) {
                    AUDIO_HANDLE_STUB *h = &g_audio_handles[cmd.handle_id];
                    h->playing = 0;
                    h->dirty = 1;
                    if (h->opl2_channel >= 0 && h->has_engi_inst && opl2_is_ready()) {
                        unsigned int rpmv = (unsigned int)(h->current_pitch > 0 ? h->current_pitch : 0);
                        h->opl2_keyon = 0;
                        audio_opl2_set_freq(h->opl2_channel, rpmv, &h->engi_inst, 0);
                    }
                }
                break;
            case AUDIO_SB_CMD_SET_VOLUME:
                if (audio_is_valid_handle(cmd.handle_id)) {
                    AUDIO_HANDLE_STUB *h = &g_audio_handles[cmd.handle_id];
                    h->volume = (short)audio_clamp_u7(cmd.value0);
                    h->dirty = 1;
                    if (h->opl2_channel >= 0 && h->has_engi_inst && opl2_is_ready()) {
                        audio_opl2_set_volume_ch(h->opl2_channel, (int)h->volume, &h->engi_inst);
                    }
                    if (h->opl2_skid_channel >= 0 && h->opl2_skid_keyon &&
                        h->has_skid_inst && opl2_is_ready()) {
                        audio_opl2_set_volume_ch(h->opl2_skid_channel, (int)h->volume, &h->skid_inst);
                    }
                    if (h->opl2_crash_channel >= 0 && h->opl2_crash_keyon && opl2_is_ready()) {
                        const AUDIO_VCE_INSTRUMENT *si =
                            audio_get_sfx_inst_for_chunk(h, (int)h->active_chunk);
                        if (si) audio_opl2_set_volume_ch(h->opl2_crash_channel, (int)h->volume, si);
                    }
                }
                break;
            case AUDIO_SB_CMD_SET_PITCH:
                if (audio_is_valid_handle(cmd.handle_id)) {
                    g_audio_handles[cmd.handle_id].target_pitch = cmd.value0;
                    g_audio_handles[cmd.handle_id].dirty = 1;
                }
                break;
            case AUDIO_SB_CMD_SELECT_CHUNK:
                if (audio_is_valid_handle(cmd.handle_id)) {
                    AUDIO_HANDLE_STUB *h = &g_audio_handles[cmd.handle_id];
                    h->active_chunk = cmd.value0;
                    h->dirty = 1;
                    if (cmd.value0 == AUDIO_INVALID_CHUNK) {
                        g_audio_chunk_fx_ticks[cmd.handle_id] = 0;
                        g_audio_chunk_noise_hold[cmd.handle_id] = 0;
                        /* Key-off the OPL2 crash channel */
                        audio_opl2_silence_channel(h->opl2_crash_channel);
                        h->opl2_crash_keyon = 0;
                    } else {
                        /* Key-on: select the matching OPL2 instrument (validated
                         * against restunts seg028 loc_388A2 / audiodriverbinary+33
                         * flow: instrument determined by HDR1 name→VCE lookup). */
                        const AUDIO_VCE_INSTRUMENT *sfx_ins =
                            audio_get_sfx_inst_for_chunk(h, cmd.value0);
                        if (sfx_ins && h->opl2_crash_channel >= 0) {
                            audio_opl2_sfx_keyon(h->opl2_crash_channel,
                                                  sfx_ins, (int)h->volume);
                            h->opl2_crash_keyon = 1;
                        }
                    }
                }
                break;
            case AUDIO_SB_CMD_SELECT_SKID_CHUNK:
                if (audio_is_valid_handle(cmd.handle_id)) {
                    AUDIO_HANDLE_STUB *h = &g_audio_handles[cmd.handle_id];
                    int prev_skid = (int)h->active_skid_chunk;
                    h->active_skid_chunk = cmd.value0;
                    h->dirty = 1;
                    if (cmd.value0 == AUDIO_INVALID_CHUNK) {
                        g_audio_skid_fx_ticks[cmd.handle_id] = 0;
                        g_audio_skid_noise_hold[cmd.handle_id] = 0;
                        /* Key-off skid OPL2 channel — restunts SKID T0S0 sustain
                         * loop runs until audio_init_chunk2 resets the handle. */
                        audio_opl2_silence_channel(h->opl2_skid_channel);
                        h->opl2_skid_keyon = 0;
                    } else if (!h->opl2_skid_keyon || prev_skid != cmd.value0) {
                        /* Key-on: SKID T0S0 track plays a single sustained note
                         * using the SKID VCE instrument (validated from restunts
                         * seg028 sub_38702: note=0 → HDR1[0] → VCE SKID record,
                         * audiodriverbinary+33 key-on, loop sustains via 255). */
                        const AUDIO_VCE_INSTRUMENT *sfx_ins =
                            audio_get_sfx_inst_for_chunk(h, cmd.value0);
                        if (!sfx_ins && h->has_skid_inst)
                            sfx_ins = &h->skid_inst; /* direct fallback */
                        if (sfx_ins && h->opl2_skid_channel >= 0) {
                            audio_opl2_sfx_keyon(h->opl2_skid_channel,
                                                  sfx_ins, (int)h->volume);
                            h->opl2_skid_keyon = 1;
                        }
                    }
                }
                break;
            default:
                break;
        }
    }
}

/**
 * @brief Generate @p sample_count mono PCM samples and push to the DMA ring.
 *
 * Mixes OPL2 FM engine output with noise-based SFX and KMS menu music.
 * Writes silence when DMA, speaker, or the audio-flag2 gate is inactive.
 *
 * @param sample_count  Number of samples to generate.
 */

static void audio_sb_generate_dma_samples(unsigned int sample_count) {
    unsigned int sample_index;

    if (!g_audio_flag2_enabled || !g_audio_dsp.speaker_on || !g_audio_dsp.dma_running) {
        for (sample_index = 0; sample_index < sample_count; sample_index++) {
            audio_dma_ring_write_sample(0);
        }
        return;
    }

    /* Pre-render OPL2 engine samples for the entire tick before the per-sample loop.
     * All active engine handles share a single OPL2 chip instance whose channels
     * were programmed in audio_init_engine / keyon/off / freq-update. */
    {
        unsigned int opl2_n = (sample_count < 4096u) ? sample_count : 4096u;
        if (!g_audio_menu_music_enabled && opl2_is_ready()) {
            opl2_generate(g_opl2_sample_buf, (int)opl2_n);
        } else {
            unsigned int si2;
            for (si2 = 0; si2 < opl2_n; si2++) g_opl2_sample_buf[si2] = 0;
        }
    }

    for (sample_index = 0; sample_index < sample_count; sample_index++) {
        int mixed = 0;
        short handle_id;

        /* Engine sound: OPL2 FM synthesis output */
        if (!g_audio_menu_music_enabled) {
            mixed += (int)g_opl2_sample_buf[sample_index < 4096u ? sample_index : 0u];
        }

/* Chunk FX renderer: render crash slot (active_chunk) and skid slot
         * (active_skid_chunk) independently, matching original [si+14h]/[si+16h]. */
        for (handle_id = 0; handle_id < AUDIO_MAX_HANDLES; handle_id++) {
            AUDIO_HANDLE_STUB *h = &g_audio_handles[handle_id];
            int slot;
            if (!h->allocated) {
                continue;
            }
            /* slot 0 = crash (active_chunk), slot 1 = skid (active_skid_chunk) */
            for (slot = 0; slot < 2; slot++) {
                int fx_chunk = (slot == 0) ? (int)h->active_chunk : (int)h->active_skid_chunk;

                /* If OPL2 is handling this slot already, skip the noise fallback.
                 * Validated against restunts: SFX sounds use audiodriverbinary+33
                 * FM synthesis, NOT PCM noise data from the chunk bytes. */
                if (slot == 0 && h->opl2_crash_channel >= 0 && h->opl2_crash_keyon) continue;
                if (slot == 1 && h->opl2_skid_channel  >= 0 && h->opl2_skid_keyon)  continue;
                int *ns_state  = (slot == 0) ? &g_audio_chunk_noise_state[handle_id]
                                             : &g_audio_skid_noise_state[handle_id];
                int *ns_target = (slot == 0) ? &g_audio_chunk_noise_target[handle_id]
                                             : &g_audio_skid_noise_target[handle_id];
                unsigned short *ns_hold = (slot == 0) ? &g_audio_chunk_noise_hold[handle_id]
                                                      : &g_audio_skid_noise_hold[handle_id];
                uint32_t *prng = (slot == 0) ? &h->phase_accum : &h->skid_phase_accum;

                if (fx_chunk < 0) {  /* AUDIO_INVALID_CHUNK is -1 */
                    continue;
                }
                {
                    int vol = (int)audio_clamp_u7((int)h->volume);
                    int amp;
                    int hold_samples;
                    int lpf_shift;
                    uint32_t state = *prng;
                    uint32_t seed = (uint32_t)((unsigned short)fx_chunk + 1u);
                    int target;
                    int sample;

                    if ((unsigned int)fx_chunk < (unsigned int)g_audio_sfx_chunk_count) {
                        unsigned short so = g_audio_sfx_chunk_ofs[fx_chunk];
                        unsigned short se = g_audio_sfx_chunk_end[fx_chunk];
                        if (se > so + 8u && h->voice_res != 0) {
                            const unsigned char *sfx = (const unsigned char *)h->voice_res;
                            unsigned int len = (unsigned int)(se - so);
                            seed ^= (uint32_t)sfx[so + (sample_index % len)] << 8;
                            seed ^= (uint32_t)sfx[so + ((sample_index * 3u) % len)] << 16;
                        }
                    }

                    if (vol < 48) {
                        vol = 48;
                    }

                    if (fx_chunk == 5 || fx_chunk == 4) {
                        /* SKID/SKI2: tire screech — mid-speed noise texture.
                         * At 22050 Hz, hold=48 → target changes every ~2ms (460 Hz).
                         * lpf_shift=3 → 12.5% convergence/sample, mostly tracks target.
                         * amp calibrated so peak output ≈ 10000 (30% of full scale). */
                        amp = 26000 + (vol * 4);
                        hold_samples = 48;
                        lpf_shift = 3;
                    } else if (fx_chunk <= 2) {
                        /* BLOW/BUMP/CRAS: crash impact — fast harsh noise */
                        amp = 24000 + (vol * 4);
                        hold_samples = 20;
                        lpf_shift = 2;
                    } else {
                        /* SCRA and others */
                        amp = 22000 + (vol * 4);
                        hold_samples = 36;
                        lpf_shift = 3;
                    }

                    if (*ns_hold == 0u) {
                        state ^= (state << 13);
                        state ^= (state >> 17);
                        state ^= (state << 5);
                        state += (seed * 1103515245u) + 12345u;
                        *prng = state;

                        target = (int)((int16_t)(state >> 16));
                        *ns_target = target;
                        *ns_hold = (unsigned short)hold_samples;
                    } else {
                        (*ns_hold)--;
                    }

                    *ns_state += (*ns_target - *ns_state) >> lpf_shift;

                    sample = (*ns_state * amp) >> 15;
                    mixed += sample;
                }
            }
        }

        if (g_audio_menu_music_enabled && !g_audio_menu_music_paused && g_audio_kms_n_tracks > 0u) {
            unsigned int t2;
            unsigned int sr = g_audio_output_rate_hz;
            if (sr == 0u) sr = 22050u;
            int32_t music_mixed = 0;
            int32_t music_contributors = 0;

            for (t2 = 0u; t2 < g_audio_kms_n_tracks; t2++) {
                AUDIO_KMS_TRACK_INFO *tr2 = &g_audio_kms_tracks[t2];
                AUDIO_KMS_VOICE      *v   = &g_audio_kms_voices[t2];
                if (tr2->count == 0u || tr2->loop_ticks == 0u) continue;

                /* Advance cursor when gap exhausted */
                while (v->frame_in_gap >= v->gap_frames) {
                    v->frame_in_gap -= v->gap_frames;
                    v->ei++;
                    if (v->ei >= tr2->count) v->ei = 0u;
                    audio_kms_gap_for_event(tr2, v->ei, sr,
                                           &v->gap_frames, &v->snd_frames);
                }

                if (v->frame_in_gap < v->snd_frames) {
                    const AUDIO_KMS_NOTE *ev = &g_audio_kms_evt_pool[tr2->start_idx + v->ei];
                    if (ev->pitch > 0u) {
                        /* xorshift32 noise */
                        v->lfsr ^= v->lfsr << 13u;
                        v->lfsr ^= v->lfsr >> 17u;
                        v->lfsr ^= v->lfsr << 5u;
                        int noise = (int)(int16_t)(v->lfsr >> 16u);
                        uint32_t fp = v->frame_in_gap;
                        uint32_t sf = v->snd_frames;

                        if (tr2->synth_type == 2) {
                            /* Hi-hat: pure noise burst, 18 ms, linear decay */
                            uint32_t cap = sr * 18u / 1000u;
                            uint32_t eff = (sf < cap) ? sf : cap;
                            if (fp < eff) {
                                int env = (int)(52u * (eff - fp) / eff);
                                music_mixed += (noise >> 3) * env / 128;
                                music_contributors++;
                            }
                        } else if (tr2->synth_type == 1) {
                            unsigned int nhz = audio_note_to_hz(ev->pitch);
                            unsigned int stp = (unsigned int)(
                                ((uint64_t)nhz << 16u) / (uint64_t)sr);
                            if (ev->pitch <= 25u) {
                                /* Kick: noise + pitched tone, 80 ms, quadratic decay */
                                uint32_t cap = sr * 80u / 1000u;
                                uint32_t eff = (sf < cap) ? sf : cap;
                                if (fp < eff) {
                                    uint32_t rem = eff - fp;
                                    int env = (int)(
                                        (uint64_t)110u * rem * rem / ((uint64_t)eff * eff));
                                    v->phase += stp;
                                    int tone = audio_parabolic_sine(v->phase);
                                    music_mixed += (noise / 2 + tone / 2) * env / 128;
                                    music_contributors++;
                                }
                            } else {
                                /* Snare/Tom: 70% noise + 30% tone x2, 65 ms, linear decay */
                                uint32_t cap = sr * 65u / 1000u;
                                uint32_t eff = (sf < cap) ? sf : cap;
                                if (fp < eff) {
                                    int env = (int)(85u * (eff - fp) / eff);
                                    v->phase += stp * 2u;
                                    int tone = audio_parabolic_sine(v->phase);
                                    music_mixed += (noise * 7 / 10 + tone * 3 / 10) * env / 128;
                                    music_contributors++;
                                }
                            }
                        } else {
                            /* Melodic: sawtooth + OPL2-style ADSR */
                            unsigned int nhz = audio_note_to_hz(ev->pitch);
                            unsigned int stp = (unsigned int)(
                                ((uint64_t)nhz << 16u) / (uint64_t)sr);
                            unsigned int attack  = (sr * 3u / 1000u) + 1u;
                            unsigned int decay   = (sf / 8u) + 1u;
                            unsigned int release = (sf / 6u) + 1u;
                            unsigned int peak    = 120u;
                            unsigned int sustain = 68u;
                            unsigned int env;
                            if (fp < attack) {
                                env = (fp * peak) / attack;
                            } else if (fp < (uint32_t)(attack + decay)) {
                                uint32_t dp = fp - (uint32_t)attack;
                                env = peak - (unsigned int)(dp * (peak - sustain) / decay);
                            } else if (sf > fp && (sf - fp) < release) {
                                env = ((sf - fp) * sustain) / release;
                            } else {
                                env = sustain;
                            }
                            v->phase += stp;
                            music_mixed += (int32_t)audio_soft_wave(v->phase) * (int32_t)env / 128;
                            music_contributors++;
                        }
                    }
                }
                v->frame_in_gap++;
            }

            if (music_contributors > 0)
                music_mixed /= (int32_t)g_audio_kms_n_tracks;
            mixed += (int)music_mixed;
        }

        if (mixed > 32767) {
            mixed = 32767;
        }
        if (mixed < -32768) {
            mixed = -32768;
        }
        audio_dma_ring_write_sample((int16_t)mixed);
        g_audio_samples_generated++;

        if (g_audio_dsp.dma_bytes_remaining > 0u) {
            g_audio_dsp.dma_bytes_remaining--;
            if (g_audio_dsp.dma_bytes_remaining == 0u) {
                if (g_audio_dsp.dma_auto_init) {
                    g_audio_dsp.dma_bytes_remaining = (unsigned int)g_audio_dsp.dma_block_len + 1u;
                } else {
                    g_audio_dsp.dma_running = 0;
                    break;
                }
            }
        }
    }
}

/**
 * @brief Test whether a handle ID is valid and allocated.
 *
 * @param handle_id  Handle to validate.
 * @return 1 if valid and allocated, 0 otherwise.
 */

static int audio_is_valid_handle(short handle_id) {
    if (handle_id < 0 || handle_id >= AUDIO_MAX_HANDLES) {
        return 0;
    }
    return g_audio_handles[handle_id].allocated != 0;
}

/* -----------------------------------------------------------------------
 * OPL2 channel helpers
 * Slot register offsets for 9 two-operator FM channels on YM3812:
 *   ch 0-2: op0 at reg 0,1,2   op1 at reg 3,4,5
 *   ch 3-5: op0 at reg 8,9,10  op1 at reg 11,12,13  (6,7 unused)
 *   ch 6-8: op0 at reg 16,17,18 op1 at reg 19,20,21 (14,15 unused)
 * --------------------------------------------------------------------- */
static const int s_opl2_op0_reg[9] = {  0, 1, 2,  8, 9,10, 16,17,18 };
static const int s_opl2_op1_reg[9] = {  3, 4, 5, 11,12,13, 19,20,21 };

/* Op field indices (matches VCE record layout from AD15.DRV analysis) */
#define OPL2F_AR   0
#define OPL2F_DR   1
#define OPL2F_SL   2
#define OPL2F_RR   3
#define OPL2F_TL   4
#define OPL2F_KSL  5
#define OPL2F_MULT 6
#define OPL2F_KSR  7
#define OPL2F_EGT  8
#define OPL2F_VIB  9
#define OPL2F_AM  10
#define OPL2F_WS  11

/*
 * Write all OPL2 FM registers for one instrument into channel 'ch'.
 * Must only be called when opl2_is_ready() returns 1.
 */
/** @brief Program an OPL2 channel with instrument operator settings. */
static void audio_opl2_program_channel(int ch, const AUDIO_VCE_INSTRUMENT *ins) {
    int s0 = s_opl2_op0_reg[ch];
    int s1 = s_opl2_op1_reg[ch];
    /* 32: AM | VIB | EGT | KSR | MULT */
    opl2_write(32+s0, (ins->op0[OPL2F_AM]<<7)|(ins->op0[OPL2F_VIB]<<6)|
                        (ins->op0[OPL2F_EGT]<<5)|(ins->op0[OPL2F_KSR]<<4)|ins->op0[OPL2F_MULT]);
    opl2_write(32+s1, (ins->op1[OPL2F_AM]<<7)|(ins->op1[OPL2F_VIB]<<6)|
                        (ins->op1[OPL2F_EGT]<<5)|(ins->op1[OPL2F_KSR]<<4)|ins->op1[OPL2F_MULT]);
    /* 64: KSL | TL */
    opl2_write(64+s0, (ins->op0[OPL2F_KSL]<<6)|ins->op0[OPL2F_TL]);
    opl2_write(64+s1, (ins->op1[OPL2F_KSL]<<6)|ins->op1[OPL2F_TL]);
    /* 96: AR | DR */
    opl2_write(96+s0, (ins->op0[OPL2F_AR]<<4)|ins->op0[OPL2F_DR]);
    opl2_write(96+s1, (ins->op1[OPL2F_AR]<<4)|ins->op1[OPL2F_DR]);
    /* 128: SL | RR */
    opl2_write(128+s0, (ins->op0[OPL2F_SL]<<4)|ins->op0[OPL2F_RR]);
    opl2_write(128+s1, (ins->op1[OPL2F_SL]<<4)|ins->op1[OPL2F_RR]);
    /* 192: feedback | connection */
    opl2_write(192+ch,  (ins->opl_fb<<1)|ins->opl_con);
    /* 224: waveform select */
    opl2_write(224+s0, ins->op0[OPL2F_WS]&3);
    opl2_write(224+s1, ins->op1[OPL2F_WS]&3);
}

/*
 * Update carrier TL on channel 'ch' to reflect 0-127 volume.
 * TL=0 = loudest, TL=63 = nearly silent.
*/
/** @brief Apply channel volume to OPL2 carrier/modulator operators. */
static void audio_opl2_set_volume_ch(int ch, int volume, const AUDIO_VCE_INSTRUMENT *ins) {
    int s1     = s_opl2_op1_reg[ch];
    int base   = (int)ins->op1[OPL2F_TL];
    int tl     = base + (127 - volume) * (63 - base) / 127;
    if (tl > 63) tl = 63;
    if (tl <  0) tl = 0;
    opl2_write(64+s1, ((int)ins->op1[OPL2F_KSL]<<6)|tl);
}

/*
* Set OPL2 frequency (Fnum/block) and key-on for channel 'ch'.
 * Fnum = rpm / freq_div + freq_base * 16.
 * Uses the smallest block value that keeps Fnum in [0..1023].
 */
static void audio_opl2_set_freq(int ch, unsigned int rpm,
                                const AUDIO_VCE_INSTRUMENT *ins, int keyon) {
    unsigned int fdiv = ins->freq_div > 0u ? (unsigned int)ins->freq_div : 11u;
    unsigned int fnum = rpm / fdiv + (unsigned int)ins->freq_base * 16u;
    int block = 0;
    if (fnum == 0u) fnum = 1u;
    while (fnum > 1023u && block < 7) { fnum >>= 1; block++; }
    if (fnum == 0u) fnum = 1u;
    opl2_write(160+ch, (int)(fnum & 255u));
    opl2_write(176+ch, ((keyon?1:0)<<5)|(block<<2)|(int)((fnum>>8u)&3u));
}

/*
 * Key-on an OPL2 SFX channel with the given instrument at fixed pitch.
* Uses freq_base*16 as Fnum (same parameterisation as audio_opl2_set_freq
 * with rpm=0), validated against the restunts seg028 sub_38DE6 flow where
 * audiodriverbinary+9 uses instrument freq_base for SFX pitch.
*/
/** @brief Start a one-shot OPL2 SFX note on the selected channel. */
static void audio_opl2_sfx_keyon(int ch, const AUDIO_VCE_INSTRUMENT *ins, int vol) {
    if (ch < 0 || ch >= 9 || !opl2_is_ready()) return;
    audio_opl2_program_channel(ch, ins);
    audio_opl2_set_volume_ch(ch, vol, ins);
    audio_opl2_set_freq(ch, 0u, ins, 1);
}

/*
 * Forcibly silence an OPL2 channel by setting both operators to maximum
* attenuation (TL=63) and clearing the key-on bit.  Required because VCE
 * instruments use RR=0 (zero release rate), so a plain key-off via
* register 176 leaves the sound ringing indefinitely.
 */
/** @brief Silence an OPL2 channel immediately by forcing key-off. */
static void audio_opl2_silence_channel(int ch) {
    if (ch < 0 || ch >= 9 || !opl2_is_ready()) return;
    int s0 = s_opl2_op0_reg[ch];
    int s1 = s_opl2_op1_reg[ch];
    opl2_write(64 + s0, 63); /* op0 TL=63 (max attenuation) */
    opl2_write(64 + s1, 63); /* op1 TL=63 (max attenuation) */
    opl2_write(176 + ch, 0);    /* key-off */
}

/*
 * Map an active_chunk index to the right SFX instrument for the given handle.
 * In the original game SKID/SKI2 → SKID inst; SCRA/crash1 → SCRA;
 * CRAS/crash2 → CRAS; BLOW → BLOW; BUMP → BUMP.
 * Returns NULL when no OPL2 instrument matches.
 */
static const AUDIO_VCE_INSTRUMENT *audio_get_sfx_inst_for_chunk(
        const AUDIO_HANDLE_STUB *h, int chunk) {
    if (chunk < 0) return NULL;
    /* Skid variants */
    if (chunk == (int)h->chunk_alt1 || chunk == (int)h->chunk_alt2) {
        return h->has_skid_inst ? &h->skid_inst : NULL;
    }
    /* crash/scrape variants — chunk_crash1=SCRA, chunk_crash2=CRAS */
    if (chunk == (int)h->chunk_crash1) {
        if (h->has_scra_inst) return &h->scra_inst;
        if (h->has_bump_inst) return &h->bump_inst;
    }
    if (chunk == (int)h->chunk_crash2) {
        if (h->has_cras_inst) return &h->cras_inst;
        if (h->has_blow_inst) return &h->blow_inst;
    }
    /* Remaining: try BUMP then BLOW as generic crash fallbacks */
    if (h->has_bump_inst) return &h->bump_inst;
    if (h->has_blow_inst) return &h->blow_inst;
    return NULL;
}

/**
 * @brief Reset a single audio handle to its power-on state.
 *
 * Releases any assigned OPL2 channels, silences them, clears all cached
 * VCE instruments, and zeros all per-handle state arrays.
 *
* @param handle_id  Index into g_audio_handles[] (0..AUDIO_MAX_HANDLES-1).
 */

static void audio_reset_handle(short handle_id) {
    AUDIO_HANDLE_STUB * handle;
    int expected_owner_engine;
    int expected_owner_skid;
    int expected_owner_crash;
    if (handle_id < 0 || handle_id >= AUDIO_MAX_HANDLES) {
        return;
    }
    handle = &g_audio_handles[handle_id];
    expected_owner_engine = (int)handle_id;
    expected_owner_skid = ((int)handle_id) | 256;
    expected_owner_crash = ((int)handle_id) | 512;
    handle->allocated = 0;
    handle->playing = 0;
    handle->dirty = 0;
    handle->pending_restart = 0;
    handle->init_mode = 0;
    handle->volume = 127;
    handle->last_volume = 127;
    handle->rpm = 0;
    handle->current_pitch = 0;
    handle->target_pitch = 0;
    handle->active_chunk = AUDIO_INVALID_CHUNK;
    handle->active_skid_chunk = AUDIO_INVALID_CHUNK;
    handle->chunk_alt1 = AUDIO_INVALID_CHUNK;
    handle->chunk_alt2 = AUDIO_INVALID_CHUNK;
    handle->chunk_crash1 = AUDIO_INVALID_CHUNK;
    handle->chunk_crash2 = AUDIO_INVALID_CHUNK;
    handle->engine_active_ticks = 0;
    handle->phase_accum = 1575931494u; /* non-zero: avoid xorshift(0)=0 fixed point */
    handle->skid_phase_accum = 2746324095u; /* different seed for skid slot */
    handle->data_ptr = 0;
    handle->song_res = 0;
    handle->voice_res = 0;
    handle->freq_div  = 6;   /* PCENG1 ENGI default: RPM/6 = Fnum */
    handle->freq_base = 0;
    /* Release OPL2 channel */
    if (handle->allocated && handle->opl2_channel >= 0 && handle->opl2_channel < 9) {
        audio_opl2_silence_channel(handle->opl2_channel);
        if (g_opl2_ch_owner[handle->opl2_channel] == expected_owner_engine) {
            g_opl2_ch_owner[handle->opl2_channel] = -1;
        }
    }
    handle->opl2_channel    = -1;
    handle->opl2_keyon      = 0;
    handle->opl2_programmed = 0;
    handle->has_engi_inst   = 0;
    memset(&handle->engi_inst, 0, sizeof(handle->engi_inst));
    /* Release OPL2 SFX channels */
    if (handle->allocated && handle->opl2_skid_channel >= 0 && handle->opl2_skid_channel < 9) {
        audio_opl2_silence_channel(handle->opl2_skid_channel);
        if (g_opl2_ch_owner[handle->opl2_skid_channel] == expected_owner_skid) {
            g_opl2_ch_owner[handle->opl2_skid_channel] = -1;
        }
    }
    if (handle->allocated && handle->opl2_crash_channel >= 0 && handle->opl2_crash_channel < 9) {
        audio_opl2_silence_channel(handle->opl2_crash_channel);
        if (g_opl2_ch_owner[handle->opl2_crash_channel] == expected_owner_crash) {
            g_opl2_ch_owner[handle->opl2_crash_channel] = -1;
        }
    }
    handle->opl2_skid_channel  = -1;
    handle->opl2_crash_channel = -1;
    handle->opl2_skid_keyon    = 0;
    handle->opl2_crash_keyon   = 0;
    handle->has_skid_inst = 0; handle->has_scra_inst = 0;
    handle->has_cras_inst = 0; handle->has_blow_inst = 0; handle->has_bump_inst = 0;
    memset(&handle->skid_inst, 0, sizeof(handle->skid_inst));
    memset(&handle->scra_inst, 0, sizeof(handle->scra_inst));
    memset(&handle->cras_inst, 0, sizeof(handle->cras_inst));
    memset(&handle->blow_inst, 0, sizeof(handle->blow_inst));
    memset(&handle->bump_inst, 0, sizeof(handle->bump_inst));
    g_audio_chunk_fx_ticks[handle_id] = 0;
    g_audio_crash_fx_ticks[handle_id] = 0;
    g_audio_crash_fx_kind[handle_id] = 0;
    g_audio_chunk_noise_state[handle_id] = 0;
    g_audio_chunk_noise_target[handle_id] = 0;
    g_audio_chunk_noise_hold[handle_id] = 0;
    g_audio_skid_fx_ticks[handle_id] = 0;
    g_audio_skid_noise_state[handle_id] = 0;
    g_audio_skid_noise_target[handle_id] = 0;
    g_audio_skid_noise_hold[handle_id] = 0;
}

/**
 * @brief Reset all audio handles to their initial state.
 */

static void audio_reset_all_handles(void) {
    short i;
    for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
        audio_reset_handle(i);
    }
}

/**
 * @brief Reset all DSP, ring-buffer, and music sequencer runtime state.
 *
 * Resets emulated SB DSP registers, command queue indices, DMA ring
 * pointers, OPL2 channel ownership, and menu-music state to power-on
 * defaults.
 */

static void audio_reset_runtime_state(void) {
    g_audio_dsp.speaker_on = 0;
    g_audio_dsp.dma_running = 0;
    g_audio_dsp.pending_cmd = 0;
    g_audio_dsp.pending_bytes_expected = 0;
    g_audio_dsp.pending_bytes_count = 0;
    g_audio_dsp.pending_data[0] = 0;
    g_audio_dsp.pending_data[1] = 0;
    g_audio_dsp.dma_block_len = 0;
    g_audio_dsp.sample_rate_hz = 22050;
    g_audio_dsp.time_constant = audio_sb_time_constant_for_rate(22050u);
    g_audio_dsp.dma_auto_init = 0;
    g_audio_dsp.dma_bytes_remaining = 0;
    g_audio_dsp.sample_gen_accum = 0;

    g_audio_cmd_write_idx = 0;
    g_audio_cmd_read_idx = 0;
    g_audio_dma_ring_write = 0;
    g_audio_dma_ring_read = 0;
    g_audio_phase_accum = 0;
    g_audio_output_rate_hz = 22050u;
    g_audio_samples_generated = 0;
    g_audio_samples_consumed = 0;
    g_audio_samples_emitted = 0;
    g_audio_refresh_hz = audio_default_refresh_hz();
    g_audio_refresh_accum = 0u;
    g_audio_music_phase = 0;
    g_audio_music_tick_counter = 0;
    g_audio_music_note_index = 0;
    g_audio_menu_music_enabled = 0;
    g_audio_menu_music_paused = 0;
    g_audio_menu_music_name[0] = '\0';
    g_audio_menu_note_transpose = -12;
    g_audio_menu_duration_scale = 1u;
    g_audio_menu_mod_phase = 0;
    g_audio_menu_resource_ticks_total = 0;
    g_audio_menu_resource_tick_pos = 0;
    g_audio_menu_resource_current_instrument = 0;
    g_audio_menu_resource_current_velocity = 96;
    g_audio_vce_instrument_count = 0;
    /* Reset OPL2 channel ownership */
    {
        int chx;
        for (chx = 0; chx < 9; chx++) g_opl2_ch_owner[chx] = -1;
    }
    if (opl2_is_ready()) opl2_reset();
}

/**
 * @brief Register the audio driver timer callback and reset all handles.
 *
 * No-op if the timer is already registered.
 */

void audio_add_driver_timer(void) {
    if (g_audio_driver_timer_registered) {
        return;
    }
    audio_reset_all_handles();
    timer_reg_callback(audio_driver_timer);
    g_audio_driver_timer_registered = 1;
}

/**
 * @brief Unregister the audio driver timer callback and reset all state.
 *
 * No-op if the timer is not registered.
 */

void audio_remove_driver_timer(void) {
    if (!g_audio_driver_timer_registered) {
        return;
    }
    timer_remove_callback(audio_driver_timer);
    g_audio_driver_timer_registered = 0;
    audio_reset_all_handles();
    audio_reset_runtime_state();
}

/**
 * @brief Allocate and initialise a new audio engine handle.
 *
 * Finds a free slot, resets it, sets resource pointers, parses SFX chunks
 * and VCE instruments, caches ENGI/SKID/SCRA/CRAS/BLOW/BUMP instruments,
* and assigns up to three OPL2 channels (engine, skid, crash).
 *
 * @param unused     Unused legacy init-mode parameter.
 * @param data_ptr   Pointer to the raw SFX data.
 * @param song_res   Pointer to the VCE voice resource.
 * @param voice_res  Pointer to the SFX resource directory.
 * @return Allocated handle ID (0..AUDIO_MAX_HANDLES-1), or -1 on error.
 */

short audio_init_engine(short unused, void * data_ptr, void * song_res, void * voice_res) {
    short i;
    AUDIO_HANDLE_STUB * handle;
    unsigned short vi;
    (void)unused;

    for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
        if (g_audio_handles[i].allocated == 0) {
            handle = &g_audio_handles[i];
            audio_reset_handle(i);
            handle->allocated = 1;
            handle->init_mode = unused;
            handle->data_ptr = data_ptr;
            handle->song_res = song_res;
            handle->voice_res = voice_res;
            handle->playing = 1;
            handle->engine_active_ticks = 30;

            audio_sfx_parse_directory(voice_res);
            handle->chunk_alt1 = audio_sfx_find_chunk("SKID");
            handle->chunk_alt2 = audio_sfx_find_chunk("SKI2");
            handle->chunk_crash1 = audio_sfx_find_chunk("SCRA");
            handle->chunk_crash2 = audio_sfx_find_chunk("CRAS");
            if (handle->chunk_crash1 == AUDIO_INVALID_CHUNK) {
                handle->chunk_crash1 = audio_sfx_find_chunk("BUMP");
            }
            if (handle->chunk_crash2 == AUDIO_INVALID_CHUNK) {
                handle->chunk_crash2 = audio_sfx_find_chunk("BLOW");
            }
            /* Parse VCE instruments from song_res (= ADENG1.VCE with AdLib driver).
             * This updates the global instrument table; engine init is mutually
             * exclusive with menu-music in the original game flow. */
            if (song_res) {
                audio_parse_vce_instruments(song_res);
            }

            /* Find ENGI instrument; copy freq params + full FM data to handle */
            handle->freq_div  = 6;    /* fallback: PCENG1 value */
            handle->freq_base = 0;
            for (vi = 0; vi < g_audio_vce_instrument_count; vi++) {
                const AUDIO_VCE_INSTRUMENT *ins = &g_audio_vce_instruments[vi];
                if (!ins->valid) continue;
                if (ins->name[0]=='E' && ins->name[1]=='N' &&
                    ins->name[2]=='G' && ins->name[3]=='I') {
                    handle->engi_inst     = *ins;  /* deep copy */
                    handle->has_engi_inst = 1;
                    if (ins->freq_div > 0u) {
                        handle->freq_div  = ins->freq_div;
                        handle->freq_base = ins->freq_base;
                    }
                    break;
                }
            }

            /* Assign a free OPL2 channel to this handle */
            {
                int ch;
                for (ch = 0; ch < 9; ch++) {
                    if (g_opl2_ch_owner[ch] < 0) {
                        g_opl2_ch_owner[ch] = (int)i;
                        handle->opl2_channel = ch;
                        break;
                    }
                }
            }

            /* Program the OPL2 channel with ENGI FM parameters and send key-on */
            if (handle->opl2_channel >= 0 && handle->has_engi_inst &&
                handle->engi_inst.has_fm && opl2_is_ready()) {
                audio_opl2_program_channel(handle->opl2_channel, &handle->engi_inst);
                handle->opl2_programmed = 1;
                audio_opl2_set_volume_ch(handle->opl2_channel, (int)handle->volume,
                                         &handle->engi_inst);
                handle->opl2_keyon = 1;
                /* Use idle RPM until the first audio_update_engine_sound call sets a real value */
                audio_opl2_set_freq(handle->opl2_channel, 1800u, &handle->engi_inst, 1);
            }

/* Cache SFX VCE instruments (SKID/SCRA/CRAS/BLOW/BUMP) and assign
             * two additional OPL2 channels for tire squeal and crash effects.
             * All SFX instruments have freq_div=0, so we play fixed-pitch notes. */
            for (vi = 0; vi < g_audio_vce_instrument_count; vi++) {
                const AUDIO_VCE_INSTRUMENT *ins = &g_audio_vce_instruments[vi];
                if (!ins->valid || !ins->has_fm) continue;
                if      (ins->name[0]=='S'&&ins->name[1]=='K'&&ins->name[2]=='I'&&ins->name[3]=='D') {
                    handle->skid_inst = *ins; handle->has_skid_inst = 1;
                } else if (ins->name[0]=='S'&&ins->name[1]=='C'&&ins->name[2]=='R'&&ins->name[3]=='A') {
                    handle->scra_inst = *ins; handle->has_scra_inst = 1;
                } else if (ins->name[0]=='C'&&ins->name[1]=='R'&&ins->name[2]=='A'&&ins->name[3]=='S') {
                    handle->cras_inst = *ins; handle->has_cras_inst = 1;
                } else if (ins->name[0]=='B'&&ins->name[1]=='L'&&ins->name[2]=='O'&&ins->name[3]=='W') {
                    handle->blow_inst = *ins; handle->has_blow_inst = 1;
                } else if (ins->name[0]=='B'&&ins->name[1]=='U'&&ins->name[2]=='M'&&ins->name[3]=='P') {
                    handle->bump_inst = *ins; handle->has_bump_inst = 1;
                }
            }
            /* Assign OPL2 skid channel */
            {
                int ch;
                for (ch = 0; ch < 9; ch++) {
                    if (g_opl2_ch_owner[ch] < 0) {
                        g_opl2_ch_owner[ch] = (int)i | 256; /* mark as SFX-skid owner */
                        handle->opl2_skid_channel = ch;
                        break;
                    }
                }
            }
            /* Assign OPL2 crash channel */
            {
                int ch;
                for (ch = 0; ch < 9; ch++) {
                    if (g_opl2_ch_owner[ch] < 0) {
                        g_opl2_ch_owner[ch] = (int)i | 512; /* mark as SFX-crash owner */
                        handle->opl2_crash_channel = ch;
                        break;
                    }
                }
            }
            return i;
        }
    }

    fatal_error("InitEngine: All handles used.");
    return -1;
}

/**
 * @brief Queue a NOTE_ON command for the given engine handle.
 *
* @param handle_id  Engine handle ID returned by audio_init_engine().
 */

void audio_start_engine_note(short handle_id) {
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }
    audio_sb_queue_command(AUDIO_SB_CMD_NOTE_ON, handle_id, 0, 0);
}

/**
 * @brief Queue a NOTE_OFF command for the given engine handle.
 *
 * @param handle_id  Engine handle ID.
 */

void audio_stop_engine_note(short handle_id) {
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }
    audio_sb_queue_command(AUDIO_SB_CMD_NOTE_OFF, handle_id, 0, 0);
}

/**
 * @brief Update engine sound parameters each game frame.
 *
 * Computes volume from the 3-D distance metric, clamps to [0..127], then
 * queues SET_VOLUME and SET_PITCH. Auto-issues NOTE_ON for a silent handle.
 *
 * @param handle_id         Engine handle ID.
 * @param rpm               Engine RPM.
 * @param distance_x        Distance component X.
 * @param distance_y        Distance component Y.
 * @param distance_z        Distance component Z.
 * @param unused_a          Unused.
 * @param unused_c          Unused.
 * @param unused_e          Unused.
* @param distance_divisor  Distance divisor (minimum 1).
 */

void audio_update_engine_sound(short handle_id, short rpm, short distance_x, short distance_y, short distance_z, short unused_a, short unused_c, short unused_e, short distance_divisor) {
    AUDIO_HANDLE_STUB * handle;
    int distance_metric;
    int scaled;
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }

    handle = &g_audio_handles[handle_id];
    handle->rpm = rpm;

    distance_metric = distance_x;
    if (distance_metric < 0) {
        distance_metric = -distance_metric;
    }
    if (distance_y < 0) {
        distance_metric += -distance_y;
    } else {
        distance_metric += distance_y;
    }
    if (distance_z < 0) {
        distance_metric += -distance_z;
    } else {
        distance_metric += distance_z;
    }

    if (distance_divisor <= 0) {
        distance_divisor = 1;
    }

    scaled = 127 - (distance_metric / distance_divisor);
    if (scaled < 0) {
        scaled = 0;
    }
    if (scaled > 127) {
        scaled = 127;
    }
    if (rpm > 0 && scaled < 8) {
        scaled = 8;
    }

    audio_sb_queue_command(AUDIO_SB_CMD_SET_VOLUME, handle_id, (short)scaled, 0);
    audio_sb_queue_command(AUDIO_SB_CMD_SET_PITCH, handle_id, rpm, 0);
    handle->rpm = rpm;
    handle->engine_active_ticks = 30;
    if (rpm > 0 && !handle->playing) {
        audio_sb_queue_command(AUDIO_SB_CMD_NOTE_ON, handle_id, 0, 0);
    }

    (void)unused_a;
    (void)unused_c;
    (void)unused_e;
}

/**
* @brief Select the crash2 (CRAS/BLOW) effect without restart logic.
 *
 * @param handle_id  Engine handle ID.
 */

/**
 * @brief Select the crash2 (CRAS/BLOW) effect, restarting if already active.
 *
 * @param handle_id  Engine handle ID.
 */

void audio_select_crash2_fx_and_restart(short handle_id) {
    AUDIO_HANDLE_STUB * handle;
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }
    handle = &g_audio_handles[handle_id];
    if (handle->active_chunk == handle->chunk_crash2 && g_audio_chunk_fx_ticks[handle_id] > 0u) {
        return;
    }
    g_audio_crash_fx_kind[handle_id] = 2;
    handle->active_chunk = handle->chunk_crash2;
    g_audio_chunk_fx_ticks[handle_id] = audio_sfx_duration_ticks(handle->chunk_crash2);
    g_audio_crash_fx_ticks[handle_id] = g_audio_chunk_fx_ticks[handle_id];
    handle->pending_restart = 1;
    audio_sb_queue_command(AUDIO_SB_CMD_SELECT_CHUNK, handle_id, handle->active_chunk, 0);
    audio_stop_engine_note(handle_id);
}

/**
* @brief Select the crash1 (SCRA/BUMP) effect.
 *
 * Deduplication guard prevents resetting the tick timer on repeated contacts.
 *
 * @param handle_id  Engine handle ID.
 */

void audio_select_crash1_fx(short handle_id) {
    AUDIO_HANDLE_STUB * handle;
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }
    handle = &g_audio_handles[handle_id];
    /* Deduplication guard: the original ASM's audio_check_flag rejects
re-triggers when the chunk is already playing (byte [bx+5] != 1).
       Without this, repeated physics flags (e.g. wall contact) reset the
       tick timer every frame and the sound never finishes. */
    if (handle->active_chunk == handle->chunk_crash1 && g_audio_chunk_fx_ticks[handle_id] > 0u) {
        return;
    }
    g_audio_crash_fx_kind[handle_id] = 1;
    handle->active_chunk = handle->chunk_crash1;
    g_audio_chunk_fx_ticks[handle_id] = audio_sfx_duration_ticks(handle->chunk_crash1);
    g_audio_crash_fx_ticks[handle_id] = g_audio_chunk_fx_ticks[handle_id];
    audio_sb_queue_command(AUDIO_SB_CMD_SELECT_CHUNK, handle_id, handle->active_chunk, 0);
}

/** @brief Select the crash2 sound effect for the given engine handle. */
void audio_select_crash2_fx(short handle_id) {
    AUDIO_HANDLE_STUB * handle;
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }
    handle = &g_audio_handles[handle_id];
    /* Deduplication guard: see audio_select_crash1_fx comment. */
    if (handle->active_chunk == handle->chunk_crash2 && g_audio_chunk_fx_ticks[handle_id] > 0u) {
        return;
    }
    g_audio_crash_fx_kind[handle_id] = 2;
    handle->active_chunk = handle->chunk_crash2;
    g_audio_chunk_fx_ticks[handle_id] = audio_sfx_duration_ticks(handle->chunk_crash2);
    g_audio_crash_fx_ticks[handle_id] = g_audio_chunk_fx_ticks[handle_id];
    audio_sb_queue_command(AUDIO_SB_CMD_SELECT_CHUNK, handle_id, handle->active_chunk, 0);
}

/**
* @brief Activate sustained tire-squeal (SKID) effect on the skid slot.
 *
 * Sets the skid tick counter to AUDIO_FX_TICKS_PERSIST (255).
 *
 * @param handle_id  Engine handle ID.
 */

void audio_select_skid1_fx(short handle_id) {
    AUDIO_HANDLE_STUB * handle;
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }
    handle = &g_audio_handles[handle_id];
    if (handle->chunk_alt1 == AUDIO_INVALID_CHUNK) return;
    handle->active_skid_chunk = handle->chunk_alt1;
    g_audio_skid_fx_ticks[handle_id] = AUDIO_FX_TICKS_PERSIST;
    audio_sb_queue_command(AUDIO_SB_CMD_SELECT_SKID_CHUNK, handle_id, handle->chunk_alt1, 0);
}

/**
* @brief Activate sustained tire-squeal variant 2 (SKI2) on the skid slot.
 *
 * @param handle_id  Engine handle ID.
 */

void audio_select_skid2_fx(short handle_id) {
    AUDIO_HANDLE_STUB * handle;
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }
    handle = &g_audio_handles[handle_id];
    if (handle->chunk_alt2 == AUDIO_INVALID_CHUNK) return;
    handle->active_skid_chunk = handle->chunk_alt2;
    g_audio_skid_fx_ticks[handle_id] = AUDIO_FX_TICKS_PERSIST;
    audio_sb_queue_command(AUDIO_SB_CMD_SELECT_SKID_CHUNK, handle_id, handle->chunk_alt2, 0);
}

/**
 * @brief Clear the skid-slot SFX and silence its OPL2 channel.
 *
 * Resets skid tick counter, invalidates active_skid_chunk, clears noise
 * hold, and queues SELECT_SKID_CHUNK with AUDIO_INVALID_CHUNK.
 *
 * @param handle_id  Engine handle ID.
 */

void audio_clear_chunk_fx(short handle_id) {
    if (!audio_is_valid_handle(handle_id)) {
        return;
    }
    g_audio_skid_fx_ticks[handle_id] = 0;
    g_audio_handles[handle_id].active_skid_chunk = AUDIO_INVALID_CHUNK;
    g_audio_skid_noise_hold[handle_id] = 0;
    audio_sb_queue_command(AUDIO_SB_CMD_SELECT_SKID_CHUNK, handle_id, AUDIO_INVALID_CHUNK, 0);
}

/**
* @brief Timer ISR for the audio driver (called at g_audio_refresh_hz).
 *
 * Acquires SDL lock, drains pending DSP commands, advances music sequencer,
 * ticks per-handle FX countdowns, generates PCM samples into the ring
 * buffer, and submits queued audio to SDL. Guards against re-entrant calls.
 */

void audio_driver_timer(void) {
    short i;
    unsigned int dispatch_hz;
    unsigned int refresh_hz;
    unsigned int refresh_ticks;
    unsigned int active_handles;

    if (!g_audio_driver_loaded) {
        return;
    }
    if (!g_audio_flag6_enabled) {
        return;
    }

    audio_sdl_lock();

    dispatch_hz = (unsigned int)timer_get_dispatch_hz();

    refresh_hz = g_audio_refresh_hz;
    if (refresh_hz < 30u) {
        refresh_hz = 30u;
    }
    if (refresh_hz > 200u) {
        refresh_hz = 200u;
    }

    refresh_ticks = (unsigned int)timer_get_subticks_for_rate((unsigned long)refresh_hz, &g_audio_refresh_accum);
    if (refresh_ticks == 0u) {
        audio_sdl_queue_from_ring();
        audio_sdl_unlock();
        return;
    }
    if (refresh_ticks > 8u) {
        refresh_ticks = 8u;
    }

    while (refresh_ticks-- > 0u) {
        active_handles = 0u;

        audio_sb_process_commands();
        g_audio_music_tick_counter++;

        if (g_audio_kms_n_tracks == 0u && g_audio_menu_music_enabled && !g_audio_menu_music_paused && g_audio_menu_use_resource && g_audio_menu_resource_count > 0u) {
            if (g_audio_menu_resource_ticks_left == 0u) {
                g_audio_menu_resource_index = (unsigned short)((g_audio_menu_resource_index + 1u) % g_audio_menu_resource_count);
                g_audio_menu_resource_current_note = g_audio_menu_resource_notes[g_audio_menu_resource_index];
                g_audio_menu_resource_ticks_left = g_audio_menu_resource_durations[g_audio_menu_resource_index];
                g_audio_menu_resource_ticks_total = g_audio_menu_resource_durations[g_audio_menu_resource_index];
                g_audio_menu_resource_tick_pos = 0;
                g_audio_menu_resource_current_instrument = g_audio_menu_resource_instruments[g_audio_menu_resource_index];
                g_audio_menu_resource_current_velocity = g_audio_menu_resource_velocities[g_audio_menu_resource_index];
            }
            if (g_audio_menu_resource_ticks_left > 0u) {
                g_audio_menu_resource_ticks_left--;
                g_audio_menu_resource_tick_pos++;
            }
        }

        for (i = 0; i < AUDIO_MAX_HANDLES; i++) {
            AUDIO_HANDLE_STUB * handle = &g_audio_handles[i];
            if (!handle->allocated) {
                continue;
            }

            if (handle->current_pitch != handle->target_pitch) {
                int delta = (int)handle->target_pitch - (int)handle->current_pitch;
                handle->current_pitch = (short)(handle->current_pitch + (short)(delta / 4));
                handle->dirty = 1;
                /* Immediately reflect slewed pitch in OPL2 frequency registers */
                if (handle->opl2_channel >= 0 && handle->has_engi_inst && opl2_is_ready()) {
                    unsigned int rpmv = (unsigned int)(handle->current_pitch > 0 ? handle->current_pitch : 0);
                    audio_opl2_set_freq(handle->opl2_channel, rpmv,
                                        &handle->engi_inst, (int)handle->opl2_keyon);
                }
            }

            if (handle->last_volume != handle->volume) {
                handle->last_volume = handle->volume;
                handle->dirty = 1;
            }

            if (handle->pending_restart && !handle->playing) {
                handle->playing = 1;
                handle->pending_restart = 0;
                handle->dirty = 1;
            }

            if (handle->engine_active_ticks > 0u) {
                handle->engine_active_ticks--;
            }

            if (g_audio_chunk_fx_ticks[i] > 0u) {
                if (g_audio_chunk_fx_ticks[i] != AUDIO_FX_TICKS_PERSIST) {
                    g_audio_chunk_fx_ticks[i]--;
                    if (g_audio_chunk_fx_ticks[i] == 0u) {
                        handle->active_chunk = AUDIO_INVALID_CHUNK;
                        if (handle->opl2_crash_channel >= 0 && handle->opl2_crash_keyon) {
                            audio_opl2_silence_channel(handle->opl2_crash_channel);
                        }
                        handle->opl2_crash_keyon = 0;
                    }
                }
            }

/* Skid slot tick countdown (separate from crash) */
            if (g_audio_skid_fx_ticks[i] > 0u) {
                if (g_audio_skid_fx_ticks[i] != AUDIO_FX_TICKS_PERSIST) {
                    g_audio_skid_fx_ticks[i]--;
                    if (g_audio_skid_fx_ticks[i] == 0u) {
                        handle->active_skid_chunk = AUDIO_INVALID_CHUNK;
                        if (handle->opl2_skid_channel >= 0 && handle->opl2_skid_keyon) {
                            audio_opl2_silence_channel(handle->opl2_skid_channel);
                        }
                        handle->opl2_skid_keyon = 0;
                    }
                }
            }

            if (g_audio_crash_fx_ticks[i] > 0u) {
                g_audio_crash_fx_ticks[i]--;
                if (g_audio_crash_fx_ticks[i] == 0u) {
                    g_audio_crash_fx_kind[i] = 0;
                }
            }

            if (handle->playing || handle->engine_active_ticks > 0u
                || handle->active_chunk != AUDIO_INVALID_CHUNK
                || handle->active_skid_chunk != AUDIO_INVALID_CHUNK) {
                active_handles++;
            }

            if (handle->dirty) {
                handle->dirty = 0;
            }
        }

        if ((active_handles > 0u || (g_audio_menu_music_enabled && !g_audio_menu_music_paused)) && !g_audio_dsp.dma_running) {
            g_audio_dsp.dma_running = 1;
            if (g_audio_dsp.dma_block_len == 0u) {
                g_audio_dsp.dma_block_len = 4095u;
            }
            g_audio_dsp.dma_bytes_remaining = (unsigned int)g_audio_dsp.dma_block_len + 1u;
        }

        g_audio_dsp.sample_gen_accum += g_audio_output_rate_hz;
        {
            unsigned int samples_this_tick = g_audio_dsp.sample_gen_accum / refresh_hz;
            g_audio_dsp.sample_gen_accum %= refresh_hz;
            if (samples_this_tick > 0u) {
                audio_sb_generate_dma_samples(samples_this_tick);
            }
        }
    }

    (void)dispatch_hz;

    audio_sdl_queue_from_ring();

    audio_sdl_unlock();
}

/**
 * @brief Initialise and open the audio driver.
 *
 * Reads tuning parameters from environment variables, opens the SDL output
 * device, initialises the OPL2 emulator, and programs the emulated SB DSP.
 *
* @param driver  Ignored (legacy driver name).
 * @param a2      Ignored.
 * @param a3      Ignored.
 * @return 0 on success, 1 on failure.
 */

short audio_load_driver(const char* driver, short a2, short a3) {
    const char * backend;
    const char * menu_gain_env;
    const char * menu_note_ticks_env;
    const char * refresh_hz_env;
    const char * menu_transpose_env;
    const char * menu_duration_scale_env;
    (void)driver;
    (void)a2;
    (void)a3;

    backend = getenv("STUNTS_AUDIO_BACKEND");
    menu_gain_env = getenv("STUNTS_AUDIO_MENU_GAIN");
    menu_note_ticks_env = getenv("STUNTS_AUDIO_MENU_NOTE_TICKS");
    refresh_hz_env = getenv("STUNTS_AUDIO_REFRESH_HZ");
    menu_transpose_env = getenv("STUNTS_AUDIO_MENU_TRANSPOSE");
    menu_duration_scale_env = getenv("STUNTS_AUDIO_MENU_DURATION_SCALE");
    if (menu_gain_env != 0 && *menu_gain_env != '\0') {
        int gain = atoi(menu_gain_env);
        if (gain < 1000) {
            gain = 1000;
        }
        if (gain > 22000) {
            gain = 22000;
        }
        g_audio_menu_gain = gain;
    } else {
        g_audio_menu_gain = 5200;
    }
    if (menu_note_ticks_env != 0 && *menu_note_ticks_env != '\0') {
        int ticks = atoi(menu_note_ticks_env);
        if (ticks < 8) {
            ticks = 8;
        }
        if (ticks > 80) {
            ticks = 80;
        }
        g_audio_menu_note_ticks = (unsigned int)ticks;
    } else {
        g_audio_menu_note_ticks = 24u;
    }
    if (menu_transpose_env != 0 && *menu_transpose_env != '\0') {
        int semitones = atoi(menu_transpose_env);
        if (semitones < -48) {
            semitones = -48;
        }
        if (semitones > 24) {
            semitones = 24;
        }
        g_audio_menu_note_transpose = semitones;
    } else {
        g_audio_menu_note_transpose = 0;
    }
    if (menu_duration_scale_env != 0 && *menu_duration_scale_env != '\0') {
        int scale = atoi(menu_duration_scale_env);
        if (scale < 1) {
            scale = 1;
        }
        if (scale > 8) {
            scale = 8;
        }
        g_audio_menu_duration_scale = (unsigned int)scale;
    } else {
        g_audio_menu_duration_scale = 1u;
    }
    if (refresh_hz_env != 0 && *refresh_hz_env != '\0') {
        int hz = atoi(refresh_hz_env);
        if (hz < 30) {
            hz = 30;
        }
        if (hz > 200) {
            hz = 200;
        }
        g_audio_refresh_hz = (unsigned int)hz;
    } else {
        g_audio_refresh_hz = audio_default_refresh_hz();
    }
    g_audio_refresh_accum = 0u;
    if (backend != 0) {
        if (audio_str_ieq(backend, "off") || audio_str_ieq(backend, "none") || audio_str_ieq(backend, "null")) {
            g_audio_driver_loaded = 0;
            g_audio_flag2_enabled = 0;
            g_audio_flag6_enabled = 0;
            return 1;
        }
    }

    (void)driver;
    g_audio_driver_loaded = 1;
    g_audio_flag2_enabled = 1;
    g_audio_flag6_enabled = 1;
    if (!audio_sdl_open_device()) {
        g_audio_driver_loaded = 0;
        return 1;
    }
    g_audio_dsp.sample_rate_hz = (unsigned short)g_audio_output_rate_hz;
    g_audio_dsp.time_constant = audio_sb_time_constant_for_rate(g_audio_output_rate_hz);
    /* Initialise OPL2 emulator at the SDL sample rate */
    opl2_init((int)g_audio_output_rate_hz);
    g_audio_menu_music_enabled = 0;
    g_audio_menu_music_paused = 0;
    g_audio_menu_resource_count = 0;
    g_audio_menu_resource_index = 0;
    g_audio_menu_resource_ticks_left = 0;
    g_audio_menu_resource_current_note = 60;
    g_audio_menu_use_resource = 0;
    audio_sb_write_port(556u, 209u);
    audio_sb_write_port(556u, 64u);
    audio_sb_write_port(556u, (unsigned char)g_audio_dsp.time_constant);
    audio_sb_write_port(556u, 72u);
    audio_sb_write_port(556u, 255u);
    audio_sb_write_port(556u, 0u);
    audio_sb_write_port(556u, 28u);
    if (!g_audio_driver_timer_registered) {
        timer_reg_callback(audio_driver_timer);
        g_audio_driver_timer_registered = 1;
    }
    return 0;
    g_audio_menu_phase2 = 0;
    g_audio_menu_lp_state = 0;
}

/**
 * @brief Toggle the audio flag2 enable gate.
 *
 * @return 1 if flag2 is now enabled, 0 if now disabled.
 */

short audio_toggle_flag2(void) {
    g_audio_flag2_enabled = (unsigned char)!g_audio_flag2_enabled;
    return g_audio_flag2_enabled ? 1 : 0;
}

/**
 * @brief Shutdown handler - stop all audio and close the SDL device.
 *
 * Disables both audio flags, drains pending DSP commands, removes the
 * timer callback, destroys the OPL2 emulator, and closes SDL audio.
 */

void audiodrv_atexit(void) {
    g_audio_driver_loaded = 0;
    g_audio_flag2_enabled = 0;
    g_audio_flag6_enabled = 0;
    audio_sb_queue_command(AUDIO_SB_CMD_STOP_DMA, -1, 0, 0);
    audio_sb_queue_command(AUDIO_SB_CMD_SPEAKER_OFF, -1, 0, 0);
    audio_sb_process_commands();
    audio_remove_driver_timer();
    opl2_destroy();
    audio_sdl_close_device();
}

/** audio_driver_func3F: stop/fade the audio driver.
* mode 2 = stop (only mode called from audio_unload).
 * In the original DOS driver, this faded out the OPL/SB and stopped the
 * timer loop. In our SDL port we simply disable menu music and reset the
 * KMS playback state so it starts fresh next time a menu loads. */
void audio_driver_func3F(int mode) {
    (void)mode;
    /* Stop menu music and reset KMS playback */
    g_audio_menu_music_enabled = 0;
    g_audio_menu_music_paused  = 0;
    g_audio_kms_n_tracks       = 0u;
    memset(g_audio_kms_voices, 0, sizeof(g_audio_kms_voices));
    memset(g_audio_kms_tracks, 0, sizeof(g_audio_kms_tracks));
    g_audio_menu_use_resource  = 0;
}

/**
* @brief Disable audio DMA mixing (flag2 gate).
 */

void audio_disable_flag2(void) {
    g_audio_flag2_enabled = 0;
}

/**
* @brief Re-enable audio DMA mixing (flag2 gate).
 */

void audio_enable_flag2(void) {
    g_audio_flag2_enabled = 1;
}

/**
 * @brief Pause or resume menu music playback.
 *
 * @param paused  Non-zero to pause, 0 to resume.
* @return Previous paused state (0 or 1).
 */

unsigned short audio_set_menu_music_paused(unsigned short paused) {
    unsigned short previous = (unsigned short)g_audio_menu_music_paused;
    g_audio_menu_music_paused = (unsigned char)(paused != 0 ? 1 : 0);
    return previous;
}

/**
 * @brief Callback invoked when the game replay mode changes.
 *
 * Currently a no-op stub; reserved for future audio state synchronisation.
 */

void audio_on_replay_mode_changed(void) {}

/**
 * @brief Initialise audio resources for a menu or in-game session.
 *
 * Determines whether @p name is a menu-music track. If so, parses the VCE
 * instruments and extracts KMS note data.
 *
 * @param songptr   Pointer to the KMS song resource.
 * @param voiceptr  Pointer to the VCE voice resource.
 * @param name      4-character resource name used to detect menu tracks.
* @return Always returns (void*)1 (legacy convention).
 */

void * init_audio_resources(void * songptr, void * voiceptr, const char* name) {
    unsigned short vce_count;
    g_audio_last_init_is_menu = (unsigned char)(audio_is_menu_track_name(name) ? 1 : 0);
    if (!g_audio_last_init_is_menu) {
        return (void*)1;
    }
    audio_menu_music_set_name(name);
    vce_count = audio_parse_vce_instruments(voiceptr);
    {
        unsigned short extracted = audio_extract_menu_resource_notes(songptr);
        g_audio_menu_use_resource = (unsigned char)(extracted >= 8u ? 1u : 0u);
        (void)vce_count;
    }
    return (void*)1;
}

/**
 * @brief Finalise audio loading and start menu music playback if applicable.
 *
* Activates menu-music playback when the most recent init_audio_resources()
 * call was for a menu track with successfully extracted note data.
 * Re-registers the driver timer if it was removed at game end.
 *
 * @param audiores  Ignored (legacy opaque resource pointer).
 */

void load_audio_finalize(void * audiores) {
    (void)audiores;
    if (!g_audio_last_init_is_menu) {
        return;
    }
    g_audio_menu_music_enabled = g_audio_menu_use_resource;
    g_audio_menu_music_paused = 0;
    if (g_audio_menu_music_enabled) {
        audio_sb_queue_command(AUDIO_SB_CMD_SPEAKER_ON, -1, 0, 0);
        audio_sb_queue_command(AUDIO_SB_CMD_START_DMA, -1, (short)g_audio_dsp.dma_block_len, 1);
        /* Re-register the audio timer callback in case it was removed at game
         * end (audio_remove_driver_timer). Without it no samples are generated
         * and the menu music is silent after returning from gameplay. */
        if (!g_audio_driver_timer_registered) {
            timer_reg_callback(audio_driver_timer);
            g_audio_driver_timer_registered = 1;
        }
    }
}
