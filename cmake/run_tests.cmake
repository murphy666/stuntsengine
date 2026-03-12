# cmake/run_tests.cmake
#
# Runs every GTest binary with full colored output, numbered suite banners,
# and a grand total summary.
#
# Required arguments (passed with -D):
#   CMAKE_COMMAND          - path to cmake executable (for cmake_echo_color)
#   TEST_TARGETS_FILE      - path to generated cmake file that sets TEST_EXECUTABLES

cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0007 NEW)
cmake_policy(SET CMP0057 NEW)   # IN_LIST operator

if(NOT TEST_TARGETS_FILE OR NOT EXISTS "${TEST_TARGETS_FILE}")
    message(FATAL_ERROR "TEST_TARGETS_FILE not set or missing: '${TEST_TARGETS_FILE}'")
endif()

include("${TEST_TARGETS_FILE}")

# ── helpers ──────────────────────────────────────────────────────────────────

macro(echo_color)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E cmake_echo_color ${ARGN})
endmacro()

set(SEP "══════════════════════════════════════════════════════════════")

# ── header ───────────────────────────────────────────────────────────────────

list(LENGTH TEST_EXECUTABLES num_suites)

echo_color("")
echo_color(--bold --cyan  "${SEP}")
echo_color(--bold --white "    STUNTS ENGINE TEST RUNNER  ·  ${num_suites} suites")
echo_color(--bold --cyan  "${SEP}")
echo_color("")

# ── run each suite ───────────────────────────────────────────────────────────

set(total_tests   0)
set(total_passed  0)
set(total_skipped 0)
set(total_failed  0)
set(failed_suites "")
set(idx 0)
set(_zidx 0)

foreach(exe IN LISTS TEST_EXECUTABLES)
    math(EXPR idx "${idx} + 1")
    get_filename_component(suite_name "${exe}" NAME_WE)

    # Resolve per-test working directory (falls back to CMAKE_BINARY_DIR)
    if(DEFINED TEST_WORKING_DIRS)
        list(GET TEST_WORKING_DIRS ${_zidx} _test_wdir)
    else()
        set(_test_wdir "")
    endif()
    if(NOT _test_wdir)
        set(_test_wdir "${CMAKE_BINARY_DIR}")
    endif()

    echo_color(--bold --white " ▶  [${idx}/${num_suites}]  ${suite_name}")
    echo_color(--cyan "${SEP}")

    execute_process(
        COMMAND "${exe}" --gtest_color=yes
        WORKING_DIRECTORY "${_test_wdir}"
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _out
        ERROR_VARIABLE  _err
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(_out)
        message("${_out}")
    endif()
    if(_err)
        message("${_err}")
    endif()

    # Strip ANSI color codes before regex matching
    string(ASCII 27 _ESC)
    string(REGEX REPLACE "${_ESC}\\[[0-9;]*m" "" _plain "${_out}")

    # Tally from standard GTest summary lines
    # "ran" line counts all executed (pass + fail + skip)
    set(_suite_ran    0)
    set(_suite_passed 0)
    set(_suite_failed 0)
    set(_suite_skipped 0)

    if(_plain MATCHES "\\[==========\\] ([0-9]+) tests? from [0-9]+ test suite[s]? ran")
        set(_suite_ran "${CMAKE_MATCH_1}")
        math(EXPR total_tests "${total_tests} + ${_suite_ran}")
    endif()
    if(_plain MATCHES "\\[  PASSED  \\] ([0-9]+) test")
        set(_suite_passed "${CMAKE_MATCH_1}")
        math(EXPR total_passed "${total_passed} + ${_suite_passed}")
    endif()
    if(_plain MATCHES "\\[  SKIPPED \\] ([0-9]+) test")
        set(_suite_skipped "${CMAKE_MATCH_1}")
        math(EXPR total_skipped "${total_skipped} + ${_suite_skipped}")
    endif()
    if(_plain MATCHES "\\[  FAILED  \\] ([0-9]+) test")
        set(_suite_failed "${CMAKE_MATCH_1}")
        math(EXPR total_failed "${total_failed} + ${_suite_failed}")
        list(APPEND failed_suites "${suite_name}")
    endif()

    # Non-GTest binary that exited non-zero (no standard summary line found)
    if(NOT _rc EQUAL 0 AND _suite_ran EQUAL 0)
        math(EXPR total_failed "${total_failed} + 1")
        math(EXPR total_tests  "${total_tests} + 1")
        if(NOT suite_name IN_LIST failed_suites)
            list(APPEND failed_suites "${suite_name}")
        endif()
    endif()

    echo_color("")
    math(EXPR _zidx "${_zidx} + 1")
endforeach()

# ── grand total ───────────────────────────────────────────────────────────────

echo_color(--bold --cyan "${SEP}")
if(total_failed EQUAL 0)
    if(total_skipped GREATER 0)
        echo_color(--bold --green
            " ✔  ${total_passed}/${total_tests} PASSED  (${total_skipped} skipped)  —  ${num_suites} suite(s)")
    else()
        echo_color(--bold --green
            " ✔  ALL ${total_tests} TESTS PASSED  —  ${num_suites} suite(s), 0 failures")
    endif()
    echo_color(--bold --green "${SEP}")
    echo_color("")
else()
    echo_color(--bold --red
        " ✘  ${total_failed} FAILED  /  ${total_tests} total")
    foreach(s IN LISTS failed_suites)
        echo_color(--bold --red "    • ${s}")
    endforeach()
    echo_color(--bold --red "${SEP}")
    echo_color("")
    message(FATAL_ERROR "Test failures detected.")
endif()
