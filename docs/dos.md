# DOS (DJGPP) port

Takes the engine back to its original platform: 32-bit protected-mode DOS via the
DJGPP GCC cross-compiler, on SDL3's DOS backend (VGA mode 13h, Sound Blaster,
gameport joystick).

## Building

Requires a DJGPP cross-compiler (`i586-pc-msdosdjgpp-gcc` / `-g++`) on `PATH`.

```bash
cmake --preset dos
cmake --build --preset dos
```

Produces `build_cmake_dos/stunts.exe`. SDL3 is fetched and built from source
during configure — there are no DJGPP distro packages for it.

## Running

The executable needs a DPMI host: keep `CWSDPMI.EXE` beside it (it is not
included here — get it from <https://sandmann.dotster.com/cwsdpmi/>). The game
data files must sit in the same directory, as on the desktop builds.

Under DOSBox, with the binary and data in one directory:

```text
mount c /path/to/dos-build
c:
STUNTS.EXE
```

`/ns` disables sound, as in the original.

## How the platform is selected

`cmake/toolchain-djgpp.cmake` sets `CMAKE_SYSTEM_NAME=DOS`; `CMakeLists.txt`
keys `STUNTS_DOS` off that and:

- Fetches a DOS-capable SDL3 (upstream `libsdl-org/SDL`, pinned by URL +
  SHA256) instead of looking for a system package. DOS support is not in any
  upstream *release* yet: as of `release-3.4.16`, the newest, there is still no
  `src/video/dos`. It exists only on `main`, so this has to be a commit pin
  rather than a tag — worth revisiting once a release carries it.

  The specific commit is the one that made the DOS joystick probe the gameport
  at `0x201` directly, replacing the BIOS INT 15h probe whose polling loop
  interferes with SB16 IRQ timing. That matters here because the game opens a
  gamepad, so anything older would need the fix carried separately.
- Compiles as `gnu11` rather than `c11`, and does not define `_POSIX_C_SOURCE`.
  DJGPP hides `strcasecmp` and friends behind `!__STRICT_ANSI__` *and*
  `!_POSIX_SOURCE`, so the strict-ANSI dialect would hide what `compat_fs.h`
  needs.
- Builds the OPL2 backend that drives real hardware instead of the software
  synthesizer (see below).

`src/stuntsengine.c` includes `<SDL3/SDL_main.h>` so the real entry point runs
through `SDL_RunApp`, which sets the DJGPP crt0 memory-locking flags and calls
`__djgpp_nearptr_enable()` before SDL touches the framebuffer. It is a no-op on
the desktop targets, and suppressed on Windows, which defines `SDL_MAIN_HANDLED`
to keep its console entry point.

## Not done

- **Joystick is untested.** The engine opens a *gamepad*
  (`SDL_OpenGamepad()` in `src/keyboard.c`), which needs a gamepad mapping; a
  raw DOS gameport stick may well not have one, in which case no device opens
  and only the keyboard responds. f15se2-ex hit the related problem that the
  generic mapping reads missing trigger axes as half-pressed, and carries a
  bindings workaround for it. Verifying this needs a stick on real hardware or a
  DOSBox joystick.
