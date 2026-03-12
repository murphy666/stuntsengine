# Stunts 3D Engine — Detailed Design & Internals Reference

**Date:** 2026-03-12  
**Purpose:** Exhaustive reference for debugging the 3D engine, physics, and rendering pipeline.

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Coordinate Systems & Numeric Conventions](#2-coordinate-systems--numeric-conventions)
3. [Math Library Internals](#3-math-library-internals)
4. [3D Shape Data Format](#4-3d-shape-data-format)
5. [Shape Resource Loading](#5-shape-resource-loading)
6. [Projection System](#6-projection-system)
7. [Camera System](#7-camera-system)
8. [Frame Rendering Pipeline (update_frame)](#8-frame-rendering-pipeline-update_frame)
9. [World Tile Assembly & Culling](#9-world-tile-assembly--culling)
10. [3D Shape Transformation Pipeline](#10-3d-shape-transformation-pipeline)
11. [Depth Sorting System](#11-depth-sorting-system)
12. [Back-Face & Direction Culling](#12-back-face--direction-culling)
13. [Near Plane Clipping](#13-near-plane-clipping)
14. [Polygon Info System](#14-polygon-info-system)
15. [Rasterizer (Scanline Fill)](#15-rasterizer-scanline-fill)
16. [Material & Color System](#16-material--color-system)
17. [Skybox Rendering](#17-skybox-rendering)
18. [Car Shape & Wheel Rendering](#18-car-shape--wheel-rendering)
19. [Special Effects Rendering](#19-special-effects-rendering)
20. [Sprite / 2D Layer System](#20-sprite--2d-layer-system)
21. [Framebuffer & SDL2 Presentation](#21-framebuffer--sdl2-presentation)
22. [Dirty Rectangle System](#22-dirty-rectangle-system)
23. [Track Data System](#23-track-data-system)
24. [Track Object & Collision Geometry (build_track_object)](#24-track-object--collision-geometry-build_track_object)
25. [Vehicle Physics Engine](#25-vehicle-physics-engine)
26. [Collision Detection System](#26-collision-detection-system)
27. [Crash State Machine](#27-crash-state-machine)
28. [Game Loop & Timing](#28-game-loop--timing)
29. [Replay & Input System](#29-replay--input-system)
30. [Global State Variable Reference](#30-global-state-variable-reference)
31. [Key Numeric Constants](#31-key-numeric-constants)
32. [Common Bug Patterns & Debugging Notes](#32-common-bug-patterns--debugging-notes)

---

## 1. Architecture Overview

This engine is a C port of the original DOS **Stunts** (4D Sports Driving, DSI/Brøderbund, 1990). The code preserves the original 16-bit x86 structure: `goto`-based control flow, fixed-point arithmetic, packed binary structs, and segmented source layout.

### Source File Map

| File | Lines | Role |
|------|-------|------|
| `src/frame.c` | 1618 | Main 3D scene builder (`update_frame`) |
| `src/shape3d.c` | 5311 | 3D shape transform, projection, polygon emit, rasterizer core |
| `src/shape3d.h` | 72 | Shape/transform structs |
| `src/render.c` | 2124 | Scene composition, skybox, track preview, intro, frame present |
| `src/video.c` | 481 | VGA mode management, SDL2 init |
| `src/framebuffer.c` | 247 | SDL2 texture/palette/present backend |
| `src/shape2d.c` | 4141 | 2D sprite system, scanline fill, RLE decoding, dashboard |
| `src/shape2d.h` | 179 | SPRITE/SHAPE2D structs |
| `src/math.c` | 1405 | Trig tables, matrix ops, vector projection, plane math |
| `src/math.h` | 119 | VECTOR, MATRIX, PLANE, RECTANGLE structs |
| `src/track.c` | 3119 | Track grid, validation, path-building, editor |
| `src/track_object.c` | 2314 | Per-frame tile physics resolver (`build_track_object`) |
| `src/carstate.c` | 3337 | Vehicle physics, collision, crash state machine |
| `src/stuntsengine.c` | 2728 | Main loop, session management, resource loading |
| `src/gamemech.c` | 1983 | Frame callback, replay, dashboard updates, camera |
| `src/data_core.c`, `data_game.c`, `data_runtime.c`, `data_menu_track.c` | 981 | Global variable definitions, split by domain (formerly `data_global.c`) |
| `include/game_types.h` | 228 | CARSTATE/GAMESTATE/SIMD structs; `include/data_game.h` | 594 | external declarations |

### Runtime Data Flow

```
Timer ISR (timer.c)
  → frame_callback (gamemech.c)
    → replay_capture_frame_input → advances elapsed_time2

Main Loop: run_game (stuntsengine.c)
  while game_frame < elapsed_time2:
    update_gamestate()
      → update_car_speed()        (carstate.c)
      → update_grip()             (carstate.c)
      → update_player_state()     (carstate.c)  ← THE BIG ONE (~3500 lines)

  Rendering:
    update_frame()               (frame.c)
      → camera setup
      → tile visibility loop (23 tiles)
        → per-tile: terrain, fences, track elements, overlays, cars, obstacles
        → shape3d_render_transformed() per object   (shape3d.c)
          → vertex transform (model→camera)
          → perspective projection
          → cull + clip
          → emit polygon info records → sorted linked list
      → render_skybox_layer()    (render.c)
      → get_a_poly_info()        (shape3d.c)
        → walk sorted list back-to-front
        → dispatch: preRender_default / preRender_sphere / preRender_line / preRender_wheel
          → draw_filled_lines / draw_patterned_lines   (shape2d.c)
      → explosion/crack/sinking overlays
      → HUD text
    render_present_ingame_view() (render.c)
      → dirty rect blit → video_present → SDL2 present
```

---

## 2. Coordinate Systems & Numeric Conventions

### World Coordinate Space

- **X axis**: East (positive = right when facing north)
- **Y axis**: Up (positive = upward)
- **Z axis**: South-to-North (positive = northward, but grid row 0 is at maximum Z)
- **Unit scale**: Each track tile = **1024 world units** (0x400)
- **Grid**: 30×30 tiles (indices 0–29 = 0x00–0x1D)

### Position Formats

| Context | Format | Conversion |
|---------|--------|------------|
| `car_posWorld1` (VECTORLONG) | 32-bit fixed-point, 16.16-ish | `>> 6` for 16-bit render coords; `>> 10` for tile index |
| Render/physics positions | 16-bit signed (`short`) | Direct world units |
| Tile indices | Integer 0–29 | `worldCoord >> 10` |
| Screen coordinates | Integer pixels | X: 0–319, Y: 0–199 |

### Angle System — 10-bit Circular

Full circle = **1024 units** (0x400). Angles are masked with `& 0x3FF`.

| Angle | Value | Hex |
|-------|-------|-----|
| 0° (North/+Z) | 0 | 0x000 |
| 90° (East/+X) | 256 | 0x100 |
| 180° (South/−Z) | 512 | 0x200 |
| 270° (West/−X) | 768 | 0x300 |

Each unit ≈ 0.3516°.

### Fixed-Point Arithmetic — Q1.14

All matrix elements, trig function outputs, and trig-derived values use **14-bit fractional scaling**:

- **1.0** = `0x4000` (16384)
- **Multiply**: `((long)a * b) >> 14`
- `sin_fast(0x100)` returns `0x4000` (= sin 90° = 1.0)
- `cos_fast(0)` returns `0x4000`

The helper `multiply_and_scale(a, b)` computes `(a * b * 4) >> 16` with rounding, which is equivalent to `(a * b) >> 14`.

### Row/Column Inversion

**Critical for debugging**: The Z axis is inverted relative to grid rows:
```c
tile_row = -((world_z >> 10) - 0x1D)   // in frame.c camera tile coords
```
Row 0 is at the **south** end of the map (maximum Z), row 29 is at the **north** end (minimum Z). This causes frequent sign negations and `0x1D` offsets throughout the codebase.

---

## 3. Math Library Internals

### Trigonometric Functions (`src/math.c`)

**`sin_fast(unsigned short s)`**: Quarter-wave lookup from `sintab[257]` (covering indices 0–256 = 0°–90°). Quadrant selected by bits 8–9 of the angle:
- Q0 (0–255): `+sintab[idx]`
- Q1 (256–511): `+sintab[256 - idx]`
- Q2 (512–767): `-sintab[idx]`
- Q3 (768–1023): `-sintab[256 - idx]`

**`cos_fast(unsigned short s)`**: Simply `sin_fast(s + 0x100)`.

Both return **Q1.14 signed** values in range [−16384, +16384].

### Polar Coordinate Functions

**`polarAngle(int z, int y)`**: Computes `atan2(z, y)` using `atantable[256]`. Uses a 3-bit flag from sign/magnitude to select one of 8 octants. Computes `(z << 16) / y` as an index. Returns 10-bit angle. **Undefined for (0, 0)** — this is a potential bug source.

**`polarRadius2D(int z, int y)`**: Computes $\sqrt{z^2 + y^2}$ without sqrt — uses `polarAngle` to get the angle, then divides the dominant axis by the corresponding trig value: `(axis << 14) / trig_value`.

**`polarRadius3D(struct VECTOR* vec)`**: Nested call: `polarRadius2D(polarRadius2D(x, y), z)`.

### Matrix Operations

**Storage**: 3×3, **column-major** as `int16_t vals[9]`:
```
vals[0]=_11  vals[1]=_21  vals[2]=_31   (column 1)
vals[3]=_12  vals[4]=_22  vals[5]=_32   (column 2)
vals[6]=_13  vals[7]=_23  vals[8]=_33   (column 3)
```

**`mat_mul_vector(invec, mat, outvec)`**: Standard `mat × invec` multiply with Q14 scaling. Each product: `((long)m_ij * v_j) >> 14`. Skips zero terms (legacy 16-bit optimization).

**`mat_multiply(rmat, lmat, outmat)`**: `rmat × lmat → outmat`. Counter-based loop with pointer arithmetic matching the column-major layout.

**`mat_invert(inmat, outmat)`**: **Transpose** (valid only for orthonormal rotation matrices). Supports in-place.

**`mat_rot_zxy(z, x, y, use_alt_mult_order)`**: Composite rotation:
- Normal: `Y × (Z × X)`
- Alt (`use_alt_mult_order & 1`): `Z × (Y × X)`

**Important**: Caches the Y rotation matrix (`mat_y_rot_angle`). This cache can cause bugs if the angle is reused across contexts without invalidation.

### Perspective Projection

**`vector_to_point(vec, outpt)`**: Projects camera-space VECTOR to screen-space POINT2D:
```
screen_x = projectiondata5 + (cam_x × projectiondata9) / cam_z
screen_y = projectiondata8 - (cam_y × projectiondata10) / cam_z
```
- **Behind camera** (`z ≤ 0`): returns sentinel `(0x8000, 0x8000)`
- **Overflow**: clamps to `0x7D00 / 0x8300` sentinel values
- `projectiondata5` = screen center X, `projectiondata8` = screen center Y
- `projectiondata9` = horizontal focal length, `projectiondata10` = vertical focal length

**`vector_lerp_at_z(vec1, vec2, outvec, z)`**: Linear interpolation at a given Z:
```
outvec.x = (v1.x - v2.x) * (z - v2.z) / (v1.z - v2.z) + v2.x
```
Used for near-plane clipping.

### Rectangle Operations

- `rect_compare_point(rect, x, y)` — Cohen-Sutherland outcode: bit 0=above, bit 1=below, bit 2=left, bit 3=right
- `rect_adjust_from_point(rect, x, y)` — Grow rect to include point
- `rect_union(r1, r2)` — Bounding union
- `rect_intersect(r1, r2)` — Clip r1 to r2, returns 1 if no overlap
- `rect_is_overlapping(r1, r2)` — Tests overlap
- `rectlist_add_rect(list, rect)` — Merges rect into non-overlapping list with splitting
- `rect_array_sort_by_top(n, arr)` — Heapsort by top coordinate

---

## 4. 3D Shape Data Format

### SHAPE3D Structure (`src/shape3d.h`)

```c
struct SHAPE3D {
    unsigned short shape3d_numverts;       // vertex count
    struct VECTOR*  shape3d_verts;         // vertex array (local/model space)
    unsigned short shape3d_numprimitives;  // primitive count
    unsigned short shape3d_numpaints;      // paint job count (material slot count)
    char* shape3d_primitives;              // bytecoded primitive stream
    char* shape3d_cull1;                   // per-primitive direction cull bitmask (32-bit)
    char* shape3d_cull2;                   // per-primitive back-face cull bitmask (32-bit)
};
```

### On-Disk Resource Layout

Parsed by `shape3d_init_shape()`:

| Offset | Content | Size |
|--------|---------|------|
| 0 | `SHAPE3DHEADER` (numverts, numprims, numpaints, reserved) | 4 bytes |
| 4 | Vertex array | `numverts × 6` bytes |
| 4 + V×6 | cull1 table (direction culling) | `numprims × 4` bytes |
| 4 + V×6 + P×4 | cull2 table (back-face culling) | `numprims × 4` bytes |
| 4 + V×6 + P×8 | Primitive stream | variable length |

### Primitive Stream Encoding

Each primitive entry:

| Byte | Content |
|------|---------|
| 0 | `fileprimtype` — index into `primidxcounttab[]` and `primtypetab[]` |
| 1 | `primitive_flags` — bitmask |
| 2 .. 2+numpaints−1 | Paint job array (one byte per paint slot) |
| 2+numpaints .. end | Vertex index list (byte indices into vertex array) |

**Stream is terminated by a null byte** (`primitives[0] == 0`).

### Primitive Type Lookup Tables

```c
primidxcounttab[16] = { 0,1,2,3,4,5,6,7,8,9,10, 2, 6, 3, 0,0 };
//                        vertex count per fileprimtype

primtypetab[16]     = { 0,5,1,0,0,0,0,0,0,0, 0, 2, 3, 4, 0,0 };
//                        logical type: 0=polygon, 1=line, 2=sphere, 3=wheel, 5=pixel
```

| fileprimtype | Vertex Count | Logical Type |
|--------------|-------------|--------------|
| 0 | 0 (terminator) | polygon |
| 1 | 1 | pixel (5) |
| 2 | 2 | line (1) |
| 3 | 3 | polygon (0) — triangle |
| 4 | 4 | polygon (0) — quad |
| 5–10 | 5–10 | polygon (0) |
| 11 | 2 | sphere (2) |
| 12 | 6 | wheel (3) |
| 13 | 3 | (type 4) |

### Primitive Flags (byte 1)

| Bit | Mask | Effect |
|-----|------|--------|
| 0 | 0x01 | **Double-sided**: disables back-face culling for this primitive |
| 1 | 0x02 | **Continuation/group**: when parent is culled, skip all subsequent with this flag |

### Paint Job Selection

The `material` field of `TRANSFORMEDSHAPE3D` selects which paint slot to use. For each primitive:
```c
paint_byte = primitives[2 + material]
```
If `material >= numpaints`, it wraps or uses slot 0. The paint byte is used as the **material index** for color/pattern lookup.

---

## 5. Shape Resource Loading

### `shape3d_load_all()` (`src/shape3d.c`)

Loads `game1.p3s` and `game2.p3s` resource packs, then calls `shape3d_init_shape()` for each of the 116 (0x74) shape definitions stored in `game3dshapes[130]`.

### `shape3d_load_car_shapes()` (`src/shape3d.c`)

Loads player + opponent car shape resources. Extracts:
- High-detail and low-detail car body shapes
- Wheel geometry (4 wheels, each with reference vertices for steering animation)
- Stores into `game3dshapes[124..129]` (car shape slots)

### DOS Offset to Shape Pointer Translation

`dos_ofs_to_shape3d(unsigned short ofs)` converts legacy DOS segment offsets to modern `game3dshapes[]` pointers:
```c
index = (ofs - 0x764C) / 22;   // GAME3DSHAPES_DOS_BASE=0x764C, stride=22
if (index >= 0 && index < 130)
    return &game3dshapes[index];
```

**Debugging note**: If shapes appear missing or wrong, check:
1. The DOS offset in the track object list against `GAME3DSHAPES_DOS_BASE` (0x764C)
2. That `shape3d_verts != 0` (shape actually loaded)
3. The stride is exactly 22 bytes per shape slot

---

## 6. Projection System

### `set_projection(i1, i2, i3, i4)` (`src/shape3d.c`)

**Parameters**: `i1` = horizontal FOV (degrees), `i2` = vertical FOV (degrees), `i3` = viewport width, `i4` = viewport height.

**Projection parameter computation**:
```c
projectiondata1 = (i1 << 11) / 360 >> 1    // horizontal half-angle
projectiondata2 = (i2 << 11) / 360 >> 1    // vertical half-angle
projectiondata3 = width / 2                 // half-width
projectiondata5 = half_width + offsetX      // center X
projectiondata6 = height / 2               // half-height
projectiondata8 = half_height + offsetY    // center Y
projectiondata9 = cos(hfov_half) * half_width / sin(hfov_half)   // horizontal focal length
projectiondata10 = cos(vfov_half) * half_height / sin(vfov_half) // vertical focal length
```

If `projectiondata2 == 0` (no explicit vertical FOV), derives: `projectiondata10 = projectiondata9 * 0.8125` (4:3 aspect ratio approximation).

### Projection Parameter Reference

| Parameter | Role |
|-----------|------|
| `projectiondata1` | Horizontal half-angle (angular units) |
| `projectiondata2` | Vertical half-angle |
| `projectiondata3` | Half-width of viewport (pixels) |
| `projectiondata4` | X offset |
| `projectiondata5` | Center X = half-width + offset |
| `projectiondata6` | Half-height of viewport |
| `projectiondata7` | Y offset |
| `projectiondata8` | Center Y = half-height + offset |
| `projectiondata9` | Horizontal focal length |
| `projectiondata10` | Vertical focal length |

### Sphere Radius Projection

`transformed_shape_op_helper2(worldRadius, cam_z)`:
```c
screenRadius = (projectiondata9 * worldRadius) / cam_z
```

### In-Game Projection Setup

In `run_game()`:
```c
set_projection(0x23, dashbmp_y / 6, 0x140, dashbmp_y);
// 0x23 = 35° horizontal FOV, 0x140 = 320 pixels wide
```

---

## 7. Camera System

### Camera State Variables

```c
var_vec4          // Camera world position
var_vec5          // Subject (car) world position
var_angY          // Camera yaw (10-bit, or -1 = "look at target")
var_angX          // Camera pitch (10-bit)
var_angZ          // Camera roll (10-bit)
cameramode        // 0=cockpit, 1=fixed, 2=orbit, 3=waypoint
followOpponentFlag // 0=player, !=0=opponent
```

### Mode 0 — Cockpit/Hood View

1. Take car rotation angles directly as camera angles
2. Build inverse rotation matrix: `mat_rot_zxy(-angZ, -angX, -angY, 0)`
3. Rotate local offset `(0, car_height - 6, 0)` by inverse matrix
4. Camera position = car position + rotated offset
5. `var_angY != -1` → no look-at logic

### Mode 1 — Fixed Point View

Camera placed at static position `state.game_vec1[followOpponentFlag]`. `var_angY = -1` triggers look-at.

### Mode 2 — Orbit/Chase View

1. Rotate `(0, 0, 0x4000)` by car's inverse rotation → forward vector
2. Compute horizontal heading: `polarAngle(forward.x, forward.z)`
3. Rotate `(0, 0, rotation_x_angle)` by orbit control matrix
4. Camera = car position + result
5. `var_angY = -1` triggers look-at

### Mode 3 — Track Waypoint View

Camera at `trackdata9[current_waypoint]`, elevated by `camera_y_offset + 0x5A`. `var_angY = -1` triggers look-at.

### External Look-At Logic (when `var_angY == -1`)

Fires for modes 1, 2, 3:

1. `build_track_object(&camera_pos)` — get terrain height at camera position
2. Clamp camera Y above terrain
3. Collision check with `plane_get_collision_point` — push camera above surfaces
4. Yaw: `var_angY = -polarAngle(car.x - cam.x, car.z - cam.z) & 0x3FF`
5. Pitch: `var_angX = polarAngle(car.y - cam.y + 0x32, distance2D) & 0x3FF`

The `0x32` offset (50 units) makes the camera look slightly above the car.

### View Matrix Construction

```c
var_mat = *mat_rot_zxy(var_angZ, var_angX, 0, 1);   // roll + pitch only
var_52 = select_cliprect_rotate(var_angZ, var_angX, var_angY, cliprect, 0);
// ↑ sets mat_temp = full view matrix (all 3 angles), resets polyinfo
```

`select_cliprect_rotate()` stores the combined camera matrix in `mat_temp` (the global view matrix used by all subsequent transforms). Returns the heading angle used for tile lookahead table selection.

---

## 8. Frame Rendering Pipeline (`update_frame`)

`void update_frame(char arg_0, struct RECTANGLE* arg_cliprectptr)` in `src/frame.c`.

### Phase-by-Phase Breakdown

| Phase | Lines | Description |
|-------|-------|-------------|
| 0. Init | 254–330 | Dirty rect init, double-buffer swap, set rendering flags |
| 1. Camera | 331–449 | Select subject, compute camera position/angles per mode |
| 2. View Matrix | 497–530 | Build view matrix, compute sky direction flag |
| 3. Clouds | 533–570 | Render 8 distant cloud/mountain objects |
| 4. Tile Grid | 575–695 | Determine 23 visible tiles from lookahead tables |
| 5. Car-Tile | 699–790 | Determine which tile each car is on |
| 6. Per-Tile Loop | 793–1470 | **Main loop**: render terrain, fences, elements, cars, obstacles |
| 7. Skybox | 1471–1500 | Render horizon background behind all geometry |
| 8. Rasterize | 1501–1507 | `get_a_poly_info()` flushes polygon list to framebuffer |
| 9. Explosions | 1508–1554 | Draw crash explosion sprites |
| 10. Effects | 1558–1587 | Windshield crack / water sinking effects |
| 11. HUD Timer | 1589–1605 | Draw elapsed time in LED font |
| 12. Text/Rects | 1607–1619 | In-game text overlay + dirty rect finalization |

### Key Detail: Rendering Order

**Important for debugging**: The render order is:
1. Clouds (sky objects) rendered first
2. Tiles rendered back-to-front (by lookahead table order)
3. Within each tile: immediate shapes (terrain/fences) → sorted shapes (buildings/cars)
4. Skybox rendered **after** all geometry (fills remaining background)
5. `get_a_poly_info()` rasterizes **all** accumulated polygons at once (painter's algorithm)

This means all polygon info records from phases 3–6 are accumulated in a single sorted linked list, then drawn in one pass during phase 8.

---

## 9. World Tile Assembly & Culling

### Tile Maps

- **`td14_elem_map_main[900]`** — Track element ID per tile (30×30). Values: 0=empty, 1–0xB5=elements, 0xFD/FE/FF=multi-tile markers
- **`td15_terr_map_main[901]`** — Terrain type per tile (0–18)
- Indexed as: `td14_elem_map_main[trackrows[row] + col]`

### Camera to Tile Conversion

```c
tile_col = camera.x >> 10;                       // X → column
tile_row = -((camera.z >> 10) - 0x1D);            // Z → row (inverted!)
```

### Lookahead Tile Table

8 pre-computed tables (`polygon_stroke_pattern_0` through `_6` + `unk_3BF6C`) contain 23 entries × 3 bytes each: `(dx, dz, detail_level)`. Selected by heading octant: `(heading & 0x3FF) >> 7`.

Each entry describes a tile offset from the camera tile and a minimum detail level required to render it.

### Per-Tile Visibility Tests

For each of 23 tile slots (iterated **backward**, far to near):

1. **Already processed** (`var_32[si] != 0`): skip
2. **LOD threshold**: `detail_level <= sprite_clip_region_selector[timertestflag2]` — if not met, skip
3. **Bounds check**: column and row must be in [0, 0x1D]
4. **Hill-road substitution**: terrain 7–10 → `subst_hillroad_track()` converts flat road elements to hill variants
5. **Multi-tile redirect**: 0xFD→(−1,−1), 0xFE→(0,−1), 0xFF→(−1,0) — look at the anchor tile
6. **Physics culling** (`timertestflag2 ≥ 3`): suppress non-essential tiles
7. **Multi-tile sibling marking**: prevent double-rendering of multi-tile elements

### Multi-Tile Element Types

| `multi_tile_flag` | Layout | Marker |
|-------------------|--------|--------|
| 0 | 1×1 | None |
| 1 | 1×2 (extends south) | 0xFE below |
| 2 | 2×1 (extends east) | 0xFF right |
| 3 | 2×2 | 0xFF right, 0xFE below, 0xFD diagonal |

### Tile Center Position Tables

```c
trackcenterpos2[col]   // X world coordinate of tile column center
trackcenterpos[row]    // Z world coordinate of tile row center
trackpos2[col+1]       // X world coordinate of right edge
trackpos[row]          // Z world coordinate of top edge
```

---

## 10. 3D Shape Transformation Pipeline

### TRANSFORMEDSHAPE3D Structure

```c
struct TRANSFORMEDSHAPE3D {
    struct VECTOR pos;                     // world position relative to camera
    struct SHAPE3D* shapeptr;              // 3D model
    struct RECTANGLE* rectptr;             // dirty rect tracking (if flag 8)
    struct VECTOR rotvec;                  // rotation angles (Z, X, Y Euler)
    unsigned short shape_visibility_threshold; // distance for LOD culling
    unsigned char ts_flags;                // behavior flags
    unsigned char material;                // paint job index
};
```

### ts_flags Bitmask

| Bit | Mask | Effect |
|-----|------|--------|
| 0 | 0x01 | Force **unsorted** insertion (polygon placed at current list position) |
| 1 | 0x02 | **Pre-transformed** position (skip mat_temp multiply; disable direction culling) |
| 3 | 0x08 | Track **bounding rectangle** (expand rectptr from projected vertices) |

### Transform Pipeline in `shape3d_render_transformed()`

The function does NOT construct world-space intermediates. It composes model→camera directly:

**Step 1 — Build rotation matrix**:
```c
var_rotmatptr = mat_rot_zxy(rotvec.x, rotvec.y, rotvec.z, 0);
```

**Step 2 — Transform position to camera space**:
```c
mat_mul_vector(&pos, &mat_temp, &var_vec);   // pos → camera space
```
(Skipped if `ts_flags & 2`)

**Step 3 — Compose model→camera matrix**:
```c
mat_multiply(var_rotmatptr, &mat_temp, &var_mat2);   // model rot × camera = combined
```

**Step 4 — Direction culling setup**:
```c
mat_invert(&var_mat2, &var_mat);
var_vec2 = {0, 0, 0x1000};
mat_mul_vector(&var_vec2, &var_mat, &var_vec3);    // camera forward in model space
bucket = vector_direction_bucket32(&var_vec3);      // → 0–31
var_45C = invpow2tbl[bucket];                       // bitmask for culling
```

**Step 5 — Per-vertex transform** (lazy, on first reference):
```c
mat_mul_vector(&local_vert, &var_mat2, &camera_vert);
camera_vert.x += var_vec.x;
camera_vert.y += var_vec.y;
camera_vert.z += var_vec.z;
```

**Step 6 — Near-plane check**: `camera_vert.z < 0x0C` → flagged as clipped

**Step 7 — Perspective projection**: `vector_to_point(&camera_vert, &screen_pt)`

**Step 8 — Viewport clip test**: `rect_compare_point(cliprect, pt.px, pt.py)` → 4-bit outcode

### Lazy Vertex Evaluation

Vertices use a flag table `var_vertflagtbl[]`:
- `0xFF` = not yet transformed
- `0` = transformed, in front of near plane
- `1` = behind near plane (Z < 12)

Results cached in `var_vecarr[]` (camera space) and `var_vecarr2[]` (screen space POINT2D).

### Early Rejection

Before processing primitives, the first `min(numverts, 8)` vertices are transformed. If all projected vertices share the same clip outcode bit (all left, all right, all above, or all below), the entire shape is rejected → returns −1.

### Half-Scale Preview Mode

When `select_rect_scale_preview != 0`, all vertex coordinates are halved before matrix multiply. Used for thumbnail/track preview rendering.

---

## 11. Depth Sorting System

### Two-Level Sorting

**Level 1 — Per-tile shape sorting** (`frame.c`):

Within each tile, shapes are either:
- **Immediately rendered** (`trkobj_ignore_zbias & 1`): terrain, fences, road surfaces — always behind other objects
- **Added to sort queue**: `transformed_shape_add_for_sort(zadjust, type)`

After all objects for a tile are queued:
```c
transformedshape_sort_desc(zvals, indices, count);  // bubble sort, descending Z
```
Then sorted shapes are rendered far-to-near via `shape3d_render_transformed()`.

**Level 2 — Per-polygon depth sorting** (`shape3d.c`):

Every polygon emitted by `shape3d_render_transformed()` is inserted into a **global sorted linked list** (`zorder_shape_list[401]`). This list is maintained in **descending depth order** (farthest first = painter's algorithm).

### `transformed_shape_add_for_sort()` (`render.c`)

1. Transform shape position by `mat_temp` → camera-space Z
2. Store `Z + zadjust` as sort key in `transformedshape_zarray[slot]`
3. Store type tag (0=generic, 2=player, 3=opponent) in `transformedshape_arg2array[slot]`
4. Increment `transformedshape_counter`, advance `curtransshape_ptr`

### Z-Bias Adjustments

| Object Type | Z adjust | Purpose |
|-------------|----------|---------|
| Generic | 0 | Default |
| Car (ground contact) | −0x800 | Push cars closer → drawn later (on top) |
| Car (with overlay) | −0x400 | Reduced bias when overlay present |
| Start marker | −0x800 & var_12A | var_12A=0 on start tile (no bias) |

### Polygon Sorted Linked List

Array-based linked list of up to 400 polygon entries + 1 sentinel:

- `zorder_shape_list[401]`: next-index links. Index 0x190 (400) = head sentinel
- `polyinfoptrs[401]`: pointers to polygon info data
- Traversal: head → farthest, tail → nearest

**Insertion** (`transformed_shape_op_helper(depth_key, sorted)`):
- `sorted=0` (unsorted): insert at current cursor position (for continuation groups)
- `sorted=1`: walk backward from `zorder_next_link` up to `zorder_tail_counter` links, compare depths, insert in order

**Traversal** (`get_a_poly_info()`): walk from `zorder_shape_list[0x190]` following links → back-to-front rendering.

---

## 12. Back-Face & Direction Culling

### Direction Culling (32-bucket system)

Each primitive has a `cull1` bitmask (32 bits). The camera's viewing direction in model space is mapped to one of 32 buckets via `vector_direction_bucket32()`. The corresponding bit in `invpow2tbl[bucket]` is tested:

```c
if ((cull1[prim_idx] & var_45C) == 0) → skip primitive
```

**When disabled**: If shape is very close (within `shape_visibility_threshold`), `var_45C = 0xFFFFFFFF` — all primitives pass.

### Back-Face Culling (screen-space winding)

After projection, for non-double-sided polygons:

```c
if (primitive_flags & 1) → accept (double-sided)
if (var_A & *cull2) → accept (cull2 override)
if (transformed_shape_op_helper3(screen_pts) == 0) → reject
```

`transformed_shape_op_helper3()` computes the 2D cross product of edges p1→p0 and p1→p2:
```c
cross = (dx1 * dy0) - (dx0 * dy1)
return cross <= 0 ? 0 : 1;   // 0 = CCW = back-facing → cull
```

### Primitive Group Skipping

When a primitive is culled and `primitive_flags & 2` is set on the **next** primitive, all subsequent continuation primitives are skipped:
```c
while (next_prim_flags & 2) {
    advance past primitive;
}
```
This implements static LOD detail groups.

---

## 13. Near Plane Clipping

### Near Plane Distance

`Z_NEAR = 0x0C` (12 world units). Vertices with camera-space Z < 12 are flagged as behind the near plane.

### Polygon Clipping (Sutherland-Hodgman style)

When a polygon has a mix of near/far vertices:

For each edge (vertex pair):
- **Both behind**: edge dropped
- **Both in front**: edge kept
- **One in, one out**: interpolate new vertex at Z=12:
  ```c
  vector_lerp_at_z(&front, &behind, &clipped, 0x0C);
  vector_to_point(&clipped, &clipped_screen);
  ```

The clipped vertices are inserted into the output polygon, replacing the behind-camera portion.

### Line Clipping

Same lerp approach — if one endpoint is behind near plane, clip to Z=12.

### Wheel Clipping

No partial clipping — if ANY wheel vertex is behind near plane, the entire wheel is skipped.

### Viewport Clipping

**No geometric polygon clipping against viewport edges.** Only trivial rejection via Cohen-Sutherland outcode AND. Actual per-pixel clipping happens in the scanline rasterizer (`draw_filled_lines` clamps X per scanline).

---

## 14. Polygon Info System

### Polygon Info Record Format

Each polygon info entry lives in a contiguous buffer (`s_polyinfo_base`, 0x28A0 = 10400 bytes):

| Byte | Size | Content |
|------|------|---------|
| 0–1 | uint16 | **Depth** (average camera-space Z) |
| 2 | uint8 | **Material/paint** index |
| 3 | uint8 | **Vertex count** |
| 4 | uint8 | **Primitive type** (0=poly, 1=line, 2=sphere, 3=wheel, 5=pixel) |
| 5 | uint8 | (padding) |
| 6+ | N × POINT2D (8 bytes each) | Screen-space vertex data |

Total record size: `6 + numverts × 8` bytes.

### For Spheres (type 2)

- `vertices[0]` = center point
- `vertices[1].px` = projected radius

### For Wheels (type 3)

- 4 POINT2D entries defining the axle quad

### Buffer Overflow Protection

- Max polygons: `polyinfonumpolys >= 0x190` (400) → overflow error
- Max buffer: `polyinfoptrnext > 0x2872` → overflow error

### Reset

`polyinfo_reset()`: Called at frame start and by `get_a_poly_info()` at end:
```c
polyinfonumpolys = 0;
polyinfoptrnext = 0;
polygon_op_error_code = 0;
zorder_shape_list[0x190] = -1;
polygon_info_head = 0x190;
```

---

## 15. Rasterizer (Scanline Fill)

### `get_a_poly_info()` — Dispatch Loop

Walks the sorted linked list (back-to-front) and dispatches each polygon to its rasterizer:

| Type | Handler | Fill Function |
|------|---------|---------------|
| 0 (polygon), pattern=0 | `preRender_default()` | `draw_filled_lines()` (solid) |
| 0 (polygon), pattern=1 | `preRender_patterned()` | `draw_patterned_lines()` (dithered) |
| 0 (polygon), pattern=2 | `preRender_two_color_pattern()` | `draw_two_color_patterned_lines()` |
| 1 (line) | `preRender_line()` | `putpixel_line1_maybe()` (Bresenham) |
| 2 (sphere) | `preRender_sphere()` | LUT or 32-vertex ellipse + `draw_filled_lines()` |
| 3 (wheel) | `preRender_wheel()` | 16 quad fills + 2 cap fills |
| 5 (pixel) | `putpixel_single_maybe()` | Direct pixel write |

### `preRender_default_impl()` — Core Polygon Rasterizer

**Scanline buffer**: `int16_t var_798[960]` — two 480-entry arrays:
- `var_798[0..479]` = left edge x1 — initialized to `sprite_widthsum` (far right)
- `var_798[480..959]` = right edge x2 — initialized to `sprite_left2 - 1` (far left)

**Pipeline**:

1. **AABB reject**: Compute bounding box of all vertices. If entirely outside sprite bounds → return
2. **Degenerate case**: If minX==maxX or minY==maxY → single fill call
3. **Edge walking**: For each polygon edge (i → i+1 % N):
   - Skip horizontal edges (y0 == y1)
   - Sort so y0 < y1
   - Clamp Y to `[sprite_top, sprite_height)`
   - Compute fixed-point X step: `x_step = (Δx << 16) / Δy`
   - Walk y_start to y_end: update min(x1) / max(x2) per scanline
4. **Fill**: Call `spritefunc(color, count, y_start, &x2[y], &x1[y])`

### `draw_filled_lines()` (`src/shape2d.c`) — Solid Fill

Parameters: `(color, numlines, y, x2arr[], x1arr[])`

Per scanline:
1. Read x1, x2 (signed 16-bit)
2. Skip if `x2 < x1` or fully outside `[sprite_left2, sprite_widthsum-1]`
3. Clamp to bounds
4. Compute offset: `lineofs[y + i] + x1`
5. Write `color` byte for `(x2 - x1 + 1)` pixels

### `draw_patterned_lines()` — Dithered Fill

Uses `polygon_pattern_type` — 16-bit value, two alternating 8-bit patterns:

Per scanline:
1. Pattern = low byte of `polygon_pattern_type`
2. ROL-rotate by `(x1 & 7)` to align with screen X
3. For each pixel: ROL pattern by 1; if carry bit was 1 → write pixel, else skip
4. After line: swap high/low bytes of `polygon_pattern_type` → creates checkerboard effect

### `draw_two_color_patterned_lines()` — Two-Color Fill

Same as patterned, but: bit=1 → write `polygon_alternate_color`; bit=0 → write primary color. Every pixel is written (no transparency).

---

## 16. Material & Color System

### Material Lookup Tables

Three parallel 129-entry × 2-byte tables:

| Table | Purpose |
|-------|---------|
| `material_color_list[]` | Primary VGA palette index per material |
| `material_pattern_list[]` | Pattern type: 0=solid, 1=patterned, 2=two-color |
| `material_pattern2_list[]` | Sub-pattern bitmask or alternate color |

Pointer indirection (`material_clrlist_ptr_cpy`, etc.) allows per-scenery override when loading different terrain sets (desert/alpine/etc.).

### Lookup in `get_a_poly_info()`

```c
mattype = polyinfoptr[2];                  // material index from polygon
matcolor = clrlist[mattype * 2];           // VGA color byte
matpattern = patlist[mattype * 2];         // pattern type (0/1/2)
```

### Special Material Indices

| Index | Function | Color Variable |
|-------|----------|----------------|
| 0x10 | Ground | `skybox_grd_color` |
| 0x11 | Sky | `skybox_sky_color` |
| 0x2D | Brake light (off) | Overridden by `backlights_paint_override` |
| 0x2E | Brake light color (off) | Fixed in palette |
| 0x2F | Brake light color (on) | Fixed in palette |
| 0x64 | Water | `skybox_wat_color` |

### Paint Override for Car Materials

When rendering car shapes, `material` field selects paint job. The `backlights_paint_override` global is set based on braking state:
- **Braking**: `backlights_paint_override = 0x2F` (bright)
- **Not braking**: `backlights_paint_override = 0x2E` (dim)

This applies to any primitive with material index 0x2D.

### Road Surface Pattern

`var_E4 = polygon_edge_direction_table[game_frame & 0xF]` — cycles road markings per frame. Negative `trkobj_surface` values use this cycling pattern.

---

## 17. Skybox Rendering

### `render_skybox_layer()` (`src/render.c`)

Called **after** all 3D geometry is batched but **before** `get_a_poly_info()` rasterizes. This means the skybox is drawn first and geometry is painted on top (painter's algorithm — skybox is furthest).

**Pipeline**:
1. Compute two horizon vectors by offsetting camera angles
2. Transform via camera matrix
3. Project to screen → find horizon line
4. **Off-screen tests**: If horizon entirely above/below screen → fill with sky or ground
5. **Normal path**: Compute horizon angle, draw sky polygon above, ground polygon below
6. Draw panorama texture strips at horizon line

### Skybox Data

5 named skybox sets: desert, tropical, alpine, city, country. Each contains 4 panorama strip images. Loaded by `load_skybox()`.

### Color Sources

```c
skybox_sky_color = render_get_material_color(0x11)
skybox_grd_color = render_get_material_color(0x10)
skybox_wat_color = render_get_material_color(0x64)
```

### Cloud Objects

8 distant scenery objects (mountains, clouds) rendered in Phase 3 of `update_frame`:
- Only when `timertestflag2 == 0` (full detail)
- Each has an angle offset, view-frustum culled against [0x87, 0x379]
- Positioned at Y = sky_altitude − camera_y, Z fixed at 15000
- Shape from `off_3BE44[cloud_index]`

---

## 18. Car Shape & Wheel Rendering

### Car Shape Setup

Cars use `trkObjectList[2]` (player) and `trkObjectList[3]` (opponent). LOD selection:
- High-detail: when `timertestflag2 <= 2` and `var_FC == 0` (near camera)
- Low-detail: otherwise (uses `trkobj_loshape`)

### Wheel Vertex Animation

`shape3d_update_car_wheel_vertices()` modifies the shape's vertex array to animate front wheel steering:
- Reads baseline wheel vertex positions
- Rotates by `car_steeringAngle` via sin/cos
- Writes back modified vertices
- Only for high-detail model

### Car Position/Rotation

```c
pos.x = car_posWorld1.lx >> 6 - camera.x
pos.y = (car_posWorld1.ly >> 6) - camera.y
pos.z = car_posWorld1.lz >> 6 - camera.z

rotvec.x = -car_rotate.z    // roll
rotvec.y = -car_rotate.y    // pitch
rotvec.z = -car_rotate.x    // yaw (NOTE: x/z swap!)
```

**Critical convention**: `car_rotate.x` = yaw, `.y` = pitch, `.z` = roll. But the renderer expects `rotvec.x` = roll, `.y` = pitch, `.z` = yaw. Hence the swap/negation.

### Wheel Rendering (`preRender_wheel`)

Wheel primitives (type 3) use 6 vertices defining axle geometry:
1. `preRender_wheel_helper()` generates two 16-point ring outlines offset by the axle vector
2. 16 quad fills between inner and outer rings (the tire)
3. 2 cap fills (flat disc faces)
4. 3 colors: body, rim, tread

### Debris/Obstacle Rendering

When car crashes into a destructible obstacle:
- `game_obstacle_flags[slot]` set to 1
- Debris particles spawned into 24-slot arrays
- Each particle has position, velocity, rotation angle
- Rendered as small 3D shapes with `shape3d_render_transformed()`

---

## 19. Special Effects Rendering

### Explosion Sprites (Phase 9)

For crashed cars that were successfully rendered (`var_DC[i] == 1`):
1. Compute center of car's screen bounding box
2. Select explosion frame: `game_frame >> 2` cycles through 3 frames
3. Scale proportional to bounding box size
4. Draw using `shape_op_explosion()` (nearest-neighbor scaled sprite, 0xFF transparent)

### Windshield Crack (Phase 10, cockpit mode)

`init_crak(framecount, offset, height)`:
1. Load crack shape data from game resources
2. Compute frame index from `framecount / (framespersec / 7)`
3. Read crack line coordinates from packed data
4. For each line: draw 3 lines (black outline at Y−1 and Y+1, colored center)

### Water Sinking (Phase 10, cockpit mode)

`do_sinking(framecount, offset, height)`:
1. Interpolate: `si = height * framecount / (framespersec * 4)`
2. Fill growing band at bottom of viewport with `skybox_wat_color`

---

## 20. Sprite / 2D Layer System

### Core Data Structures (`src/shape2d.h`)

**SHAPE2D** (16 bytes):
```c
struct SHAPE2D {
    unsigned short s2d_width, s2d_height;
    unsigned short s2d_hotspot_x, s2d_hotspot_y;    // for scaling/centering
    unsigned short s2d_pos_x, s2d_pos_y;            // default draw position
    unsigned char  s2d_plane_nibbles[4];             // EGA plane config
};
// Pixel data follows immediately (width × height bytes)
```

**SPRITE** (render target + clip rect):
```c
struct SPRITE {
    struct SHAPE2D* sprite_bitmapptr;   // backing buffer
    unsigned int*   sprite_lineofs;     // per-row byte offset table
    unsigned short  sprite_left, sprite_right, sprite_top, sprite_height;  // clip bounds
    unsigned short  sprite_pitch;       // bytes per row
    unsigned short  sprite_left2, sprite_widthsum;  // secondary bounds (for fill functions)
};
```

### Global Render Targets

- **`sprite1`** — primary render target. All drawing primitives write here.
- **`sprite2`** — secondary target for save/restore.
- Both initialized to the VGA framebuffer (320×200, pitch=320).

### Sprite Window Stack

Windows are stack-allocated from `wnd_defs[]` (0xE10 bytes):
- `sprite_wnd_stack_reserve()` — allocate SPRITE + lineofs from stack top (LIFO)
- `sprite_wnd_stack_release()` — free top entry
- `sprite_make_wnd()` — allocate backing bitmap + SPRITE for off-screen rendering

### Copy Operations

| Function | Blend Mode |
|----------|------------|
| `sprite_putimage()` | Direct copy (at shape's default pos) |
| `sprite_putimage_and()` | `dst &= src` (mask — punch hole) |
| `sprite_putimage_or()` | `dst |= src` (overlay into hole) |
| `sprite_putimage_transparent()` | Copy, skip 0xFF (transparent) |

### Dashboard Rendering

The dashboard uses three off-screen window sprites:
- `whlsprite1` — instruments work area
- `whlsprite2` — gear knob work area
- `whlsprite3` — cached dashboard background

Dashboard overlay uses AND+OR two-pass technique:
```c
shape2d_render_bmp_as_mask(mask)    // AND mask: creates hole
shape2d_draw_rle_or(overlay)       // OR overlay: fills hole with colored pixels
```

### RLE Compression

Used for dashboard shapes. Format:
- Byte > 0: run (repeat next byte N times)
- Byte < 0: literal (copy next −N bytes)
- Byte == 0: end

---

## 21. Framebuffer & SDL2 Presentation

### VGA Simulation

- `sdl_framebuffer[64000]` — 320×200 indexed-color byte array (replaces VGA segment 0xA000)
- `sdl_fb.palette[256][3]` — 6-bit VGA palette (0–63 per channel)

### Presentation Pipeline (`framebuffer.c`)

```
video_present():
  memcpy(sdl_fb.pixels, sdl_framebuffer, 64000)
  vga_sdl2_present():
    vga_fb_to_rgba(sdl_fb, ctx.rgba, 64000)     // indexed → ARGB8888
    SDL_UpdateTexture(texture, rgba, 320*4)
    SDL_RenderClear + SDL_RenderCopy + SDL_RenderPresent
```

### Color Expansion

6-bit VGA → 8-bit: `r8 = (r6 << 2) | (r6 >> 4)` (e.g., 0x3F → 0xFF)

### SDL Window

Created at 2× scale (640×400). `SDL_RenderSetLogicalSize(renderer, 320, 200)` handles upscaling. Software renderer preferred via `SDL_HINT_RENDER_DRIVER = "software"`.

### Hardening

SDL context pointers are validated against saved copies before each present to silently recover from pointer corruption.

---

## 22. Dirty Rectangle System

### Concept

15 slots of `RECTANGLE` for tracking changed screen regions across double-buffered frames:
- `rect_array_unk[15]` — current frame dirty rects
- `rect_array_unk2[15]` — previous frame dirty rects

### Tracking

When `timertestflag_copy != 0`, shapes with `ts_flags & 8` expand their `rectptr` to encompass all projected vertices. These rects accumulate in per-frame arrays.

### Presentation

In `render_present_ingame_view()`:
1. Mark changed rect slots
2. `rectlist_add_rects()` computes minimal set of changed sub-rects
3. `rect_array_sort_by_top()` sorts by Y for cache-friendly blitting
4. Blit only changed regions instead of full screen

---

## 23. Track Data System

### Track Grid

30×30 tiles. Two parallel maps:
- `td14_elem_map_main[]` — element IDs (0–0xB5, 0xFD–0xFF for multi-tile markers)
- `td15_terr_map_main[]` — terrain types (0–18)

### Terrain Types

| Value | Type | Height Effect |
|-------|------|---------------|
| 0 | Flat grass | 0 |
| 1 | Lake center | Water surface |
| 2–5 | Coast edges | Partial water (4 rotations) |
| 6 | Elevated plateau | +0x1C2 (450 units) |
| 7–10 | Hill slopes | N/E/S/W directional slopes |
| 11–14 | Concave hills | Corner slopes (inner) |
| 15–18 | Convex hills | Corner slopes (outer) |

### Track Object Record (14 bytes)

`trkObjectList[element_id * 14]`:

| Offset | Field | Purpose |
|--------|-------|---------|
| 0–1 | info_ofs | Offset to TRKOBJINFO |
| 2–3 | rot_y | Y rotation (0/0x100/0x200/0x300) |
| 4–5 | shape_ofs | Hi-detail 3D shape offset |
| 6–7 | lo_shape_ofs | Lo-detail shape offset |
| 8 | overlay | Overlay element index (road markings etc.) |
| 9 | surface_type | Surface material (negative = use cycling road pattern) |
| 10 | ignore_zbias | Bit 0: render immediately (vs. sorted) |
| 11 | multi_tile | 0/1/2/3 = tile span layout |
| 12 | physical_model | Physics switch case (0–74) |
| 13 | scene_unk5 | Unknown flag |

### `track_setup()` — Validation & Path Building

~900 lines, 11 phases:
1. Allocate backtrack buffer
2. Initialize arrays
3. Validate terrain connectivity (E↔W, N↔S edge compatibility)
4. (continued)
5. Find start/finish line (exactly one required)
6. Initialize tracking arrays
7. Follow track connectivity using TRKOBJINFO entry/exit points
8. Verify loop completion
9. Record path arrays (`td21_col`, `td22_row`, `td17_elem_ordered`)
10. Distribute checkpoints along path
11. Cleanup

### Hill-Road Substitution

`subst_hillroad_track()` maps flat-road elements to hill variants:

| Terrain | Example: 0x04 (straight road) → |
|---------|--------------------------------|
| 7 (N slope) | 0xB6 |
| 8 (E slope) | 0xB7 |
| 9 (S slope) | 0xB8 |
| 10 (W slope) | 0xB9 |

Returns 0 if element cannot be placed on that hill direction.

---

## 24. Track Object & Collision Geometry (`build_track_object`)

### Purpose

Called every physics frame per wheel. Determines the physics surface, collision planes, and wall data for a world position.

### Output Globals

| Global | Purpose |
|--------|---------|
| `planindex` | Surface plane index → `planptr[planindex]` |
| `wallindex` | Wall collision index (−1 = none) |
| `wallHeight` | Wall collision height |
| `current_surf_type` | Surface type (0=airborne, 1=tarmac, 4=grass, 5=water) |
| `terrainHeight` | Y elevation offset |
| `track_object_render_enabled` | Can this tile render |
| `elem_xCenter` / `elem_zCenter` | Tile center in world coords |
| `wallOrientation` | Wall angle |
| `wallStartX` / `wallStartZ` | Wall collision start point |
| `corkFlag` | Set inside corkscrews |

### Processing Steps

1. **World→Tile**: `col = x >> 10`, `row = z >> 10`
2. **Terrain height**: type 6 → +450, types 2–5 → sin/cos coast test
3. **Element lookup**: resolve 0xFD/FE/FF markers
4. **Coordinate rotation**: by element's `rot_y` (0/90/180/270°)
5. **Physical model switch** (75 cases): determines planindex, wallindex, surface type

### Physical Model Examples

| Model | Element | Surface Test |
|-------|---------|-------------|
| 0 | Start/finish | Road (x < 0x78) + lap planindex |
| 1 | Straight road | `|x| < 0x78` → pavement |
| 2 | Sharp corner | `polarRadius2D` between 0x188–0x278 |
| 3 | Large corner | `polarRadius2D` between 0x588–0x678 |
| 11 | Highway | Dual-lane `|x| < 0x168`, center wall |
| 16–17 | Ramp | Z-dependent planindex |
| 27 | Loop | 6 Z-slices, X-bound interpolation |
| 28 | Tunnel | 4-wall collision box |
| 30–31 | Pipe/half-pipe | 12 radial sections |
| 32–33 | Corkscrew | 24 spiral segments via `polarAngle` |

### planindex Encoding

```c
planindex = (base_plane << 2) + orientation_offset
```
Where orientation_offset = 0 (0°), 1 (270°), 2 (180°), 3 (90°).

### Grass Height Wobble

```c
if (surface == GRASS) {
    hashBit = ((z ^ x) >> 8) & 1;  // position-based hash
    terrainHeight += hashBit;        // 0 or 1
} else {
    terrainHeight += 2;              // paved roads slightly above grade
}
```

---

## 25. Vehicle Physics Engine

### Architecture

Three main functions per simulation frame (all in `src/carstate.c`):
1. `update_car_speed()` — drivetrain
2. `update_grip()` — tire traction
3. `update_player_state()` — position/orientation integration (~3500 lines!)

### CARSTATE Structure (per car)

| Field | Description |
|-------|-------------|
| `car_speed2` | Forward speed (fixed-point) |
| `car_gear` | Current gear (0–6) |
| `car_rpm` | Engine RPM |
| `car_rotate` | Euler angles: `.x`=yaw, `.y`=pitch, `.z`=roll |
| `car_posWorld1` | 32-bit world position |
| `car_posWorld2` | Previous frame position |
| `car_whlWorldCrds1[4]` | Current wheel world coords |
| `car_whlWorldCrds2[4]` | Previous wheel coords |
| `car_surfaceWhl[4]` | Surface type per wheel |
| `car_rc1[4]` | Free-fall accumulator (gravity) |
| `car_rc2[4]` | Suspension spring displacement |
| `car_rc5[4]` | Suspension damper state |
| `car_crashBmpFlag` | 0=normal, 1=crashed, 2=water, 3=finished |
| `car_steeringAngle` | Front wheel angle |
| `car_36MwhlAngle` | Impact angle for crash bounce |

### SIMD Structure (vehicle specs)

| Field | Description |
|-------|-------------|
| `gear_top[7]` | Max speed per gear |
| `gear_ratio[7]` | Gear ratios (16-bit fixed-point) |
| `torque_curve[64]` | Torque by RPM (indexed `rpm >> 7`) |
| `car_mass` | Mass for acceleration: $a = \frac{\Delta v \times 25}{\text{mass}} \gg 1$ |
| `aero_resistance` | Drag coefficient |
| `grip` | Base grip value |
| `sliding[4]` | Per-wheel sliding coefficient |
| `wheel_coords[4]` | Wheel positions in body space |
| `collide_points[5]` | OBB collision box vertices |

### Drivetrain (`update_car_speed`)

**Gear shift**: Up at RPM > 8000 (`0x1F40`), down at RPM < 3000 (`0xBB8`).

**Torque**: `torque_curve[rpm >> 7]` (64-entry lookup).

**Acceleration**: $a = \frac{\Delta v \times 0\text{x19}}{\text{car\_mass}} \gg 1$

**Aero drag**: `aero_table[speed >> 8]` (pre-computed quadratic).

**Speed clamp**: Max 0xF500 (≈245 mph).

**FPS scaling**: At 10fps, acceleration values doubled.

### Grip/Traction (`update_grip`)

**Grip demand**: $\text{demand} = \frac{\text{speed}^2}{64} \times \frac{|\text{steering}|}{8}$ (16-bit, wrapping allowed)

**Combined grip**: Sum of `simd.sliding[wheel]` for grounded wheels.

**When demand > grip**:
- Steering residual accumulates
- `angle_z` drift per frame
- Speed penalty proportional to sliding
- `car_slidingFlag` set

### Position Integration (`update_player_state`) — The Big One

**Phase 1 — Setup**: Save previous position, compute pseudoGravity from orientation matrix, compute speed scaling factor.

**Phase 2 — Wheel projection**: For each of 4 wheels:
1. Read body-local position from SIMD
2. Add `rc2[i]` suspension displacement
3. Apply drift matrix (if `angle_z ≠ 0`)
4. Transform through car orientation matrix
5. Add to world position → new wheel world position

**Phase 3 — Terrain query**: For each wheel:
1. `build_track_object()` → tile/plane/surface
2. `plane_get_collision_point()` → signed distance to surface

**Phase 4 — Wall collision**: For each wheel when `wallindex >= 0`:
1. Rotate into wall-local space
2. Test if previous/current positions cross wall
3. If crossing: compute approach angle, check speed threshold
4. If over threshold → crash via `update_crash_state(1, ...)`
5. Set bounce angle `car_36MwhlAngle`

**Phase 5 — Gravity**: Airborne wheels:
```c
car_rc1[wheel] += gravity_coefficient;
position.ly -= car_rc1[wheel];
```
Fatal at `rc1 > 0x5AEB` (extreme free-fall).

**Phase 6 — Ground plane intersection**: When wheel contacts surface:
1. Transform to plane-local space
2. Uphill crash detection (both Y < −12, current < −24 → crash type 5)
3. If Y sign changes: interpolate exact contact point
4. Apply plane rotation to post-contact displacement

**Phase 7 — Orientation reconstruction**:
$$\text{yaw} = \text{polarAngle}(\text{rear}_X - \text{front}_X, -(\text{rear}_Z - \text{front}_Z))$$
$$\text{pitch} = \text{polarAngle}(-\Delta Z_{\text{front-rear}}, \Delta Y_{\text{front-rear}}) - 0\text{x100}$$
$$\text{roll} = \text{polarAngle}(\Delta X_{\text{left-right}}, \Delta Y_{\text{left-right}}) - 0\text{x100}$$

Deadzone: |angle| < 2 → snap to 0.

**Phase 8 — Boundary clamping**: X, Z clamped to `[0xF00, 0x1DF0FF]` (1 tile margin).

### Suspension Model (`carState_update_wheel_suspension`)

**Damper** (`rc5`): Decays toward 0 at rate 4/frame.

**Spring** (`rc2`):
- Compression: `+= input`, capped at 0xC0/frame, max 0x180
- Extension: 3/4 damping below −0x120, min −0x180

---

## 26. Collision Detection System

### Car-vs-Car (OBB)

`car_car_detect_collision()`:
1. **AABB rejection** on each world axis
2. **OBB test A→B**: Transform 4 corners of car A into car B's local space, test containment
3. **OBB test B→A**: Symmetric

`car_car_speed_adjust_collision()`:
- Decompose velocities using sin/cos of yaw
- Compute relative speed
- Reduction: `0x300 × relSpeed / 4`
- Set bounce angle: `car_36MwhlAngle = otherAngle - myAngle`

### Car-vs-Obstacle

1. Convert car center to tile coords
2. `bto_auxiliary1(col, row)` finds static obstacles
3. OBB test against obstacle bounding boxes
4. If hit → crash

### Car-vs-Destructible

1. `trackdata19[tile]` → obstacle slot
2. Skip if already destroyed (`game_obstacle_flags[slot]`)
3. OBB test against `td10_track_check_rel` positions
4. If hit: mark destroyed, spawn debris particles

### Start/Finish Pole Collision

Poles at `trackcenterpos ± sin(track_angle ± 0x100) × 0x7E`. OBB test against pole boxes. Hit → crash, `car_36MwhlAngle -= 0x200`.

### Plane-Crossing Crash Detection

Secondary pass: for each wheel, compare `plane_get_collision_point` between previous and current positions. If sign flips (wheel passed through terrain) → `update_crash_state(5, ...)`.

---

## 27. Crash State Machine

`update_crash_state(flag, ...)`:

| Flag | Trigger | Actions |
|------|---------|---------|
| 1 | Standard crash | `crashBmpFlag=1`, debris, record `impactSpeed`, slow-mo (`game_frames_per_sec = fps×4`) |
| 2 | Water/void | `crashBmpFlag=2`, speed=0, crash audio |
| 3 | Finish line | `crashBmpFlag=3`, record finish time, normal speed |
| 4 | System event | Sets `game_frame_in_sec=1` |
| 5 | Terrain tunneling | Redirected to flag 1 with speed zeroed |

On crash/finish for player, copies scoring state to `gState_*` variables.

---

## 28. Game Loop & Timing

### Timer System (`src/timer.c`)

- `timer_dispatch_elapsed()` computes elapsed milliseconds from monotonic clock
- Converts to ticks: `ticks = elapsed_ms / GAME_TIMER_MS_EFF`
- Each tick calls all registered callbacks (including `frame_callback`)

### Frame Callback (`src/gamemech.c`)

- `frame_callback()` gates on reentrancy + frame budget
- Calls `replay_capture_frame_input(0)` which advances `elapsed_time2`

### Main Loop (`run_game`, `src/stuntsengine.c`)

```
while game_finish_state == 0:
  timer_get_counter()                    // dispatch timer callbacks
  while state.game_frame < elapsed_time2:
    update_gamestate()                   // physics catch-up
  // synchronized:
  set_projection(0x23, dashbmp_y/6, 0x140, dashbmp_y)
  update_frame()                         // 3D scene
  shape2d_render_bmp_as_mask()           // dashboard
  render_present_ingame_view()           // present
```

### FPS Modes

- **20 fps** (`framespersec2 = 20`): full-speed mode on fast systems
- **10 fps** (`framespersec2 = 10`): half-speed mode, physics values doubled to compensate

---

## 29. Replay & Input System

### Input Recording

`replay_capture_frame_input()` stores one byte per tick at `td16_rpl_buffer[replay_frame_counter]`:
- Bits 0–1: throttle/brake (2=accel, 1=brake)
- Bits 2–3: steering (4=right, 8=left)
- Bit 4: gear up (0x10)
- Bit 5: gear down (0x20)

### Replay Playback

In replay mode, reads from `td16_rpl_buffer` instead of polling input.

### Buffer Compaction

At tick 0x2EE0 (12000), if less than half a lap completed, the buffer is compacted: shifts data down by `30 × framespersec` frames to prevent overflow.

### CVX Checkpoints

Every `fps_times_thirty` frames, entire GAMESTATE is checkpointed into CVX buffer for replay seeking. `restore_gamestate()` finds nearest checkpoint and fast-forwards.

---

## 30. Global State Variable Reference

### Rendering Pipeline

| Variable | Type | Role |
|----------|------|------|
| `mat_temp` | MATRIX | Current camera/view matrix |
| `polyinfonumpolys` | uint16 | Count of polygon info records |
| `polyinfoptrnext` | uint16 | Byte offset to next free polyinfo slot |
| `zorder_shape_list[401]` | int[] | Sorted polygon linked list |
| `polyinfoptrs[401]` | int*[] | Polygon info data pointers |
| `currenttransshape[29]` | TRANSFORMEDSHAPE3D[] | Per-tile shape buffer |
| `curtransshape_ptr` | ptr | Current write position in shape buffer |
| `transformedshape_counter` | char | Shapes queued for current tile sort |
| `transformedshape_zarray[]` | short[] | Z-depth sort keys |
| `transformedshape_indices[]` | short[] | Sorted index permutation |
| `game3dshapes[130]` | SHAPE3D[] | All loaded 3D shapes |
| `backlights_paint_override` | char | Brake light color override |
| `timertestflag2` | uchar | Detail level: 0=full, ≥3=minimal |
| `timertestflag_copy` | ushort | Dirty rect tracking enable |
| `cameramode` | char | Camera mode (0–3) |
| `followOpponentFlag` | char | Follow player (0) or opponent |

### Physics

| Variable | Type | Role |
|----------|------|------|
| `state[1120]` | uchar[] | Raw GAMESTATE buffer |
| `simd_player` | SIMD | Player vehicle specs |
| `planindex` | int | Current collision plane index |
| `wallindex` | int | Current wall index (−1 = none) |
| `terrainHeight` | int | Current terrain elevation |
| `current_surf_type` | int | Surface type under current query |
| `planptr[]` | PLANE[] | Collision plane array |
| `elem_xCenter`, `elem_zCenter` | int | Current tile center |

### Track

| Variable | Type | Role |
|----------|------|------|
| `td14_elem_map_main[]` | uchar[] | 30×30 element map |
| `td15_terr_map_main[]` | uchar[] | 30×30 terrain map |
| `trackrows[]`, `terrainrows[]` | int[] | Row stride tables |
| `trackcenterpos[]`, `trackcenterpos2[]` | int[] | Tile center coords |
| `startcol2`, `startrow2` | int | Start line tile |
| `track_angle` | int | Start line heading |
| `hillFlag` | int | Hill height set index |

---

## 31. Key Numeric Constants

| Constant | Hex | Decimal | Meaning |
|----------|-----|---------|---------|
| Tile size | 0x400 | 1024 | World units per tile |
| Full circle | 0x400 | 1024 | Angle units per revolution |
| 90 degrees | 0x100 | 256 | Quarter circle in angle units |
| Angle mask | 0x3FF | 1023 | 10-bit angle wrap mask |
| Fixed 1.0 | 0x4000 | 16384 | Q1.14 representation of 1.0 |
| Near plane Z | 0x0C | 12 | Near clip distance |
| Max polygons | 0x190 | 400 | Polygon info list capacity |
| Polyinfo buffer | 0x28A0 | 10400 | Polygon info buffer size (bytes) |
| Max shapes/tile | 29 | 29 | `currenttransshape[]` capacity |
| DOS shape base | 0x764C | 30284 | Base offset for shape translation |
| DOS shape stride | 22 | 22 | Bytes per shape slot in original |
| Elevated height | 0x1C2 | 450 | Plateau terrain height |
| Max speed | 0xF500 | 62720 | Speed clamp |
| Gravity/tick | 0x13 | 19 | Free-fall gravity constant |
| Fatal free-fall | 0x5AEB | 23275 | Crash threshold for rc1 |
| Max suspension | 0x180 | 384 | Max spring displacement |
| Susp compress/frame | 0xC0 | 192 | Max compression per tick |
| Road half-width | 0x78 | 120 | Standard road half-width |
| Map lower bound | 0xF00 | 3840 | Position clamp minimum |
| Map upper bound | 0x1DF0FF | ~31 tiles | Position clamp maximum |
| RPM upshift | 0x1F40 | 8000 | Gear shift up threshold |
| RPM downshift | 0xBB8 | 3000 | Gear shift down threshold |
| Trackdata alloc | 0x6BF3 | 27635 | Total trackdata buffer size |

---

## 32. Common Bug Patterns & Debugging Notes

### Coordinate System Pitfalls

1. **Z-axis inversion**: Grid rows grow south but Z grows north. Off-by-one in `0x1D - row` conversions cause tile misalignment.
2. **car_rotate axis swap**: `car_rotate.x` = yaw, `.y` = pitch, `.z` = roll. But `rotvec` in transforms expects `.x` = roll, `.z` = yaw. Mismapping causes wrong orientation.
3. **Fixed-point shift amounts**: `car_posWorld1 >> 6` for rendering, `>> 10` for tile index, `>> 16` for some physics contexts. Wrong shift = position errors.

### Overflow / Wraparound Issues

4. **Grip demand wraps at 16 bits**: `(speed² / 64) × (|steering| / 8)` deliberately wraps — this is intentional, but bugs can occur if intermediate values change signedness.
5. **Polygon buffer overflow**: Max 400 polygons, 10400 bytes. Complex scenes near this limit may silently corrupt. Check `polygon_op_error_code` after rendering.
6. **Angle masking**: All angles must be `& 0x3FF`. Missing masks cause incorrect trig lookups and matrix construction.

### Pointer / State Issues

7. **DOS offset translation**: `dos_ofs_to_shape3d()` range-checks against 0x764C base and 130 max shapes. Invalid offsets in track data produce NULL shapes → skip rendering (not crash, but invisible objects).
8. **mat_rot_zxy Y-angle cache**: Caches the Y rotation matrix. If `mat_y_rot_angle` isn't updated between contexts, stale rotation is used.
9. **sprite1 validation**: `shape2d_sanitize_sprite()` clamps corrupted sprite fields. If `sprite_bitmapptr < 0x100000`, it's replaced with the framebuffer default. This masks buffer corruption bugs.

### Rendering Artifacts

10. **No viewport polygon clipping**: Only trivial rejection + per-scanline X clamp. Polygons partially outside the viewport may have incorrect edges if the AABB reject doesn't catch them.
11. **Depth sort accuracy**: Bubble sort for per-tile shapes. Z-bias values (−0x800, −0x400) affect draw order — wrong bias causes Z-fighting or cars drawing behind track objects.
12. **Primitive continuation groups**: `primitive_flags & 2` causes bulk skip when parent is culled. If group boundaries are wrong in shape data, visible primitives get skipped.

### Physics Bugs

13. **polarAngle(0, 0)**: Returns undefined value. Called when camera and car are at exact same position = division by zero in the arctangent lookup.
14. **Terrain tunneling**: `update_crash_state(5, ...)` fires when wheels pass through terrain between frames. At high speed / low FPS, this can trigger false positives.
15. **Suspension overload flag** (`0x20`): Set when `rc1 > 0xFA` but doesn't immediately crash — the flag persists and may trigger audio/visual effects out of sync.
16. **Wall speed threshold**: Uses angle-dependent formula `-(angle × 0x46 >> 8 - 0x64) << 8`. At certain approach angles, the threshold can become negative = impossible-to-hit wall crash.

### Timing Bugs

17. **FPS-dependent physics**: Many values are halved/doubled based on `framespersec`. If `framespersec` changes mid-session (e.g., crash slow-mo), physics constants may be stale.
18. **Frame callback reentrancy**: Uses `palette_fade_state_buffer` as spinlock. If callback takes longer than tick interval, frames are dropped.

### Memory/Resource Bugs

19. **Sprite window stack**: LIFO only. Out-of-order release corrupts stack. Check `sprite_wnd_stack_reserve/release` pairing.
20. **RLE decoder row spanning**: RLE data is not row-aligned — runs can span row boundaries. If `pixels_left` tracking is wrong, pixels write to wrong rows.
