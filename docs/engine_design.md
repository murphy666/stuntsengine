# Stunts Engine Design Document

**Date:** 2026-03-13  
**Project:** `stuntsengine`

---

## 1) Scope and intent

This document describes the runtime design of the game engine with focus on:

- Main loops (application loop, gameplay loop, timer-driven frame stepping)
- 2D rendering pipeline (sprites / UI / HUD / replay bar)
- 3D rendering pipeline (camera, world assembly, transform / sort / draw)
- Vehicle physics (speed, grip, wheel/ground interaction, rigid-body orientation updates)
- Car collision (car-vs-car and car-vs-world triggers)
- Audio subsystem overview (see [sound_design.md](sound_design.md) for full detail)

All descriptions are based on the current C port implementation.

---

## 2) High-level architecture

### Core modules

- `src/stuntsengine.c`
  - Process entry (`main`), boot/menu flow, track setup, and the primary gameplay loop (`run_game`).
- `src/gamemech.c`
  - Timer callback integration (`set_frame_callback`, `frame_callback`), replay/input sampling (`replay_unk2`), replay bar loop (`loop_game`).
- `src/timer.c`
  - Real-time dispatch of game timer ticks based on monotonic clock and `GAME_TIMER_MS_EFF`.
- `src/frame.c`
  - Main scene renderer (`update_frame`), including camera, terrain/object visibility, transform staging, polygon build, skybox, effects, HUD overlays.
- `include/shape3d.h` (+ `src/shape3d.c`)
  - 3D shape representation and transform/projection entry points (`transformed_shape_op`, `set_projection`, `get_a_poly_info`).
- `include/shape2d.h` (+ `src/shape2d.c`)
  - 2D sprite windows, shape blits, primitive line fills, dashboard/replay/UI rendering helpers.
- `src/carstate.c` (merged state unit)
  - Drivetrain and speed update (`update_car_speed`), plus merged gameplay/grip/integration/crash logic from former `state.c`, `stateply.c`, and `statecrs.c`.
- `src/data_core.c`, `src/data_game.c`, `src/data_runtime.c`, `src/data_menu_track.c`
  - Multi-file shared data definitions, split from the former `data_global.c` by domain. Extern declarations live in the module headers under `include/`.
- `src/ui_screen.c`
  - Screen lifecycle and event loop (`UIScreen`, `ui_screen_push/pop/run`, `ui_screen_run_modal`).
- `src/ui_widgets.c`
  - Reusable widget primitives (`ui_button_menu_run`, `ui_draw_panel`, `ui_dialog_run`, text layout helpers).
- `src/audio.c` (+ `include/audio.h`)
  - Complete audio driver: handle allocation, SB-DSP command queue, OPL2 FM engine sound, tire/crash SFX, KMS menu-music sequencer, SDL2 output pipeline. Registered as an independent timer callback — runs independently of the gameplay loop.
- `src/opl2.c` (+ `include/opl2.h`)
  - Thin wrapper around Nuked OPL3 in OPL2-compatibility mode (YM3812 emulation). Provides `opl2_init / opl2_write / opl2_generate / opl2_reset / opl2_destroy`.

> Full audio design is documented in [sound_design.md](sound_design.md).

### Runtime model in one sentence

The timer subsystem advances a logical frame counter (`elapsed_time2`) via callback; the gameplay loop in `run_game` consumes those steps by repeatedly calling `update_gamestate`, and renders with `update_frame` once simulation catches up.

---

## 3) Main loops design

## 3.1 Application loop

Implemented in `main` (`src/stuntsengine.c`):

1. Startup / intro / menu orchestration.
2. Track and resource preparation (`track_setup`, audio loads, CVX buffer allocation).
3. Enter gameplay session through `run_game()`.
4. On session end, run post-race/highscore flow, restore track/menu state, return to menu loop.

This loop is long-lived and repeats across races without restarting process state.

## 3.2 Timer-driven simulation clock

### Dispatch layer (`src/timer.c`)

- `timer_dispatch_elapsed()` computes elapsed real milliseconds from monotonic clock.
- Converts elapsed real time to logical ticks:
  - `ticks_to_dispatch = elapsed_ms / GAME_TIMER_MS_EFF`
