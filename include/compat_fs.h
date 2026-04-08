/*
 * Copyright (c) 2026 Stunts Engine Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef COMPAT_FS_H
#define COMPAT_FS_H

/* ── Filesystem portability helpers ─────────────────────────────────── */

/* Maximum path length (matches historical DOS/Win32 MAX_PATH) */
#define FS_PATH_MAX 260

/* Path separator test — accepts both forward and back slash */
#define FS_IS_SEP(c) ((c) == '/' || (c) == '\\')

#ifdef _WIN32
#include <io.h>
#define fs_strcasecmp(a, b) _stricmp((a), (b))
/* dirent emulation is provided by a third-party header on Windows;
 * if not available, the directory-scan functions are no-ops. */
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif
#else
#include <strings.h> /* strcasecmp */
#include <dirent.h>  /* DIR, struct dirent, opendir, readdir, closedir */
#define fs_strcasecmp(a, b) strcasecmp((a), (b))
#endif

#endif /* COMPAT_FS_H */
