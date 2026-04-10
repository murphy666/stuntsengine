cmake_minimum_required(VERSION 3.16)
project(stunts C CXX)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
option(STUNTS_ASAN    "Build with AddressSanitizer (GCC/Clang only)"  OFF)

# Generate compile_commands.json for clang-tidy / IDEs.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# C standard
# ---------------------------------------------------------------------------
set(CMAKE_C_STANDARD   11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)   # use -std=c11, not gnu11 (fmopl gets gnu11 explicitly)

# C++ standard
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# ---------------------------------------------------------------------------
# SDL2
#
# On Linux:  found via pkg-config / system headers automatically.
# On Windows (cross-compile): auto-downloaded to library/sdl2-windows/.
#   Override with -DSDL2_DIR=<path> if you have a custom installation.
# ---------------------------------------------------------------------------

# Auto-download SDL2 MinGW development archive for Windows cross-builds
if(WIN32)
    set(_SDL2_VERSION  "2.32.2")
    set(_SDL2_ARCHIVE  "SDL2-devel-${_SDL2_VERSION}-mingw.tar.gz")
    set(_SDL2_URL      "https://github.com/libsdl-org/SDL/releases/download/release-${_SDL2_VERSION}/${_SDL2_ARCHIVE}")
    set(_SDL2_LOCAL    "${CMAKE_CURRENT_SOURCE_DIR}/library/sdl2-windows")
    set(_SDL2_SENTINEL "${_SDL2_LOCAL}/SDL2-${_SDL2_VERSION}/x86_64-w64-mingw32/lib/cmake/SDL2/sdl2-config.cmake")

    if(NOT EXISTS "${_SDL2_SENTINEL}")
        message(STATUS "Downloading SDL2 ${_SDL2_VERSION} MinGW development archive ...")
        file(MAKE_DIRECTORY "${_SDL2_LOCAL}")
        set(_SDL2_TARBALL "${_SDL2_LOCAL}/${_SDL2_ARCHIVE}")
        file(DOWNLOAD "${_SDL2_URL}" "${_SDL2_TARBALL}"
            STATUS _sdl2_dl_status
            SHOW_PROGRESS)
        list(GET _sdl2_dl_status 0 _sdl2_dl_code)
        if(NOT _sdl2_dl_code EQUAL 0)
            list(GET _sdl2_dl_status 1 _sdl2_dl_msg)
            message(FATAL_ERROR
                "Failed to download SDL2: ${_sdl2_dl_msg}\n"
                "URL: ${_SDL2_URL}\n"
                "Download it manually and extract to ${_SDL2_LOCAL}/")
        endif()
        message(STATUS "Extracting ${_SDL2_ARCHIVE} ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${_SDL2_TARBALL}"
            WORKING_DIRECTORY "${_SDL2_LOCAL}"
            RESULT_VARIABLE _sdl2_tar_result)
        if(NOT _sdl2_tar_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract ${_SDL2_ARCHIVE}")
        endif()
        file(REMOVE "${_SDL2_TARBALL}")
    endif()

    set(SDL2_DIR "${_SDL2_LOCAL}/SDL2-${_SDL2_VERSION}/x86_64-w64-mingw32/lib/cmake/SDL2"
        CACHE PATH "SDL2 cmake config directory" FORCE)
    unset(_SDL2_VERSION)
    unset(_SDL2_ARCHIVE)
    unset(_SDL2_URL)
    unset(_SDL2_LOCAL)
    unset(_SDL2_SENTINEL)
endif()

find_package(SDL2 REQUIRED)

# Normalise: older FindSDL2.cmake exports SDL2_INCLUDE_DIRS / SDL2_LIBRARIES;
# vcpkg and the official SDL2 package export SDL2::SDL2.
if(NOT TARGET SDL2::SDL2)
    add_library(SDL2::SDL2 INTERFACE IMPORTED)
    set_target_properties(SDL2::SDL2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_INCLUDE_DIRS}"
        INTERFACE_LINK_LIBRARIES      "${SDL2_LIBRARIES}")
endif()

# The official SDL2 MinGW cmake config sets INTERFACE_INCLUDE_DIRECTORIES to
# "<arch>/include/SDL2" — one level too deep for #include <SDL2/SDL.h>.
# We add the parent dir ("<arch>/include") so both forms work.
if(WIN32 AND TARGET SDL2::SDL2)
    get_target_property(_sdl2_raw_inc SDL2::SDL2 INTERFACE_INCLUDE_DIRECTORIES)
    if(_sdl2_raw_inc)
        get_filename_component(_sdl2_inc_parent "${_sdl2_raw_inc}" DIRECTORY)
        if(IS_DIRECTORY "${_sdl2_inc_parent}")
            set_property(TARGET SDL2::SDL2 APPEND PROPERTY
                INTERFACE_INCLUDE_DIRECTORIES "${_sdl2_inc_parent}")
        endif()
    endif()
    unset(_sdl2_raw_inc)
    unset(_sdl2_inc_parent)
