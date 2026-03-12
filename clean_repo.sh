#!/usr/bin/env bash
# clean_repo.sh — strip build artifacts, FetchContent downloads, and
#                 git-submodule content, then optionally create an archive.
#
# Usage:
#   ./clean_repo.sh           # clean only
#   ./clean_repo.sh --archive # clean + create stuntsengine-clean.tar.gz

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
ARCHIVE=0
[[ "${1:-}" == "--archive" ]] && ARCHIVE=1

echo "==> Working in: $REPO_ROOT"
cd "$REPO_ROOT"

# ── 1. Build directories ──────────────────────────────────────────────────────
# All CMake build trees follow the build_cmake* naming convention.
# A plain 'build' directory is also cleaned for manual/legacy builds.
for dir in build build_cmake build_cmake_release build_cmake_asan build_cmake_windows; do
    if [[ -d "$dir" ]]; then
        echo "--> Removing $dir/"
        rm -rf "$dir"
    fi
done

# ── 2. Downloaded SDL2 Windows dev package ────────────────────────────────────
# CMake re-downloads it automatically on the next Windows cross-compile if the
# sentinel file (x86_64-w64-mingw32/lib/cmake/SDL2/sdl2-config.cmake) is gone.
SDL2_WIN_DIR="library/sdl2-windows"
if compgen -G "${SDL2_WIN_DIR}/SDL2-*" > /dev/null 2>&1; then
    echo "--> Removing ${SDL2_WIN_DIR}/SDL2-*/"
    rm -rf "${SDL2_WIN_DIR}"/SDL2-*/
fi
# Remove any leftover tarball too
rm -f "${SDL2_WIN_DIR}"/SDL2-devel-*.tar.gz 2>/dev/null || true

# ── 2. FetchContent / CMake cache leftovers in any other build dirs ───────────
find . -maxdepth 2 -name "CMakeCache.txt" \
    ! -path "./.git/*" \
    -exec sh -c 'echo "--> Removing $(dirname "$1")/" && rm -rf "$(dirname "$1")"' _ {} \;

# ── 3. Git submodule content ──────────────────────────────────────────────────
if [[ -f ".gitmodules" ]]; then
    echo "--> Deinitialising git submodules"
    git submodule deinit --all --force 2>/dev/null || true
    # Remove any leftover content inside submodule paths
    git config --file .gitmodules --get-regexp 'submodule\..*\.path' \
    | awk '{print $2}' \
    | while read -r sub_path; do
        if [[ -d "$sub_path" ]]; then
            echo "--> Cleaning submodule dir: $sub_path/"
            # Keep the directory so git knows the mount point, just empty it
            find "$sub_path" -mindepth 1 -delete 2>/dev/null || true
        fi
    done
    # Remove cached submodule objects from .git/modules/
    if [[ -d ".git/modules" ]]; then
        echo "--> Removing .git/modules/"
        rm -rf ".git/modules"
    fi
fi

# ── 4. Common generated junk ──────────────────────────────────────────────────
find . \( \
    -name "*.o" -o -name "*.a" -o -name "*.so" -o -name "*.dylib" \
    -o -name "*.exe" -o -name "*.pdb" -o -name "*.ilk" \
    -o -name "*.pyc" -o -name "__pycache__" \
\) ! -path "./.git/*" -delete 2>/dev/null || true

echo "==> Clean done."

# ── 5. Optional archive ───────────────────────────────────────────────────────
if [[ $ARCHIVE -eq 1 ]]; then
    ARCHIVE_NAME="$(basename "$REPO_ROOT")-clean.tar.gz"
    PARENT="$(dirname "$REPO_ROOT")"
    BASENAME="$(basename "$REPO_ROOT")"
    echo "==> Creating $PARENT/$ARCHIVE_NAME ..."
    tar -czf "$PARENT/$ARCHIVE_NAME" \
        --exclude=".git" \
        --exclude="*.tar.gz" \
        -C "$PARENT" "$BASENAME"
    echo "==> Archive written to: $PARENT/$ARCHIVE_NAME"
    ls -lh "$PARENT/$ARCHIVE_NAME"
fi
