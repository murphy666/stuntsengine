# Stunts Engine — Sound Design Document

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [OPL2 Hardware Background](#2-opl2-hardware-background)
3. [OPL2 Emulation Backend (opl2.c)](#3-opl2-emulation-backend-opl2c)
4. [Resource File Formats](#4-resource-file-formats)
   - 4.1 [VCE — Voice / Instrument Files](#41-vce--voice--instrument-files)
   - 4.2 [SFX / KMS — Sound Effects Files](#42-sfx--kms--sound-effects-files)
5. [Audio Driver Layer (audio.c)](#5-audio-driver-layer-audioc)
   - 5.1 [Global State Tables](#51-global-state-tables)
   - 5.2 [Emulated SB-DSP Command Layer](#52-emulated-sb-dsp-command-layer)
   - 5.3 [Handle Lifecycle](#53-handle-lifecycle)
   - 5.4 [OPL2 Channel Allocation](#54-opl2-channel-allocation)
6. [Engine Sound System](#6-engine-sound-system)
   - 6.1 [Instrument Setup](#61-instrument-setup)
   - 6.2 [RPM-to-Fnum Mapping](#62-rpm-to-fnum-mapping)
   - 6.3 [Volume from 3-D Distance](#63-volume-from-3-d-distance)
   - 6.4 [Pitch Slew](#64-pitch-slew)
7. [SFX System — Tire & Crash Sounds](#7-sfx-system--tire--crash-sounds)
   - 7.1 [Chunk Directory (SFX Resource)](#71-chunk-directory-sfx-resource)
   - 7.2 [OPL2 SFX Path](#72-opl2-sfx-path)
   - 7.3 [Noise-Fallback Path](#73-noise-fallback-path)
   - 7.4 [Tick Countdown & PERSIST Flag](#74-tick-countdown--persist-flag)
   - 7.5 [SFX Selector Functions](#75-sfx-selector-functions)
8. [Menu Music System](#8-menu-music-system)
   - 8.1 [KMS Multi-Track Sequencer](#81-kms-multi-track-sequencer)
   - 8.2 [Per-Track Synthesis](#82-per-track-synthesis)
   - 8.3 [Legacy Note Sequencer (fallback)](#83-legacy-note-sequencer-fallback)
9. [Audio Driver Timer & Mix Loop](#9-audio-driver-timer--mix-loop)
   - 9.1 [Timer Registration](#91-timer-registration)
   - 9.2 [Mix Loop](#92-mix-loop)
   - 9.3 [DMA Ring Buffer](#93-dma-ring-buffer)
   - 9.4 [SDL2 Output Queue](#94-sdl2-output-queue)
10. [OPL2 Register Programming Reference](#10-opl2-register-programming-reference)
    - 10.1 [Slot / Operator Register Map](#101-slot--operator-register-map)
    - 10.2 [VCE Instrument Fields → OPL2 Registers](#102-vce-instrument-fields--opl2-registers)
    - 10.3 [Frequency Number Calculation](#103-frequency-number-calculation)
11. [Car Audio Synchronisation (gamemech / carstate)](#11-car-audio-synchronisation-gamemech--carstate)
12. [Environment Variable Tuning](#12-environment-variable-tuning)
13. [Key Constants Reference](#13-key-constants-reference)

---

## 1. Architecture Overview

The Stunts engine originally targeted real AdLib (YM3812 / OPL2) hardware and a Creative SoundBlaster DSP for PCM playback.  The C port emulates both subsystems in software:

```
Game logic (carstate / gamemech)
   │  audio_update_engine_sound()      per-frame RPM + distance
   │  audio_apply_crash_flags()        crash/skid event triggers
   │  audio_sync_car_audio()           sync all car handles each frame
   │
   ▼
audio.c  ── command queue (AUDIO_SB_COMMAND ring, 256 slots)
   │
   ├── OPL2 FM path ─────── audio_opl2_program_channel()
   │                          audio_opl2_set_freq()           engine tone
   │                          audio_opl2_sfx_keyon()          skid/crash one-shot
   │                          opl2_generate() ──── opl2.c (Nuked OPL3 in OPL2 mode)
   │
   ├── Noise fallback path   xorshift32 PRNG + 1st-order IIR LPF
   │                          (used when OPL2 instrument not available)
   │
   ├── KMS menu music ──── multi-track event pool + per-voice soft-synth
   │
   └── DMA ring (16384 S16 mono samples)
          │
          └── SDL2 audio queue (stereo S16, 22050 Hz, 1024-sample packets)
```

### Source Files

| File | Role |
|------|------|
| `src/audio.c` | Entire audio driver: handles, command queue, OPL2 helpers, SFX/music |
| `src/opl2.c` | Thin wrapper around Nuked OPL3 in OPL2-compatible mode |
| `include/audio.h` | Public API surface |
| `include/opl2.h` | OPL2 backend interface |

---

## 2. OPL2 Hardware Background

The **Yamaha YM3812** (marketed as OPL2 — FM Operator Type-L 2) is a monophonic FM synthesis chip providing **9 two-operator channels**.  Each channel's two operators (modulator op0 and carrier op1) have independent ADSR envelopes and waveform selectors.

Key properties relevant to this emulation:

| Property | Value |
|---|---|
| Channels | 9 (ch 0–8), each with op0 (modulator) and op1 (carrier) |
| Clock | 3 579 545 Hz (standard AdLib / SB clock) |
| Register width | 8 bits, addressed via 8-bit address port |
| Frequency number | `Fnum` 10-bit value in registers A0/B0, plus 3-bit `Block` (octave) |
| Waveform select | 4 shapes (sine, half-sine, abs-sine, quarter-sine); enabled via reg 1 bit 5 |
| Connection mode | `opl_con=0`: FM (op0 modulates op1); `opl_con=1`: AM (additive) |

The per-game-mode channel usage under normal gameplay is:
- **ch 0** — player engine (ENGI instrument from ADENG1.VCE)
- **ch 1** — player skid (SKID instrument)
- **ch 2** — player crash (CRAS/SCRA/BLOW/BUMP instrument)
- **ch 3–8** — opponent cars, up to 3 opponents each claiming 3 channels in the same pattern

---

## 3. OPL2 Emulation Backend (opl2.c)

The backend uses **Nuked OPL3** (a high-accuracy YMF262 emulator written by nukeykt).  It is run in OPL2-compatibility mode by suppressing the OPL3 mode bit and enabling waveform-select (required for `WS != 0`).

### API

```c
void  opl2_init(int sample_rate);   // reset chip at given output rate
void  opl2_reset(void);             // full register reset, re-enables WS
void  opl2_write(int reg, int val); // write one OPL register
void  opl2_generate(short *buf, int n); // render n mono S16 samples
int   opl2_is_ready(void);
void  opl2_destroy(void);
```

### Initialisation sequence

```c
OPL3_Reset(&s_opl, sample_rate);
OPL3_WriteReg(&s_opl, 261, 0);   // keep OPL3 mode bit clear → OPL2 compat
OPL3_WriteReg(&s_opl, 1,   32);  // reg 0x01 bit 5 = waveform-select enable
```

`opl2_generate()` calls `OPL3_GenerateResampled()` once per sample and averages the stereo L+R output to mono.

---

## 4. Resource File Formats

All audio resources use the standard Stunts named-chunk container (see `docs/ressources.md` §1).  The 6-byte header variant is used universally:

```
[u32 total_size][u16 N][N × 4-char name][N × u32 offset][data area...]
```

Offsets are relative to the start of the data area (= offset 6 + N×8 from file start).  Each record begins with a `u32 LE record_size` that covers the record itself (including those 4 bytes).

### 4.1 VCE — Voice / Instrument Files

**Files:** `ADENG1.VCE`, `PCENG1.VCE`, `MTENG1.VCE`, `ADSKIDMS.VCE`, `PCSKIDMS.VCE`, `MTSKIDMS.VCE`

Each chunk inside the VCE container is one instrument. The record layout at offset 0 within each chunk record (after the leading `u32 record_size`):

| Byte offset | Field | Description |
|---|---|---|
| `rec[00..13]` | misc legacy fields | Envelope / brightness metadata |
| `rec[14]` | `freq_div` | RPM-to-Fnum divisor. `Fnum = RPM / freq_div + freq_base * 16` |
| `rec[15]` | `freq_base` | Fnum base offset (× 16) |
| `rec[34]` bits 7:4 | `brightness` | Legacy 4-bit brightness hint |
| `rec[68]` | `opl_con` | OPL2 connection: 0 = FM, 1 = AM |
| `rec[69]` | `opl_fb` | OPL2 operator feedback (0–7) |
| `rec[70..81]` | `op0[12]` | Modulator parameters (12 bytes, see below) |
| `rec[82..93]` | `op1[12]` | Carrier parameters (12 bytes, same layout) |

**OPL2 FM parameters are only present when `record_size >= 94`** (ADENG1.VCE format).  Files with smaller records (PCENG1.VCE, MTENG1.VCE) provide only the freq mapping; the engine falls back to the software noise synthesiser for those drivers.

**Operator field layout** (both `op0` and `op1`):

| Index | Symbol | OPL2 meaning |
|---|---|---|
| 0 | `AR` | Attack rate (0–15) |
| 1 | `DR` | Decay rate (0–15) |
| 2 | `SL` | Sustain level (0–15) |
| 3 | `RR` | Release rate (0–15) |
| 4 | `TL` | Total level / attenuation (0–63, 0 = loudest) |
| 5 | `KSL` | Key scale level (0–3) |
| 6 | `MULT` | Frequency multiplier (0–15) |
| 7 | `KSR` | Key scale rate (0/1) |
| 8 | `EGT` | Envelope generator type / sustain mode (0/1) |
| 9 | `VIB` | Vibrato enable (0/1) |
| 10 | `AM` | Amplitude modulation (tremolo) enable (0/1) |
| 11 | `WS` | Waveform select (0–3) |

**Named instruments in ADENG1.VCE:**

| Name tag | Sound | Notes |
|---|---|---|
| `ENGI` | Engine tone | Main FM carrier, key-on sustained, pitch follows RPM |
| `SKID` | Tire squeal | Fixed-pitch sustained note on skid slot |
| `SKI2` | Tire squeal variant 2 | As SKID, different timbre |
| `SCRA` | Tire scrape / gravel | Crash slot, one-shot |
| `CRAS` | Crash boom | Crash slot, one-shot |
| `BLOW` | Blowout / explosion | Crash slot, one-shot fallback |
| `BUMP` | Bump impact | Crash slot, one-shot fallback |

### 4.2 SFX / KMS — Sound Effects Files

**Files:** `GEENG.SFX`, `ADENG.SFX` (etc.); also `SKIDOVER.KMS`, `SKIDSLCT.KMS`

Each chunk inside the SFX container is an independently playable sound event.  Every chunk is itself a nested KMS sub-container whose parsed bytecode is a KMS VLQ event stream.  The engine navigates to the `T0S0` sub-chunk within each record to reach the raw bytecodes.

**KMS bytecode stream format:**

```
[VLQ delta_ticks]  [event_byte]  [opt: velocity] [VLQ duration_ticks]
```

- `event_byte` 0–216: note event; `pitch = event & 127` (MIDI pitch).  If `event > 128` one extra velocity byte follows.
- `event_byte` 217 (`0xD9`): `END` opcode — stop parsing.
- `event_bytes` 218–234: command opcodes; trailing bytes per `audio_kms_cmd_extra[]` table (see §14).
- Delta and duration are 7-bit group, MSB-continuation VLQ integers.
- Music tick rate: **48 ticks/second** for `DD 120` (tempo = 120 BPM, `100 × 128 × 120 / 32000`).

**Chunk directory parsing** (`audio_sfx_parse_directory`):  
Iterates the top-level container, navigates each `T0S0` sub-chunk, and runs `audio_sfx_compute_kms_duration()` to pre-compute the playback duration in ticks (capped at 254 below `AUDIO_FX_TICKS_PERSIST=255`).

**Named SFX chunks** (canonical from GEENG.SFX / ADENG.SFX):

| Name | Sound | Tick duration |
|---|---|---|
| `SKID` | Tire squeal sustained | 255 (PERSIST) |
| `SKI2` | Tire squeal variant 2 | 255 (PERSIST) |
| `SCRA` | Scrape / gravel contact | short (≤32) |
| `CRAS` | Crash boom | short |
| `BLOW` | Blowout | short |
| `BUMP` | Bump | short |

---

## 5. Audio Driver Layer (audio.c)

### 5.1 Global State Tables

```c
AUDIO_HANDLE_STUB  g_audio_handles[25]       // per-car engine handle slots
int                g_opl2_ch_owner[9]        // OPL2 channel → handle_id owner
                                             //   plain handle_id = engine channel
                                             //   handle_id | 256 = skid channel
                                             //   handle_id | 512 = crash channel
int16_t            g_audio_dma_ring[16384]   // mono S16 DMA ring buffer
short              g_opl2_sample_buf[4096]   // OPL2 render scratch buffer
AUDIO_SB_COMMAND   g_audio_cmd_queue[256]    // DSP command ring
AUDIO_SB_DSP_STATE g_audio_dsp               // emulated SB DSP register state
```

### 5.2 Emulated SB-DSP Command Layer

The original Stunts driver communicated with the SoundBlaster DSP via port-I/O writes.  The emulation preserves this with:

1. **`audio_sb_write_port(port=556, value)`** — accepts the same DSP opcodes the original driver issued:
   | DSP opcode | Meaning |
   |---|---|
   | `0xD1` (209) | Speaker ON |
   | `0xD3` (211) | Speaker OFF |
   | `0xDA` (218) | Stop DMA |
   | `0x40` (64) + 1 byte | Set time constant |
   | `0x14` (20) + 2 bytes | Start single-cycle DMA |
   | `0x48` (72) + 2 bytes | Set DMA block length |
   | `0x1C` (28) | Start auto-init DMA |

2. **`audio_sb_queue_command(cmd, handle_id, v0, v1)`** — all API calls translate to one of the `AUDIO_SB_CMD_*` enumerators pushed into the 256-slot ring:

   | Command | Trigger |
   |---|---|
   | `AUDIO_SB_CMD_NOTE_ON` | `audio_start_engine_note()` |
   | `AUDIO_SB_CMD_NOTE_OFF` | `audio_stop_engine_note()` |
   | `AUDIO_SB_CMD_SET_VOLUME` | `audio_update_engine_sound()` |
   | `AUDIO_SB_CMD_SET_PITCH` | `audio_update_engine_sound()` |
   | `AUDIO_SB_CMD_SELECT_CHUNK` | all crash/SFX selectors |
   | `AUDIO_SB_CMD_SELECT_SKID_CHUNK` | skid selectors |

3. **`audio_sb_process_commands()`** — drains the ring each timer tick; applies state changes and immediately updates matching OPL2 registers.

### 5.3 Handle Lifecycle

```
audio_init_engine()  →  allocate handle  →  parse VCE instruments
                     →  find ENGI record  →  assign OPL2 engine ch
                     →  program OPL2 ch  →  key-on at idle RPM (1800)
                     →  cache SKID/SCRA/CRAS/BLOW/BUMP records
                     →  assign OPL2 skid ch + OPL2 crash ch
                     →  return handle_id

audio_update_engine_sound()   →  SET_VOLUME + SET_PITCH commands
audio_select_crash1_fx()      →  SELECT_CHUNK (SCRA/BUMP)
audio_select_crash2_fx()      →  SELECT_CHUNK (CRAS/BLOW)
audio_select_skid1_fx()       →  SELECT_SKID_CHUNK (SKID)
audio_select_skid2_fx()       →  SELECT_SKID_CHUNK (SKI2)
audio_clear_chunk_fx()        →  SELECT_SKID_CHUNK(INVALID)

audio_unload()  →  audio_driver_func3F(mode=2)  →  audiodrv_atexit()
                →  stops all OPL2, resets handles, closes SDL, destroys OPL2
```

**Handle fields of interest:**

| Field | Type | Description |
|---|---|---|
| `opl2_channel` | `int` | Engine FM channel (-1 = not assigned) |
| `opl2_skid_channel` | `int` | Skid SFX OPL2 channel |
| `opl2_crash_channel` | `int` | Crash SFX OPL2 channel |
| `opl2_keyon` | `uint8` | Current key-on state for engine channel |
| `current_pitch` | `short` | Slewed RPM value, drives Fnum |
| `target_pitch` | `short` | Target RPM from last `audio_update_engine_sound` |
| `volume` | `short` | Current volume [0..127] |
| `active_chunk` | `short` | Active crash SFX slot index or `AUDIO_INVALID_CHUNK` |
| `active_skid_chunk` | `short` | Active skid SFX slot index or `AUDIO_INVALID_CHUNK` |
| `chunk_alt1/2` | `short` | Cached SKID / SKI2 chunk indices |
| `chunk_crash1/2` | `short` | Cached SCRA + CRAS chunk indices |
| `engi_inst` | `AUDIO_VCE_INSTRUMENT` | Deep-copied ENGI FM parameters |
| `skid_inst`, `cras_inst`, etc. | `AUDIO_VCE_INSTRUMENT` | Per-SFX cached instruments |
| `engine_active_ticks` | `uint16` | Ticks since last `audio_update_engine_sound`; suppresses premature silence |

### 5.4 OPL2 Channel Allocation

Nine chip channels are available.  Each car engine handle claims up to **three** channels at `audio_init_engine()` time, from index 0 upward, using `g_opl2_ch_owner[]` as a flat ownership map:

```
g_opl2_ch_owner[ch] == handle_id          → engine FM channel for handle_id
g_opl2_ch_owner[ch] == handle_id | 0x100  → skid SFX channel
g_opl2_ch_owner[ch] == handle_id | 0x200  → crash SFX channel
g_opl2_ch_owner[ch] == -1                 → free
```

Maximum handles on 9 channels: 3 cars (channels 0–8 exhausted).  A fourth car handle allocation fails gracefully with `opl2_channel = -1` and falls through to the noise path for all three of its slots.

---

## 6. Engine Sound System

### 6.1 Instrument Setup

At `audio_init_engine()`, once the VCE resource has been parsed into `g_audio_vce_instruments[]`, the engine searches for the `ENGI` record and deep-copies it into `handle->engi_inst`.  It then calls:

```c
audio_opl2_program_channel(ch, &engi_inst);   // write all FM regs
audio_opl2_set_volume_ch(ch, 127, &engi_inst);
audio_opl2_set_freq(ch, 1800u, &engi_inst, keyon=1); // idle RPM
```

The OPL2 channel stays key-on continuously while the engine is running.  Pitch and volume are updated in-place via register writes without key-cycling (no keyoff/keyon between frames), replicating the original driver's behaviour.

### 6.2 RPM-to-Fnum Mapping

```
Fnum = RPM / freq_div  +  freq_base × 16
```

Where `freq_div` and `freq_base` come from `rec[14]` and `rec[15]` of the VCE ENGI record.

- **ADENG1.VCE ENGI** defaults: `freq_div = 11`, `freq_base = 0` — maps ~0–11264 RPM to Fnum 0–1023 at block 0.
- **PCENG1.VCE fallback**: `freq_div = 6` (software path only, no FM data).

`audio_opl2_set_freq(ch, rpm, ins, keyon)`:
```
fnum = rpm / freq_div + freq_base * 16
block = 0
while (fnum > 1023 && block < 7) { fnum >>= 1; block++ }
write reg 0xA0+ch = fnum & 0xFF
write reg 0xB0+ch = (keyon<<5) | (block<<2) | (fnum>>8 & 3)
```

### 6.3 Volume from 3-D Distance

`audio_update_engine_sound(handle_id, rpm, dx, dy, dz, …, divisor)`:

```c
dist = |dx| + |dy| + |dz|       // L1 Manhattan distance
vol  = 127 - (dist / divisor)   // linear attenuation
vol  = clamp(vol, 0, 127)
if (rpm > 0 && vol < 8) vol = 8  // minimum audible floor for running engine
```

The volume is translated to OPL2 carrier TL via `audio_opl2_set_volume_ch()`:

```c
tl = base_TL + (127 - volume) × (63 - base_TL) / 127
tl = clamp(tl, 0, 63)
write reg 0x40 + s1 = (KSL << 6) | tl
```

where `base_TL` is `op1[TL]` from the VCE instrument (typical value 0 for the ENGI carrier).

### 6.4 Pitch Slew

In `audio_driver_timer()`, once per audio tick, target pitch is slewed toward current:

```c
delta = target_pitch - current_pitch
current_pitch += delta / 4
```

This provides a 4× lower slew rate than the raw RPM signal, giving the engine sound a natural lag.  The OPL2 Fnum register is immediately updated after each slew step.

---

## 7. SFX System — Tire & Crash Sounds

### 7.1 Chunk Directory (SFX Resource)

`audio_sfx_parse_directory(sfxres)` populates three flat arrays (up to 32 chunks):

```c
g_audio_sfx_chunk_name[i][5]      // 4-char name + NUL
g_audio_sfx_chunk_ofs[i]          // absolute byte offset of record in sfxres
g_audio_sfx_chunk_end[i]          // absolute end byte
g_audio_sfx_chunk_duration[i]     // pre-computed KMS duration in ticks
```

At `audio_init_engine()`, chunk indices are looked up immediately and cached:

```c
handle->chunk_alt1   = find("SKID")
handle->chunk_alt2   = find("SKI2")
handle->chunk_crash1 = find("SCRA") or find("BUMP")
handle->chunk_crash2 = find("CRAS") or find("BLOW")
```

### 7.2 OPL2 SFX Path

When an SFX event fires (e.g. `audio_select_skid1_fx()`), a `SELECT_CHUNK` or `SELECT_SKID_CHUNK` command is queued.  In `audio_sb_process_commands()`:

1. `audio_get_sfx_inst_for_chunk(h, chunk_id)` maps the chunk index to the pre-cached VCE instrument pointer (`skid_inst`, `scra_inst`, `cras_inst`, `blow_inst`, `bump_inst`).
2. `audio_opl2_sfx_keyon(ch, inst, vol)` is called:
   - `audio_opl2_program_channel(ch, inst)` — write all FM registers
   - `audio_opl2_set_volume_ch(ch, vol, inst)` — set carrier TL
   - `audio_opl2_set_freq(ch, rpm=0, inst, keyon=1)` — `Fnum = freq_base × 16` (fixed pitch, no RPM dependency)
3. For `SELECT_SKID_CHUNK(INVALID)` or chunk expiry, `audio_opl2_silence_channel(ch)` is called, forcing `TL=63` on both operators before clearing the key-on bit (required because VCE SFX instruments use `RR=0`, leaving the channel ringing without this explicit kill).

### 7.3 Noise-Fallback Path

When `opl2_crash/skid_channel == -1` (no free OPL2 channel) or the instrument had no FM data (`has_fm == 0`), the sample-level loop in `audio_sb_generate_dma_samples()` generates noise:

Each SFX slot (crash and skid are independent) maintains per-handle state:

```c
g_audio_chunk_noise_state[id]    // 1st-order IIR filter state
g_audio_chunk_noise_target[id]   // current noise target level
g_audio_chunk_noise_hold[id]     // samples remaining at current target
h->phase_accum                   // xorshift32 PRNG for crash slot
h->skid_phase_accum              // xorshift32 PRNG for skid slot
```

Noise parameters per chunk type:

| Chunk type | `amp` | `hold_samples` | `lpf_shift` | Character |
|---|---|---|---|---|
| `SKID` / `SKI2` (idx 4–5) | 26000 + vol×4 | 48 | 3 | Mid-speed squeal, ~460 Hz change rate |
| `BLOW` / `BUMP` / `CRAS` (idx 0–2) | 24000 + vol×4 | 20 | 2 | Fast harsh crash noise |
| `SCRA` (idx 3) | 22000 + vol×4 | 36 | 3 | Scrape texture |

PRNG per step (xorshift32):
```c
state ^= state << 13;
state ^= state >> 17;
state ^= state << 5;
state += (seed × 1103515245) + 12345;  // seed mixing from chunk bytes
target = (int16_t)(state >> 16);
```

IIR low-pass:
```c
noise_state += (target - noise_state) >> lpf_shift;
sample = (noise_state × amp) >> 15;
```

### 7.4 Tick Countdown & PERSIST Flag

All SFX have associated per-handle tick counters:

```
g_audio_chunk_fx_ticks[id]   // crash slot countdown
g_audio_skid_fx_ticks[id]    // skid slot countdown
```

The special value `AUDIO_FX_TICKS_PERSIST = 255` means "sustain indefinitely" (used for SKID/SKI2 which must play until `audio_clear_chunk_fx()` is called).  All other values count down by 1 per timer tick; when they reach 0 the slot is cleared and the OPL2 channel is silenced.

Pre-computed KMS durations (from `audio_sfx_compute_kms_duration()`) are used as the initial tick values, giving one-shot sounds the correct natural length.

### 7.5 SFX Selector Functions

| Function | Slot | Chunk | Duration |
|---|---|---|---|
| `audio_select_crash1_fx(id)` | crash | SCRA or BUMP | KMS duration |
| `audio_select_crash2_fx(id)` | crash | CRAS or BLOW | KMS duration |
| `audio_select_crash2_fx_and_restart(id)` | crash | CRAS/BLOW, restarts if same | KMS duration |
| `audio_select_skid1_fx(id)` | skid | SKID | PERSIST (255) |
| `audio_select_skid2_fx(id)` | skid | SKI2 | PERSIST (255) |
| `audio_clear_chunk_fx(id)` | skid | INVALID (clears) | — |

**Deduplication guard:** `crash1/crash2` selectors silently no-op if the same chunk is already playing (`active_chunk == target && ticks > 0`).  This prevents physics-loop re-triggers (e.g. sustained wall contact) from resetting the one-shot timer every frame.

---

## 8. Menu Music System

### 8.1 KMS Multi-Track Sequencer

Menu music is stored as a KMS resource (packed inside the song `.RES` file under track names `TITL`, `SLCT`, `VICT`, `OVER`, `DEMO`).

`audio_extract_menu_resource_notes(songptr)` scans the flat byte stream for `0xE7`-prefixed track headers.  Each track header format:

```
0xE7  [name_len u8]  [name bytes]  [KMS event stream...]  0xE7 (next track or EOF)
```

Each track is parsed with `audio_kms_parse_track()` into a flat `AUDIO_KMS_NOTE` event pool of up to 4096 events across up to 8 tracks.  Track metadata is stored in `AUDIO_KMS_TRACK_INFO`:

```c
start_idx    // offset into g_audio_kms_evt_pool
count        // number of note events in this track
loop_ticks   // total KMS ticks for full loop length
synth_type   // 0=melodic, 1=percussive, 2=hi-hat
name[20]     // track name (from E7 header)
```

Track classification (`audio_kms_synth_type`) is by name string pattern:
- Contains "Kick"/"kick"/"KICK"/"Snare"/"Tom" → percussive (type 1)
- Contains "Hat"/"Cym" → hi-hat (type 2)
- All others → melodic (type 0)

Playback state per voice (`AUDIO_KMS_VOICE`):

```c
ei              // current event index within track
frame_in_gap    // samples elapsed since event onset
gap_frames      // samples until next event
snd_frames      // samples the note should sound (≤ gap_frames)
phase           // oscillator phase accumulator
lfsr            // xorshift32 PRNG state (initialised per-track with unique seed)
```

### 8.2 Per-Track Synthesis

All synthesis runs in `audio_sb_generate_dma_samples()`, one sample at a time.

**Melodic tracks (type 0) — sawtooth + ADSR envelope:**

```
step = (note_hz << 16) / sample_rate
phase += step

envelope:
  attack  phase  → env = fp × peak / attack_frames
  decay   phase  → env = peak - dp × (peak - sustain) / decay_frames
  sustain phase  → env = sustain
  release phase  → env = remaining_frames × sustain / release_frames

waveform: audio_soft_wave(phase)
  = h1 - h2/2 + h3/3 - h4/4  (parabolic sine harmonics 1–4, × 4/5)
  gives a sawtooth-approximation timbre matching OPL2 FM character

output = soft_wave(phase) × env / 128
```

**Percussive tracks (type 1) — pitch-dependent:**

*Kick drum (pitch ≤ 25):*
```
80 ms cap, quadratic decay: env = 110 × rem² / cap²
output = (noise/2 + tone/2) × env / 128
```

*Snare / Tom (pitch > 25):*
```
65 ms cap, linear decay: env = 85 × rem / cap
output = (noise × 0.7 + tone × 0.3) × env / 128
```

**Hi-hat / Cymbal (type 2) — pure noise:**
```
18 ms cap, linear decay: env = 52 × rem / cap
output = (noise >> 3) × env / 128
```

All tracks are mixed by averaging: `music_mixed /= n_tracks`.

**Menu music gates:**
- `g_audio_menu_music_enabled` enabled by `init_audio_resources()` when a menu-track name is used.
- When enabled, OPL2 engine sample generation is suppressed and replaced by the KMS sequencer.
- `audio_set_menu_music_paused(1)` zeroes music contributions without stopping the sequencer clock.

### 8.3 Legacy Note Sequencer (fallback)

When `g_audio_kms_n_tracks == 0` (no E7-prefixed track headers found), the legacy sequencer operates on `g_audio_menu_resource_notes[]` / `g_audio_menu_resource_durations[]` arrays populated by scanning the KMS stream for simple note events.  It advances one note per `ticks_left` decrement.  This is the pre-KMS fallback path.

---

## 9. Audio Driver Timer & Mix Loop

### 9.1 Timer Registration

`audio_add_driver_timer()` calls `timer_reg_callback(audio_driver_timer)`.  The timer subsystem fires at `GAME_DISPLAY_HZ` (60 Hz), which drives `g_audio_refresh_hz` (clamped 30–200 Hz, default = `timer_get_dispatch_hz()`).

### 9.2 Mix Loop

`audio_driver_timer()` runs each timer dispatch step and does:

1. Acquire SDL audio device lock.
2. Compute how many "audio ticks" have elapsed using `timer_get_subticks_for_rate()` with the `g_audio_refresh_accum` accumulator.  Up to 8 ticks per call to prevent runaway.
3. Per audio tick:
   - `audio_sb_process_commands()` — drain DSP command ring, apply OPL2 state changes.
   - Music sequencer `g_audio_music_tick_counter++` and legacy note advance if applicable.
   - Per-handle slew (`current_pitch → target_pitch / 4`) and OPL2 Fnum update.
   - FX tick countdowns (`chunk_fx_ticks`, `skid_fx_ticks`, `crash_fx_ticks`); expire → silence + INVALID.
   - Auto-start DMA if handles are active.
   - `audio_sb_generate_dma_samples(samples_this_tick)` where:
     ```
     samples_this_tick = (output_rate_hz × elapsed_ticks) / refresh_hz
     ```
4. `audio_sdl_queue_from_ring()` — drain DMA ring to SDL queue.
5. Release SDL lock.

### 9.3 DMA Ring Buffer

A 16384-sample mono S16 ring buffer decouples the generation rate from the SDL queue submission rate:

- **Write:** `audio_dma_ring_write_sample()` — overwrites oldest on overflow (ring is always current).
- **Read:** `audio_dma_ring_read_sample()` — returns 0 if empty.
- Generation statistics tracked in `g_audio_samples_generated`, `_consumed`, `_emitted`.

### 9.4 SDL2 Output Queue

`audio_sdl_queue_from_ring()` periodically drains the ring into an SDL2 queued audio device:

- Target queue depth: `output_rate_hz × 4 bytes / 5` (≈200 ms at 22050 Hz).
- Maximum submission per call: 4096 stereo frames (8192 interleaved S16 values).
- SDL device opened at: `22050 Hz, AUDIO_S16SYS, 2 channels (stereo), 1024-sample packets, callback=NULL (push mode)`.
- Mono samples from the ring are duplicated to L+R for stereo output.

---

## 10. OPL2 Register Programming Reference

### 10.1 Slot / Operator Register Map

OPL2 has two **operator slot address spaces** (`s_opl2_op0_reg` for op0/modulator, `s_opl2_op1_reg` for op1/carrier):

```
ch:  0   1   2   3   4   5   6   7   8
op0: 0   1   2   8   9  10  16  17  18
op1: 3   4   5  11  12  13  19  20  21
```

### 10.2 VCE Instrument Fields → OPL2 Registers

| Register base | + slot | Field | Bits |
|---|---|---|---|
| `0x20` (32) | + `s0` | AM \| VIB \| EGT \| KSR \| MULT | 7,6,5,4 / 3:0 |
| `0x20` (32) | + `s1` | same for op1 | |
| `0x40` (64) | + `s0` | KSL \| TL | 7:6 / 5:0 |
| `0x40` (64) | + `s1` | KSL \| TL (volume controlled here) | |
| `0x60` (96) | + `s0` | AR \| DR | 7:4 / 3:0 |
| `0x60` (96) | + `s1` | AR \| DR | |
| `0x80` (128) | + `s0` | SL \| RR | 7:4 / 3:0 |
| `0x80` (128) | + `s1` | SL \| RR | |
| `0xC0` (192) | + `ch` | FB \| CON | 3:1 / 0 |
| `0xE0` (224) | + `s0` | WS | 1:0 |
| `0xE0` (224) | + `s1` | WS | |

Volume control acts exclusively on `reg 0x40 + s1` (carrier TL).  The modulator TL remains at the instrument's base value.

### 10.3 Frequency Number Calculation

```
Fnum = RPM / freq_div  +  freq_base × 16          (integer division, minimum 1)

Block selection (auto):
  block = 0
  while Fnum > 1023 and block < 7:
    Fnum >>= 1; block++

OPL2 write:
  reg 0xA0 + ch  =  Fnum & 0xFF
  reg 0xB0 + ch  =  (KeyOn << 5) | (Block << 2) | (Fnum >> 8 & 3)
```

The resulting audible frequency in Hz:  
$$f_{Hz} = \frac{F_{num} \times ClkOPL}{2^{(Block+20)}} = \frac{F_{num} \times 3579545}{2^{Block+20}}$$

---

## 11. Car Audio Synchronisation (gamemech / carstate)

`audio_sync_car_audio()` is called once per rendered frame from `gamemech.c`.  For each active car that has an audio handle:

1. Computes relative position `(dx, dy, dz)` between the car and the player camera.
2. Calls `audio_update_engine_sound(handle_id, rpm, dx, dy, dz, …, divisor)`.
3. For the player car, `divisor` is typically 1 (undivided, full volume range).  For opponent cars, `divisor` scales with their lateral separation.

`audio_apply_crash_flags(flags, sound_id)` is called from `carstate.c` when the physics state machine sets crash or skid bits:

| `flags` bit | Action |
|---|---|
| bit 0 | `audio_select_crash1_fx(sound_id)` — impact / scrape |
| bit 1 | `audio_select_crash2_fx_and_restart(sound_id)` — crash boom |
| bit 2 | `audio_select_skid1_fx(sound_id)` — tire squeal |
| bit 3 | `audio_select_skid2_fx(sound_id)` — alternate squeal |
| bit 4 | `audio_clear_chunk_fx(sound_id)` — squeal off |

`audio_replay_update_engine_sounds(info, sound_id)` is the replay-mode equivalent that reconstructs per-frame RPM and distance from replay data.

---

## 12. Environment Variable Tuning

All tuning variables are read once at `audio_load_driver()`:

| Variable | Default | Range | Effect |
|---|---|---|---|
| `STUNTS_AUDIO_BACKEND` | (use SDL) | `off`/`none`/`null` to disable | Disables audio subsystem entirely |
| `STUNTS_AUDIO_REFRESH_HZ` | `timer_get_dispatch_hz()` | 30–200 | Audio tick rate; lower = more CPU-friendly, higher = tighter latency |
| `STUNTS_AUDIO_MENU_GAIN` | 5200 | 1000–22000 | Menu music soft-synth amplitude scale |
| `STUNTS_AUDIO_MENU_NOTE_TICKS` | 24 | 8–80 | Legacy sequencer note duration in ticks |
| `STUNTS_AUDIO_MENU_TRANSPOSE` | 0 | −48 to +24 semitones | Shifts all menu music notes by this many MIDI semitones |
| `STUNTS_AUDIO_MENU_DURATION_SCALE` | 1 | 1–8 | Multiplies note durations (slows menu music) |

---

## 13. Key Constants Reference

```c
AUDIO_MAX_HANDLES         25       // max simultaneous engine instances
AUDIO_INVALID_CHUNK       -1       // sentinel: no active SFX slot
AUDIO_FX_TICKS_PERSIST    255      // skid sound: sustain indefinitely
AUDIO_CMD_QUEUE_SIZE      256      // DSP command ring capacity
AUDIO_DMA_RING_SAMPLES    16384    // DMA ring size (mono S16)
AUDIO_MENU_RESOURCE_NOTES_MAX 512  // legacy note table capacity
AUDIO_VCE_MAX_INSTRUMENTS 64       // VCE parse table capacity
AUDIO_SFX_MAX_CHUNKS      32       // SFX directory table capacity
AUDIO_KMS_MAX_TRACKS      8        // KMS multi-track pool
AUDIO_KMS_EVENTS_POOL     4096     // flat note event pool
AUDIO_KMS_MUSIC_HZ        48       // KMS music ticks/second (tempo=120)
OPL2_CLOCK                3579545  // YM3812 reference clock Hz
SDL output rate           22050    // Hz, stereo S16
SDL packet size           1024     // samples per SDL buffer
```

**KMS command extra-bytes table** (`audio_kms_cmd_extra[]`, index = opcode − 217):

| Opcode | Hex | Extra bytes | Notes |
|---|---|---|---|
| D9 | 217 | 0 | END — stop parsing |
| DA | 218 | 0 | |
| DB | 219 | 0 | |
| DC | 220 | 1 | |
| DD | 221 | 1 | Tempo parameter |
| DE | 222 | 1 | |
| DF | 223 | 2 | |
| E0 | 224 | 1 | |
| E1 | 225 | 1 | |
| E2 | 226 | 1 | |
| E3 | 227 | 0 | |
| E4 | 228 | 1 | |
| E5 | 229 | 2 | |
| E6 | 230 | 5 | |
| E7 | 231 | −1 | Track separator — terminate track parsing |
| E8 | 232 | −1 | Track separator variant |
| E9 | 233 | 1 | |
| EA | 234 | 1 | |

---