endif()

# ---------------------------------------------------------------------------
# Compiler warning flags (GCC / Clang / MSVC)
# ---------------------------------------------------------------------------
if(MSVC)
    set(STUNTS_WARN_FLAGS /W3 /wd4996 /wd4244 /wd4267)
else()
    set(STUNTS_WARN_FLAGS
        -Wall
        -Wextra
        -Wno-comment
        -Wunused-variable
        -Wunused-but-set-variable
        -Wunused-function
        -Wunused-parameter
        -Wunused-value)
endif()

# ---------------------------------------------------------------------------
# AddressSanitizer
# ---------------------------------------------------------------------------
if(STUNTS_ASAN)
    if(MSVC)
        set(ASAN_COMPILE_FLAGS /fsanitize:address)
        set(ASAN_LINK_FLAGS    /fsanitize:address)
    else()
        set(ASAN_COMPILE_FLAGS -fsanitize=address -fno-omit-frame-pointer)
        set(ASAN_LINK_FLAGS    -fsanitize=address)
    endif()
endif()

# ---------------------------------------------------------------------------
# Main game sources
# ---------------------------------------------------------------------------
file(GLOB ALL_SRC CONFIGURE_DEPENDS "src/*.c")

# ---------------------------------------------------------------------------
# Git submodule auto-init  (library/adplug)
# If the submodule checkout is missing, run `git submodule update --init`
# so that a fresh clone + cmake configure "just works" on Linux and Windows.
# ---------------------------------------------------------------------------
find_package(Git QUIET)
set(_adplug_sentinel "${CMAKE_CURRENT_SOURCE_DIR}/library/adplug/src/fmopl.c")
if(NOT EXISTS "${_adplug_sentinel}")
    if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        message(STATUS "Initialising git submodule: library/adplug ...")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init library/adplug
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            RESULT_VARIABLE _git_submod_result
        )
        if(NOT _git_submod_result EQUAL 0)
            message(FATAL_ERROR
                "git submodule update --init library/adplug failed (${_git_submod_result}).\n"
                "Please run it manually.")
        endif()
    else()
        message(FATAL_ERROR
            "library/adplug is missing and git is not available.\n"
            "Clone with --recurse-submodules or run:\n"
            "  git submodule update --init library/adplug")
    endif()
endif()
unset(_adplug_sentinel)

