# ---------------------------------------------------------------------------
# CMake toolchain file: Linux → Windows x86-64 cross-compilation via MinGW-w64
#
# Usage (one-time configure):
#   cmake -S . -B build_cmake_windows \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64-x86_64.cmake \
#         -DSDL2_MINGW_ROOT=library/sdl2-windows/SDL2-2.32.2/x86_64-w64-mingw32 \
#         -DCMAKE_BUILD_TYPE=Release
#
#   cmake --build build_cmake_windows
#
# The resulting stunts.exe and SDL2.dll are placed in build_cmake_windows/.
# ---------------------------------------------------------------------------

set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# ----- Compiler binaries ---------------------------------------------------
# Prefer the POSIX threading model (required for std=c11 POSIX extensions like
# nanosleep / clock_gettime, though those code paths are wrapped in #ifndef
# _WIN32 guards already).
find_program(MINGW_CC  x86_64-w64-mingw32-gcc-posix
                       x86_64-w64-mingw32-gcc)
find_program(MINGW_CXX x86_64-w64-mingw32-g++-posix
                       x86_64-w64-mingw32-g++)
find_program(MINGW_RC  x86_64-w64-mingw32-windres)

if(NOT MINGW_CC)
    message(FATAL_ERROR
        "x86_64-w64-mingw32-gcc not found.\n"
        "Install with:  sudo apt-get install mingw-w64")
endif()

set(CMAKE_C_COMPILER   "${MINGW_CC}")
set(CMAKE_CXX_COMPILER "${MINGW_CXX}")
set(CMAKE_RC_COMPILER  "${MINGW_RC}")

# ----- Sysroot / search prefix ---------------------------------------------
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ----- SDL2 location (set via -DSDL2_MINGW_ROOT=...) ----------------------
# The variable SDL2_MINGW_ROOT should point to the arch-specific sub-directory
# of the SDL2 MinGW developer package, e.g.:
#   library/sdl2-windows/SDL2-2.32.2/x86_64-w64-mingw32
#
# If not provided on the command line, fall back to a sensible default relative
# to the source tree so the preset can be used without extra flags.
if(NOT SDL2_MINGW_ROOT)
    set(SDL2_MINGW_ROOT
        "${CMAKE_SOURCE_DIR}/library/sdl2-windows/SDL2-2.32.2/x86_64-w64-mingw32")
endif()

# Expose the SDL2 root to find_package
list(APPEND CMAKE_PREFIX_PATH "${SDL2_MINGW_ROOT}")
list(APPEND CMAKE_PREFIX_PATH "${SDL2_MINGW_ROOT}/lib/cmake/SDL2")
# SDL2_DIR must be set so find_package(SDL2) can locate sdl2-config.cmake even
# when CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY is in effect.
set(SDL2_DIR "${SDL2_MINGW_ROOT}/lib/cmake/SDL2" CACHE PATH "SDL2 CMake config dir" FORCE)
set(ENV{SDL2DIR} "${SDL2_MINGW_ROOT}")
