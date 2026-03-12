# Stunts Engine — Resource File Formats Reference

Comprehensive documentation of every game resource file format used by the Stunts engine, derived from source code analysis.

---

## Table of Contents

1. [Resource Container Format (Archive Format)](#1-resource-container-format)
2. [RES Files — General Resource Archives](#2-res-files)
3. [PRE Files — Compressed Resource Archives](#3-pre-files)
4. [PVS Files — Pre-rendered 2D Shape Archives](#4-pvs-files)
5. [PES Files — Planar-Encoded Shape Archives](#5-pes-files)
6. [P3S Files — 3D Shape Archives](#6-p3s-files)
7. [TRK Files — Track Data](#7-trk-files)
8. [HIG Files — High Score Data](#8-hig-files)
9. [RPL Files — Replay Data](#9-rpl-files)
10. [KMS Files — Music/Sound Songs](#10-kms-files)
11. [VCE Files — Voice/Instrument Data](#11-vce-files)
12. [SFX Files — Sound Effects](#12-sfx-files)
13. [FNT Files — Font Data](#13-fnt-files)
14. [DAT Files — Setup Data (SETUP.DAT / stunts.cfg)](#14-dat-files)
15. [SHAPE2D Format — 2D Sprite/Bitmap Format](#15-shape2d-format)
16. [SHAPE3D Format — 3D Polygon Model Format](#16-shape3d-format)
17. [SIMD — Car Physics Data](#17-simd-format)
18. [GAMEINFO — Game Configuration](#18-gameinfo-format)
19. [GAMESTATE — Runtime Game State](#19-gamestate-format)
20. [Compression Formats](#20-compression-formats)

---

## 1. Resource Container Format

All `.RES`, `.PRE`, `.PVS`, `.PES`, `.P3S`, `.SFX`, `.KMS`, and `.VCE` files share a common **named-chunk container** format. This is the fundamental archive format of the Stunts engine.

### Container Header

Two header variants exist (auto-detected by the engine):

**6-byte header variant:**
```
Offset  Size   Field
0x00    4      uint32   Total data size (bytes)
0x04    2      uint16   Number of sub-chunks (N)
0x06    N*4    char[4]  Chunk name table (4 chars each, space-padded)
0x06+N*4 N*4   uint32   Chunk offset table (relative to end of header)
header_end = 6 + N*8    Data area starts here
```

**10-byte header variant:**
```
Offset  Size   Field
0x00    4      uint32   Total data size (bytes)
0x04    2      uint16   Chunks (6-byte mode count)
0x06    2      uint16   Reserved
0x08    2      uint16   Chunks (10-byte mode count)
0x0A    N*4    char[4]  Chunk name table
0x0A+N*4 N*4   uint32   Chunk offset table
header_end = 10 + N*8   Data area starts here
```

### Name Lookup

Names are 4 characters, space-padded. Short names have trailing spaces:
- `"simd"` → `'s','i','m','d'`
- `"car"` → `'c','a','r',' '`

**Functions:** `locate_resource()`, `locate_shape_alt()`, `locate_shape_fatal()`, `locate_shape_nofatal()`, `locate_text_res()` (in `memmgr.c`)

**Text resources:** `locate_text_res()` prepends the `textresprefix` character (default `'e'` for English) to a 3-char name, e.g. `locate_text_res(ptr, "wai")` looks up `"ewai"`.

---

## 2. RES Files

**Extension:** `.RES`  
**Files:** `MAIN.RES`, `CRED.RES`, `OPP1.RES`, `CAR*.RES` (e.g. `CARCOUN.RES`, `CARANSX.RES`, `CARFGTO.RES`...)  
**Format:** Uncompressed named-chunk container  

### Purpose

General-purpose resource archives containing named sub-resources of any type: text strings, 2D shapes, 3D shapes, car physics data (`simd`), game parameters, etc.

### Loading

```c
void* file_load_resfile(const char* filename);  // tries .res then .pre
void* file_load_resource(int type, const char* filename);
// type 0,1 = raw binary
// type 2,8 = shape2d (PVS/PES)
// type 3   = shape2d from .res/.pre
// type 4   = KMS song
// type 5   = VCE voice
// type 6   = SFX
// type 7   = compressed (file_decomp)
```

### MAIN.RES Contents

Contains UI text resources accessed by 3-char IDs via `locate_text_res()`:
- `"wai"` — "Please wait..." dialog
- `"key"` — Keyboard configuration dialog
- `"mou"` — Mouse configuration dialog  
- `"pau"` — Pause dialog
- `"dos"` — Quit to DOS dialog
- `"mer"` — Memory error
- `"dea"` — Disk error (abort)
- `"der"` — Disk error (retry)
- `"hs1"`..`"hs5"` — High score text strings
- `"sav"` — Save dialog
- `"loa"` — Load dialog
- `"joy"` — Joystick dialog
- `"son"` — Sound on/off
- `"mon"`/`"mof"` — Music on/off
- `"mrl"` — Menu resource list

### CAR*.RES Contents

Named chunks inside `CARxxxx.RES`:
- `"simd"` — Car physics simulation data (`SIMD_RESOURCE` struct, see §20)
- `"gnam"` — Car full name string (player)
- `"gsna"` — Car short name string (opponent)
- 3D shape sub-resources for the car model

### OPP*.RES Contents

Opponent driver resources containing:
- `"nam"` — Opponent name string
- `"sped"` — Speed table data

### GAME.RES Contents

- `"plan"` — Track plan/path data (`PLANE` struct)
- `"wall"` — Wall collision data
- `"rbf"` — Replay buffer full dialog
- `"cop"` — Copyright text
- Various game text resources

---

## 3. PRE Files

**Extension:** `.PRE`  
**Files:** `GAME.PRE`, `OPP1.PRE`..`OPP6.PRE`, `MISC.PRE`, `TEDIT.PRE`  
**Format:** RLE/VLE compressed named-chunk container  

### Purpose

Compressed resource archives. Identical internal structure to `.RES` after decompression.

### Loading

```c
void* file_decomp(const char* filename, int fatal);  // decompresses in-place
void* file_load_resfile(const char* filename);        // tries .res then .pre
```

The decompression is handled by `file_decomp()` which detects the compression type from the first byte (see §23).

### MISC.PRE Contents

- `"elt"` — Element text
- `"con"` — Continue text  
- `"ppt"` — Performance text

### TEDIT.PRE Contents

Track editor resources with UI shapes and track element icons.

---

## 4. PVS Files

**Extension:** `.PVS`  
**Files:** `SDMAIN.PVS`, `ALPINE.PVS`, `CITY.PVS`, `COUNTRY.PVS`, `DESERT.PVS`, `OPP*WIN.PVS`, `OPP*LOSE.PVS`, `SDCSEL.PVS`  
**Format:** RLE/VLE compressed SHAPE2D container  

### Purpose

Pre-rendered 2D scene/screen images stored as compressed SHAPE2D bitmaps. Used for backgrounds, splash screens, scenery, and pre-rendered UI.

### Internal Structure

After decompression, a PVS contains a SHAPE2D resource chunk (see §18). The pixel data may be **column-major transposed** ("flipped") and requires un-flipping via `file_unflip_shape2d()`.

### Loading

```c
void* file_load_shape2d(char* shapename, int fatal);
// Tries extensions: .PVS, .XVS, .PES, .ESH, .VSH
// For PVS: decompress → unflip → expand → palette remap
```

### SDMAIN.PVS Contents

Special resource containing:
- `"!pal"` — 256-color VGA palette (768 bytes = 256 × 3 RGB6 values)
- `"smou"` — Small mouse cursor shape
- `"mmou"` — Medium mouse cursor shape

**Palette format:** 768 bytes, 256 entries × 3 bytes (R, G, B), each value 0-63 (VGA 6-bit DAC format).

### Scenery PVS Files

`ALPINE.PVS`, `CITY.PVS`, `COUNTRY.PVS`, `DESERT.PVS` — Scenery backdrop shapes selected by the terrain type byte at `td14_elem_map_main[0x384]`.

---

## 5. PES Files

**Extension:** `.PES`  
**Files:** `SDCRED.PES`, `SDTEDIT.PES`  
**Format:** RLE/VLE compressed SHAPE2D container (planar-encoded variant)  

### Purpose

Similar to PVS but uses a planar pixel encoding (4-plane EGA/VGA style). Used for credit screens and track editor UI.

### Un-flipping

Uses `file_unflip_shape2d_pes()` which handles per-plane transposition:
```c
// For each of 4 bitplanes (controlled by s2d_plane_nibbles):
//   Transpose height×width → width×height per plane
```

---

## 6. P3S Files

**Extension:** `.P3S`  
**Files:** `GAME1.P3S`, `GAME2.P3S`, `ST*.P3S` (car shapes), `TITLE.P3S`  
**Format:** RLE/VLE compressed named-chunk container of 3D shapes  

### Purpose

3D polygon model archives. Contains named SHAPE3D sub-resources for track objects, car bodies, wheels, and scene elements.

### Loading

```c
void* file_load_3dres(const char* filename);
// Tries .p3s then .3sh extensions
// Returns decompressed container

int shape3d_load_all(void);
// Loads GAME1.P3S and GAME2.P3S
// Initializes game3dshapes[0..0x73] (116 track shapes)
```

### GAME1.P3S / GAME2.P3S

Contain 116 named track object shapes. Shape names are 4 characters stored in the `track_object_shape_names[]` lookup table at 5-byte stride (4 chars + NUL). Shapes are first searched in GAME1, fallback to GAME2.

### Car Shape P3S Files

`ST*.P3S` files (e.g. `STCOUN.P3S`) contain:
- `"car1"` — Player car body shape
- `"car2"` — Opponent car body shape  
- `"exp0"` — Explosion/crash shape
- Wheel shapes

---

## 7. TRK Files

**Extension:** `.TRK`  
**Files:** `DEFAULT.TRK`, `CTRACK01.TRK`..`CTRACK11.TRK`, user tracks  
**Format:** Raw binary, uncompressed  

### Purpose

Track layout data defining the 30×30 grid of track elements and terrain.

### Structure

```
Total size: 0x70A bytes (1802 bytes) = TRACK_FILE_SAVE_BYTES

Offset  Size    Field
0x000   0x384   Element map — 900 bytes (30×30 grid)
                Each byte is a track element ID (index into trkObjectList)
                Grid indexed as: trackrows[row] + col
                Row 0 = top (North), stored bottom-up in trackrows[]
0x384   1       Scenery/terrain type byte
                (selects ALPINE/CITY/COUNTRY/DESERT backdrop)
0x385   0x385   Terrain map — 901 bytes (30×30 grid + 1)
                Each byte is a terrain type:
                  1-5 = flat variants
                  6   = water
                  7-10 = hill variants
```

### Constants

```c
TRACK_SIZE = 30                       // Grid dimension
TRACK_TILE_COUNT = 900                // 30 × 30
TRACK_TRACKDATA_ELEM_BYTES = 0x384    // 900 bytes element map
TRACK_TRACKDATA_TOTAL_BYTES = 0x385   // 901 bytes per map section
TRACK_FILE_SAVE_BYTES = 0x70A         // Total file size
TRACK_ELEM_MAX_VALID = 0xB6           // Highest valid element ID
```

### Track Grid Coordinate System

```c
// Row lookup (Y-axis inverted):
trackrows[row] = 30 * (29 - row)       // row 0 = bottom of grid
terrainrows[row] = 30 * row             // terrain indexed normally

// World coordinates:
trackpos[i] = (29 - i) << 10           // World position (1024 per cell)
trackcenterpos[i] = trackpos[i] + 0x200 // Center of cell (+512)
STN_TRACK_CELL_SHIFT = 10              // Bits per cell (1024 units)
STN_TRACK_CELL_CENTER_OFFSET = 0x200   // Half-cell offset
```

### Track Element IDs

- `0x00` = Empty
- Multi-tile elements use marker bytes: `0xFD` (corner), `0xFE` (vertical), `0xFF` (horizontal)
- Start/finish elements detected in `track_setup()` Phase 5

### Track Validation

`track_setup()` in `track.c` performs 10+ phases:
1. Allocate `tcomp` buffer (64 × 14 = 896 bytes)
2. Initialize visit arrays
3. West-to-East terrain connectivity check
4. North-to-South terrain connectivity check
5. Scan for start/finish tiles
6-10. Path tracing, checkpoint validation, waypoint generation

---

## 8. HIG Files

**Extension:** `.HIG`  
**Files:** `DEFAULT.HIG`, `CTRACK01.HIG`..`CTRACK11.HIG`, user track `.HIG` files  
**Format:** Raw binary, uncompressed  

### Purpose

High score tables, one per track (matched by track name).

### Structure

```
Total size: 0x16C bytes (364 bytes) = 7 entries × 0x34 bytes each

Each entry (0x34 = 52 bytes):
Offset  Size    Field
0x00    17      Player name (null-terminated, '.' padded)
0x11    24      Additional text/info ("......................")
0x29    1       Null terminator
0x2A    7       Date string ("../....", e.g. "03/2026")
0x32    2       uint16  Score (finish time in frames, 0xFFFF = empty)
```

### Loading / Saving

```c
unsigned short highscore_write_a_(unsigned short write_defaults);
// write_defaults=0: reads from file via file_read_with_mode(0x000A, ...)
// write_defaults=1: initializes 7 empty entries

unsigned short highscore_write_b_(void);
// Writes sorted entries (7 × 0x34 bytes = 0x16C) to .hig file

unsigned short enter_hiscore_(unsigned short score, ...);
// Inserts a new score, shifts lower entries down
```

### Score Indexing

```c
unsigned short highscore_primary_index[7];  // Sorting permutation
// Entries are stored sorted by score (ascending frame count = faster time)
```

---

## 9. RPL Files

**Extension:** `.RPL`  
**Files:** `DEFAULT.RPL`, user replay files  
**Format:** Raw binary, uncompressed  

### Purpose

Recorded input replay data for race playback.

### Structure

```
Offset  Size         Field
0x00    0x1A (26)    GAMEINFO header (race configuration)
0x1A    0x385 (901)  Element map copy (track data)
0x39F   0x385 (901)  Terrain map copy
0x724   variable     Replay input buffer (gameconfig.game_recordedframes bytes)

Total: 0x1A + 0x385 + 0x385 + recorded_frames
     = 26 + 901 + 901 + N
```

### GAMEINFO Header (26 bytes)

```c
struct GAMEINFO {           // Total: 26 bytes (0x1A)
    char game_playercarid[4];       // Player car ID (e.g. "COUN")
    char game_playermaterial;        // Material/paint index
    char game_playertransmission;    // 0=auto, 1=manual
    char game_opponenttype;          // 0=none, 1-6=opponent
    char game_opponentcarid[4];      // Opponent car ID
    char game_opponentmaterial;
    char game_opponenttransmission;
    char game_trackname[9];          // Track name (null-terminated)
    unsigned short game_framespersec; // FPS (10 or 20)
    unsigned short game_recordedframes; // Total frames recorded
};
```

### Input Buffer

Each byte in `td16_rpl_buffer[]` encodes the player's input for one frame:
- `0` = no input / waiting
- `1` = accelerate
- `2` = brake
- Bitfield encoding for steering + throttle

### Loading / Saving

```c
short file_load_replay(const char* dir, const char* name);
// Reads .rpl into td13_rpl_header (which is contiguous with td14/td15/td16)

short file_write_replay(const char* filename);
// Writes: 0x1A + 0x385 + 0x385 + game_recordedframes bytes
```

---

## 10. KMS Files

**Extension:** `.KMS`  
**Files:** `SKIDOVER.KMS`, `SKIDSLCT.KMS`, `SKIDMS.KMS` (via audio resource names)  
**Format:** Named-chunk container with VLQ-encoded bytecode tracks  

### Purpose

Music song data for the game's sequencer. KMS is a custom tracker format using Variable-Length-Quantity (VLQ) encoded note events and command opcodes.

### Container Structure

KMS files use the standard named-chunk container format:
```
Header: u16 total_size, u16 reserved, u16 count
Name table: count × 4-char names
Offset table: count × u32 LE offsets
Data area: chunk records
```

Each named chunk (e.g. `"TITL"`, `"SLCT"`, `"VICT"`, `"OVER"`, `"DEMO"`) contains sub-tracks.

### Sub-chunk Navigation

Each chunk is a nested container with `T0S0`, `T1S0`, etc. sub-entries:
```
chunk[0..3] = u32 LE record_size
chunk[4..5] = u16 LE sub_count
Sub-names: sub_count × 4 chars (e.g. "T0S0")
Sub-offsets: sub_count × u32 LE
Sub-data records: each starts with u32 LE record_size, then bytecodes
```

### KMS Bytecode Format

```
[VLQ delta] [event_byte] [optional velocity] [VLQ duration]

Event types:
  0x00-0xD8: Note events
    - pitch = event & 0x7F (MIDI note number)
    - If event >= 0x80: 1 velocity byte follows
    - VLQ duration follows

  0xD9-0xEA: Command opcodes
    D9 = END (stop)                   0 extra bytes
    DA, DB = control                  0 extra bytes
    DC, DD, DE = param commands       1 extra byte each
    DF = 2-byte param                 2 extra bytes
    E0, E1, E2 = 1-byte params       1 extra byte each
    E3 = control                      0 extra bytes
    E4 = 1-byte param                 1 extra byte
    E5 = 2-byte param                 2 extra bytes
    E6 = 5-byte param                 5 extra bytes
    E7, E8 = track separators         variable (stop parsing)
    E9, EA = 1-byte params            1 extra byte each

VLQ encoding: 7 bits per byte, MSB = continuation flag
  byte & 0x80 → more bytes follow
  byte & 0x7F → data bits
```

### Music Tick Rate

```c
#define AUDIO_KMS_MUSIC_HZ 48  // 48 ticks/sec for DD param=120
// Formula: 100 * 128 * param / 32000
```

### Menu Music Track Names

Recognised as menu tracks: `"TITL"`, `"SLCT"`, `"VICT"`, `"OVER"`, `"DEMO"`

### Related Functions

```c
audio_extract_menu_resource_notes(void* songptr);
audio_kms_parse_track(data, start, end, tmp, max, out_loop_ticks);
audio_kms_read_vlq(data, pos, end);
```

---

## 11. VCE Files

**Extension:** `.VCE`  
**Files:** `ADENG1.VCE`, `MTENG1.VCE`, `PCENG1.VCE`, `ADSKIDMS.VCE`, `MTSKIDMS.VCE`, `PCSKIDMS.VCE`  
**Format:** Named-chunk container of instrument definitions  

### Purpose

Voice/instrument data for the audio engine. Contains OPL2 FM synthesis parameters and frequency mapping tables.

### Naming Convention

Prefix determines audio driver:
- `AD` = AdLib / SoundBlaster
- `MT` = MT-32  
- `PC` = PC Speaker

### Container Structure

Same container format as other resources. Each named instrument is a variable-length record.

### Instrument Record Structure

```c
typedef struct AUDIO_VCE_INSTRUMENT {
    unsigned char valid;
    unsigned char has_fm;           // 1 = full OPL2 FM params (ADENG1.VCE)
    char name[5];                   // 4-char instrument name + NUL
    unsigned short record_size;
    unsigned char freq_div;         // rec[0x0E]: RPM-to-Fnum divisor
    unsigned char freq_base;        // rec[0x0F]: RPM-to-Fnum base multiplier
    unsigned char opl_con;          // rec[0x44]: connection (0=FM, 1=AM)
    unsigned char opl_fb;           // rec[0x45]: feedback (0-7)
    unsigned char op0[12];          // rec[0x46]: Modulator: AR,DR,SL,RR,TL,KSL,MULT,KSR,EGT,VIB,AM,WS
    unsigned char op1[12];          // rec[0x52]: Carrier:   AR,DR,SL,RR,TL,KSL,MULT,KSR,EGT,VIB,AM,WS
    unsigned char brightness;       // rec[34] >> 4 & 0x0F
} AUDIO_VCE_INSTRUMENT;
```

### RPM-to-Frequency Mapping

```
Fnum = RPM / freq_div + freq_base * 16
Default: freq_div=6 for PCENG1 ENGI instrument
```

### OPL2 FM Parameters (record_size >= 0x5E)

Full FM synthesis parameters at offsets 0x44-0x5D:
- `rec[0x44]` = Connection type (0=FM, 1=Additive)
- `rec[0x45]` = Operator feedback level
- `rec[0x46..0x51]` = Operator 0 (modulator): 12 params
- `rec[0x52..0x5D]` = Operator 1 (carrier): 12 params

### Known Instrument Names

- `"ENGI"` — Engine sound
- `"SKID"` — Tire squeal
- `"SCRA"` — Tire scrape
- `"CRAS"` — Crash sound
- `"BLOW"` — Blowout
- `"BUMP"` — Bump impact

### Loading

```c
unsigned short audio_parse_vce_instruments(void* voiceptr);
// Populates g_audio_vce_instruments[] (max 64 instruments)
```

---

## 12. SFX Files

**Extension:** `.SFX`  
**Files:** `GEENG.SFX`, `ADENG.SFX`, `MTENG.SFX`, `PCENG.SFX`  
**Format:** KMS container with named sound effect chunks  

### Purpose

Sound effect data for engine sounds, crashes, skids, etc. Each chunk contains nested KMS bytecode tracks.

### Naming Convention

Prefix `GE` = General Engine (fallback), or driver-specific prefix (`AD`/`MT`/`PC`).

### Loading Priority

```c
// 1. Try driver-specific prefix: e.g. "adeng.sfx" for SoundBlaster
// 2. Fallback to "geeng.sfx" (general engine)
// 3. Last resort: unprefixed
```

### Internal Structure

Same KMS container format as song files. Each named chunk (4-char name) contains:
- A nested sub-container with `T0S0` bytecode track
- Duration computed from bytecode scanning

```c
#define AUDIO_SFX_MAX_CHUNKS 32

// Parsed into:
char g_audio_sfx_chunk_name[32][5];      // Chunk names
unsigned short g_audio_sfx_chunk_ofs[32]; // Byte offsets
unsigned short g_audio_sfx_chunk_end[32]; // End offsets
unsigned char g_audio_sfx_chunk_duration[32]; // Duration in ticks
```

---

## 13. FNT Files

**Extension:** `.FNT`  
**Files:** `FONTDEF.FNT`, `FONTLED.FNT`, `FONTN.FNT`  
**Format:** Binary font definition + bitmap data  

### Purpose

Bitmap font files for text rendering at different sizes and styles.

### Font Definition Layout

```
Offset  Size   Field
0x00    2      uint16  Foreground color (set by font_set_colors)
0x02    2      uint16  Background color
0x04    4      Reserved
0x08    2      uint16  Cursor X position
0x0A    2      uint16  Cursor Y position
0x0C    1      uint8   Proportional mode:
                         0 = fixed-width
                         1 = proportional (if prop1_width==0, per-char width byte)
                         2 = per-character width byte
0x0E    2      uint16  Font height (pixels)
0x10    2      uint16  Proportional mode 1 fixed width
0x12    2      uint16  Default character width
0x14    2      Reserved
0x16    512    uint16[256]  Character offset table
                Each entry points into font data; 0 = char not in font
...            Glyph bitmap data (addressed by char table offsets)
```

### Proportional Mode Width

When `proportional == 2` or `(proportional == 1 && prop1_width == 0)`:
- The first byte at each character's offset is the character width
- Followed by bitmap data

When `proportional == 1 && prop1_width != 0`:
- All characters use `prop1_width` as fixed width

When `proportional == 0`:
- All characters use `default_charwidth` (offset 0x12)

### Functions

```c
void font_set_fontdef2(void* data);      // Set active font
void font_set_fontdef(void);             // Use default fontdefptr
void set_fontdefseg(void* data);         // Set raw font pointer
void font_set_colors(int fg, unsigned short bg);
unsigned short font_get_text_width(char* text);
void font_draw_text(char* str, unsigned short x, unsigned short y);
int font_get_centered_x(char* text);
```

### Font Files

- **FONTDEF.FNT** — Default UI font (loaded at startup)
- **FONTLED.FNT** — LED-style font for dashboard instruments (loaded during gameplay)
- **FONTN.FNT** — Narrow/alternate font

---

## 14. DAT Files

**Extension:** `.DAT` / `.cfg`  
**Files:** `SETUP.DAT` (original), `stunts.cfg` (modern engine)  

### Purpose

Persistent game configuration storage.

### Modern Format (stunts.cfg)

```c
struct STN_PERSIST_CONFIG {
    char magic[8];              // "STNCFG01"
    unsigned short version;     // STN_PERSIST_VERSION = 1
    struct GAMEINFO gameconfig; // 26 bytes — current race setup
    char track_path[82];        // Track highscore path buffer
    char replay_path[82];       // Replay file path buffer
    char mouse_mode;            // Mouse motion state flag
    char joystick_mode;         // Joystick assigned flags
};
```

---

## 15. SHAPE2D Format

### SHAPE2D Header (16 bytes)

```c
struct SHAPE2D {                    // 16 bytes total
    unsigned short s2d_width;       // Width in pixels (or bytes for planar)
    unsigned short s2d_height;      // Height in pixels
    unsigned short s2d_hotspot_x;   // Hotspot X offset
    unsigned short s2d_hotspot_y;   // Hotspot Y offset
    unsigned short s2d_pos_x;       // Default draw position X
    unsigned short s2d_pos_y;       // Default draw position Y
    unsigned char s2d_plane_nibbles[4]; // Plane/encoding metadata
};
// Pixel data immediately follows the header
```

### Plane Nibbles

`s2d_plane_nibbles[0..3]` encode bitplane masks and transposition flags:
- Lower nibble of each byte: bitplane presence mask
- `s2d_plane_nibbles[2] >> 4`: transposition/flip mode (0-3)
- `s2d_plane_nibbles[3] & 0xF0`: if non-zero, skip transposition

### SHAPE2D Resource Chunk Format

Multiple shapes stored in a resource chunk:

```
Offset  Size       Field
0x00    4          uint32   Total chunk size
0x04    2          uint16   Shape count (N)
0x06    N*4        uint16[N*2]  Shape ID table (N pairs)
0x06+N*4  N*4      uint32[N]    Offset table (relative to data area)
data_area:          SHAPE2D headers + pixel data
```

### Accessing Shapes

```c
unsigned short shape2d_resource_shape_count(const unsigned char* memchunk);
struct SHAPE2D* shape2d_resource_get_shape(unsigned char* memchunk, int index);
// Computes: offset = header_size + offsets[index]
//           data_start = 6 + count*8
//           shape_ptr = memchunk + data_start + offset
```

### SPRITE Structure

Runtime wrapper for rendering SHAPE2D data:

```c
struct SPRITE {
    struct SHAPE2D* sprite_bitmapptr;  // Pointer to bitmap data
    unsigned short sprite_unk1;
    unsigned short sprite_unk2;
    unsigned short sprite_unk3;
    unsigned int* sprite_lineofs;      // Line offset table for clipping
    unsigned short sprite_left;
    unsigned short sprite_right;
    unsigned short sprite_top;
    unsigned short sprite_height;
    unsigned short sprite_pitch;       // Row stride in bytes
    unsigned short sprite_unk4;
    unsigned short sprite_width2;
    unsigned short sprite_left2;
    unsigned short sprite_widthsum;
};
```

### Rendering Modes

- **Copy** — Direct pixel copy
- **AND** — Bitwise AND (masking)
- **OR** — Bitwise OR (overlay)
- **RLE** — Run-length encoded rendering
- **Clipped** — Viewport-bounded rendering

---

## 16. SHAPE3D Format

### SHAPE3D Header (4 bytes)

```c
struct SHAPE3DHEADER {
    unsigned char header_numverts;       // Number of vertices
    unsigned char header_numprimitives;  // Number of primitives
    unsigned char header_numpaints;      // Number of paint entries
    unsigned char header_reserved;       // Reserved/padding
};
```

### Memory Layout After Header

```
[4-byte header]
[numverts × 6 bytes]       — Vertex array (VECTOR: 3 × int16)
[numprimitives × 4 bytes]  — Cull table 1 (back-face culling data)
[numprimitives × 8 bytes]  — Cull table 2 (additional culling)
[variable]                  — Primitive descriptors
```

### Constants

```c
#define SHAPE3D_HEADER_SIZE_BYTES     4
#define SHAPE3D_VERTEX_SIZE_BYTES     6   // struct VECTOR = 3 × short
#define SHAPE3D_CULL_ENTRY_SIZE_BYTES 4
#define SHAPE3D_PRIMITIVE_SIZE_BYTES  8
#define SHAPE3D_TOTAL_SHAPES          130
#define SHAPE3D_TRACK_SHAPE_COUNT     0x74  // 116 track shapes
```

### SHAPE3D Runtime Structure

```c
struct SHAPE3D {
    unsigned short shape3d_numverts;
    struct VECTOR* shape3d_verts;           // Vertex positions
    unsigned short shape3d_numprimitives;
    unsigned short shape3d_numpaints;
    char* shape3d_primitives;               // Primitive descriptors
    char* shape3d_cull1;                    // Back-face cull data
    char* shape3d_cull2;                    // Additional cull data
};
```

### Primitive Types

```c
#define PRIMITIVE_TYPE_POLYGON  0   // Filled polygon
#define PRIMITIVE_TYPE_LINE     1   // Line segment
#define PRIMITIVE_TYPE_SPHERE   2   // Sphere (for wheels/lights)
#define PRIMITIVE_TYPE_WHEEL    3   // Wheel (special rendering)
#define PRIMITIVE_TYPE_PIXEL    5   // Single pixel
```

### Transformed Shape (for Rendering)

```c
struct TRANSFORMEDSHAPE3D {
    struct VECTOR pos;                    // World position
    struct SHAPE3D* shapeptr;             // Shape data
    struct RECTANGLE* rectptr;            // Clip rectangle
    struct VECTOR rotvec;                 // Rotation angles (x, y, z)
    unsigned short shape_visibility_threshold;
    unsigned char ts_flags;               // Transform flags
    unsigned char material;               // Material/paint index
};
```

---

## 17. SIMD Format — Car Physics Data

### On-Disk Format (SIMD_RESOURCE)

```c
struct SIMD_RESOURCE {               // Binary layout in .RES files
    char num_gears;                  // Number of gears (max 7)
    char simd_reserved;
    int16_t car_mass;                // Car mass
    int16_t braking_eff;             // Braking efficiency
    int16_t idle_rpm;                // Idle RPM
    int16_t downshift_rpm;           // Downshift threshold RPM
    int16_t upshift_rpm;             // Upshift threshold RPM
    int16_t max_rpm;                 // Maximum RPM
    uint16_t gear_ratios[7];         // Gear ratio table
    struct POINT2D_DOS knob_points[7]; // Transmission knob positions
    int16_t aero_resistance;         // Aerodynamic drag coefficient
    char idle_torque;                // Idle torque
    char torque_curve[104];          // Torque curve (104 sampling points)
    char simd_reserved_a3;
    int16_t grip;                    // Tire grip factor
    int16_t simd_reserved_a6[7];
    int16_t sliding;                 // Sliding friction
    int16_t surface_grip[4];         // Per-surface-type grip (4 surfaces)
    char simd_unk3[10];
    struct POINT2D_DOS collide_points[2]; // Collision bounding box corners
    int16_t car_height;              // Car height
    struct VECTOR wheel_coords[4];   // Wheel positions (4 wheels × 3 coords)
    char steeringdots[62];           // Steering response curve
    struct POINT2D_DOS spdcenter;    // Speedometer center position
    int16_t spdnumpoints;            // Speedometer needle points count
    char spdpoints[208];             // Speedometer needle geometry
    struct POINT2D_DOS revcenter;    // Tachometer center position
    int16_t revnumpoints;            // Tachometer needle points count
    char revpoints[256];             // Tachometer needle geometry
    uint32_t aerorestable_dos;       // DOS segment pointer (unused)
};
```

### Aero Resistance Table

Computed at runtime from `aero_resistance`:
```c
// 64 entries: drag force for speeds 0..63
for (i = 0; i < 0x40; i++) {
    aerotable[i] = (aero_resistance * i * i) >> 9;
}
```

### Named Sub-Resources in CAR*.RES

- `"simd"` — SIMD_RESOURCE binary data
- `"gnam"` — Full car name (null-terminated string)
- `"gsna"` — Short car name

---

## 18. GAMEINFO Format

```c
struct GAMEINFO {                    // 26 bytes (0x1A), packed
    char game_playercarid[4];        // e.g. "COUN", "ANSX", "FGTO"
    char game_playermaterial;         // Paint job index
    char game_playertransmission;     // 0=automatic, 1=manual
    char game_opponenttype;           // 0=none, 1-6=OPP1-OPP6
    char game_opponentcarid[4];       // 0xFF in [0] = auto-select
    char game_opponentmaterial;
    char game_opponenttransmission;
    char game_trackname[9];           // Null-terminated track name
    unsigned short game_framespersec; // 10 or 20
    unsigned short game_recordedframes; // Replay frame count
};
```

---

## 19. GAMESTATE Format

The full runtime state saved to CVX checkpoints every `fps_times_thirty` frames:

```c
struct GAMESTATE {                   // ~1200+ bytes
    int32_t game_longs1[24];         // Obstacle X positions
    int32_t game_longs2[24];         // Obstacle Y positions
    int32_t game_longs3[24];         // Obstacle Z positions
    struct VECTOR game_vec1[2];      // Player[0] and opponent[1] positions
    struct VECTOR game_vec3;         // Camera target
    struct VECTOR game_vec4;         // Camera position
    short game_frame_in_sec;
    short game_frames_per_sec;
    int32_t game_travDist;           // Travel distance
    short game_frame;                // Current frame counter
    short game_total_finish;         // Finish time + penalty
    short game_frame_update_target;
    short game_pEndFrame;            // Player finish frame
    short game_oEndFrame;            // Opponent finish frame
    short game_penalty;              // Penalty counter
    unsigned short game_impactSpeed; // Maximum impact speed
    unsigned short game_topSpeed;    // Maximum speed achieved
    short game_jumpCount;            // Number of jumps
    struct CARSTATE playerstate;     // Full player car state
    struct CARSTATE opponentstate;   // Full opponent car state
    short game_current_waypoint_index;
    short game_track_segment_working_index;
    short game_startcol, game_startcol2;
    short game_startrow, game_startrow2;
    short game_obstacle_rotx[24];    // Obstacle rotations
    short game_obstacle_roty[24];
    short game_obstacle_rotz[24];
    short game_obstacle_active[24];
    char game_obstacle_metadata[48];
    char kevinseed[6];               // PRNG seed for determinism
    char game_checkpoint_valid;
    char game_inputmode;             // 0=waiting, 1=active, 2=no input
    char game_3F6autoLoadEvalFlag;
    char game_track_indices[2];      // Player[0], opponent[1]
    char game_track_lookup_temp;
    char game_obstacle_flags[48];
    char game_obstacle_count;
    char game_obstacle_shape[24];
    char game_obstacle_status[24];
    char game_flyover_state;
    char game_flyover_counter;
    char game_collision_type;
    char game_flyover_check;
    char game_misc_state_counter;
};
```

### CVX Checkpoint Buffer

```c
#define RST_CVX_NUM 20  // Max checkpoint slots
// Allocated as: sizeof(GAMESTATE) * 20
// Checkpoint saved every fps_times_thirty frames (= fps * 30)
```

---

## 20. Compression Formats

All compressed files (`.PVS`, `.PES`, `.P3S`, `.PRE`) use the same multi-pass decompression system.

### File Header

```
Byte 0 (method byte):
  bit 7 set:  Multi-pass. Passes = byte0 & 0x7F. Then byte 4 = first pass type.
  bit 7 clear: Single pass. byte0 = compression type (1 or 2).
```

### Compression Type 1 — RLE (Run-Length Encoding)

```
Offset  Size     Field
0x00    1        Type byte (0x01)
0x01    3        uint24 LE — Decompressed size
0x04    3        uint24 LE — Compressed data size
0x07    1        Reserved
0x08    1        Escape count (low 7 bits) + sequence flag (bit 7)
0x09    esclen   Escape byte lookup table
0x09+esclen      Compressed data

Single-byte RLE: escape bytes trigger run encoding:
  escval=1: [count][value] — repeat value count times
  escval=3: [count_lo][count_hi][value] — 16-bit count
  escval>1: repeat (escval-1) times

Sequence RLE (bit 7 of byte 8 clear):
  First decompressed with sequence escape, then single-byte RLE
```

### Compression Type 2 — VLE (Variable-Length Encoding / Huffman)

```
Offset  Size     Field
0x00    1        Type byte (0x02)
0x01    3        uint24 LE — Decompressed size
0x04    1        Escape length + additive flag (bit 7)
                 Alphabet symbol table follows

Huffman-style bit-packed encoding:
  - Width distribution builds a symbol decode table
  - Symbols shorter than 8 bits use direct lookup
  - Longer symbols use bit-by-bit tree traversal
  - Additive mode: output[i] = prev_symbol + decoded_symbol
```

### Multi-Pass Decompression

```
Byte 0 = 0x80 | pass_count
Bytes 1-3 = ignored (total size hint)
Byte 4 onward = first pass data

Each pass output becomes the next pass input.
Pass types: 1 (RLE) or 2 (VLE), any combination.
Max passes: 8
```

---

## Track Data Memory Layout

The engine allocates a contiguous `STN_TRACKDATA_TOTAL_SIZE` (0x6BF3 = 27,635 bytes) block partitioned as:

```
Offset   Size    Variable                 Purpose
0x0000   0x70A   td01_track_file_cpy      Track file copy (element + terrain maps)
0x070A   0x70A   td02_penalty_related     Penalty arrow data
0x0E14   0x70A   trackdata3               Waypoint lookup indices
0x151E   0x080   td04_aerotable_pl        Player aero drag table (64 entries)
0x159E   0x080   td05_aerotable_op        Opponent aero drag table
0x161E   0x080   trackdata6               
0x169E   0x080   trackdata7               
0x171E   0x060   td08_direction_related   Track direction data
0x177E   0x180   trackdata9               
0x18FE   0x120   td10_track_check_rel     Track validation
0x1A1E   0x16C   td11_highscores          High score table (7 × 52 bytes)
0x1B8A   0x0F0   trackdata12              
0x1C7A   0x01A   td13_rpl_header          Replay GAMEINFO header
0x1C94   0x385   td14_elem_map_main       Track element map (30×30 + 1)
0x2019   0x385   td15_terr_map_main       Terrain map (30×30 + 1)
0x239E   0x2EE0  td16_rpl_buffer          Replay input buffer (~12,000 bytes)
0x527E   0x385   td17_trk_elem_ordered    Ordered track elements (path)
0x5603   0x385   trackdata18              
0x5988   0x385   trackdata19              Used during track validation
0x5D0D   0x7AC   td20_trk_file_appnd      Track + path buffer
0x64B9   0x385   td21_col_from_path       Column indices from path
0x683E   0x385   td22_row_from_path       Row indices from path
0x6BC3   0x030   trackdata23              Track object indices (48 bytes)
```

---

## Track Object Data

### TRACKOBJECT (14 bytes each, 215 entries in trkObjectList)

```c
struct TRACKOBJECT {
    struct TRKOBJINFO* ss_trkObjInfoPtr; // Pointer to shape info
    short ss_rotY;                       // Y rotation (horizontal)
    struct SHAPE3D* ss_shapePtr;         // 3D shape (hi-detail)
    struct SHAPE3D* ss_loShapePtr;       // 3D shape (lo-detail)
    unsigned char ss_ssOvelay;           // Overlay flag
    char ss_surfaceType;                 // Surface paint type
    char ss_ignoreZBias;                 // Z-bias override
    char ss_multiTileFlag;               // 0=1-tile, 1=2V, 2=2H, 3=4-tile
    char ss_physicalModel;               // Physics model type
    char scene_unk5;                     // Always zero
};
```

### TRKOBJINFO (14 bytes each)

```c
struct TRKOBJINFO {
    char si_noOfBlocks;       // Sub-blocks composing the element
    char si_entryPoint;       // Tile connectivity entry
    char si_exitPoint;        // Tile connectivity exit
    char si_entryType;        // Element type connectivity entry
    char si_exitType;         // Element type connectivity exit
    char si_arrowType;        // Penalty arrow behavior type
    short si_arrowOrient;     // Arrow orientation angle
    short* si_cameraDataOffset; // Camera position data
    char si_opp1;             // Opponent AI approach parameter
    char si_opp2;
    char si_opp3;
    char si_oppSpedCode;      // Opponent speed code
};
```

---

## Palette Format

```c
#define STN_PALETTE_BYTES  0x300   // 768 bytes
#define STN_PALETTE_COLORS 0x100   // 256 colors

// Format: 256 × 3 bytes (R, G, B), each 0-63 (VGA 6-bit DAC)
// Stored in SDMAIN.PVS as sub-resource "!pal"
// Shape dimensions encode total byte count: width × height = 768

// Setting palette:
video_set_palette(0, 256, palette);  // start_index, count, data
```

---

## Screen Constants

```c
#define STN_SCREEN_WIDTH     0x140   // 320 pixels
#define STN_SCREEN_HEIGHT    0xC8    // 200 pixels
#define STN_SCREEN_DEPTH     0x0F    // 15 (bit depth parameter)
#define STN_CLIPRECT_BOTTOM  0x5F    // 95 (3D viewport bottom)
#define STN_DASH_REPLAYBAR_Y 0x97    // 151 (replay bar Y position)
#define STN_FPS_DEFAULT      10      // Default frame rate
#define STN_FPS_ALT          20      // Alternative frame rate
```

---

## File Loading Summary

| Type | Extension(s) | Loader Function | Compression |
|------|-------------|-----------------|-------------|
| Resource archive | `.RES` | `file_load_resfile()` → `file_load_resource(1,...)` | None |
| Compressed archive | `.PRE` | `file_load_resfile()` → `file_decomp()` | RLE/VLE |
| 2D shapes | `.PVS` | `file_load_shape2d()` → `file_decomp()` + unflip | RLE/VLE |
| 2D shapes (planar) | `.PES` | `file_load_shape2d()` → `file_decomp()` + unflip_pes | RLE/VLE |
| 3D shapes | `.P3S` | `file_load_3dres()` → `file_decomp()` | RLE/VLE |
| Track data | `.TRK` | `file_read_fatal()` | None |
| High scores | `.HIG` | `file_read_with_mode()` | None |
| Replays | `.RPL` | `file_load_replay()` → `file_read_fatal()` | None |
| Songs | `.KMS` | `file_load_resource(4,...)` | None |
| Voice/instruments | `.VCE` | `file_load_resource(5,...)` | None |
| Sound effects | `.SFX` | `file_load_resource(6,...)` | None |
| Fonts | `.FNT` | `file_load_resource(0,...)` | None |
| Config | `stunts.cfg` | `fread()` direct | None |
---