- Each tick:
  - Increments `timer_callback_counter`
  - Calls all registered callbacks (including `frame_callback`)

`GAME_TIMER_MS_EFF` comes from `src/game_timing.h` and scales global logic speed.

### Game callback layer (`src/gamemech.c`)

- `set_frame_callback()` registers `frame_callback` with timer.
- `frame_callback()` gates and schedules logical frame progress:
  - Handles replay/pause/autoload conditions
  - Calls `replay_unk2(0)` when frame should advance
- `replay_unk2()` performs input sampling/record/replay stepping and advances `elapsed_time2`.

Net effect: `elapsed_time2` is the authoritative “simulation produced up to this frame” index.

## 3.3 Gameplay loop (`run_game` in `src/stuntsengine.c`)

The core race loop is structured as a producer/consumer synchronization:

1. `timer_get_counter()` drives timer dispatch and callback execution.
2. If `state.game_frame != elapsed_time2`, simulation is behind callback-produced ticks:
   - Call `update_gamestate()` repeatedly until caught up.
3. Once synchronized, proceed with render and UI work for current frame:
   - Backbuffer/window setup
   - `update_frame(...)`
   - Dashboard, replay bar, overlays, mouse compositing
   - Replay interaction (`loop_game(3, ...)`) when active

### Why this split matters

- Simulation cadence is clocked by timer callbacks.
- Rendering cadence is opportunistic and can be faster/slower.
- Keeps physics/AI progression deterministic relative to tick budget while allowing variable presentation cost.

## 3.4 Replay/UI loop hooks

`loop_game` in `src/gamemech.c` has mode-based behavior:

- `arg_0 == 0/2`: initialize replay bar button state
- `arg_0 == 1`: update replay bar visuals/time/progress/camera button highlight
- `arg_0 == 3`: handle replay bar input, camera control, zoom/seek interactions

This is integrated into the gameplay loop rather than being a separate scene system.

---

## 4) 2D rendering design

## 4.1 Framebuffer and presentation model

- Logical rendering target remains VGA-like indexed buffer (320×200, palette-driven).
- SDL2 backend (`src/framebuffer.c`) provides platform presentation:
  - Converts palette-indexed pixels to ARGB32 (`vga_fb_to_rgba`)
  - Uploads texture and presents (`vga_sdl2_present`)

## 4.2 2D primitive/sprite layer

From `src/shape2d.h` and usage sites:

- Sprite windows (`SPRITE`) support double-buffer style copying and partial regions.
- Core operations include:
  - `sprite_copy_*` variants (buffer/page transfers)
  - `shape2d_render_bmp_as_mask`, `shape2d_op_unk4` (masked overlays)
  - Primitive line fills (`draw_filled_lines`, `draw_patterned_lines`)

## 4.3 2D composition in gameplay

Inside `update_frame` + `run_game`, 2D is layered after major 3D work:

- Dashboard mask/render (`shape2d_render_bmp_as_mask` and related)
- In-game text (`draw_ingame_text`, timer string render)
- Crash overlays (crack/sinking effects)
- Replay bar (`loop_game(1,...)` updates)
- Mouse cursor opaque/transparent passes around UI draws

The code relies on clip rectangles and sprite subregion sizing (`sprite_set_1_size`) to limit overdraw.

---

## 5) 3D rendering design

## 5.1 Entry and responsibilities

`update_frame(char arg_0, struct RECTANGLE* arg_cliprectptr)` in `src/frame.c` is the central 3D frame builder.

Major responsibilities:

1. Resolve camera source and orientation
2. Select visible tile neighborhood and object LOD
3. Build transformed shape queue (immediate + sorted)
4. Execute transform to polygon info
5. Render skybox and post-shape effects
6. Convert accumulated polygons to raster pass (`get_a_poly_info`)

## 5.2 Camera modes

Camera position/angles are derived from active car state and `cameramode`:

- Mode 0: in-car/near-car orientation-following camera
- Mode 1: external camera from `state.game_vec1`
- Mode 2: orbit/chase-style computed from car orientation and control offsets
- Mode 3: track-camera data from `trackdata9`

If orientation is not directly known, camera looks toward target using `polarAngle` / radius calculations.

## 5.3 World assembly and culling

