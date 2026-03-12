# Stunts Engine

A modern C reimplementation of the classic **4D Sports Driving / Stunts** (1990)
racing game engine, building on the reverse-engineering work of the
[Restunts](https://github.com/4d-stunts/restunts) project.

Targets modern operating systems only — no DOS support.

## How

Here is how this Stunts porting and replica project was started:

I wanted to port the old DOS game Stunts that I liked as a kid, customizable tracks and crazy crashes!! I check online and found out about the Restunts project, which had already done much of the reverse-engineering groundwork... and decided that it feal achievable to port this game to modern hardware using Copilot.

At the start of the porting it was just ok-ish; The testing at this stage ran entirely inside DOSBox on Linux, it was slow, recompiling with a DOS BORLAND/TASM toolchain and running the result inside DOSBox. The initial porting from "ASM to C" goals was very minimal; That the game audio should still be heard and that it could reach the main menu (no game crashing). After many retries to port the code properly to C (Copilot was doing stupid "workaround" code and not properly porting and investigating). 
In the end I was able to get a very broken but still running game that loaded ressources and run with almost no assembly code remaining.

Now I decided to drop DOS support entirely, It was time to move to modern hardware which I expected it will help a lot better for debugging and testing.

### Move from DOS to Modern Linux

The initial move to a Linux/GCC build was quick using SDL2 as the backend. After getting the game to start, a lot of work was needed to port to modern hardware, dealing with a lot of 16-bit to 64-bit arch issues (The ASAN build preset helped a lot). Fixing resource loading at first, then video output to SDL, and finally display the in-game main menu. Then I moved to getting a working game. Using Copilot I started comparing the original ASM code to the ported C code and fixing mismatches. Again a long process of back and forth, but still much faster than doing it manually. Having C code alongside the original ASM allowed Copilot to make good progress by 
directly comparing the two.

## Current port state

Yes the code is generated in major part with Copilot but I reviewed and update code as needed to a more usable and modern source code. But still the code is a weird mix of a lot old logics code with hardcoded values and "ASM like" C functions, Many globals and some new modern refactor SDL2 logics. But now with this foundation it is easy to debug and doing refactor and clean-up as needed. 

The game is playable but with many bugs, for example opponents are not really working and there is many visual glitches.

This is considered as ALPHA code release. A lot need to be updated before being a stunts replica.

## Enhancements

The goal is to preserve the original gameplay and visuals while adding improvements and updating the engine for modern hardware. For example, the game now runs at a proper 30 fps and the draw distance is significantly greater than the original. 

## Repository Layout

```
include/        Public header files
src/            C source files (engine, rendering, audio, menus, physics …)
tests/          Unit and integration tests (Google Test, C++)
library/
  adplug/       Git submodule – OPL2 FM emulator (auto-cloned on first build)
  sdl2-windows/ SDL2 MinGW dev archive (auto-downloaded for Windows builds)
ressources/     Game resource files (see "Game Resources" below)
docs/           Design documents and the original game manual
cmake/          CMake toolchain file for MinGW cross-compilation
clean_repo.sh   Script to remove all build artifacts and downloaded deps
```

---

## Prerequisites

### Linux

```bash
sudo apt-get install build-essential cmake libsdl2-dev git
```

| Tool | Minimum version |
|------|----------------|
| GCC or Clang | any modern version |
| CMake | 3.16 |
| SDL2 dev headers | 2.x |
| Git | any |

### Windows (cross-compile from Linux)

Install the Linux prerequisites above, then add MinGW-w64:

```bash
sudo apt-get install mingw-w64
```

The SDL2 Windows development archive is **downloaded automatically** by CMake
on the first configure run. No manual SDL2 setup is needed.

---

## Game Resources

The engine requires the original **Stunts** game data files to run.
These are **not** included in this repository for copyright reasons.

Copy the following files from a legitimate copy of the game into the
`ressources/` directory before building:

| Pattern | Description |
|---------|-------------|
| `*.RES` | Car and UI resource archives |
| `*.P3S` | 3-D shape files |
| `*.PVS` | Pre-rendered scene / menu artwork |
| `*.PRE` | Opponent preset data |
| `*.TRK` / `*.HIG` | Track layouts and high-score tables |
| `*.KMS` | Music song files |
| `*.VCE` | Voice / sound data |
| `*.SFX` | Sound effects |
| `*.FNT` | Bitmap fonts |
| `DEFAULT.RPL` | Default replay |

---

## Building

```bash
git clone --recurse-submodules https://github.com/user/stuntsengine.git
cd stuntsengine
```

### Linux

```bash
# Debug (tests enabled)
cmake --preset linux-debug
cmake --build --preset linux-debug

# Release
cmake --preset linux-release
cmake --build --preset linux-release
```

### Windows (cross-compile from Linux)

```bash
cmake --preset windows-x64
cmake --build --preset windows-x64
```

SDL2 is downloaded automatically on the first configure run.
The output (`stunts.exe` + `SDL2.dll`) can be copied directly to a Windows machine.

### All presets

| Preset | Platform | Type |
|--------|----------|------|
| `linux-debug` | Linux | Debug, tests enabled |
| `linux-release` | Linux | Release, optimised |
| `linux-asan` | Linux | Debug + AddressSanitizer |
| `linux-coverage` | Linux | Debug + gcov line-coverage |
| `windows-x64` | Windows (MinGW) | Release |

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `STUNTS_TESTS` | `ON` | Build and register test executables |
| `STUNTS_ASAN` | `OFF` | Enable AddressSanitizer |
| `STUNTS_COVERAGE` | `OFF` | Instrument with gcov for line-coverage reporting |

---

## Testing

Tests are enabled in the Debug preset (`STUNTS_TESTS=ON`) and disabled in Release.
All tests use **Google Test**, fetched automatically via CMake FetchContent.

### Run all tests

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug --target check
```

Builds all test targets, runs them via CTest, and writes `test_results.xml`
(JUnit format) to the build directory.

### Filter tests with ctest

```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
ctest --preset linux-debug -R Shape3D
```

### Line-coverage report (gcov / lcov)

```bash
cmake --preset linux-coverage
cmake --build --preset linux-coverage --target coverage
```

Runs all tests with gcov instrumentation and generates an HTML report at
`build_cmake_coverage/coverage_html/index.html`.

---

## Cleaning

`clean_repo.sh` removes all generated and downloaded content:

```bash
./clean_repo.sh             # remove build dirs, SDL2 package, submodule content
./clean_repo.sh --archive   # same + create ../stuntsengine-clean.tar.gz
```

---

## Acknowledgements

- [Restunts](https://github.com/4d-stunts/restunts) – the original reverse-engineering project
- [Stunts Wiki](https://wiki.stunts.hu/) – community documentation
