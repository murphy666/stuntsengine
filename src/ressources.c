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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "compat_fs.h"
#include "ressources.h"

short is_audioloaded = 0;
char  g_is_busy      = 0;
#include "memmgr.h"
#include "shape2d.h"
#include "stunts.h"

static char g_find_query[FS_PATH_MAX];
static char g_find_matches[256][FS_PATH_MAX];
static int  g_find_match_count;
static int  g_find_match_index;
static int  file_resolve_existing_path(const char* filename, char* out, size_t out_size);

static const char* const g_file_search_prefixes[] = { "", "ressources/", "build/", NULL };

/* ── Decompression constants ────────────────────────────────────────── */

#define RS_RLE_ESCLEN_MAX    16
#define RS_RLE_ESCLOOKUP_LEN 256
#define RS_RLE_ESCSEQ_POS    1
#define RS_VLE_ESC_LEN       16
#define RS_VLE_ALPH_LEN      256
#define RS_VLE_ESC_WIDTH     64
#define RS_VLE_NUM_SYMB      128

static uint32_t file_read_u24_le(const uint8_t* p)
{
	return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16);
}

/* ── RLE decompression ──────────────────────────────────────────────── */

static int file_decomp_rle_seq_gnu(const uint8_t* src, size_t src_len, uint8_t esc,
								   uint8_t* dst, size_t dst_cap, size_t* out_len)
{
	const uint8_t* src_end = src + src_len;
	size_t written = 0;

	while (src < src_end) {
		uint8_t cur = *src++;
		if (cur == esc) {
			const uint8_t* seq_start = src;
			const uint8_t* seq_end;
			uint8_t rep;
			size_t seq_len, rep_idx;

			while (src < src_end && *src != esc) {
				if (written >= dst_cap) return 0;
				dst[written++] = *src++;
			}
			if (src + 1 >= src_end) return 0;
			src++;
			rep = (uint8_t)(*src++ - 1);
			seq_end = src;
			if (seq_end < seq_start + 2) return 0;

			seq_len = (size_t)((seq_end - 2) - seq_start);
			for (rep_idx = 0; rep_idx < rep; rep_idx++) {
				if (written + seq_len > dst_cap) return 0;
				memcpy(&dst[written], seq_start, seq_len);
				written += seq_len;
			}
		} else {
			if (written >= dst_cap) return 0;
			dst[written++] = cur;
		}
	}
	*out_len = written;
	return 1;
}

static int file_decomp_rle_single_gnu(const uint8_t* src, size_t src_len,
									  const uint8_t* esclookup, uint8_t* dst, size_t dst_len)
{
	size_t src_pos = 0, dst_pos = 0;

	while (dst_pos < dst_len) {
		uint8_t cur, escv;
		if (src_pos >= src_len) return 0;

		cur = src[src_pos++];
		escv = esclookup[cur];
		if (escv != 0) {
			uint32_t rep = 0;
			uint8_t value;
			if (escv == 1) {
				if (src_pos + 1 >= src_len) return 0;
				rep = src[src_pos++]; value = src[src_pos++];
			} else if (escv == 3) {
				if (src_pos + 2 >= src_len) return 0;
				rep = (uint32_t)src[src_pos++];
				rep |= (uint32_t)src[src_pos++] << 8;
				value = src[src_pos++];
			} else {
				if (src_pos >= src_len) return 0;
				rep = (uint32_t)escv - 1u; value = src[src_pos++];
			}
			if (dst_pos + rep > dst_len) return 0;
			memset(&dst[dst_pos], value, rep);
			dst_pos += rep;
		} else {
			dst[dst_pos++] = cur;
		}
	}
	return 1;
}

