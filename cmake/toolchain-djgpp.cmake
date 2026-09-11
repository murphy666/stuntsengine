# CMake toolchain file for DJGPP (32-bit protected-mode DOS).
#
# Usage:
#
#   1. Install a DJGPP cross-compiler and put the directory containing
#      i586-pc-msdosdjgpp-gcc on PATH.
#   2. cmake --preset dos     (or pass -DCMAKE_TOOLCHAIN_FILE=<this file>)
#
# Derived from SDL's DJGPP toolchain file.

set(CMAKE_SYSTEM_NAME DOS)
set(CMAKE_SYSTEM_PROCESSOR i386)

set(DJGPP TRUE)

# CMake's Platform/DOS.cmake assumes OpenWatcom naming conventions (no prefix,
# .lib suffix).  DJGPP uses standard Unix/GCC conventions for its system
# libraries (lib prefix, .a suffix — e.g. libm.a), so we override the platform
# defaults via CMAKE_USER_MAKE_RULES_OVERRIDE, which runs *after* the platform
# module has set its defaults, giving us the final say on these variables.
# The path must be cached because CMake re-parses the toolchain file during
# try_compile, where CMAKE_CURRENT_LIST_DIR may point elsewhere.
set(DJGPP_PLATFORM_OVERRIDES "${CMAKE_CURRENT_LIST_DIR}/djgpp-platform-overrides.cmake"
    CACHE FILEPATH "" FORCE)
set(CMAKE_USER_MAKE_RULES_OVERRIDE "${DJGPP_PLATFORM_OVERRIDES}")

set(CMAKE_STATIC_LIBRARY_PREFIX "lib")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".a")
set(CMAKE_SHARED_LIBRARY_PREFIX "")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_IMPORT_LIBRARY_PREFIX "lib")
set(CMAKE_IMPORT_LIBRARY_SUFFIX ".a")
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_LINK_LIBRARY_SUFFIX "")
set(CMAKE_DL_LIBS "")

set(CMAKE_FIND_LIBRARY_PREFIXES "lib")
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

find_program(CMAKE_C_COMPILER
    NAMES "i586-pc-msdosdjgpp-gcc" "i386-pc-msdosdjgpp-gcc" REQUIRED)
find_program(CMAKE_CXX_COMPILER
    NAMES "i586-pc-msdosdjgpp-g++" "i386-pc-msdosdjgpp-g++" REQUIRED)

# Derive the sysroot(s) from the compiler's own library search path so that
# find_library()/find_package() look at the DJGPP tree and never at the host's
# /usr/lib.
execute_process(COMMAND "${CMAKE_C_COMPILER}" -print-search-dirs
    RESULT_VARIABLE CC_SEARCH_DIRS_RESULT
    OUTPUT_VARIABLE CC_SEARCH_DIRS_OUTPUT)

if(CC_SEARCH_DIRS_RESULT)
    message(FATAL_ERROR "Could not determine DJGPP search dirs")
endif()

string(REGEX MATCH ".*libraries: (.*).*" CC_SD_LIBS "${CC_SEARCH_DIRS_OUTPUT}")
string(STRIP "${CMAKE_MATCH_1}" CC_SEARCH_DIRS)
string(REPLACE ":" ";" CC_SEARCH_DIRS "${CC_SEARCH_DIRS}")

foreach(CC_SEARCH_DIR ${CC_SEARCH_DIRS})
    if(CC_SEARCH_DIR MATCHES "=.*")
        string(REGEX MATCH "=(.*)" CC_LIB "${CC_SEARCH_DIR}")
        set(CC_SEARCH_DIR "${CMAKE_MATCH_1}")
    endif()
    if(IS_DIRECTORY "${CC_SEARCH_DIR}")
        if(IS_DIRECTORY "${CC_SEARCH_DIR}/../include"
           OR IS_DIRECTORY "${CC_SEARCH_DIR}/../lib"
           OR IS_DIRECTORY "${CC_SEARCH_DIR}/../bin")
            list(APPEND CC_ROOTS "${CC_SEARCH_DIR}/..")
        else()
            list(APPEND CC_ROOTS "${CC_SEARCH_DIR}")
        endif()
    endif()
endforeach()

list(APPEND CMAKE_FIND_ROOT_PATH ${CC_ROOTS})

# Programs come from the host, everything else from the target tree.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
if(NOT DEFINED CACHE{CMAKE_FIND_ROOT_PATH_MODE_LIBRARY})
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
endif()
if(NOT DEFINED CACHE{CMAKE_FIND_ROOT_PATH_MODE_INCLUDE})
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
if(NOT DEFINED CACHE{CMAKE_FIND_ROOT_PATH_MODE_PACKAGE})
    set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
endif()
