cmake_minimum_required(VERSION 3.16)
project(stunts C CXX)

# ---------------------------------------------------------------------------
# Build options
# ---------------------------------------------------------------------------
option(STUNTS_ASAN    "Build with AddressSanitizer (GCC/Clang only)"  OFF)
option(STUNTS_TESTS   "Build test executables"                         ON)

# ---------------------------------------------------------------------------
# C standard
# ---------------------------------------------------------------------------
set(CMAKE_C_STANDARD   11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)   # use -std=c11, not gnu11 (fmopl gets gnu11 explicitly)

# C++ standard (for GTest-based tests and any future .cc sources)
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
# Coverage (gcov / lcov)
# ---------------------------------------------------------------------------
option(STUNTS_COVERAGE "Build all test targets with gcov line-coverage instrumentation" OFF)
if(STUNTS_COVERAGE AND NOT MSVC)
    set(COVERAGE_COMPILE_FLAGS --coverage -fprofile-arcs -ftest-coverage -O0 -g)
    set(COVERAGE_LINK_FLAGS    --coverage)
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
# console app.  Instead use SDL_MAIN_HANDLED + -mconsole linker flag.
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
    get_filename_component(_sdl2_lib_cmake "${_sdl2_cmake_dir}"   DIRECTORY)  # lib/cmake
    get_filename_component(_sdl2_lib       "${_sdl2_lib_cmake}"   DIRECTORY)  # lib
    get_filename_component(_sdl2_arch_root "${_sdl2_lib}"         DIRECTORY)  # arch root
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
# Tests
# ---------------------------------------------------------------------------
if(STUNTS_TESTS)
    enable_testing()
    set(STUNTS_TEST_TARGETS "")

    # --- Google Test via FetchContent (falls back to system install) ---
    include(FetchContent)
    find_package(GTest QUIET)
    if(NOT GTest_FOUND)
        FetchContent_Declare(
            googletest
            GIT_REPOSITORY https://github.com/google/googletest.git
            GIT_TAG        v1.14.0
        )
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
        set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
        set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(googletest)
    endif()
    include(GoogleTest)

    # Helper macro: add a Google Test executable.
    # Each TEST() case is registered as an individual CTest entry via
    # gtest_discover_tests(), giving a sensible per-case count in ctest.
    # Usage: stunts_add_gtest(target_name test_src.cc  [extra_src.c ...])
    # Usage: stunts_add_gtest(target [WORKING_DIRECTORY dir] src1.cc [src2.c ...])
    macro(stunts_add_gtest _target)
        cmake_parse_arguments(_GTEST "" "WORKING_DIRECTORY" "" ${ARGN})
        set(_srcs ${_GTEST_UNPARSED_ARGUMENTS})
        set(_all_exist TRUE)
        foreach(_s ${_srcs})
            if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_s}")
                set(_all_exist FALSE)
                break()
            endif()
        endforeach()
        if(_all_exist)
            add_executable(${_target} ${_srcs})
            target_include_directories(${_target} PRIVATE src include)
            if(STUNTS_COVERAGE AND NOT MSVC)
                target_compile_options(${_target} PRIVATE ${COVERAGE_COMPILE_FLAGS})
                target_link_options(${_target} PRIVATE ${COVERAGE_LINK_FLAGS})
            endif()
            if(NOT MSVC)
                target_link_libraries(${_target} PRIVATE m)
            endif()
            target_link_libraries(${_target} PRIVATE GTest::gtest_main)
            if(_GTEST_WORKING_DIRECTORY)
                gtest_discover_tests(${_target}
                    WORKING_DIRECTORY "${_GTEST_WORKING_DIRECTORY}"
                    EXTRA_ARGS --gtest_color=yes
                    PROPERTIES ENVIRONMENT "GTEST_COLOR=yes")
            else()
                gtest_discover_tests(${_target}
                    EXTRA_ARGS --gtest_color=yes
                    PROPERTIES ENVIRONMENT "GTEST_COLOR=yes")
            endif()
            list(APPEND STUNTS_TEST_TARGETS ${_target})
            if(_GTEST_WORKING_DIRECTORY)
                set_property(TARGET ${_target} PROPERTY STUNTS_WORKING_DIR "${_GTEST_WORKING_DIRECTORY}")
            else()
                set_property(TARGET ${_target} PROPERTY STUNTS_WORKING_DIR "${CMAKE_BINARY_DIR}")
            endif()
        else()
            message(STATUS "[STUNTS] ${_target}: DISABLED — missing source files")
        endif()
    endmacro()

    # Helper for tests that need SDL2 + GTest
    macro(stunts_add_sdl_gtest _target)
        set(_srcs ${ARGN})
        set(_all_exist TRUE)
        foreach(_s ${_srcs})
            if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${_s}")
                set(_all_exist FALSE)
                break()
            endif()
        endforeach()
        if(_all_exist)
            add_executable(${_target} ${_srcs})
            target_include_directories(${_target} PRIVATE src include)
            target_compile_options(${_target} PRIVATE
                $<$<CONFIG:Debug>:-O0 -g>)
            if(STUNTS_COVERAGE AND NOT MSVC)
                target_compile_options(${_target} PRIVATE ${COVERAGE_COMPILE_FLAGS})
                target_link_options(${_target} PRIVATE ${COVERAGE_LINK_FLAGS})
            endif()
            target_link_libraries(${_target} PRIVATE SDL2::SDL2 GTest::gtest_main)
            if(NOT MSVC)
                target_link_libraries(${_target} PRIVATE m)
            endif()
            if(WIN32 AND TARGET SDL2::SDL2main)
                target_link_libraries(${_target} PRIVATE SDL2::SDL2main)
            endif()
            gtest_discover_tests(${_target}
                EXTRA_ARGS --gtest_color=yes
                PROPERTIES ENVIRONMENT "GTEST_COLOR=yes")
            list(APPEND STUNTS_TEST_TARGETS ${_target})
        else()
            message(STATUS "[STUNTS] ${_target}: DISABLED — missing source files")
        endif()
    endmacro()

    # --- Unit tests (no SDL2) ---
    stunts_add_gtest(test_memmgr_stub
        tests/test_memmgr_stub.cc
        src/memmgr.c)

    stunts_add_sdl_gtest(test_framebuffer
        tests/test_framebuffer.cc
        src/framebuffer.c)

    stunts_add_gtest(test_shape2d_resource
        tests/test_shape2d_resource.cc)

    stunts_add_gtest(test_font
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        tests/test_font.cc)

    stunts_add_gtest(test_shape3d_core
        tests/test_shape3d_core.cc)

    stunts_add_gtest(test_shape3d_resource
        tests/test_shape3d_resource.cc)

    stunts_add_gtest(test_shape3d_polyinfo
        tests/test_shape3d_polyinfo.cc)

    stunts_add_gtest(test_shape3d_pipeline
        tests/test_shape3d_pipeline.cc)

    stunts_add_gtest(test_shape3d_pointer_table_regression
        tests/test_shape3d_pointer_table_regression.cc)

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/test_outputs")
    stunts_add_gtest(test_polygon_render_image
        tests/test_polygon_render_image.cc)

    stunts_add_gtest(test_terrain_track_frame_check
        tests/test_terrain_track_frame_check.cc)

    stunts_add_gtest(test_math
        tests/test_math.cc
        src/math.c)

    stunts_add_gtest(test_shape2d_pure
        tests/test_shape2d_pure.cc
        tests/test_shape2d_pure_stubs.c
        src/shape2d.c)

    stunts_add_gtest(test_shape3d_layout
        tests/test_shape3d_layout.cc)

    stunts_add_gtest(test_header_centralization
        tests/test_header_centralization.cc)

    stunts_add_gtest(test_hill_terrain_render
        tests/test_hill_terrain_render.cc
        tests/test_hill_terrain_render_stubs.c
        src/shape3d.c
        src/math.c
        src/data_game.c)

    # --- SDL2 tests ---
    stunts_add_sdl_gtest(test_sdl_audio_tone
        tests/test_sdl_audio_tone.cc)

    stunts_add_sdl_gtest(test_intro_music_audio
        tests/test_intro_music_audio.cc)

    stunts_add_gtest(test_timer
        tests/test_timer.cc
        src/timer.c)
    if(TARGET test_timer)
        target_compile_definitions(test_timer PRIVATE _POSIX_C_SOURCE=200809L)
    endif()

    stunts_add_gtest(test_ressources
        tests/test_ressources.cc
        tests/test_ressources_stubs.c
        src/ressources.c)
    if(TARGET test_ressources)
        target_compile_definitions(test_ressources PRIVATE _GNU_SOURCE)
    endif()

    stunts_add_gtest(test_data_tables
        tests/test_data_tables.cc
        src/data_game.c
        src/data_menu_track.c
        src/data_runtime.c)

    stunts_add_gtest(test_track_decode
        tests/test_track_decode.cc)

    stunts_add_gtest(test_string_format
        tests/test_string_format.cc)

    stunts_add_gtest(test_steering
        tests/test_steering.cc)

    stunts_add_gtest(test_carstate_pure
        tests/test_carstate_pure.cc)

    # --- UI system tests ---
    stunts_add_gtest(test_ui_keys
        tests/test_ui_keys.cc)

    stunts_add_gtest(test_ui_widgets_pure
        tests/test_ui_widgets_pure.cc)

    stunts_add_gtest(test_ui_screen
        tests/test_ui_screen.cc
        tests/test_ui_screen_stubs.c
        src/ui_screen.c)

    # Generate a cmake file (at generate time) listing every test binary path
    # and its required working directory.
    # This file is consumed by cmake/run_tests.cmake so it knows what to run.
    set(_exe_lines "")
    set(_wdir_lines "")
    foreach(_t ${STUNTS_TEST_TARGETS})
        string(APPEND _exe_lines "  \"$<TARGET_FILE:${_t}>\"\n")
        string(APPEND _wdir_lines "  \"$<TARGET_PROPERTY:${_t},STUNTS_WORKING_DIR>\"\n")
    endforeach()
    file(GENERATE
        OUTPUT "${CMAKE_BINARY_DIR}/test_runner_targets.cmake"
        CONTENT "set(TEST_EXECUTABLES\n${_exe_lines})\nset(TEST_WORKING_DIRS\n${_wdir_lines})\n"
    )

    # 'make check' — numbered suite banners + grand total, fully colored
    add_custom_target(check
        COMMAND ${CMAKE_COMMAND}
            -DCMAKE_COMMAND=${CMAKE_COMMAND}
            -DTEST_TARGETS_FILE=${CMAKE_BINARY_DIR}/test_runner_targets.cmake
            -P ${CMAKE_SOURCE_DIR}/cmake/run_tests.cmake
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        DEPENDS ${STUNTS_TEST_TARGETS}
        COMMENT "Running all tests"
        USES_TERMINAL
    )

    # 'make run_tests' — alias for check
    add_custom_target(run_tests
        DEPENDS check
    )

    # 'make coverage' — run tests with gcov instrumentation, produce lcov HTML
    if(STUNTS_COVERAGE AND NOT MSVC)
        find_program(LCOV_EXECUTABLE    lcov    REQUIRED)
        find_program(GENHTML_EXECUTABLE genhtml REQUIRED)
        set(_cov_info "${CMAKE_BINARY_DIR}/coverage.info")
        set(_cov_html "${CMAKE_BINARY_DIR}/coverage_html")
        add_custom_target(coverage
            COMMAND ${LCOV_EXECUTABLE} --directory . --zerocounters
            COMMAND ${CMAKE_COMMAND} -E env GTEST_COLOR=yes
                ${CMAKE_CTEST_COMMAND} --output-on-failure -C $<CONFIG>
            COMMAND ${LCOV_EXECUTABLE}
                --directory .
                --base-directory "${CMAKE_SOURCE_DIR}"
                --capture
                --output-file "${_cov_info}"
                --ignore-errors inconsistent,inconsistent
            COMMAND ${LCOV_EXECUTABLE}
                --extract "${_cov_info}"
                "${CMAKE_SOURCE_DIR}/src/*"
                --output-file "${_cov_info}"
                --ignore-errors unused
            COMMAND ${GENHTML_EXECUTABLE}
                "${_cov_info}"
                --output-directory "${_cov_html}"
                --title "Stunts Engine Coverage"
                --legend
                --quiet
            COMMAND ${CMAKE_COMMAND} -E echo
                "Report ready: ${_cov_html}/index.html"
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            DEPENDS ${STUNTS_TEST_TARGETS}
            COMMENT "Generating gcov/lcov line-coverage report"
            USES_TERMINAL
        )
    endif()

endif() # STUNTS_TESTS