Renderer uses a precomputed lookahead table (`var_50`) selected by heading sector.

Per candidate slot:

- Compute map tile coordinates relative to camera
- Reject out-of-bounds and detail-skipped entries
- Resolve element map (`td14_elem_map_main`) and terrain map (`td15_terr_map_main`)
- Handle multi-tile and substitution cases (`0xFD/0xFE/0xFF`, hill-road substitution)

Objects are either:

- Transformed immediately for special cases (e.g., some fences/flags), or
- Added to depth-sort arrays (`transformed_shape_add_for_sort`) for ordered draw.

## 5.4 Car and dynamic shape insertion

Player/opponent cars are inserted as transformed 3D shapes with:

- Position from `car_posWorld1 >> 6`
- Rotation from `car_rotate` (mapped to renderer axis convention)
- Optional low-detail shape selection for distant/detail modes
- Wheel deformation update via `sub_204AE` when high-detail model used

Debris/scene dynamic fragments from `state.field_42A` arrays are also inserted.

## 5.5 Transform, sort, raster bridge

Pipeline stages:

1. Sort transformed shape indices by Z key (`transformedshape_sort_desc`) when needed.
2. For each sorted item, call `transformed_shape_op`.
3. Skybox pass (`skybox_op`) contributes sky/background geometry.
4. `get_a_poly_info()` finalizes polygon info for rasterization backend.

The engine uses clipping rectangles and shape flags to control clipping, z-bias behavior, and material application.

---

## 6) Physics design

Physics is distributed across three principal functions called each simulation frame (compiled from `src/carstate.c`):

1. `update_car_speed` (`src/carstate.c`)
2. `update_grip` (`src/carstate.c`)
3. `update_player_state` (`src/carstate.c`)

## 6.1 Speed / drivetrain (`update_car_speed`)

Key behaviors:

- Gear logic (manual/auto behavior, upshift/downshift thresholds)
- Gear change animation/state (`car_changing_gear`, knob target interpolation)
- RPM update from speed and gear ratio (`update_rpm_from_speed`)
- Engine limiter and acceleration/brake application to `car_speed2`
- Aerodynamic and torque-curve based speed deltas (table-driven via SIMD data)

Timing calibration depends on `framespersec` (notably 20 vs 10 FPS branches).

## 6.2 Grip/slip model (`update_grip`)

Uses wheel surface codes and steering to derive traction state:

- Computes demanded grip from speed² and steering angle.
- Computes available grip from car grip + per-surface sliding table.
- If demanded > available:
  - Sets sliding flag
  - Adjusts effective steering and drift angle (`car_angle_z` / `car_36MwhlAngle`)
  - Applies speed penalties
- Grass surfaces apply additional deceleration.

Outputs include:

- `car_demandedGrip`, `car_surfacegrip_sum`
- `car_slidingFlag`
- Modified steering/wheel state used by integrator

## 6.3 Position/orientation integrator (`update_player_state`)

This function performs wheel-based rigid-body update against world geometry:

- Builds local->world wheel positions from suspension state and car transform.
- For each wheel, queries terrain/plane/wall interaction (`build_track_object`, `plane_origin_op`, wall checks).
- Updates suspension accumulators (`car_rc*` fields), surface contact codes, and pseudo-gravity.
- Reconstructs car body orientation from wheel deltas (pitch/roll/yaw reconciliation).
- Clamps world bounds and updates final `car_posWorld1` / `car_rotate`.

Crash transitions are triggered here when thresholds are exceeded (e.g., heavy impacts, invalid support conditions, sinking states).

---

## 7) Collision design

Collision behavior has layered checks:

## 7.1 Car-vs-car broad/shape check

In `update_player_state` (`src/carstate.c`):

- Build collision descriptors from each car:
  - car world pose (`car_posWorld1`, `car_rotate`)
  - model collision points (`SIMD.collide_points`)
- `car_car_coll_detect_maybe(...)` tests overlap/proximity.

If collision detected:

  - `car_car_speed_adjust_maybe(...)` in `src/carstate.c` computes relative velocity impact and adjusts both cars:
  - speed reductions proportional to relative speed
  - sets bounce wheel angles (`car_36MwhlAngle`)
- High relative impacts escalate to crash state (`update_crash_state(1, ...)` for both participants).

