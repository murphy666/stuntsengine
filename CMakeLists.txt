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
# SDL3
#
# On Linux:  found via CMake config or pkg-config / system headers.
# On Windows (cross-compile): auto-downloaded to library/sdl3-windows/.
#   Override with -DSDL3_DIR=<path> if you have a custom installation.
# ---------------------------------------------------------------------------

# Auto-download SDL3 MinGW development archive for Windows cross-builds.
if(WIN32)
    set(_SDL3_VERSION "3.4.4")
    set(_SDL3_ARCHIVE "SDL3-devel-${_SDL3_VERSION}-mingw.tar.gz")
    set(_SDL3_URL "https://github.com/libsdl-org/SDL/releases/download/release-${_SDL3_VERSION}/${_SDL3_ARCHIVE}")
    set(_SDL3_LOCAL "${CMAKE_CURRENT_SOURCE_DIR}/library/sdl3-windows")

    file(GLOB _SDL3_CONFIGS
        "${_SDL3_LOCAL}/*/x86_64-w64-mingw32/lib/cmake/SDL3/SDL3Config.cmake")

    if(NOT _SDL3_CONFIGS)
        message(STATUS "Downloading SDL3 ${_SDL3_VERSION} MinGW development archive ...")
        file(MAKE_DIRECTORY "${_SDL3_LOCAL}")
        set(_SDL3_TARBALL "${_SDL3_LOCAL}/${_SDL3_ARCHIVE}")
        file(DOWNLOAD "${_SDL3_URL}" "${_SDL3_TARBALL}"
            STATUS _sdl3_dl_status
            SHOW_PROGRESS)
        list(GET _sdl3_dl_status 0 _sdl3_dl_code)
        if(NOT _sdl3_dl_code EQUAL 0)
            list(GET _sdl3_dl_status 1 _sdl3_dl_msg)
            message(FATAL_ERROR
                "Failed to download SDL3: ${_sdl3_dl_msg}\n"
                "URL: ${_SDL3_URL}\n"
                "Download it manually and extract to ${_SDL3_LOCAL}/")
        endif()
        message(STATUS "Extracting ${_SDL3_ARCHIVE} ...")
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf "${_SDL3_TARBALL}"
            WORKING_DIRECTORY "${_SDL3_LOCAL}"
            RESULT_VARIABLE _sdl3_tar_result)
        if(NOT _sdl3_tar_result EQUAL 0)
            message(FATAL_ERROR "Failed to extract ${_SDL3_ARCHIVE}")
        endif()
        file(REMOVE "${_SDL3_TARBALL}")
        file(GLOB _SDL3_CONFIGS
            "${_SDL3_LOCAL}/*/x86_64-w64-mingw32/lib/cmake/SDL3/SDL3Config.cmake")
    endif()

    list(LENGTH _SDL3_CONFIGS _SDL3_CONFIG_COUNT)
    if(_SDL3_CONFIG_COUNT EQUAL 0)
        message(FATAL_ERROR "SDL3Config.cmake not found after extraction in ${_SDL3_LOCAL}")
    endif()
    list(GET _SDL3_CONFIGS 0 _SDL3_CONFIG)
    get_filename_component(_SDL3_CONFIG_DIR "${_SDL3_CONFIG}" DIRECTORY)

    set(SDL3_DIR "${_SDL3_CONFIG_DIR}"
        CACHE PATH "SDL3 cmake config directory" FORCE)
    unset(_SDL3_VERSION)
    unset(_SDL3_ARCHIVE)
    unset(_SDL3_URL)
    unset(_SDL3_LOCAL)
    unset(_SDL3_CONFIGS)
    unset(_SDL3_CONFIG_COUNT)
    unset(_SDL3_CONFIG)
    unset(_SDL3_CONFIG_DIR)
endif()

find_package(SDL3 QUIET CONFIG)

if(TARGET SDL3::SDL3)
    set(STUNTS_SDL_TARGET SDL3::SDL3)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL3 REQUIRED IMPORTED_TARGET sdl3)
    set(STUNTS_SDL_TARGET PkgConfig::SDL3)
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
target_link_libraries(stunts PRIVATE ${STUNTS_SDL_TARGET})

# Math library — not needed on Windows (MSVC CRT), required on Linux/MinGW
if(NOT MSVC)
    target_link_libraries(stunts PRIVATE m)
endif()

# Windows: keep the console entry point and bypass SDL's optional main wrapper.
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

# On Windows cross-build: copy SDL3.dll next to the executable so it can run.
if(WIN32)
    # Derive the SDL3 arch root from SDL3_DIR:
    # SDL3_DIR = .../x86_64-w64-mingw32/lib/cmake/SDL3  (go up 3 levels)
    get_filename_component(_sdl3_cmake_dir "${SDL3_DIR}" ABSOLUTE)
    get_filename_component(_sdl3_lib_cmake "${_sdl3_cmake_dir}"   DIRECTORY)
    get_filename_component(_sdl3_lib       "${_sdl3_lib_cmake}"   DIRECTORY)
    get_filename_component(_sdl3_arch_root "${_sdl3_lib}"         DIRECTORY)
    set(_sdl3_dll "${_sdl3_arch_root}/bin/SDL3.dll")
    if(EXISTS "${_sdl3_dll}")
        add_custom_command(TARGET stunts POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${_sdl3_dll}"
                "$<TARGET_FILE_DIR:stunts>"
            COMMENT "Copying SDL3.dll")
    else()
        message(WARNING "SDL3.dll not found at ${_sdl3_dll} — copy it manually next to stunts.exe")
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