static int file_decomp_rle_gnu(const uint8_t* src, size_t src_len, uint8_t** out, size_t* out_len)
{
	uint32_t len, srclen;
	uint8_t esclen, skipseq, esclookup[RS_RLE_ESCLOOKUP_LEN];
	uint8_t* dst;
	int i;

	if (src_len < 10 || !out || !out_len) return 0;

	len    = file_read_u24_le(&src[1]);
	srclen = file_read_u24_le(&src[4]);
	esclen = src[8];
	skipseq = (uint8_t)((esclen & 128u) != 0u);
	esclen &= 127u;
	if (esclen > RS_RLE_ESCLEN_MAX) return 0;
	if (src_len < (size_t)(9u + esclen + srclen)) return 0;

	memset(esclookup, 0, sizeof(esclookup));
	for (i = 0; i < (int)esclen; i++)
		esclookup[src[9 + i]] = (uint8_t)(i + 1);

	dst = (uint8_t*)malloc((size_t)len);
	if (!dst) return 0;

	if (skipseq == 0u) {
		uint8_t* seqbuf = (uint8_t*)malloc((size_t)len);
		size_t seq_len = 0;
		int ok;
		if (!seqbuf) { free(dst); return 0; }
		ok = file_decomp_rle_seq_gnu(&src[9 + esclen], srclen,
					src[9 + RS_RLE_ESCSEQ_POS], seqbuf, (size_t)len, &seq_len);
		if (!ok || !file_decomp_rle_single_gnu(seqbuf, seq_len, esclookup, dst, (size_t)len)) {
			free(seqbuf); free(dst); return 0;
		}
		free(seqbuf);
	} else {
		if (!file_decomp_rle_single_gnu(&src[9 + esclen], srclen, esclookup, dst, (size_t)len)) {
			free(dst); return 0;
		}
	}
	*out = dst;
	*out_len = (size_t)len;
	return 1;
}

/* ── VLE decompression ──────────────────────────────────────────────── */

