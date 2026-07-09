# Stunts Engine: Complete How-To Customization & Engine Reference

This guide provides a comprehensive architectural explanation of the Stunts engine, detailing how the game flow, resources, rendering, physics, sound, and tracks work. Use this document as your map for understanding, modifying, and customizing the game.

---

## 1. The Game Loop Workflow & Clock

The Stunts engine separates **logical time** from **visual presentation**. The simulation progresses at a fixed rate, while frames are drawn as fast as the host hardware allows.

### Time-Stepping State Flow

                  [ Hardware Timer Clock ]
                             │
                             ▼
         [ Advance 'elapsed_time2' (Logic Tick counter) ]
                             │
                             ├──────────────────────────┐
                             │ (Timer Callback)         │ (Timer Callback)
                             ▼                          ▼
               [ frame_callback() ]            [ audio_driver_timer() ]
               - Sample input                  - Synthesize OPL2 FM sound
               - Advance simulation clock      - Mix SFX & music to DMA ring
                             │
                             ▼
                     [ run_game() Loop ]
                             │
                             ▼
             Is state.game_frame < elapsed_time2?
               ├── Yes ──► [ update_gamestate() ]
               │           - update_car_speed() (Drivetrain Engine)
               │           - update_grip() (Suspension & Slide Check)
               │           - update_player_state() (Rigid Body/Geometry contacting)
               │           - Increments state.game_frame +1
               │           - Re-evaluates loop...
               │
               └── No ───► [ Render & Present ]
                           - update_frame() (Draw 3D scene)
                           - 2D dashboard comps
                           - video_refresh() (Blit out to SDL2)

* **Key Clock Code Entry Points:**
  - `main()` (`src/stuntsengine.c`): Game startup, track selection, and primary loop initialization.
  - `run_game()` (`src/stuntsengine.c`): Coordinates frame rendering iterations with simulation catch-up loops.
  - `timer_dispatch_elapsed()` (`src/timer.c`): Hardware-to-logical game tick converter.
  - `frame_callback()` (`src/gamemech.c`): Registered callback that increments `elapsed_time2` and triggers inputs.
  - `update_gamestate()` (`src/stuntsengine.c`): High level manager that advances physics clock ticks.

---

## 2. The 3D Rendering Process

The 3D engine is a CPU-only software vector renderer using fixed-point Q1.14 math. A camera position and rotation matrix are calculated from the player's car state to build a list of visible elements.

### Rendering Pipeline Step-by-Step

```
[ Camera Setup ]
   │ Resolved in update_frame() based on camera mode settings.
   ▼
[ Tile Scan Classification ]
   │ Lookahead Tables select a search footprint of 23 tiles in a 45-degree octant.
   ▼
[ Object Sorting ]
   │ For each tile (front-to-back):
   │  - Draw immediate shapes (terrain heights, border fences).
   │  - Queue deferred shapes (cars, track items, road loops).
   │  - Sort queued tile objects by camera-space Z-depth.
   ▼
[ Transform & Clip ]
   │ Vertices are transformed to camera space, projected to screen coords,
   │ and clipped against the Z-near plane (12 units) using Sutherland-Hodgman clipping.
   ▼
[ Global Depth Queue ]
   │ Emitted polygons are inserted into 'zorder_shape_list' backend.
   ▼
[ Rasterization (get_a_poly_info) ]
   │ Output final pixels from back-to-front (Painter's Algorithm).
   ▼
[ Overlays ]
   │ Draw skybox background, windshield cracks, explosions, and HUD text.
```

* **Key Rendering Code Entry Points:**
  - `update_frame()` (`src/frame.c`): Coordinates camera selection, tile scanning, culling, sorting, and scene composition.
  - `select_cliprect_rotate()` (`src/frame.c`): Determines camera viewport culling octant and builds the transformation matrix.
  - `transformed_shape_add_for_sort()` (`src/render.c`): Pushes track elements and car models onto the sorting queue.
  - `shape3d_render_transformed()` (`src/shape3d.c`): Multiplies shapes by rotation matrix, projects vertices, clips, and defines polygon shapes.
  - `transformedshape_sort_desc()` (`src/frame.c`): Sorts objects in a tile by Z depth (back-to-front).
  - `get_a_poly_info()` (`src/shape3d.c`): Walks the compiled global polygon list and triggers pixel rasterization.
  - `render_present_ingame_view()` (`src/render.c`): Directs active dirty-rect updates and double-buffer blitting.
  - `video_present_frame()` (`src/video.c`): Triggers SDL2 sync boundaries.

---

## 3. Vehicle Physics & Collision System

Every physics tick, the engine processes rigid-body movement based on wheel geometry, suspension dampening, and surface parameters.

### Logic Pipeline

```
1. DRIVETRAIN (update_car_speed)
   - Gear ratio selection & manual/automatic shifting logic.
   - Calculates engine RPM based on current velocity.
   - Applies throttle force, engine limits, and aerodynamic drag coefficients.

2. SUSPENSION & CONTACTS (update_player_state)
   - Builds local->world wheel positions based on car orientation.
   - Raycasts 4 individual wheel locations against the terrain map.
   - Updates local suspension springs (compressions and offsets).
   - Reconstructs body orientation (pitch, roll, yaw) based on wheel heights.
   - If wheels lose contact, applies pure gravity (freefall physics).

4. GRIP & SLIP (update_grip)
   - Reads contact surface under each wheel (grass, tarmac, gravel, ice).
   - Computes turning forces (Demanded Grip = speed² * steering angle).
   - Queries available grip from surface traction limits.
   - If Demanded > Available: flags the car as sliding, limits steering, and applies drift angles.

5. COLLISIONS & CRASH MACHINE
   - Checks bounding-box intersection against the opponent car.
   - Probes intersections with track objects (walls, fences, start gates).
   - Small hits adjust velocities and bounce angles.
   - Massive impacts trigger a transition to the crash state (breaking sound, crack masks, zero speed).
```

