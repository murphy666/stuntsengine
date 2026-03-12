# Stunts Engine — Game Rendering & Refresh Design
**Date:** 2026-03-13  
**Scope:** Frame construction, framebuffer management, SDL2 presentation, dirty-rect tracking, refresh timing, skybox, crash overlays, HUD, and depth-sort infrastructure.  
**Cross-references:** `docs/3d_engine.md` (3D math, shape transforms, rasterizer, polygon info), `docs/engine_design.md` (main loops, physics, collision), `docs/sound_design.md` (audio timer).

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Framebuffer Model](#2-framebuffer-model)
3. [Palette System](#3-palette-system)
4. [SDL2 Presentation Backend](#4-sdl2-presentation-backend)
5. [Refresh Timing & Frame Gating](#5-refresh-timing--frame-gating)
6. [`update_frame` Orchestration Pipeline](#6-update_frame-orchestration-pipeline)  
   6.1 [Buffer & Dirty-Rect Initialization](#61-buffer--dirty-rect-initialization)  
   6.2 [Camera Position & Mode Dispatch](#62-camera-position--mode-dispatch)  
   6.3 [Frame Material & Clip Heading](#63-frame-material--clip-heading)  
   6.4 [Starfield Pass](#64-starfield-pass)  
   6.5 [Phase 1 — Tile Scan Classification](#65-phase-1--tile-scan-classification)  
   6.6 [Car Contact Tile Detection](#66-car-contact-tile-detection)  
   6.7 [Phase 2 — Tile Render Loop](#67-phase-2--tile-render-loop)  
   6.8 [Post-Tile Pass](#68-post-tile-pass)  
   6.9 [Dirty Rect Accumulation & Union](#69-dirty-rect-accumulation--union)  
7. [`render_present_ingame_view` — 2D Blit & Refresh](#7-render_present_ingame_view--2d-blit--refresh)
8. [Rect Buffer & Dirty-Rect System](#8-rect-buffer--dirty-rect-system)
9. [Skybox Subsystem](#9-skybox-subsystem)
10. [Crash Overlays & Damage Effects](#10-crash-overlays--damage-effects)
11. [HUD & In-Game Text](#11-hud--in-game-text)
12. [Depth Sort System](#12-depth-sort-system)
13. [Render Path Classification Reference](#13-render-path-classification-reference)
14. [Key Constants Reference](#14-key-constants-reference)
15. [Common Bug Patterns](#15-common-bug-patterns)

---

## 1. Architecture Overview

The rendering pipeline is split into three distinct layers: **3D scene construction** (`src/frame.c`), **2D sprite composition** (`src/render.c`), and **pixel delivery to the display** (`src/video.c` + `src/framebuffer.c`).

```
┌─────────────────────────────────────────────────────────────────────┐
│  Game loop (src/stuntsengine.c)                                     │
│                                                                     │
│  update_frame(view_index, clip_rect)          [src/frame.c]         │
│  └─ Tile scan → shape queue → per-tile sort flush                   │
│     └─ shape3d_render_transformed()           [src/shape3d.c]       │
│        └─ Writes indexed pixels into sdl_framebuffer[]              │
│                                                                     │
│  render_present_ingame_view(frame_rect)       [src/render.c]        │
│  └─ sprite_copy_2_to_1()                                            │
│  └─ Dirty rect diff → sprite_putimage() per dirty region            │
│  └─ video_refresh()                           [src/video.c]         │
│     └─ video_present_frame()                                        │
│        ├─ video_wait_refresh_slot()  ← 60 Hz gate                   │
│        ├─ memcpy(sdl_fb.pixels, sdl_framebuffer, 64000)             │
│        └─ fb_sdl2_present()                   [src/framebuffer.c]   │
│           ├─ fb_to_rgba()  –  indexed 8-bit → ARGB32               │
│           ├─ SDL_UpdateTexture()                                     │
│           ├─ SDL_RenderCopy()                                        │
│           └─ SDL_RenderPresent()                                     │
└─────────────────────────────────────────────────────────────────────┘
```

Key design decisions inherited from the DOS original:
- The game writes pixel indices directly into a flat `uint8_t[64000]` buffer (`sdl_framebuffer`) using legacy segment-relative logic.
- At present time the buffer is copied into the `Framebuffer` struct, which is then uploaded to SDL2. This double-buffer boundary is where legacy code meets the SDL2 host.
- All 3D rendering is software-only; there is no GPU rasterization path.

Source file roles:

| File | Responsibility |
|---|---|
| `src/frame.c` | `update_frame()` — tile scan, world assembly, shape dispatch, depth sort orchestration |
| `src/render.c` | `render_present_ingame_view()`, `load_skybox()`, `draw_skybox_rect_slice()`, `transformed_shape_add_for_sort()`, `init_rect_arrays()`, crash effects, track preview, intro |
| `src/video.c` | SDL2 lifecycle, timing gate, framebuffer pointer export, palette API, scale/fullscreen |
| `src/framebuffer.c` | `Framebuffer` struct operations, palette expansion, SDL2 texture upload |
| `src/shape3d.c` | Vertex transform, back-face cull, clip, rasterizer bridge (see `docs/3d_engine.md`) |

---

## 2. Framebuffer Model

### Structs (`include/framebuffer.h`)

```c
#define FB_WIDTH   320
#define FB_HEIGHT  200
#define FB_PIXELS  (FB_WIDTH * FB_HEIGHT)   /* 64 000 */

typedef struct Framebuffer {
    uint8_t  pixels[FB_PIXELS];    /* indexed 8-bit pixel data */
    uint8_t  palette[256][3];      /* 6-bit RGB per entry (raw stored value) */
} Framebuffer;

typedef struct SDL2Context {
    SDL_Window*   window;
    SDL_Renderer* renderer;
    SDL_Texture*  texture;          /* ARGB8888 streaming, 320×200 */
    uint32_t      rgba[FB_PIXELS];  /* conversion scratch buffer */
} SDL2Context;
```

### Working Pixel Buffer (`src/video.c`)

```c
static unsigned char sdl_framebuffer[64000];   /* placed last in .bss */
static Framebuffer   sdl_fb;
```

`sdl_framebuffer` is exported via `video_get_framebuffer()` and is the target all game-side drawing routines address. Its position at the end of `.bss` is intentional: a buffer overrun extends into unmapped memory rather than corrupting live state.

At present time `video_present_frame()` copies `sdl_framebuffer → sdl_fb.pixels` with a `memcpy(sdl_fb.pixels, sdl_framebuffer, 64000)` before passing to the SDL2 path, providing a clean snapshot.

### `fb_init(fb)`

Sets all pixels to `0` and fills `palette[i]` with identity values (each R/G/B = `i` clipped to 63). In practice the game always calls `video_set_palette()` before any frame is shown.

---

## 3. Palette System

The engine emulates VGA 6-bit DAC registers. Each palette entry is stored internally as three 6-bit values (range 0–63). On present, each channel is extended to 8 bits.

### 6-bit → 8-bit Expansion

```c
/* fb_expand_6_to_8(v6): replicate upper bits into lower 2 positions */
static inline uint8_t fb_expand_6_to_8(uint8_t v6) {
    return (uint8_t)((v6 << 2) | (v6 >> 4));
}
```

This reproduces the VGA hardware expansion formula exactly. A value of `63` (0x3F) maps to `255` (0xFF); a value of `0` maps to `0`.

### Write Path

```c
void video_set_palette(unsigned short start, unsigned short count,
                       unsigned char* colors);
```

- `colors` is a packed byte array of `count × 3` bytes: R6, G6, B6 per entry.
- Internally calls `fb_set_palette_entry(fb, index, r6, g6, b6)` on `sdl_fb` for each entry.
- Palette changes take effect on the next call to `fb_sdl2_present()`.

### Color Index Conventions

| Index | Role |
|---|---|
| 16 | Sky ground/road color (`RENDER_COLOR_INDEX_GROUND`) |
| 17 | Sky color (`RENDER_COLOR_INDEX_SKY`) |
| 100 | Water color (`RENDER_COLOR_INDEX_WATER`) |

The `skybox_sky_color`, `skybox_grd_color`, and `skybox_wat_color` global variables are populated from `render_get_material_color()` during `load_skybox()`.

### ARGB32 Conversion (`fb_to_rgba`)

On each present, `fb_to_rgba()` iterates all `FB_PIXELS` entries, expands each 6-bit channel, and packs into `0xFFRRGGBB` format (alpha always `0xFF`). This is the only per-frame CPU cost in the presentation path beyond the `SDL_*` calls.

---

## 4. SDL2 Presentation Backend

### Initialization (`fb_sdl2_init`)

`video_sdl_init()` reads the `STUNTS_SCALE` environment variable (integer 1–3, default `3`) and calls `fb_sdl2_init(ctx, "Stunts Engine", sdl_scale)`.

Inside `fb_sdl2_init`:

1. Set `SDL_HINT_RENDER_SCALE_QUALITY = "0"` — force nearest-neighbor filtering (pixel-perfect upscale).
2. Set `SDL_HINT_RENDER_DRIVER = "software"` — prefer the software renderer to avoid vsync artifacts from GPU drivers.
3. `SDL_CreateWindow(title, ..., scale*320, scale*200, 0)`.
4. Try `SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE)`. If that fails, fall back to `SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)`.
5. `SDL_RenderSetLogicalSize(renderer, 320, 200)` — SDL2 maps the 320×200 logical canvas to the window regardless of physical size. The scale factor only sets the initial window size.
6. Create a streaming `SDL_PIXELFORMAT_ARGB8888` texture at 320×200.

### Present Sequence (`fb_sdl2_present`)

```
fb_to_rgba(fb, ctx->rgba, FB_PIXELS)
SDL_UpdateTexture(texture, NULL, ctx->rgba, 320 * sizeof(uint32_t))
SDL_RenderClear(renderer)
SDL_RenderCopy(renderer, texture, NULL, NULL)
SDL_RenderPresent(renderer)
```

### Scale & Fullscreen Controls

| Function | Action |
|---|---|
| `video_scale_up()` | Increment `sdl_scale` (max 3), call `fb_sdl2_set_scale(ctx, scale)` → `SDL_SetWindowSize(scale*320, scale*200)` |
| `video_scale_down()` | Decrement `sdl_scale` (min 1) |
| `video_toggle_fullscreen()` | Toggle `SDL_WINDOW_FULLSCREEN_DESKTOP` on the window |

### Corruption Guard

At the end of `video_sdl_init()`, the window, renderer, and texture pointers are saved into separate sentinel variables. On each call to `video_present_frame()`, they are compared against the live SDL context; if any mismatch is detected the pointers are restored. This guards against accidental overwrite of the SDL context struct by legacy fixed-address writes.

---

## 5. Refresh Timing & Frame Gating

### `video_present_frame()` (`src/video.c`)

This is the single serialization point between game logic and display. Its sequence:

1. **Timing gate** — `video_wait_refresh_slot(&next_present_counter, &present_accum)`, which calls `timer_wait_for_counter_rate(timer_get_display_hz(), ...)` targeting `GAME_DISPLAY_HZ = 60` Hz (from `include/game_timing.h`). This blocks the calling thread in a busy-wait loop until the next allowed present slot.
2. **Copy** — `memcpy(sdl_fb.pixels, sdl_framebuffer, FB_PIXELS)`. Takes a snapshot of the working buffer.
3. **Present** — `fb_sdl2_present(&sdl_ctx, &sdl_fb)`.

### Relationship to the Game Loop

`video_present_frame()` is called only from:
- `video_refresh()` — used during in-game race mode, called at the end of `render_present_ingame_view()`.
- `video_wait_menu_refresh()` — used in menu/UI loops where the game wants a rate-limited update.

The 60 Hz gate ensures that even if the CPU completes a game tick faster than 16.67 ms, the frame will not be presented early. Audio runs on a separate SDL2 callback thread and is unaffected by this gate (see `docs/sound_design.md`).

---

## 6. `update_frame` Orchestration Pipeline

**Signature:** `void update_frame(int view_index, struct RECTANGLE* clip_rect)` (`src/frame.c`)

`update_frame` is the central 3D scene building function. It builds the world from the track tile grid outward, placing terrain, objects, and vehicles into the scene for the given view index, then hands the completed scanline data off to the 2D composition layer.

### 6.1 Buffer & Dirty-Rect Initialization

```c
/* Select active/back rect buffers based on video_flag5_is0 and view_index */
if (video_flag5_is0 == 0) {
    rect_buffer_primary   = (view_index == 0) ? &rect_buffer_front[1] : &rect_buffer_front[8];
    rect_buffer_secondary = (view_index == 0) ? &rect_buffer_back[1]  : &rect_buffer_back[8];
} else {
    rect_buffer_primary   = &rect_buffer_front[1];
    rect_buffer_secondary = &rect_buffer_back[1];
}
```

If `timertestflag_copy != 0` (dirty-rect tracking mode enabled), all `frame_dirty_rects[15]` slots are reset to `rect_invalid`. Each drawing operation that occurs during the frame will then union its bounding rectangle into the appropriate slot.

### 6.2 Camera Position & Mode Dispatch

Camera source: `playerstate` normally, or `opponentstate` when `followOpponentFlag != 0`.

| Camera mode | Source | Notes |
|---|---|---|
| 0 | Player/opponent position + fixed height offset | Cockpit or hood view |
| 1 | `game_vec1` | Fixed-point view from external replay/editor camera |
| 2 | Player position + spherical offset computed from `rotation_x_angle` / `rotation_y_angle` | Chase/orbit view |
| 3 | `trackdata9[view_index]` | Static track waypoint cameras |

When `view_yaw == -1` the look-at direction is computed from the car velocity vector via `polarAngle()` instead of being taken from the car's facing angle.

After resolving the camera position, the view rotation matrix is built:

```c
view_rot_mat = mat_rot_zxy(view_roll, view_pitch, 0, 1);
```

This produces the world-to-camera transform. See `docs/3d_engine.md` §3 for matrix conventions.

### 6.3 Frame Material & Clip Heading

```c
/* Animated road texture material cycling */
default_material = polygon_edge_direction_table[game_frame & 15];

/* Choose tile scan octant: 3 bits from bits 7-9 of clip_heading */
clip_heading = select_cliprect_rotate(view_yaw, clip_rect);
octant = (clip_heading & FRAME_ANGLE_MASK) >> 7;
lookahead_table = tile_scan_patterns[octant]; /* 45 × 3 bytes */
```

`select_cliprect_rotate` selects which of the 8 pre-computed tile scan pattern tables is used for the current frame, based on the camera's horizontal facing. Each table is optimized for a different 45° octant so that tiles directly in front of the camera are visited first and tiles behind are visited last (or skipped entirely by the visibility threshold).

### 6.4 Starfield Pass

Before the tile loop, 8 star shapes are rendered at `FRAME_STARFIELD_Z = 15000` (far clip distance). Stars are placed at fixed angle offsets and rendered with `shape3d_render_transformed()` directly (immediate path). This pass is skipped when `timertestflag2 != 0` (alternate clip region).

### 6.5 Phase 1 — Tile Scan Classification

The classification pass runs **back-to-front** (`si = FRAME_TILE_SCAN_LAST` down to 0) through the lookahead table.

For each scan slot `si`:

1. Read `(col_offset, row_offset, detail_level)` from `lookahead_table[si*3 .. si*3+2]`.
2. Apply tile coordinate biases:
   - `tile_col = col_offset + tile_col_bias`  where `tile_col_bias = camera_pos.x >> 10`
   - `tile_row = row_offset + tile_row_bias`  where `tile_row_bias = -(camera_pos.z >> 10 - 29)`
3. Bounds-check: skip if `tile_col` or `tile_row` is outside `[0, FRAME_TILE_MAX_INDEX]` (29).
4. Read `td14_elem_map_main[tile_row][tile_col]` (track element) and `td15_terr_map_main[tile_row][tile_col]` (terrain height type).
5. **Multi-tile indirection:** element values `0xFD`, `0xFE`, `0xFF` indicate that the canonical data lives in an adjacent tile. The indirection resolves the actual element and marks the covered scan slots as `tile_scan_state[di] = 1` so the render loop skips them.
6. **Multi-tile suppression:** Large objects (e.g. loop, corkscrew) span multiple tiles. The classification pass marks all subordinate scan slots as `tile_scan_state[di] = 1` so the render loop only processes the canonical anchor slot.

### 6.6 Car Contact Tile Detection

After classification, for each car (player index 2, opponent index 3), iterate the 4 wheel world positions:

1. Convert each wheel XZ position to tile coordinates.
2. Walk the lookahead table to find the scan slot `si` whose `(col, row)` matches.
3. Store `player_contact_col` / `player_contact_row`.
4. Compute `player_sort_bias`: if the car's projected Z is close to a track object in the same tile, bias `+FRAME_SORT_BIAS` (draw car in front) or `-FRAME_SORT_BIAS` (draw car behind).

`FRAME_SORT_BIAS = 2048` (`FRAME_SORT_BIAS_OVERLAY = 1024` for overlay track shapes).

### 6.7 Phase 2 — Tile Render Loop

The render loop runs **front-to-back** (`si = 0` to `FRAME_TILE_SCAN_LAST`), skipping any slot where `tile_scan_state[si] != 0` (classified as a multi-tile subordinate).

**Per-slot processing:**

#### Border Fences
If `tile_col == 0` or `tile_col == FRAME_TILE_MAX_INDEX` (29), or likewise for `tile_row`, border fence shapes are rendered **immediately** via `shape3d_render_transformed()` without entering the depth sort queue.

#### Terrain
`terr_map_value != 0` → terrain shape is rendered **immediately**. Type 6 terrain (elevated flat sections) forces `tile_hill_height = hillHeightConsts[1]`.

Hill multi-tile objects (element codes 105–108) render each sub-tile separately with their own immediate calls.

#### Track Object — Overlay Slot
Track overlay shapes are queued to `currenttransshape[1]`. If `ts_flags & 1` is clear, they are rendered immediately; otherwise they are deferred to the sort queue with a Z-bias of `-FRAME_SORT_BIAS_OVERLAY` so they appear behind the main track object.

#### Track Object — Main Slot
The main track object (`currenttransshape[0]`):
- If `ignore_zbias` flag is set → **immediate** `shape3d_render_transformed()`.
- Otherwise → **deferred**: `transformed_shape_add_for_sort(0, 0)`.

Start/finish tiles: animated vertex offsets applied each frame; queued with a sort key based on `start_tile_sort_mask & -FRAME_SORT_BIAS`.

#### Car Insertion
When the current scan slot matches `player_contact_col/row`:
- Player car (sort key 2): `transformed_shape_add_for_sort(player_sort_bias, 2)`.
  - If `timertestflag_copy != 0`, the flag `FRAME_FLAG_TRACKED_SHAPE` (12) is stored in `ts_flags` so the dirty rect for this car can be tracked individually.
- Opponent car (sort key 3): `transformed_shape_add_for_sort(opponent_sort_bias, 3)`.

Obstacle/spawn objects: also deferred via `transformed_shape_add_for_sort`.

#### Per-Tile Sort Flush

After all shapes for a tile have been queued, the depth sort is executed:

```c
transformedshape_sort_desc(transformedshape_zarray,
                           transformedshape_indices,
                           transformedshape_counter);
```

Then each shape in the sorted index order is rendered:

```c
for (i = 0; i < transformedshape_counter; i++) {
    idx = transformedshape_indices[i];
    curtransshape_ptr = &currenttransshape[idx];
    /* Override backlights for car shapes based on brake state */
    if (transformedshape_arg2array[i] == 2 || transformedshape_arg2array[i] == 3) {
        set_car_backlight_state(brake_flags[idx - 2]);
    }
    shape3d_render_transformed(curtransshape_ptr);
}
transformedshape_counter = 0;
```

See §12 for depth sort internals.

### 6.8 Post-Tile Pass

After `exit_tile_loop:` the following occur in order:

1. **Skybox:** `render_skybox_layer(view_index, clip_rect, sky_dir_sign, &view_rot_mat, view_roll, view_yaw, camera_pos.y)` — renders the background sky/ground strip below the 3D horizon line. See §9.

2. **Sprite clipping setup:** `sprite_set_1_size(0, 320, clip_rect->top, clip_rect->bottom)` — restricts subsequent 2D blits to the in-game viewport.

3. **Polygon info finalization:** `get_a_poly_info()` — flushes any deferred polygon edge information accumulated during rasterization. See `docs/3d_engine.md` §14.

4. **Explosion overlay:** If `crash_explosion_frame >= 0`, `shape_op_explosion()` is called with the car's projected bounding rect. The effect cycles through 3 frames scaled to the car's screen size.

5. **In-car crash effects:** Selected by `car_crashBmpFlag`:
   - `init_crak()` — renders cracked windscreen overlay (camera mode 0 only).
   - `do_sinking()` — draws an animated water-fill effect rising up from `base_y`. Returns a dirty rect pointer.

6. **HUD timer:** When `game_inputmode != 0`, `format_frame_as_string()` formats the lap/race time and `intro_draw_text()` blits it into the viewport.

7. **In-game text:** `draw_ingame_text()` — renders any queued speed/gear/opponent-label text overlays.

8. **Angle state store:** `angle_rotation_state[view_index] = view_yaw` — saved for next frame's camera continuity and skybox repeat zone calculation.

### 6.9 Dirty Rect Accumulation & Union

When `timertestflag_copy != 0` (dirty-rect mode), every drawing call during the frame records its bounding rectangle. At the end of `update_frame`:

```c
if (timertestflag_copy != 0) {
    /* Union all per-shape dirty rects into frame_dirty_rects[] */
    for (i = 0; i < FRAME_DIRTY_RECT_COUNT; i++) {
        rect_union(&frame_dirty_rects[i], &shape_dirty_rects[i]);
    }
    /* If the skybox was redrawn, force the entire clip_rect dirty */
    if (skybox_result != 0) {
        rect_union(&frame_dirty_rects[FRAME_DIRTY_SKY_SLOT], clip_rect);
    }
}
```

These accumulated rects are passed to `render_present_ingame_view()` for differential blitting.

---

## 7. `render_present_ingame_view` — 2D Blit & Refresh

**Signature:** `void render_present_ingame_view(struct RECTANGLE* frame_rect)` (`src/render.c`)

Called immediately after `update_frame()` from the game loop. It composes the rendered backbuffer onto the display sprite and triggers the actual screen refresh.

### Entry Guard

```c
if (video_flag5_is0 != 0) return;
```

This flag is set during loading screens and menus where the game manages the display directly; the in-game blit path is bypassed entirely in that case.

### Page-Flip Mode (`race_condition_state_flag != 0`)

Legacy double-buffer mode. The backbuffer window sprite is blitted directly, and `video_refresh()` is called. Used for split-screen.

### Dirty-Rect Path (`timertestflag_copy != 0`)

1. Initialize `rect_sort_indices[RENDER_RECT_ARRAY_COUNT]` to `3` (all rects active).
2. If `timertestflag2 == 4` (full-redraw clip mode), override `collision_detection_state`.
3. Compare `rect_buffer_front[5]` vs `rect_buffer_back[5]`: if identical, set index 5 to `0` (unchanged, skip it).
4. Call `rectlist_add_rects()` — computes the union/diff of front vs back rect arrays and produces a `dirty_rect_array[]` with `dirty_rect_count` entries.
5. If `dirty_rect_count == 0`: set sprite size to full `frame_rect` and do a single `sprite_putimage()`.
6. Otherwise: sort dirty rects top-to-bottom via `rect_array_sort_by_top()`, then for each dirty rect call `sprite_set_1_size()` + `sprite_putimage()`.

### Non-Tracking Path (`timertestflag_copy == 0`)

Simple: `sprite_set_1_size(frame_rect)` + `sprite_putimage(wndsprite->sprite_bitmapptr)`. Blits the entire viewport unconditionally.

### Cleanup & Back-Buffer Swap

```c
video_refresh();       /* → video_present_frame() → SDL2 upload */

if (timertestflag_copy != 0) {
    collision_detection_state = sprite_transformation_angle;
    for (si = 0; si < RENDER_RECT_ARRAY_COUNT; si++)
        rect_buffer_back[si] = rect_buffer_front[si];
}
```

The `rect_buffer_front` contents are copied to `rect_buffer_back` so the next frame can compute a diff.

---

## 8. Rect Buffer & Dirty-Rect System

Two parallel rect arrays of `RENDER_RECT_ARRAY_COUNT = 15` entries each track per-frame coverage:

| Array | Role |
|---|---|
| `rect_buffer_front[]` | Current-frame dirty regions, written during `update_frame` |
| `rect_buffer_back[]` | Previous-frame dirty regions, read by `render_present_ingame_view` to compute diff |

`init_rect_arrays()` (`render.c`) initializes both arrays: slot 0 receives `fullscreen_rect`; slots 1–14 receive `rect_invalid`.

Slot allocation in `rect_buffer_front`:

| Slot | Content |
|---|---|
| 0 | Full screen (always dirty sentinel) |
| 1–7 | Per-view primary rect range (view 0) |
| 8–14 | Per-view primary rect range (view 1, split-screen) |
| 5 | Special: compared independently for camera-angle change detection |

`rect_invalid` is a sentinel value with `left > right` or `top > bottom`, treated as "no area" by all rect operations.

`rect_union(a, b)` expands `a` to encompass both `a` and `b`. `rect_union` is a no-op when `b == rect_invalid`.

`timertestflag_copy` controls whether dirty-rect tracking is active. When `0`, all present calls are full-viewport blits.

---

## 9. Skybox Subsystem

### Resources

Five named scenery sets: `desert`, `tropical`, `alpine`, `city`, `country`. Each is a `.PVS` resource file containing 4 `SHAPE2D` sub-resources loaded via `locate_many_resources(ptr, "scensce2sce3sce4", skyboxes)`.

- `skyboxes[0..3]` — sky panorama layers (sce, sce2, sce3, sce4).
- `skybox_ptr1..4` — individual heights of each layer (`SHAPE2D::s2d_height`).
- `skybox_current` — minimum layer height (topmost visible row).
- `memory_pointer_boundary_max` — maximum layer height (bottommost visible row).

### Loading (`load_skybox`)

```c
void load_skybox(unsigned char skybox_index);
```

If the same skybox index is already loaded (`texture_page_index == skybox_index`), only the palette colors are re-applied. Otherwise the old resource is `mmgr_free`'d and the new one loaded.

`RENDER_SKYBOX_ALT_FLAG = 8`: if bit 3 is set in `skybox_index`, the resource is not reloaded and only colors are updated (alt-environment mode).

### Drawing (`draw_skybox_rect_slice`)

```c
void draw_skybox_rect_slice(struct RECTANGLE* rectptr, int angY, int skyheight);
```

Called from `render_skybox_layer()` (in `frame.c`), which computes `skyheight` (the screen-space horizon row) based on `camera_pos.y` and the view roll/pitch.

Inside `draw_skybox_rect_slice`:

1. Compute how many rows above the horizon are visible within `rectptr`.
2. Fill the region above the sky layers with `skybox_sky_color` (flat clear).
3. If `timertestflag2 == 4` (alternate clip mode): skip the layered skybox, go directly to ground fill.
4. Otherwise: compute the angular offset `angY + RENDER_ANGLE_HALF_TURN`, select which skybox layer to blit at which Y position, and blit via `sprite_putimage_transparent()`.
5. Fill the ground region below the horizon with `skybox_grd_color` (flat fill).

`render_skybox_layer()` returns non-zero if the skybox actually drew something, which causes `update_frame` to mark the entire clip rect dirty for this frame.

---

## 10. Crash Overlays & Damage Effects

### Explosion Overlay

When `crash_explosion_frame >= 0`, `shape_op_explosion()` places an explosion sprite scaled to the car's projected bounding rect. The frame counter advances each tick, cycling through 3 frames in `sdgame2shapes`. When the animation ends, the counter resets to `-1`.

### Cracked Windscreen (`init_crak`, camera mode 0 only)

`init_crak()` (`render.c`) blits a static crack overlay `SHAPE2D` (`asCrak` resource from `sdgame2ptr`) into the viewport. It is only applied in camera mode 0 (cockpit/hood) since in exterior views the windscreen is not visible.

### Water Sinking (`do_sinking`)

```c
struct RECTANGLE* do_sinking(int frame_count, int base_y, int sink_height);
```

Animates a rising water fill in the viewport. The fill height is:

```
fill_y = base_y + sink_height - (sink_height * frame_count) / (framespersec * 4)
```

`framespersec * 4` defines the total duration. The fill uses `sprite_set_1_size` + `sprite_clear_sprite1_color(skybox_wat_color)` to flood the bottom of the screen with the water palette color. Returns a pointer to the computed dirty rect (`DO_SINKING_RECT_PTR`).

The two effects are mutually exclusive via `car_crashBmpFlag`:
- `car_crashBmpFlag == 0`: no overlay
- `car_crashBmpFlag == 1`: cracked windscreen overlay
- `car_crashBmpFlag == 2`: sinking water effect

---

## 11. HUD & In-Game Text

### Timer Display

Rendered when `game_inputmode != 0`:

```c
format_frame_as_string(game_timer_ticks, text_buffer);
intro_draw_text(text_buffer, hud_x, hud_y);
```

`format_frame_as_string` converts the raw tick count to `MM:SS.ff` format. `intro_draw_text` blits using the bitmap font (loaded from `FONTLED.FNT`).

### General In-Game Text

`draw_ingame_text()` renders any queued overlays accumulated during the frame — speed reading, gear indicator, opponent distance label. Text entries are written into a ring buffer by gameplay code and consumed here.

### Rect Tracking

When dirty-rect mode is active, both the timer and `draw_ingame_text()` outputs are recorded into dedicated `rect_buffer_front` slots (indices 2–4 historically, mapping to `rect_ingame_text2`, `rect_ingame_text3`, `rect_ingame_text4` static rectangles defined in `render.c`).

---

## 12. Depth Sort System

### Overview

The depth sort operates per-tile during Phase 2 of `update_frame`. All world objects, cars, and obstacles that fall within a given tile's screen footprint are queued into a fixed-size array and sorted by Z depth (descending = farthest first) before rasterization. This is a painter's algorithm applied at tile granularity.

### Data Structures

```c
/* Arrays sized for FRAME_MAX_SORT_SHAPES per tile (32 entries) */
int    transformedshape_zarray[32];       /* Z depths for sorting */
int    transformedshape_indices[32];      /* Original slot indices */
char   transformedshape_arg2array[32];    /* sort_key (0=track, 2=player, 3=opp) */
uint8_t transformedshape_counter;         /* current count; reset to 0 after flush */
```

### `transformed_shape_add_for_sort(int z_adjust, int sort_key)` (`render.c`)

1. Read the world-space position from `curtransshape_ptr->pos`.
2. Apply the current temporary matrix `mat_temp` via `mat_mul_vector` to get camera-space Z.
3. Store `z_depth + z_adjust` into `transformedshape_zarray[counter]`.
4. Store `sort_key` into `transformedshape_arg2array[counter]`.
5. Advance `curtransshape_ptr` and `transformedshape_counter`.

`z_adjust` is used to implement sort biases:
- `+FRAME_SORT_BIAS (2048)` — draw car in front of adjacent track object.
- `-FRAME_SORT_BIAS (2048)` — draw car behind.
- `-FRAME_SORT_BIAS_OVERLAY (1024)` — draw overlay shape behind main track shape.
- `0` — natural Z depth only.

### `transformedshape_sort_desc` (`src/frame.c`)

Bubble sort in descending Z order. Operates on `zarray` and `indices` in place. After sorting, `indices[0]` holds the farthest shape slot.

The sort is bounded by `transformedshape_counter` (never more than a few shapes per tile in practice), so the O(n²) bubble sort cost is negligible.

### Sort Key (`sort_key` / `arg2`)

The secondary sort key identifies what type of object the shape belongs to:
- `0` — track object (road, building, obstacle)
- `1` — track overlay
- `2` — player car
- `3` — opponent car

This is used after the sort to apply car-specific rendering state (backlight override based on brake input).

---

## 13. Render Path Classification Reference

| Object Type | Render path | Function | Notes |
|---|---|---|---|
| Border fence | Immediate | `shape3d_render_transformed` | tile_col/row == 0 or 29 |
| Terrain (non-zero terr_map_value) | Immediate | `shape3d_render_transformed` | — |
| Hill multi-tile sub-tiles | Immediate (each) | `shape3d_render_transformed` | elements 105–108 |
| Starfield stars | Immediate | `shape3d_render_transformed` | 8 shapes at Z=15000 |
| Track overlay (`ts_flags & 1 == 0`) | Immediate | `shape3d_render_transformed` | via `currenttransshape[1]` |
| Track overlay (`ts_flags & 1 != 0`) | Deferred | `transformed_shape_add_for_sort(-FRAME_SORT_BIAS_OVERLAY, 1)` | behind main |
| Track object (`ignore_zbias`) | Immediate | `shape3d_render_transformed` | flat/no Z occlusion |
| Track object (normal) | Deferred | `transformed_shape_add_for_sort(0, 0)` | — |
| Start/finish tile object | Deferred | `transformed_shape_add_for_sort(start_tile_sort_mask & -FRAME_SORT_BIAS, 0)` | animated |
| Player car | Deferred | `transformed_shape_add_for_sort(player_sort_bias, 2)` | |
| Opponent car | Deferred | `transformed_shape_add_for_sort(opponent_sort_bias, 3)` | |
| Obstacle/spawn objects | Deferred | `transformed_shape_add_for_sort(...)` | |
| Skybox background | 2D blit | `draw_skybox_rect_slice` | post-tile, via `render_skybox_layer` |
| Explosion overlay | 2D sprite | `shape_op_explosion` | post-tile |
| Crack windscreen | 2D blit | `init_crak` | post-tile, camera mode 0 only |
| Water sinking | 2D fill | `do_sinking` | post-tile |
| HUD timer | 2D text | `intro_draw_text` | post-tile |
| In-game text/speed | 2D text | `draw_ingame_text` | post-tile |

---

## 14. Key Constants Reference

### Framebuffer / Video

| Constant | Value | Definition |
|---|---|---|
| `FB_WIDTH` | 320 | `include/framebuffer.h` |
| `FB_HEIGHT` | 200 | `include/framebuffer.h` |
| `FB_PIXELS` | 64000 | `include/framebuffer.h` |
| `GAME_DISPLAY_HZ` | 60 | `include/game_timing.h` |

### Frame / Tile Scan (`src/frame.c`)

| Constant | Value | Meaning |
|---|---|---|
| `FRAME_TILE_SCAN_COUNT` | 45 | Entries per octant lookahead table |
| `FRAME_TILE_SCAN_LAST` | 44 | Last scan slot index |
| `FRAME_TILE_MAX_INDEX` | 29 | Track grid bounds (0–29 valid) |
| `FRAME_SCREEN_WIDTH` | 320 | Render target width in pixels |
| `FRAME_SCREEN_HEIGHT` | 200 | Render target height in pixels |
| `FRAME_ANGLE_MASK` | 1023 | 10-bit angle wrap mask |
| `FRAME_VISIBILITY_NEAR` | 100 | Visibility threshold: close tiles, hi detail |
| `FRAME_VISIBILITY_MEDIUM` | 300 | Visibility threshold: medium detail |
| `FRAME_VISIBILITY_FAR` | 1024 | Visibility threshold: far, low detail |
| `FRAME_VISIBILITY_HILL` | 2048 | Visibility threshold: hill objects |
| `FRAME_SORT_BIAS` | 2048 | Z-bias applied to car shapes vs track |
| `FRAME_SORT_BIAS_OVERLAY` | 1024 | Z-bias for overlay track shapes |
| `FRAME_STARFIELD_Z` | 15000 | Depth at which starfield is placed |
| `REND_DIST_MULT` | 1 | Visibility multiplier (must stay 1 in `frame.c`) |

### Render Constants (`src/render.c`)

| Constant | Value | Meaning |
|---|---|---|
| `RENDER_SCREEN_LEFT` | 0 | Left edge of screen |
| `RENDER_SCREEN_WIDTH` | 320 | Screen width |
| `RENDER_ANGLE_MASK` | 1023 | 10-bit angle wrap |
| `RENDER_ANGLE_HALF_TURN` | 512 | 180° in 10-bit units |
| `RENDER_RECT_ARRAY_COUNT` | 15 | Dirty rect array size |
| `RENDER_SKYBOX_COUNT` | 4 | Number of skybox layers per environment |
| `RENDER_COLOR_INDEX_SKY` | 17 | Palette index for sky color |
| `RENDER_COLOR_INDEX_GROUND` | 16 | Palette index for ground color |
| `RENDER_COLOR_INDEX_WATER` | 100 | Palette index for water color |
| `RENDER_SKYBOX_ALT_FLAG` | 8 | Bit: reuse loaded skybox, update colors only |
| `RENDER_SKYBOX_INDEX_MASK` | 7 | Mask: bits 0–2 are the skybox selector |
| `REND_DIST_MULT` | 8 | Visibility multiplier in `render.c` TU |

> Note: `REND_DIST_MULT` is defined independently in both `frame.c` (value `1`) and `render.c` (value `8`). The value in `frame.c` controls visibility thresholds for the tile scan; the value `1` must be preserved there to keep the paint-cull-mask behavior correct. The value `8` in `render.c` applies to track preview rendering only.

### `timertestflag_copy` Values

| Value | Meaning |
|---|---|
| `0` | Full-screen blit on every present; dirty-rect tracking disabled |
| non-zero | Dirty-rect tracking active; only changed regions are blitted |

### `timertestflag2` Values

| Value | Meaning |
|---|---|
| `0` | Normal full clip region; starfield enabled |
| `4` | Alternate clip selector; starfield disabled; simplified skybox (sky + ground only) |

---

## 15. Common Bug Patterns

**Dirty-rect corruption:** If `timertestflag_copy` is non-zero but `init_rect_arrays()` is not called at the start of the frame cycle, `rect_buffer_front/back` will contain stale values from previous frames, causing double-blits or missed updates.

**Sort bias sign error:** The car contact tile detection computes `player_sort_bias` as `±FRAME_SORT_BIAS`. Getting the sign wrong causes the car to visually clip through road surfaces or float above them when near a track piece at the same tile.

**Palette range mismatch:** `video_set_palette(start, count, colors)` uses 6-bit values. Passing 8-bit linear values produces washed-out colors because `fb_expand_6_to_8` doubles the shift.

**Skybox dirty-rect omission:** The skybox redraw flag (`skybox_result != 0`) must force `clip_rect` dirty in the dirty-rect union. If this is omitted, the sky will not repaint after a camera-angle change and terrain geometry will be visible through the sky in subsequent frames.

**`REND_DIST_MULT = 1` constraint:** Changing the `REND_DIST_MULT` value in `frame.c` from `1` to any other value disrupts the paint-cull-mask logic in `shape3d_render_transformed()` because the mask relies on a specific stride relationship between visibility bands and Z-depth encoding. Keep at `1`.

**Starfield when `timertestflag2 != 0`:** Stars must not be rendered when the alternate clip region is active because the clip rectangle no longer covers the region where the starfield is placed, and the shapes will overdraw menu UI elements.