## 7.2 Car-vs-world object collisions

`update_player_state` also checks against:

- Start/finish pole colliders
- Track-side and map-anchored interactive colliders
- Wall segment intersections from plane/wall solver

Detected collisions trigger either:

- immediate crash state change (`update_crash_state(1/2/5, ...)`), or
- non-fatal object interaction (spawn debris/events via `state_op_unk` and state flags).

## 7.3 Crash state machine

`update_crash_state` (`src/carstate.c`) controls post-impact behavior:

- `car_crashBmpFlag` states (`1`, `2`, `3`) for crash/sinking/finish-like transitions.
- Captures impact speed, end frames, timing fields.
- Triggers audiovisual side effects and stat snapshots.

This is the bridge between low-level collision detection and high-level gameplay consequences.

---

## 8) Data and coordinate conventions

Important conventions observed in code:

- World position often stored in long fixed-point-like units (`car_posWorld*`), frequently converted via `>> 6` for render/physics intermediates.
- Tile addressing commonly uses `>> 10` or `>> 16` depending subsystem context.
- Angles are 10-bit circular (`0..1023`, masked with `0x3FF`).
- Surfaces/wheel contact codes drive grip and crash decisions (`car_surfaceWhl[]`, `car_sumSurf*`).

---

## 9) End-to-end frame flow (race mode)

1. Timer dispatch (`timer_dispatch_elapsed`) computes elapsed ticks and fires all registered callbacks.
2. `frame_callback` (gameplay) calls `replay_unk2`, advancing `elapsed_time2` and recording/reading inputs.
3. `audio_driver_timer` (audio, independent callback) drains the DSP command ring, slews per-handle RPM pitch, ticks SFX countdowns, generates OPL2 + noise PCM samples into the DMA ring, and submits audio to the SDL2 queue.
4. `run_game` loop compares `state.game_frame` vs `elapsed_time2`.
5. While behind, `update_gamestate` runs:
   - `player_op` / `opponent_op`
   - `update_car_speed`, `update_grip`, `update_player_state`
   - crash-state transitions and `audio_apply_crash_flags` / `audio_sync_car_audio`
6. When synchronized, `update_frame` renders 3D scene + overlays.
7. 2D dashboard/replay bar/HUD and cursor compositing finalize the presented frame.

---

## 10) Design strengths and constraints

### Strengths

- Deterministic tick progression separated from rendering.
- Tight coupling between physical wheel contacts and visual car posture.
- Replay and live input share the same frame stepping path.
- Data-driven vehicle and track behavior via resource tables.

### Constraints

- High coupling of subsystems through shared global state.
- Legacy branch-heavy control flow (ported labels/goto structure) increases maintenance cost.
- Rendering and gameplay orchestration are intertwined in `run_game` and `update_frame`.

---

## 11) Primary entry points reference

- Main/session loop: `src/stuntsengine.c`
  - `main`, `run_game`, `update_gamestate`
- Timer/frame stepping: `src/timer.c`, `src/gamemech.c`
  - `timer_dispatch_elapsed`, `frame_callback`, `replay_unk2`
- Rendering: `src/frame.c`
  - `update_frame`
- Physics/collision: `src/carstate.c` (merged unit)
  - `update_car_speed`, `update_grip`, `update_player_state`, `car_car_speed_adjust_maybe`, `update_crash_state`
- Audio: `src/audio.c` — see [sound_design.md](sound_design.md) for full internals
  - `audio_load_driver`, `audio_add_driver_timer`, `audio_driver_timer` (timer callback)
  - `audio_init_engine`, `audio_update_engine_sound`, `audio_sync_car_audio`
  - `audio_apply_crash_flags`, `audio_select_crash1_fx`, `audio_select_skid1_fx`, `audio_clear_chunk_fx`
  - `audio_set_menu_music_paused`, `audiodrv_atexit`
- UI screens: `src/ui_screen.c`
  - `ui_screen_push`, `ui_screen_pop`, `ui_screen_run`, `ui_screen_run_modal`
- UI widgets: `src/ui_widgets.c`
  - `ui_button_menu_run`, `ui_draw_panel`, `ui_dialog_run`, `ui_dialog_info`, `ui_dialog_confirm`