* **Key Physics Code Entry Points:**
  - `update_car_speed()` (`src/carstate.c`): Processes gears, RPM calculation, throttle, brakes, and torque.
  - `update_grip()` (`src/carstate.c`): Computes tire friction limits and handles steering/slide status.
  - `update_player_state()` (`src/carstate.c`): Resolves wheel-to-ground coordinates, suspension offsets, updates positions, and tracks collisions.
  - `car_car_coll_detect_maybe()` (`src/carstate.c`): Dispatches bounding-box overlaps between the player and opponent cars.
  - `car_car_speed_adjust_maybe()` (`src/carstate.c`): Modifies velocities after impact.
  - `update_crash_state()` (`src/carstate.c`): Main state machine controller for game-over sequences.

---

## 4. Game Resource Management

Stunts bundles its components (2D sprites, 3D models, car behavior settings, font layouts) inside a structured chunk container format.

### Key Resource Code Entry Points:
* **Archive & Decompression Routing:**
  - `file_load_resfile()` (`src/stuntsengine.c`): Searches directory for `.RES` first, falling back to `.PRE`.
  - `file_load_resource()` (`src/stuntsengine.c`): Dynamic loader targeting binary buffers, shapes, sound resources, and trackers.
  - `file_decomp()` (`src/stuntsengine.c`): Decompresses archives in memory.
  - `locate_resource()` & `locate_text_res()` (`src/memmgr.c`): Chunk name header parser (`MAIN.RES` parsing).
* **2D Sprite Assets:**
  - `file_load_shape2d()` (`src/shape2d.c`): File loading helper targeting `.PVS` / `.PES` extensions.
  - `file_unflip_shape2d()` (`src/shape2d.c`): Matrix transposition un-flipping.
  - `shape2d_resource_get_shape()` (`src/shape2d.c`): Resolves shape indices.
* **3D Polygon Assets:**
  - `shape3d_load_all()` (`src/shape3d.c`): Deconstructs `GAME1.P3S` and `GAME2.P3S` into shape templates.
  - `shape3d_init_shape()` (`src/shape3d.c`): Decodes primitive streams and vertex offsets.
  - `dos_ofs_to_shape3d()` (`src/shape3d.c`): Legacy memory segment pointer converter.

---

## 5. Sound & Music Engine (KMS Tracker & Resources)

The music and audio effects of Stunts run on an emulated audio subsystem. It simulates a SoundBlaster DSP command register path for one-shot effects, alongside a direct multitrack music sequencer driving OPL2 FM stabilizer logic.

### Key Audio Code Entry Points:
* **Subsystem Controls:**
  - `audio_load_driver()` (`src/audio.c`): Detects prefixes and configures device outputs.
  - `audio_add_driver_timer()` (`src/audio.c`): Registers the timer callback structure.
  - `audio_driver_timer()` (`src/audio.c`): Coordinates music sequencers and mixer loops.
  - `opl2_init()` & `opl2_write()` (`src/opl2.c`): Interfaces directly with Nuked OPL3 registers.
  - `audio_sb_generate_dma_samples()` (`src/audio.c`): Generates noise and software fallback samples when offline.
* **Vehicle Audio Orchestration:**
  - `audio_init_engine()` (`src/audio.c`): Prepares engine notes and allocates OPL2 tracks.
  - `audio_update_engine_sound()` (`src/audio.c`): Adjusts pitch (RPM) and volume attenuation dynamically.
  - `audio_sync_car_audio()` (`src/gamemech.c`): Runs per-frame engine note ticks for player/opponent cars.
  - `audio_apply_crash_flags()` (`src/carstate.c`): Connects physical collisions to sound triggers.
* **KMS Music Tracker & Synthesis:**
  - `audio_load_menu_resource()` (`src/audio.c`): Song data container loader.
  - `audio_extract_menu_resource_notes()` (`src/audio.c`): Scans `0xE7` segment tags.
  - `audio_kms_parse_track()` (`src/audio.c`): Decodes VLQ tracker bytecodes into note events.
  - `audio_kms_advance_ticks()` (`src/audio.c`): Steps song timelines.
  - `audio_kms_note_on()` & `audio_kms_note_off()` (`src/audio.c`): Keys notes on emulated OPL2 channels.

---

## 6. Track Grid & Editor Engine

The Stunts map is an editor-friendly grid where geometry coordinates are structured in cells of 1024 units.

### Key Track & Editor Code Entry Points:
* **Geometry Processing:**
  - `build_track_object()` (`src/track_object.c`): Pinpoints terrain heights and object collision points relative to a given position coordinate.
  - `plane_origin_op()` & `plane_get_collision_point()` (`src/math.c`): Calculates planes to check if a car hits banked stunt borders.
  - `subst_hillroad_track()` (`src/frame.c`): Swaps flat track object templates on hill tiles with elevated versions.
* **Setup & Verification:**
  - `track_setup()` (`src/track.c`): Resolves multi-tile object boundaries, checkpoints, starts, paths, and validates track topologies across 10 distinct validation phases.