static int file_decomp_vle_gnu(const uint8_t* src, size_t src_len, uint8_t** out, size_t* out_len)
{
	uint32_t len, lenleft;
	uint8_t* dst;
	size_t dst_pos = 0;
	const uint8_t* p, *end;
	uint16_t esc1[RS_VLE_ESC_LEN], esc2[RS_VLE_ESC_LEN];
	uint8_t alph[RS_VLE_ALPH_LEN], symb[RS_VLE_ALPH_LEN], wdth[RS_VLE_ALPH_LEN];
	uint8_t esclen;
	int additive;
	unsigned int i, j, alphlen, width, widthdistr;
	uint8_t numsymb, symbwdth, numsymbleft, curwdt, cursymb;
	uint16_t curword;

	if (src_len < 5 || !out || !out_len) return 0;

	len = file_read_u24_le(&src[1]);
	dst = (uint8_t*)malloc((size_t)len + 1u);
	if (!dst) return 0;

	p = src + 4; end = src + src_len;
	if (p >= end) { free(dst); return 0; }

	esclen = *p++;
	additive = (esclen & 128u) != 0;
	esclen &= 127u;
	if (esclen == 0 || esclen > RS_VLE_ESC_LEN) { free(dst); return 0; }

	for (i = 0, j = 0, alphlen = 0; i < esclen; ++i, j *= 2) {
		uint8_t tmp;
		if (p >= end) { free(dst); return 0; }
		esc1[i] = (uint16_t)(alphlen - j);
		tmp = *p++;
		j += tmp; alphlen += tmp;
		esc2[i] = (uint16_t)j;
	}

	if ((size_t)(end - p) < alphlen) { free(dst); return 0; }
	memcpy(alph, p, alphlen);
	p += alphlen;

	{
		const uint8_t* wp = src + 5;
		width = 1;
		widthdistr = (esclen >= 8 ? 8 : esclen);
		numsymb = RS_VLE_NUM_SYMB;
		for (i = 0, j = 0; width <= widthdistr; ++width, numsymb >>= 1) {
			if (wp >= end) { free(dst); return 0; }
			for (symbwdth = *wp++; symbwdth > 0; --symbwdth, ++j)
				for (numsymbleft = numsymb; numsymbleft; --numsymbleft, ++i) {
					symb[i] = alph[j]; wdth[i] = (uint8_t)width;
				}
		}
		for (; i < RS_VLE_ALPH_LEN; ++i) wdth[i] = RS_VLE_ESC_WIDTH;
	}

	if ((size_t)(end - p) < 2) { free(dst); return 0; }
	curword = (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
	p += 2; curwdt = 8; cursymb = 0;

	lenleft = len + 1u;
	while (lenleft != 0) {
		uint8_t code = (uint8_t)(curword >> 8);
		uint8_t nextwdt = wdth[code];

		if (nextwdt > 8) {
			uint8_t codebyte = (uint8_t)curword;
			curword >>= 8; i = 7;
			for (;;) {
				if (curwdt == 0) {
					if (p >= end) { free(dst); return 0; }
					codebyte = *p++; curwdt = 8;
				}
				curword = (uint16_t)((curword << 1) + ((codebyte & 128u) != 0u));
				codebyte <<= 1; --curwdt; ++i;
				if (curword < esc2[i]) {
					curword = (uint16_t)(curword + esc1[i]);
					cursymb = (uint8_t)(additive ? (uint8_t)(cursymb + alph[curword]) : alph[curword]);
					if (dst_pos <= len) dst[dst_pos++] = cursymb;
					--lenleft; break;
				}
			}
			if (p >= end) { free(dst); return 0; }
			curword = (uint16_t)((codebyte << curwdt) | *p++);
			nextwdt = (uint8_t)(8 - curwdt); curwdt = 8;
		} else {
			cursymb = (uint8_t)(additive ? (uint8_t)(cursymb + symb[code]) : symb[code]);
			if (dst_pos <= len) dst[dst_pos++] = cursymb;
			--lenleft;
			if (curwdt < nextwdt) {
				if (p >= end) { free(dst); return 0; }
				curword = (uint16_t)(curword << curwdt);
				nextwdt = (uint8_t)(nextwdt - curwdt);
				curwdt = 8;
				curword = (uint16_t)(curword | *p++);
			}
		}
		curword = (uint16_t)(curword << nextwdt);
		curwdt  = (uint8_t)(curwdt - nextwdt);
	}

	if (dst_pos < len) { free(dst); return 0; }
	*out = dst;
	*out_len = (size_t)len;
	return 1;
}

/* ── Audio helpers ──────────────────────────────────────────────────── */

static void* file_try_binary_with_exts(const char* filename, const char* const* exts)
{
	if (!filename || !exts) return NULL;
	for (int i = 0; exts[i]; i++) {
		char path[FS_PATH_MAX];
		snprintf(path, sizeof(path), "%s%s", filename, exts[i]);
		void* ptr = file_load_binary_nofatal(path);
		if (ptr) return ptr;
	}
	return NULL;
}

static void* file_try_driver_prefixed_binary(const char* filename, const char* const* exts)
{
	char prefix[3], stem[FS_PATH_MAX];
	if (!filename || !exts) return NULL;

	prefix[0] = (char)tolower((unsigned char)audiodriverstring[0]);
	prefix[1] = (char)tolower((unsigned char)audiodriverstring[1]);
	prefix[2] = '\0';
	if (prefix[0] == 's' && prefix[1] == 'b') { prefix[0] = 'a'; prefix[1] = 'd'; }
	if (!prefix[0] || !prefix[1]) return NULL;

	snprintf(stem, sizeof(stem), "%s%s", prefix, filename);
	return file_try_binary_with_exts(stem, exts);
}

/* ── Case-insensitive path resolution ───────────────────────────────── */

/* Walk every path component and resolve each via case-insensitive dir scan.
   On Windows the FS is already CI so we just check existence. */
static int file_resolve_case_insensitive(const char* input, char* out, size_t out_size)
{
	if (!input || !out || out_size == 0) return 0;
#ifdef _WIN32
	snprintf(out, out_size, "%s", input);
	{ FILE* f = fopen(out, "rb"); if (f) { fclose(f); return 1; } }
	return 0;
#else
	char resolved[FS_PATH_MAX], component[FS_PATH_MAX];
	const char* p = input;
	resolved[0] = '\0';

	while (*p) {
		const char* seg;
		size_t seg_len, rlen;
		DIR* dir;
		struct dirent* ent;
		int found = 0;

		while (FS_IS_SEP(*p)) p++;
		if (!*p) break;

		seg = p;
		while (*p && !FS_IS_SEP(*p)) p++;
		seg_len = (size_t)(p - seg);
		if (seg_len >= sizeof(component)) return 0;
		memcpy(component, seg, seg_len);
		component[seg_len] = '\0';

		dir = opendir(resolved[0] ? resolved : ".");
		if (!dir) return 0;
		while ((ent = readdir(dir))) {
			if (fs_strcasecmp(ent->d_name, component) == 0) {
				rlen = strlen(resolved);
				if (rlen > 0)
					snprintf(resolved + rlen, sizeof(resolved) - rlen, "/%s", ent->d_name);
				else
					snprintf(resolved, sizeof(resolved), "%s", ent->d_name);
				found = 1; break;
			}
		}
		closedir(dir);
		if (!found) return 0;
	}
	if (!resolved[0]) return 0;
	snprintf(out, out_size, "%s", resolved);
	return 1;
#endif
}

/* Case-insensitive wildcard match (*, ?) */
static int file_match_pattern_ci(const char* pattern, const char* text)
{
	const char* star = NULL, *star_text = NULL;
	if (!pattern || !text) return 0;

	while (*text) {
		if (*pattern == '*')              { star = pattern++; star_text = text; continue; }
		if (*pattern == '?' || tolower((unsigned char)*pattern) == tolower((unsigned char)*text))
		                                  { pattern++; text++; continue; }
		if (star)                         { pattern = star + 1; text = ++star_text; continue; }
		return 0;
	}
	while (*pattern == '*') pattern++;
	return *pattern == '\0';
}

/* Split path into directory + basename at the last separator */
static const char* path_split_dir(const char* path, char* dirout, size_t dirsize)
{
	const char* sep = strrchr(path, '/');
	if (!sep) sep = strrchr(path, '\\');
	if (sep) {
		size_t n = (size_t)(sep - path);
		if (n >= dirsize) return NULL;
		memcpy(dirout, path, n);
		dirout[n] = '\0';
		return sep + 1;
	}
	dirout[0] = '.'; dirout[1] = '\0';
	return path;
}

static int file_resolve_wildcard_path(const char* query, char* out, size_t out_size)
{
	if (!query || !out || out_size == 0) return 0;

	g_find_match_count = 0;
	g_find_match_index = 0;

	for (int i = 0; g_file_search_prefixes[i]; i++) {
		char candidate[FS_PATH_MAX], dirpath[FS_PATH_MAX];
		const char* pattern;
		DIR* dir;
		struct dirent* ent;

		snprintf(candidate, sizeof(candidate), "%s%s", g_file_search_prefixes[i], query);
		pattern = path_split_dir(candidate, dirpath, sizeof(dirpath));
		if (!pattern) continue;

		dir = opendir(dirpath);
		if (!dir) continue;

		while ((ent = readdir(dir))) {
			int k, n;
			if (!file_match_pattern_ci(pattern, ent->d_name)) continue;
			if (g_find_match_count >= 256) break;

			/* Build resolved path directly into the match slot */
			if (strcmp(dirpath, ".") == 0)
				n = snprintf(g_find_matches[g_find_match_count], FS_PATH_MAX, "%s", ent->d_name);
			else
				n = snprintf(g_find_matches[g_find_match_count], FS_PATH_MAX, "%s/%s", dirpath, ent->d_name);
			if (n < 0 || n >= FS_PATH_MAX) continue;

			/* Deduplicate */
			for (k = 0; k < g_find_match_count; ++k)
				if (fs_strcasecmp(g_find_matches[k], g_find_matches[g_find_match_count]) == 0) break;

			if (k == g_find_match_count) g_find_match_count++;
		}
		closedir(dir);
	}

	if (g_find_match_count > 0) {
		snprintf(out, out_size, "%s", g_find_matches[0]);
		return 1;
	}
	return 0;
}

/* ── Path resolution (prefix search + case-insensitive fallback) ──── */

static int file_resolve_existing_path(const char* filename, char* out, size_t out_size)
{
	if (!filename || !out || out_size == 0) return 0;

	for (int i = 0; g_file_search_prefixes[i]; i++) {
		char tmp[FS_PATH_MAX];
		FILE* f;
		snprintf(tmp, sizeof(tmp), "%s%s", g_file_search_prefixes[i], filename);

		f = fopen(tmp, "rb");
		if (f) { fclose(f); snprintf(out, out_size, "%s", tmp); return 1; }
		if (file_resolve_case_insensitive(tmp, out, out_size)) return 1;
	}
	return 0;
}

/* ── Public file-find API ───────────────────────────────────────────── */

static void file_find_reset(void)
{
	g_find_query[0] = '\0';
	g_find_match_count = 0;
	g_find_match_index = 0;
}

const char* file_find(const char* query)
{
	char resolved[FS_PATH_MAX];
	if (!query) { file_find_reset(); return NULL; }

	if (strchr(query, '*') || strchr(query, '?')) {
		if (!file_resolve_wildcard_path(query, resolved, sizeof(resolved)))
			{ file_find_reset(); return NULL; }
	} else if (!file_resolve_existing_path(query, resolved, sizeof(resolved))) {
		file_find_reset(); return NULL;
	} else {
		g_find_match_count = 0;
		g_find_match_index = 0;
	}

	snprintf(g_find_query, sizeof(g_find_query), "%s", resolved);
	return g_find_query;
}

const char* file_find_next(void)
{
	if (++g_find_match_index >= g_find_match_count) return NULL;
	snprintf(g_find_query, sizeof(g_find_query), "%s", g_find_matches[g_find_match_index]);
	return g_find_query;
}

const char* file_find_next_alt(void) { return file_find_next(); }

/* ── Path building ──────────────────────────────────────────────────── */

void file_build_path(const char* dir, const char* name, const char* ext, char* dst, size_t dst_size)
{
	size_t pos = 0;
	if (!dst || dst_size == 0) return;
	dst[0] = '\0';

	if (dir && *dir) {
		pos = (size_t)snprintf(dst, dst_size, "%s", dir);
		if (pos > 0 && pos < dst_size - 1 && !FS_IS_SEP(dst[pos - 1]) && dst[pos - 1] != ':')
			{ dst[pos++] = '/'; dst[pos] = '\0'; }
	}
	if (name) pos += (size_t)snprintf(dst + pos, dst_size - pos, "%s", name);
	if (ext)  snprintf(dst + pos, dst_size - pos, "%s", ext);
}

const char* file_combine_and_find(const char* dir, const char* name, const char* ext)
{
	static char path[FS_PATH_MAX];
	file_build_path(dir, name, ext, path, sizeof(path));
	return file_find(path);
}

/* ── File size / paragraphs ─────────────────────────────────────────── */

unsigned short file_paras(const char* filename, int fatal)
{
	char resolved[FS_PATH_MAX];
	FILE* f;
	long size;

	if (!file_resolve_existing_path(filename, resolved, sizeof(resolved))) {
		if (fatal) fprintf(stderr, "file_paras: cannot open %s\n", filename ? filename : "(null)");
		return 0;
	}
	f = fopen(resolved, "rb");
	if (!f) {
		if (fatal) fprintf(stderr, "file_paras: cannot open %s\n", filename ? filename : "(null)");
		return 0;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fclose(f);
	return (size > 0) ? (unsigned short)(((unsigned long)size + 15UL) >> 4) : 0;
}

unsigned short file_paras_fatal(const char* filename)        { return file_paras(filename, 1); }
unsigned short file_paras_nofatal(const char* filename)      { return file_paras(filename, 0); }
unsigned short file_decomp_paras(const char* filename, int fatal) { return file_paras(filename, fatal); }
unsigned short file_decomp_paras_fatal(const char* filename) { return file_paras(filename, 1); }
unsigned short file_decomp_paras_nofatal(const char* filename) { return file_paras(filename, 0); }

/* ── File read / write ──────────────────────────────────────────────── */

void* file_read(const char* filename, void* dst, int fatal)
{
	char resolved[FS_PATH_MAX];
	FILE* f;
	long size;

	if (!file_resolve_existing_path(filename, resolved, sizeof(resolved))) {
		if (fatal) fprintf(stderr, "file_read: cannot open %s\n", filename ? filename : "(null)");
		return NULL;
	}
	f = fopen(resolved, "rb");
	if (!f) {
		if (fatal) fprintf(stderr, "file_read: cannot open %s\n", filename ? filename : "(null)");
		return NULL;
	}
	fseek(f, 0, SEEK_END);
	size = ftell(f);
	fseek(f, 0, SEEK_SET);
	if (size <= 0) { fclose(f); return NULL; }

	if (!dst) {
		/* 16-byte aligned alloc with padding to prevent overreads in legacy bitmap loops */
		size_t alloc_size = ((size_t)size + 31u) & ~(size_t)15u;
		dst = calloc(1, alloc_size);
	}
	if (dst) fread(dst, 1, (size_t)size, f);
	fclose(f);
	return dst;
}

void* file_read_fatal(const char* filename, void* dst)   { return file_read(filename, dst, 1); }
void* file_read_nofatal(const char* filename, void* dst) { return file_read(filename, dst, 0); }

short file_write(const char* filename, void* src, unsigned long length, int fatal)
{
	FILE* f = fopen(filename, "wb");
	if (!f) {
		if (fatal) fprintf(stderr, "file_write: cannot open %s\n", filename ? filename : "(null)");
		return -1;
	}
	if (src && length > 0) fwrite(src, 1, (size_t)length, f);
	fclose(f);
	return 0;
}

short file_write_fatal(const char* filename, void* src, unsigned long length)   { return file_write(filename, src, length, 1); }
short file_write_nofatal(const char* filename, void* src, unsigned long length) { return file_write(filename, src, length, 0); }

/* ── Decompression (multi-pass RLE/VLE) ─────────────────────────────── */

static int file_get_size_resolved(const char* filename, size_t* out_size)
{
	char resolved[FS_PATH_MAX];
	FILE* f;
	long size;

	if (!out_size || !file_resolve_existing_path(filename, resolved, sizeof(resolved)))
		return 0;
	f = fopen(resolved, "rb");
	if (!f) return 0;
	if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
	size = ftell(f);
	fclose(f);
	if (size <= 0) return 0;
	*out_size = (size_t)size;
	return 1;
}

void* file_decomp(const char* filename, int fatal)
{
	void* cached;
	uint8_t *src_raw, *src, *pass_src, *current = NULL;
	size_t src_size, src_padded_size, pass_src_len, current_len = 0, current_out_len = 0;
	uint8_t passes, pass_index;
	void* result;

	if (!filename) return NULL;
	cached = mmgr_get_chunk_by_name(filename);
	if (cached) return cached;

	if (!file_get_size_resolved(filename, &src_size)) {
		if (fatal) fprintf(stderr, "file_decomp: cannot open %s\n", filename);
		return NULL;
	}
	src_raw = (uint8_t*)file_read(filename, NULL, fatal);
	if (!src_raw || src_size < 4) { free(src_raw); return NULL; }

	src_padded_size = ((src_size + 15u) & ~15u) + 16u;
	src = (uint8_t*)calloc(1, src_padded_size);
	if (!src) { free(src_raw); return NULL; }
	memcpy(src, src_raw, src_size);
	free(src_raw);

	if ((src[0] & 128u) != 0u) {
		passes = (uint8_t)(src[0] & 127u);
		if (passes == 0 || passes > 8 || src_padded_size < 5 || (src[4] != 1 && src[4] != 2)) {
			free(src);
			if (fatal) fprintf(stderr, "invalid packed data in %s\n", filename);
			return NULL;
		}
		pass_src = src + 4;
		pass_src_len = src_padded_size - 4;
	} else {
		if (src[0] != 1 && src[0] != 2) {
			free(src);
			if (fatal) fprintf(stderr, "invalid packed data in %s\n", filename);
			return NULL;
		}
		passes = 1;
		pass_src = src;
		pass_src_len = src_padded_size;
	}

	for (pass_index = 0; pass_index < passes; pass_index++) {
		uint8_t* out_buf = NULL;
		size_t out_len = 0;
		int ok = 0;

		if (pass_src_len == 0) { free(current); free(src); return NULL; }

		if (pass_src[0] == 1)      ok = file_decomp_rle_gnu(pass_src, pass_src_len, &out_buf, &out_len);
		else if (pass_src[0] == 2) ok = file_decomp_vle_gnu(pass_src, pass_src_len, &out_buf, &out_len);

		if (!ok || !out_buf || out_len == 0) {
			free(out_buf); free(current); free(src);
			if (fatal) fprintf(stderr, "invalid packed data in %s\n", filename);
			return NULL;
		}

		free(current);
		current_out_len = out_len;

		if (pass_index + 1u < passes) {
			size_t padded_len = ((out_len + 15u) & ~15u) + 16u;
			current = (uint8_t*)calloc(1, padded_len);
			if (!current) { free(out_buf); free(src); return NULL; }
			memcpy(current, out_buf, out_len);
			current_len = padded_len;
			free(out_buf);
		} else {
			current = out_buf;
			current_len = out_len;
		}
		pass_src = current;
		pass_src_len = current_len;
	}

	result = mmgr_alloc_resbytes(filename, (long int)current_out_len);
	if (!result) { free(current); free(src); return NULL; }
	memcpy(result, current, current_out_len);
	free(current);
	free(src);
	return result;
}

void* file_decomp_fatal(const char* filename)   { return file_decomp(filename, 1); }
void* file_decomp_nofatal(const char* filename) { return file_decomp(filename, 0); }

void* file_load_binary(const char* filename, int fatal)  { return file_read(filename, NULL, fatal); }
void* file_load_binary_nofatal(const char* filename)     { return file_read(filename, NULL, 0); }
void* file_load_binary_fatal(const char* filename)       { return file_read(filename, NULL, 1); }

/* ── Resource loading ───────────────────────────────────────────────── */

void* file_load_resfile(const char* filename)
{
	char name[FS_PATH_MAX];
	void* r;
	if (!filename) return NULL;

	snprintf(name, sizeof(name), "%s.res", filename);
	r = file_load_resource(1, name);
	if (r) return r;

	snprintf(name, sizeof(name), "%s.pre", filename);
	return file_load_resource(7, name);
}

void* file_load_resource(int type, const char* filename)
{
	static const char* const song_exts[]  = { "", ".kms", NULL };
	static const char* const voice_exts[] = { "", ".vce", NULL };
	static const char* const sfx_exts[]   = { "", ".sfx", NULL };
	static const char* const shape_exts[] = { "", ".p3s", NULL };
	static const char* const res_exts[]   = { "", ".res", NULL };

	if (!filename) return NULL;

	switch (type) {
	case 0: case 1:
		return file_load_binary_nofatal(filename);
	case 2: case 8: {
		void* s = file_load_shape2d_nofatal(filename);
		return s ? s : file_try_binary_with_exts(filename, shape_exts);
	}
	case 3: {
		void* r = file_load_shape2d_res_nofatal((char*)filename);
		return r ? r : file_try_binary_with_exts(filename, res_exts);
	}
	case 4:
		return file_try_binary_with_exts(filename, song_exts);
	case 5: {
		void* r = file_try_driver_prefixed_binary(filename, voice_exts);
		return r ? r : file_try_binary_with_exts(filename, voice_exts);
	}
	case 6: {
		void* r = file_try_driver_prefixed_binary(filename, sfx_exts);
		if (!r) {
			char gepath[FS_PATH_MAX];
			snprintf(gepath, sizeof(gepath), "ge%s", filename);
			r = file_try_binary_with_exts(gepath, sfx_exts);
		}
		return r ? r : file_try_binary_with_exts(filename, sfx_exts);
	}
	case 7: {
		void* r = file_decomp_nofatal(filename);
		return r ? r : file_load_binary_nofatal(filename);
	}
	default:
		return NULL;
	}
}

void unload_resource(void* resptr) { mmgr_release((char*)resptr); }

void file_load_audiores(const char* songfile, const char* voicefile, const char* name)
{
	void* audiores;
	voicefileptr = file_load_resource(5, voicefile);
	if (!voicefileptr) { is_audioloaded = 0; return; }

	songfileptr = file_load_resource(4, songfile);
	if (!songfileptr) { is_audioloaded = 0; return; }

	audiores = init_audio_resources(songfileptr, voicefileptr, name);
	if (!audiores) { is_audioloaded = 0; return; }

	load_audio_finalize(audiores);
	is_audioloaded = 1;
}

void* file_load_3dres(const char* filename)
{
	char name[FS_PATH_MAX];
	void* r;
	if (!filename) return NULL;

	snprintf(name, sizeof(name), "%s.p3s", filename);
	r = file_load_resource(7, name);
	if (r) return r;

	snprintf(name, sizeof(name), "%s.3sh", filename);
	return file_load_resource(1, name);
}

/* ── Replay I/O ─────────────────────────────────────────────────────── */

short file_load_replay(const char* dir, const char* name)
{
	file_build_path(dir, name, ".rpl", g_path_buf, sizeof(g_path_buf));
	g_is_busy = 1;
	file_read_fatal(g_path_buf, replay_header);
	gameconfig = *(struct GAMEINFO *)replay_header;
	g_is_busy = 0;
	return 0;
}

short file_write_replay(const char* filename)
{
	short ret;
	if (!filename) return -1;
	*(struct GAMEINFO *)replay_header = gameconfig;
	g_is_busy = 1;
	ret = file_write_fatal(filename, replay_header,
		(unsigned long)(26 + 901 + 901) + gameconfig.game_recordedframes);
	g_is_busy = 0;
	return ret;
}

/* ── DOS path parsing (legacy compat) ───────────────────────────────── */

void parse_filepath_separators_dosptr(const char* src_path, void* dst_path_buffer)
{
	const char* src = src_path;
	const char* base;
	char* dest = (char*)dst_path_buffer;
	int remaining = 12;
	if (!dest || !src) return;

	base = src;
	while (*src) {
		if (FS_IS_SEP(*src) || *src == ':') base = src + 1;
		src++;
	}
	src = base;

	while (*src && *src != '.' && remaining > 0) {
		unsigned char ch = (unsigned char)*src;
		if (ch == '/') ch = '\\';
		*dest++ = (char)toupper(ch);
		src++;
		remaining--;
	}
	*dest = '\0';
}
