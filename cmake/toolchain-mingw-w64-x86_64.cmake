# ---------------------------------------------------------------------------
# CMake toolchain file: Linux → Windows x86-64 cross-compilation via MinGW-w64
#
# Usage (one-time configure):
#   cmake -S . -B build_cmake_windows \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64-x86_64.cmake \
#         -DSDL3_MINGW_ROOT=library/sdl3-windows/SDL3-3.4.4/x86_64-w64-mingw32 \
#         -DCMAKE_BUILD_TYPE=Release
#
#   cmake --build build_cmake_windows
#
# The resulting stunts.exe and SDL3.dll are placed in build_cmake_windows/.
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

# ----- SDL3 location (set via -DSDL3_MINGW_ROOT=...) ----------------------
# The variable SDL3_MINGW_ROOT should point to the arch-specific sub-directory
# of the SDL3 MinGW developer package, e.g.:
#   library/sdl3-windows/SDL3-3.4.4/x86_64-w64-mingw32
#
# If not provided on the command line, fall back to a sensible default relative
# to the source tree so the preset can be used without extra flags.
if(NOT SDL3_MINGW_ROOT)
    file(GLOB _SDL3_MINGW_ROOT_GLOB
        "${CMAKE_SOURCE_DIR}/library/sdl3-windows/*/x86_64-w64-mingw32")
    list(LENGTH _SDL3_MINGW_ROOT_GLOB _SDL3_MINGW_ROOT_COUNT)
    if(_SDL3_MINGW_ROOT_COUNT GREATER 0)
        list(GET _SDL3_MINGW_ROOT_GLOB 0 SDL3_MINGW_ROOT)
    else()
        set(SDL3_MINGW_ROOT
            "${CMAKE_SOURCE_DIR}/library/sdl3-windows/SDL3-3.4.4/x86_64-w64-mingw32")
    endif()
endif()

# Expose the SDL3 root to find_package
list(APPEND CMAKE_PREFIX_PATH "${SDL3_MINGW_ROOT}")
list(APPEND CMAKE_PREFIX_PATH "${SDL3_MINGW_ROOT}/lib/cmake/SDL3")
# SDL3_DIR must be set so find_package(SDL3) can locate SDL3Config.cmake even
# when CMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY is in effect.
set(SDL3_DIR "${SDL3_MINGW_ROOT}/lib/cmake/SDL3" CACHE PATH "SDL3 CMake config dir" FORCE)