# ---------------------------------------------------------------------------
# OPL emulator sources (fmopl + nukedopl), compiled with isolated warning flags.
# ---------------------------------------------------------------------------
set(FMOPL_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/library/adplug/src/fmopl.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/library/adplug/src/nukedopl.c")

add_library(fmopl_obj OBJECT ${FMOPL_SRC})
target_include_directories(fmopl_obj PRIVATE library/adplug/src)
if(MSVC)
    target_compile_options(fmopl_obj PRIVATE /wd4100 /wd4244 /wd4267 /wd4189 /wd4701)
else()
    # fmopl uses gnu11 C extensions
    target_compile_options(fmopl_obj PRIVATE
        -std=gnu11 -O0
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-sign-compare
        -Wno-unused-but-set-variable)
endif()

# ---------------------------------------------------------------------------
# stunts executable
# ---------------------------------------------------------------------------
add_executable(stunts ${ALL_SRC} $<TARGET_OBJECTS:fmopl_obj>)

target_include_directories(stunts PRIVATE src include library/adplug/src)
target_compile_options(stunts PRIVATE
    ${STUNTS_WARN_FLAGS})
target_link_libraries(stunts PRIVATE SDL2::SDL2)

# Math library — not needed on Windows (MSVC CRT), required on Linux/MinGW
if(NOT MSVC)
    target_link_libraries(stunts PRIVATE m)
endif()

# Windows: tell SDL not to hijack main(); we provide our own console entry point.
# SDL2main.a would provide WinMain→SDL_main bridging which we don't need for a
# console app. Instead use SDL_MAIN_HANDLED + -mconsole linker flag.
if(WIN32)
    target_compile_definitions(stunts PRIVATE SDL_MAIN_HANDLED)
    target_link_options(stunts PRIVATE -mconsole)
endif()

if(STUNTS_ASAN)
    target_compile_options(stunts PRIVATE ${ASAN_COMPILE_FLAGS})
    target_link_options(stunts PRIVATE ${ASAN_LINK_FLAGS})
endif()

# On Linux/macOS, expose POSIX.1-2008 APIs (nanosleep, clock_gettime, etc.)
# which -std=c11 hides without a feature-test macro.
if(NOT WIN32)
    target_compile_definitions(stunts PRIVATE _POSIX_C_SOURCE=200809L)
endif()

# ---------------------------------------------------------------------------
# Copy game resources next to the binary after build (optional)
# ---------------------------------------------------------------------------
if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ressources")
    add_custom_command(TARGET stunts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/ressources"
            "$<TARGET_FILE_DIR:stunts>"
        COMMENT "Copying game resources")
else()
    message(STATUS "Game resources folder not found — skipping copy")
endif()

# On Windows cross-build: copy SDL2.dll next to the executable so it can run.
if(WIN32)
    # Derive the SDL2 arch root from SDL2_DIR:
    # SDL2_DIR = .../x86_64-w64-mingw32/lib/cmake/SDL2  (go up 3 levels)
    get_filename_component(_sdl2_cmake_dir "${SDL2_DIR}" ABSOLUTE)
    get_filename_component(_sdl2_lib_cmake "${_sdl2_cmake_dir}"   DIRECTORY)
    get_filename_component(_sdl2_lib       "${_sdl2_lib_cmake}"   DIRECTORY)
    get_filename_component(_sdl2_arch_root "${_sdl2_lib}"         DIRECTORY)
    set(_sdl2_dll "${_sdl2_arch_root}/bin/SDL2.dll")
    if(EXISTS "${_sdl2_dll}")
        add_custom_command(TARGET stunts POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_sdl2_dll}"
                "$<TARGET_FILE_DIR:stunts>"
            COMMENT "Copying SDL2.dll")
    else()
        message(WARNING "SDL2.dll not found at ${_sdl2_dll} — copy it manually next to stunts.exe")
    endif()

    # Copy MinGW runtime DLLs required by the produced executable when running
    # under Wine/Windows (e.g. libwinpthread-1.dll).
    set(_mingw_runtime_dlls
        libwinpthread-1.dll
        libgcc_s_seh-1.dll
        libstdc++-6.dll)

    foreach(_dll ${_mingw_runtime_dlls})
        execute_process(
            COMMAND ${CMAKE_C_COMPILER} -print-file-name=${_dll}
            OUTPUT_VARIABLE _dll_path
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        if(IS_ABSOLUTE "${_dll_path}" AND EXISTS "${_dll_path}")
            add_custom_command(TARGET stunts POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${_dll_path}"
                    "$<TARGET_FILE_DIR:stunts>"
                COMMENT "Copying ${_dll}")
        else()
            message(WARNING "${_dll} not found via ${CMAKE_C_COMPILER} -print-file-name=${_dll}")
        endif()
    endforeach()
endif()

# ---------------------------------------------------------------------------
# clang-format targets: `format` (in-place) and `format-check` (dry-run)
# ---------------------------------------------------------------------------
find_program(CLANG_FORMAT_EXE NAMES clang-format clang-format-20 clang-format-19
                                     clang-format-18 clang-format-17)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE STUNTS_FORMAT_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.c"
        "${CMAKE_SOURCE_DIR}/include/*.h"
    )
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${STUNTS_FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running clang-format on src/ and include/"
        VERBATIM
    )
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror ${STUNTS_FORMAT_SOURCES}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Checking clang-format compliance (dry-run)"
        VERBATIM
    )
else()
    message(STATUS "clang-format not found — format/format-check targets disabled")
endif()

# ---------------------------------------------------------------------------
# clang-tidy target: `tidy` (report) and `tidy-fix` (apply fixes)
# Requires compile_commands.json — ensure you build from a configured tree.
# ---------------------------------------------------------------------------
find_program(CLANG_TIDY_EXE NAMES clang-tidy clang-tidy-20 clang-tidy-19
                                   clang-tidy-18 clang-tidy-17)
if(CLANG_TIDY_EXE)
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY_EXE}
            -p "${CMAKE_BINARY_DIR}"
            ${ALL_SRC}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running clang-tidy (report only)"
        VERBATIM
    )
    add_custom_target(tidy-fix
        COMMAND ${CLANG_TIDY_EXE}
            -p "${CMAKE_BINARY_DIR}"
            --fix
            ${ALL_SRC}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Running clang-tidy --fix"
        VERBATIM
    )
else()
    message(STATUS "clang-tidy not found — tidy/tidy-fix targets disabled")
endif()
