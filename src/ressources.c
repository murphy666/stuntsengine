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
#include <strings.h>
#include <stdint.h>
#include <ctype.h>
#include <dirent.h>
#include "ressources.h"

/* Audio / busy state — defined here, used by stuntsengine.c, highscore.c */
short is_audioloaded = 0;
char  g_is_busy      = 0;
#include "memmgr.h"
#include "shape2d.h"
#include "stunts.h"

static char g_find_query[260];
static char g_find_matches[256][260];
static int g_find_match_count;
static int g_find_match_index;
static int file_resolve_existing_path(const char* filename, char* out, size_t out_size);

static const char* const g_file_search_prefixes[] = {
	"",
	"ressources/",
	"build/",
	NULL
};

#define RS_RLE_ESCLEN_MAX    16
#define RS_RLE_ESCLOOKUP_LEN 256
#define RS_RLE_ESCSEQ_POS    1

#define RS_VLE_ESC_LEN   16
#define RS_VLE_ALPH_LEN  256
#define RS_VLE_ESC_WIDTH 64
#define RS_VLE_NUM_SYMB  128

/** @brief File read u24 le.
 * @param p Parameter `p`.
 * @return Function result.
 */
static uint32_t file_read_u24_le(const uint8_t* p)
{
	return (uint32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16));
}

/** @brief File get size resolved.
 * @param filename Parameter `filename`.
 * @param out_size Parameter `out_size`.
 * @return Function result.
 */
static int file_get_size_resolved(const char* filename, size_t* out_size)
{
	char resolved[260];
	FILE* file;
	long size;

	if (out_size == NULL) {
		return 0;
	}

	if (!file_resolve_existing_path(filename, resolved, sizeof(resolved))) {
		return 0;
	}

	file = fopen(resolved, "rb");
	if (file == NULL) {
		return 0;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return 0;
	}

	size = ftell(file);
	fclose(file);

	if (size <= 0) {
		return 0;
	}

	*out_size = (size_t)size;
	return 1;
}

/** @brief File decomp rle seq gnu.
 * @param src Parameter `src`.
 * @param src_len Parameter `src_len`.
 * @param esc Parameter `esc`.
 * @param dst Parameter `dst`.
 * @param dst_cap Parameter `dst_cap`.
 * @param out_len Parameter `out_len`.
 * @return Function result.
 */
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
			size_t seq_len;
			size_t rep_idx;

			while (src < src_end && *src != esc) {
				if (written >= dst_cap) {
					return 0;
				}
				dst[written++] = *src++;
			}

			if (src + 1 >= src_end) {
				return 0;
			}

			src++;
			rep = (uint8_t)(*src++ - 1);
			seq_end = src;

			if (seq_end < seq_start + 2) {
				return 0;
			}

			seq_len = (size_t)((seq_end - 2) - seq_start);
			for (rep_idx = 0; rep_idx < rep; rep_idx++) {
				if (written + seq_len > dst_cap) {
					return 0;
				}
				memcpy(&dst[written], seq_start, seq_len);
				written += seq_len;
			}
		} else {
			if (written >= dst_cap) {
				return 0;
			}
			dst[written++] = cur;
		}
	}

	*out_len = written;
	return 1;
}

/** @brief File decomp rle single gnu.
 * @param src Parameter `src`.
 * @param src_len Parameter `src_len`.
 * @param esclookup Parameter `esclookup`.
 * @param dst Parameter `dst`.
 * @param dst_len Parameter `dst_len`.
 * @return Function result.
 */
static int file_decomp_rle_single_gnu(const uint8_t* src, size_t src_len, const uint8_t* esclookup,
									  uint8_t* dst, size_t dst_len)
{
	size_t src_pos = 0;
	size_t dst_pos = 0;

	while (dst_pos < dst_len) {
		uint8_t cur;
		uint8_t escv;

		if (src_pos >= src_len) {
			return 0;
		}

		cur = src[src_pos++];
		escv = esclookup[cur];

		if (escv != 0) {
			uint32_t rep = 0;
			uint8_t value;

			if (escv == 1) {
				if (src_pos + 1 >= src_len) {
					return 0;
				}
				rep = src[src_pos++];
				value = src[src_pos++];
			} else if (escv == 3) {
				if (src_pos + 2 >= src_len) {
					return 0;
				}
				rep = (uint32_t)src[src_pos++];
				rep |= (uint32_t)src[src_pos++] << 8;
				value = src[src_pos++];
			} else {
				if (src_pos >= src_len) {
					return 0;
				}
				rep = (uint32_t)escv - 1u;
				value = src[src_pos++];
			}

			if (dst_pos + rep > dst_len) {
				return 0;
			}

			memset(&dst[dst_pos], value, rep);
			dst_pos += rep;
		} else {
			dst[dst_pos++] = cur;
		}
	}

	return 1;
}

/** @brief File decomp rle gnu.
 * @param src Parameter `src`.
 * @param src_len Parameter `src_len`.
 * @param out Parameter `out`.
 * @param out_len Parameter `out_len`.
 * @return Function result.
 */
static int file_decomp_rle_gnu(const uint8_t* src, size_t src_len, uint8_t** out, size_t* out_len)
{
	uint32_t len;
	uint32_t srclen;
	uint8_t esclen;
	uint8_t skipseq;
	uint8_t esclookup[RS_RLE_ESCLOOKUP_LEN];
	const uint8_t* body;
	size_t body_len;
	uint8_t* dst;
	int i;

	if (src_len < 10 || out == NULL || out_len == NULL) {
		return 0;
	}

	len = file_read_u24_le(&src[1]);
	srclen = file_read_u24_le(&src[4]);
	esclen = src[8];
	skipseq = (uint8_t)((esclen & 128u) != 0u);
	esclen &= 127u;

	if (esclen > RS_RLE_ESCLEN_MAX) {
		return 0;
	}

	if (src_len < (size_t)(9u + (size_t)esclen + (size_t)srclen)) {
		return 0;
	}

	memset(esclookup, 0, sizeof(esclookup));
	for (i = 0; i < (int)esclen; i++) {
		esclookup[src[9 + i]] = (uint8_t)(i + 1);
	}

	body = &src[9 + esclen];
	body_len = srclen;

	dst = (uint8_t*)malloc((size_t)len);
	if (dst == NULL) {
		return 0;
	}

	if (skipseq == 0u) {
		uint8_t* seqbuf = (uint8_t*)malloc((size_t)len);
		size_t seq_len = 0;
		int ok;
		if (seqbuf == NULL) {
			free(dst);
			return 0;
		}

		ok = file_decomp_rle_seq_gnu(body, body_len, src[9 + RS_RLE_ESCSEQ_POS], seqbuf, (size_t)len, &seq_len);
		if (!ok || !file_decomp_rle_single_gnu(seqbuf, seq_len, esclookup, dst, (size_t)len)) {
			free(seqbuf);
			free(dst);
			return 0;
		}
		free(seqbuf);
	} else {
		if (!file_decomp_rle_single_gnu(body, body_len, esclookup, dst, (size_t)len)) {
			free(dst);
			return 0;
		}
	}

	*out = dst;
	*out_len = (size_t)len;
	return 1;
}

/** @brief File decomp vle gnu.
 * @param src Parameter `src`.
 * @param src_len Parameter `src_len`.
 * @param out Parameter `out`.
 * @param out_len Parameter `out_len`.
 * @return Function result.
 */
static int file_decomp_vle_gnu(const uint8_t* src, size_t src_len, uint8_t** out, size_t* out_len)
{
	uint32_t len;
	uint32_t lenleft;
	uint8_t* dst;
	size_t dst_pos = 0;
	const uint8_t* p;
	const uint8_t* end;
	uint16_t esc1[RS_VLE_ESC_LEN], esc2[RS_VLE_ESC_LEN];
	uint8_t alph[RS_VLE_ALPH_LEN], symb[RS_VLE_ALPH_LEN], wdth[RS_VLE_ALPH_LEN];
	uint8_t esclen;
	int additive;
	unsigned int i, j, alphlen, width, widthdistr;
	uint8_t numsymb, symbwdth, numsymbleft;
	uint8_t curwdt, cursymb;
	uint16_t curword;

	if (src_len < 5 || out == NULL || out_len == NULL) {
		return 0;
	}

	len = file_read_u24_le(&src[1]);
	dst = (uint8_t*)malloc((size_t)len + 1u);
	if (dst == NULL) {
		return 0;
	}

	p = src + 4;
	end = src + src_len;
	if (p >= end) {
		free(dst);
		return 0;
	}

	esclen = *p++;
	additive = (esclen & 128u) != 0;
	esclen &= 127u;
	if (esclen == 0 || esclen > RS_VLE_ESC_LEN) {
		free(dst);
		return 0;
	}

	for (i = 0, j = 0, alphlen = 0; i < esclen; ++i, j *= 2) {
		uint8_t tmp;
		if (p >= end) {
			free(dst);
			return 0;
		}
		esc1[i] = (uint16_t)(alphlen - j);
		tmp = *p++;
		j += tmp;
		alphlen += tmp;
		esc2[i] = (uint16_t)j;
	}

	if ((size_t)(end - p) < alphlen) {
		free(dst);
		return 0;
	}
	memcpy(alph, p, alphlen);
	p += alphlen;

	{
		const uint8_t* wdtpos = src + 5;
		const uint8_t* wp = wdtpos;
		width = 1;
		widthdistr = (esclen >= 8 ? 8 : esclen);
		numsymb = RS_VLE_NUM_SYMB;
		for (i = 0, j = 0; width <= widthdistr; ++width, numsymb >>= 1) {
			if (wp >= end) {
				free(dst);
				return 0;
			}
			for (symbwdth = *wp++; symbwdth > 0; --symbwdth, ++j) {
				for (numsymbleft = numsymb; numsymbleft; --numsymbleft, ++i) {
					symb[i] = alph[j];
					wdth[i] = (uint8_t)width;
				}
			}
		}

		for (; i < RS_VLE_ALPH_LEN; ++i) {
			wdth[i] = RS_VLE_ESC_WIDTH;
		}
	}

	if ((size_t)(end - p) < 2) {
		free(dst);
		return 0;
	}
	curword = (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
	p += 2;
	curwdt = 8;
	cursymb = 0;

	lenleft = len + 1u;
	while (lenleft != 0) {
		uint8_t code = (uint8_t)(curword >> 8);
		uint8_t nextwdt = wdth[code];

		if (nextwdt > 8) {
			uint8_t codebyte = (uint8_t)curword;
			curword >>= 8;
			i = 7;

			for (;;) {
				if (curwdt == 0) {
					if (p >= end) {
						free(dst);
						return 0;
					}
					codebyte = *p++;
					curwdt = 8;
				}

				curword = (uint16_t)((curword << 1) + ((codebyte & 128u) != 0u));
				codebyte <<= 1;
				--curwdt;
				++i;

				if (curword < esc2[i]) {
					curword = (uint16_t)(curword + esc1[i]);
					cursymb = (uint8_t)(additive ? (uint8_t)(cursymb + alph[curword]) : alph[curword]);
					if (dst_pos <= len) {
						dst[dst_pos++] = cursymb;
					}
					--lenleft;
					break;
				}
			}

			if (p >= end) {
				free(dst);
				return 0;
			}
			curword = (uint16_t)((codebyte << curwdt) | *p++);
			nextwdt = (uint8_t)(8 - curwdt);
			curwdt = 8;
		} else {
			cursymb = (uint8_t)(additive ? (uint8_t)(cursymb + symb[code]) : symb[code]);
			if (dst_pos <= len) {
				dst[dst_pos++] = cursymb;
			}
			--lenleft;

			if (curwdt < nextwdt) {
				if (p >= end) {
					free(dst);
					return 0;
				}
				curword = (uint16_t)(curword << curwdt);
				nextwdt = (uint8_t)(nextwdt - curwdt);
				curwdt = 8;
				curword = (uint16_t)(curword | *p++);
			}
		}

		curword = (uint16_t)(curword << nextwdt);
		curwdt = (uint8_t)(curwdt - nextwdt);
	}

	if (dst_pos < len) {
		free(dst);
		return 0;
	}

	*out = dst;
	*out_len = (size_t)len;
	return 1;
}

/** @brief File try binary with exts.
 * @param filename Parameter `filename`.
 * @param exts Parameter `exts`.
 * @return Function result.
 */
static void* file_try_binary_with_exts(const char* filename, const char* const* exts)
{
	int index;
	if (filename == NULL || exts == NULL) {
		return NULL;
	}
	for (index = 0; exts[index] != NULL; index++) {
		char path[260];
		snprintf(path, sizeof(path), "%s%s", filename, exts[index]);
		{
			void* ptr = file_load_binary_nofatal(path);
			if (ptr != NULL) {
				return ptr;
			}
		}
	}
	return NULL;
}

/** @brief File try driver prefixed binary.
 * @param filename Parameter `filename`.
 * @param exts Parameter `exts`.
 * @return Function result.
 */
static void* file_try_driver_prefixed_binary(const char* filename, const char* const* exts)
{
	char prefix[3];
	char stem[260];
	void* ptr;

	if (filename == NULL || exts == NULL) {
		return NULL;
	}

	prefix[0] = (char)tolower((unsigned char)audiodriverstring[0]);
	prefix[1] = (char)tolower((unsigned char)audiodriverstring[1]);
	prefix[2] = '\0';

	if (prefix[0] == 's' && prefix[1] == 'b') {
		prefix[0] = 'a';
		prefix[1] = 'd';
	}

	if (prefix[0] == '\0' || prefix[1] == '\0') {
		return NULL;
	}

	snprintf(stem, sizeof(stem), "%s%s", prefix, filename);
	ptr = file_try_binary_with_exts(stem, exts);
	if (ptr != NULL) {
		return ptr;
	}

	return NULL;
}

/** @brief File resolve case insensitive.
 * @param input Parameter `input`.
 * @param out Parameter `out`.
 * @param out_size Parameter `out_size`.
 * @return Function result.
 */
static int file_resolve_case_insensitive(const char* input, char* out, size_t out_size)
{
	const char* slash;
	char dirpath[260];
	const char* basename;
	DIR* dir;
	struct dirent* entry;

	if (input == NULL || out == NULL || out_size == 0) {
		return 0;
	}

	slash = strrchr(input, '/');
	if (slash == NULL) {
		slash = strrchr(input, '\\');
	}

	if (slash != NULL) {
		size_t n = (size_t)(slash - input);
		if (n >= sizeof(dirpath)) {
			return 0;
		}
		memcpy(dirpath, input, n);
		dirpath[n] = '\0';
		basename = slash + 1;
	} else {
		strcpy(dirpath, ".");
		basename = input;
	}

	dir = opendir(dirpath);
	if (dir == NULL) {
		return 0;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strcasecmp(entry->d_name, basename) == 0) {
			if (strcmp(dirpath, ".") == 0) {
				snprintf(out, out_size, "%s", entry->d_name);
			} else {
				snprintf(out, out_size, "%s/%s", dirpath, entry->d_name);
			}
			closedir(dir);
			return 1;
		}
	}

	closedir(dir);
	return 0;
}

/** @brief File match pattern ci.
 * @param pattern Parameter `pattern`.
 * @param text Parameter `text`.
 * @return Function result.
 */
static int file_match_pattern_ci(const char* pattern, const char* text)
{
	const char* star = NULL;
	const char* star_text = NULL;

	if (pattern == NULL || text == NULL) {
		return 0;
	}

	while (*text != '\0') {
		if (*pattern == '*') {
			star = pattern++;
			star_text = text;
			continue;
		}

		if (*pattern == '?' || tolower((unsigned char)*pattern) == tolower((unsigned char)*text)) {
			pattern++;
			text++;
			continue;
		}

		if (star != NULL) {
			pattern = star + 1;
			text = ++star_text;
			continue;
		}

		return 0;
	}

	while (*pattern == '*') {
		pattern++;
	}

	return *pattern == '\0';
}

/** @brief File resolve wildcard path.
 * @param query Parameter `query`.
 * @param out Parameter `out`.
 * @param out_size Parameter `out_size`.
 * @return Function result.
 */
static int file_resolve_wildcard_path(const char* query, char* out, size_t out_size)
{
	char candidate[260];
	char resolved_path[260];
	char dirpath[260];
	const char* slash;
	const char* pattern;
	DIR* dir;
	struct dirent* entry;
	int i;
	int k;

	if (query == NULL || out == NULL || out_size == 0) {
		return 0;
	}

	g_find_match_count = 0;
	g_find_match_index = 0;

	for (i = 0; g_file_search_prefixes[i] != NULL; i++) {
		snprintf(candidate, sizeof(candidate), "%s%s", g_file_search_prefixes[i], query);

		slash = strrchr(candidate, '/');
		if (slash == NULL) {
			slash = strrchr(candidate, '\\');
		}

		if (slash != NULL) {
			size_t n = (size_t)(slash - candidate);
			if (n >= sizeof(dirpath)) {
				continue;
			}
			memcpy(dirpath, candidate, n);
			dirpath[n] = '\0';
			pattern = slash + 1;
		} else {
			strcpy(dirpath, ".");
			pattern = candidate;
		}

		dir = opendir(dirpath);
		if (dir == NULL) {
			continue;
		}

		while ((entry = readdir(dir)) != NULL) {
			if (!file_match_pattern_ci(pattern, entry->d_name)) {
				continue;
			}

			if (strcmp(dirpath, ".") == 0) {
				if (strlen(entry->d_name) >= sizeof(resolved_path)) {
					continue;
				}
				snprintf(resolved_path, sizeof(resolved_path), "%s", entry->d_name);
			} else {
				size_t dir_len = strlen(dirpath);
				size_t name_len = strlen(entry->d_name);
				if (dir_len + 1 + name_len >= sizeof(resolved_path)) {
					continue;
				}
				memcpy(resolved_path, dirpath, dir_len);
				resolved_path[dir_len] = '/';
				memcpy(resolved_path + dir_len + 1, entry->d_name, name_len);
				resolved_path[dir_len + 1 + name_len] = '\0';
			}

			for (k = 0; k < g_find_match_count; ++k) {
				if (strcasecmp(g_find_matches[k], resolved_path) == 0) {
					break;
				}
			}

			if (k == g_find_match_count && g_find_match_count < (int)(sizeof(g_find_matches) / sizeof(g_find_matches[0]))) {
				size_t match_len = strnlen(resolved_path, sizeof(g_find_matches[g_find_match_count]) - 1);
				memcpy(g_find_matches[g_find_match_count], resolved_path, match_len);
				g_find_matches[g_find_match_count][match_len] = '\0';
				g_find_match_count++;
			}
		}

		closedir(dir);
	}

	if (g_find_match_count > 0) {
		size_t out_len = strnlen(g_find_matches[0], out_size - 1);
		memcpy(out, g_find_matches[0], out_len);
		out[out_len] = '\0';
		return 1;
	}
	return 0;
}

/** @brief File resolve existing path.
 * @param filename Parameter `filename`.
 * @param out Parameter `out`.
 * @param out_size Parameter `out_size`.
 * @return Function result.
 */
static int file_resolve_existing_path(const char* filename, char* out, size_t out_size)
{
	FILE* file;
	char tmp[260];
	char upper[260];
	int i;

	if (filename == NULL || out == NULL || out_size == 0) {
		return 0;
	}

	for (i = 0; g_file_search_prefixes[i] != NULL; i++) {
		size_t n;
		size_t j;
		snprintf(tmp, sizeof(tmp), "%s%s", g_file_search_prefixes[i], filename);

		file = fopen(tmp, "rb");
		if (file != NULL) {
			fclose(file);
			snprintf(out, out_size, "%s", tmp);
			return 1;
		}

		/* Wine/MinGW fallback: try uppercased path directly. */
		n = strnlen(tmp, sizeof(upper) - 1);
		for (j = 0; j < n; j++) {
			upper[j] = (char)toupper((unsigned char)tmp[j]);
		}
		upper[n] = '\0';

		file = fopen(upper, "rb");
		if (file != NULL) {
			fclose(file);
			snprintf(out, out_size, "%s", upper);
			return 1;
		}

		if (file_resolve_case_insensitive(tmp, out, out_size)) {
			return 1;
		}
	}

	return 0;
}

/** @brief File find.
 * @param query Parameter `query`.
 * @return Function result.
 */
const char* file_find(const char* query)
{
	char resolved[260];

	if (query == NULL) {
		g_find_query[0] = '\0';
		g_find_match_count = 0;
		g_find_match_index = 0;
		return NULL;
	}

	if (strchr(query, '*') != NULL || strchr(query, '?') != NULL) {
		if (!file_resolve_wildcard_path(query, resolved, sizeof(resolved))) {
			g_find_query[0] = '\0';
			g_find_match_count = 0;
			g_find_match_index = 0;
			return NULL;
		}
	} else if (!file_resolve_existing_path(query, resolved, sizeof(resolved))) {
		g_find_query[0] = '\0';
		g_find_match_count = 0;
		g_find_match_index = 0;
		return NULL;
	} else {
		g_find_match_count = 0;
		g_find_match_index = 0;
	}

	strncpy(g_find_query, resolved, sizeof(g_find_query) - 1);
	g_find_query[sizeof(g_find_query) - 1] = '\0';
	return g_find_query;
}

/** @brief File find next.
 * @return Function result.
 */
const char* file_find_next(void)
{
	if (g_find_match_count <= 0) {
		return NULL;
	}

	g_find_match_index++;
	if (g_find_match_index >= g_find_match_count) {
		return NULL;
	}

	strncpy(g_find_query, g_find_matches[g_find_match_index], sizeof(g_find_query) - 1);
	g_find_query[sizeof(g_find_query) - 1] = '\0';
	return g_find_query;
}

/** @brief File find next alt.
 * @return Function result.
 */
const char* file_find_next_alt(void)
{
	return file_find_next();
}

/** @brief File build path.
 * @param dir Parameter `dir`.
 * @param name Parameter `name`.
 * @param ext Parameter `ext`.
 * @param dst Parameter `dst`.
 */
void file_build_path(const char* dir, const char* name, const char* ext, char* dst)
{
	if (dst == NULL) {
		return;
	}
	dst[0] = '\0';
	if (dir != NULL) {
		strcat(dst, dir);
		if (dst[0] != '\0') {
			size_t n = strlen(dst);
			char last = dst[n - 1];
			if (last != '/' && last != '\\' && last != ':') {
				strcat(dst, "/");
			}
		}
	}
	if (name != NULL) {
		strcat(dst, name);
	}
	if (ext != NULL) {
		strcat(dst, ext);
	}
}

/** @brief File combine and find.
 * @param dir Parameter `dir`.
 * @param name Parameter `name`.
 * @param ext Parameter `ext`.
 * @return Function result.
 */
const char* file_combine_and_find(const char* dir, const char* name, const char* ext)
{
	static char path[260];
	file_build_path(dir, name, ext, path);
	return file_find(path);
}

/** @brief File paras.
 * @param filename Parameter `filename`.
 * @param fatal Parameter `fatal`.
 * @return Function result.
 */
unsigned short file_paras(const char* filename, int fatal)
{
	char resolved[260];
	FILE* file;
	long size;

	if (!file_resolve_existing_path(filename, resolved, sizeof(resolved))) {
		if (fatal) {
			fprintf(stderr, "file_paras: cannot open %s\n", filename ? filename : "(null)");
		}
		return 0;
	}

	file = fopen(resolved, "rb");
	if (file == NULL) {
		if (fatal) {
			fprintf(stderr, "file_paras: cannot open %s\n", filename ? filename : "(null)");
		}
		return 0;
	}
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fclose(file);
	if (size <= 0) {
		return 0;
	}
	return (unsigned short)(((unsigned long)size + 15UL) >> 4);
}

unsigned short file_paras_fatal(const char* filename) { return file_paras(filename, 1); }
unsigned short file_paras_nofatal(const char* filename) { return file_paras(filename, 0); }

unsigned short file_decomp_paras(const char* filename, int fatal) { return file_paras(filename, fatal); }
unsigned short file_decomp_paras_fatal(const char* filename) { return file_paras(filename, 1); }
unsigned short file_decomp_paras_nofatal(const char* filename) { return file_paras(filename, 0); }

/** @brief File read.
 * @param filename Parameter `filename`.
 * @param dst Parameter `dst`.
 * @param fatal Parameter `fatal`.
 * @return Function result.
 */
void* file_read(const char* filename, void* dst, int fatal)
{
	char resolved[260];
	FILE* file;
	long size;

	if (!file_resolve_existing_path(filename, resolved, sizeof(resolved))) {
		if (fatal) {
			fprintf(stderr, "file_read: cannot open %s\n", filename ? filename : "(null)");
		}
		return NULL;
	}

	file = fopen(resolved, "rb");
	if (file == NULL) {
		if (fatal) {
			fprintf(stderr, "file_read: cannot open %s\n", filename ? filename : "(null)");
		}
		return NULL;
	}
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (size <= 0) {
		fclose(file);
		return NULL;
	}
	if (dst == NULL) {
		/* Round up to next 16-byte paragraph boundary and always add at least
		   16 extra zero bytes. This prevents off-by-one overreads in legacy font
		   bitmap loops when the file size is already an exact multiple of 16. */
		size_t alloc_size = ((size_t)size + 31u) & ~(size_t)15u;
		dst = malloc(alloc_size);
		if (dst != NULL) memset(dst, 0, alloc_size);
	}
	if (dst != NULL) {
		fread(dst, 1, (size_t)size, file);
	}
	fclose(file);
	return dst;
}

void* file_read_fatal(const char* filename, void* dst) { return file_read(filename, dst, 1); }
void* file_read_nofatal(const char* filename, void* dst) { return file_read(filename, dst, 0); }

/** @brief File write.
 * @param filename Parameter `filename`.
 * @param src Parameter `src`.
 * @param length Parameter `length`.
 * @param fatal Parameter `fatal`.
 * @return Function result.
 */
short file_write(const char* filename, void* src, unsigned long length, int fatal)
{
	FILE* file = fopen(filename, "wb");
	if (file == NULL) {
		if (fatal) {
			fprintf(stderr, "file_write: cannot open %s\n", filename ? filename : "(null)");
		}
		return -1;
	}
	if (src != NULL && length > 0) {
		fwrite(src, 1, (size_t)length, file);
	}
	fclose(file);
	return 0;
}

short file_write_fatal(const char* filename, void* src, unsigned long length) { return file_write(filename, src, length, 1); }
short file_write_nofatal(const char* filename, void* src, unsigned long length) { return file_write(filename, src, length, 0); }

/** @brief File decomp.
 * @param filename Parameter `filename`.
 * @param fatal Parameter `fatal`.
 * @return Function result.
 */
void* file_decomp(const char* filename, int fatal)
{
	void* cached;
	uint8_t* src_raw;
	uint8_t* src;
	size_t src_size;
	size_t src_padded_size;
	uint8_t* pass_src;
	size_t pass_src_len;
	uint8_t passes;
	uint8_t* current = NULL;
	size_t current_len = 0;
	size_t current_out_len = 0;
	uint8_t pass_index;
	void* result;

	if (filename == NULL) {
		return NULL;
	}

	cached = mmgr_get_chunk_by_name(filename);
	if (cached != NULL) {
		return cached;
	}

	if (!file_get_size_resolved(filename, &src_size)) {
		if (fatal) {
			fprintf(stderr, "file_decomp: cannot open %s\n", filename);
		}
		return NULL;
	}

	src_raw = (uint8_t*)file_read(filename, NULL, fatal);
	if (src_raw == NULL || src_size < 4) {
		free(src_raw);
		return NULL;
	}

	src_padded_size = ((src_size + 15u) & ~15u) + 16u;
	src = (uint8_t*)calloc(1, src_padded_size);
	if (src == NULL) {
		free(src_raw);
		return NULL;
	}
	memcpy(src, src_raw, src_size);
	free(src_raw);

	if ((src[0] & 128u) != 0u) {
		passes = (uint8_t)(src[0] & 127u);
		if (passes == 0 || passes > 8 || src_padded_size < 5 || (src[4] != 1 && src[4] != 2)) {
			free(src);
			if (fatal) {
				fprintf(stderr, "invalid packed data in %s\n", filename);
			}
			return NULL;
		}
		pass_src = src + 4;
		pass_src_len = src_padded_size - 4;
	} else {
		if (src[0] != 1 && src[0] != 2) {
			free(src);
			if (fatal) {
				fprintf(stderr, "invalid packed data in %s\n", filename);
			}
			return NULL;
		}
		passes = 1;
		pass_src = src;
		pass_src_len = src_padded_size;
	}

	if (passes == 0) {
		free(src);
		return NULL;
	}

	for (pass_index = 0; pass_index < passes; pass_index++) {
		uint8_t type;
		uint8_t* out_buf = NULL;
		size_t out_len = 0;
		int ok = 0;

		if (pass_src_len == 0) {
			free(current);
			free(src);
			return NULL;
		}

		type = pass_src[0];
		if (type == 1) {
			ok = file_decomp_rle_gnu(pass_src, pass_src_len, &out_buf, &out_len);
		} else if (type == 2) {
			ok = file_decomp_vle_gnu(pass_src, pass_src_len, &out_buf, &out_len);
		}

		if (!ok || out_buf == NULL || out_len == 0) {
			free(out_buf);
			free(current);
			free(src);
			if (fatal) {
				fprintf(stderr, "invalid packed data in %s\n", filename);
			}
			return NULL;
		}

		free(current);
		current_out_len = out_len;

		if (pass_index + 1u < passes) {
			size_t padded_len = ((out_len + 15u) & ~15u) + 16u;
			current = (uint8_t*)calloc(1, padded_len);
			if (current == NULL) {
				free(out_buf);
				free(src);
				return NULL;
			}
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
	if (result == NULL) {
		free(current);
		free(src);
		return NULL;
	}

	memcpy(result, current, current_out_len);
	free(current);
	free(src);
	return result;
}
void* file_decomp_fatal(const char* filename) { return file_decomp(filename, 1); }
void* file_decomp_nofatal(const char* filename) { return file_decomp(filename, 0); }

void* file_load_binary(const char* filename, int fatal) { return file_read(filename, NULL, fatal); }
void* file_load_binary_nofatal(const char* filename) { return file_read(filename, NULL, 0); }
void* file_load_binary_fatal(const char* filename) { return file_read(filename, NULL, 1); }

/** @brief File load resfile.
 * @param filename Parameter `filename`.
 * @return Function result.
 */
void* file_load_resfile(const char* filename)
{
	char name[260];
	void* result;

	if (filename == NULL) {
		return NULL;
	}

	snprintf(name, sizeof(name), "%s.res", filename);
	result = file_load_resource(1, name);
	if (result != NULL) {
		return result;
	}

	snprintf(name, sizeof(name), "%s.pre", filename);
	result = file_load_resource(7, name);
	if (result != NULL) {
		return result;
	}

	return NULL;
}

/** @brief File load resource.
 * @param type Parameter `type`.
 * @param filename Parameter `filename`.
 * @return Function result.
 */
void* file_load_resource(int type, const char* filename)
{
	static const char* const song_exts[] = { "", ".kms", NULL };
	static const char* const voice_exts[] = { "", ".vce", NULL };
	static const char* const sfx_exts[] = { "", ".sfx", NULL };

	if (filename == NULL) {
		return NULL;
	}

	switch (type) {
		case 0:
		case 1:
			{
				void* r = file_load_binary_nofatal(filename);
				return r;
			}
		case 2:
			#ifdef _WIN32
			{
				void* shape = file_load_shape2d_nofatal_thunk(filename);
				if (shape != NULL) {
					return shape;
				}
				return file_try_binary_with_exts(filename, shape_exts);
			}
			#endif
			{
				void* shape = file_load_shape2d_nofatal(filename);
				return shape;
			}
		case 3:
			#ifdef _WIN32
			{
				void* r = file_load_shape2d_res_nofatal((char*)filename);
				if (r != NULL) {
					return r;
				}
				return file_try_binary_with_exts(filename, shape_res_exts);
			}
			#endif
			{
				void* r = file_load_shape2d_res_nofatal_thunk(filename);
				return r;
			}
		case 4:
			#ifdef _WIN32
			return file_try_binary_with_exts(filename, song_exts);
			#endif
			return file_try_binary_with_exts(filename, song_exts);
		case 5:
			#ifdef _WIN32
			{
				void* r = file_try_driver_prefixed_binary(filename, voice_exts);
				if (r == NULL) {
					r = file_try_binary_with_exts(filename, voice_exts);
				}
				return r;
			}
			#endif
			{
				void* r = file_try_driver_prefixed_binary(filename, voice_exts);
				if (r == NULL) {
					r = file_try_binary_with_exts(filename, voice_exts);
				}
				return r;
			}
		case 6:
			#ifdef _WIN32
			{
				/* Try driver-specific prefix first (e.g., adeng.sfx for sb15) */
				void* r = file_try_driver_prefixed_binary(filename, sfx_exts);
				if (r == NULL) {
					/* Fallback to "ge" prefix (general engine, e.g., geeng.sfx) */
					char gepath[260];
					snprintf(gepath, sizeof(gepath), "ge%s", filename);
					r = file_try_binary_with_exts(gepath, sfx_exts);
				}
				if (r == NULL) {
					/* Last resort: try unprefixed */
					r = file_try_binary_with_exts(filename, sfx_exts);
				}
				return r;
			}
			#endif
			{
				/* Try driver-specific prefix first (e.g., adeng.sfx for sb15) */
				void* r = file_try_driver_prefixed_binary(filename, sfx_exts);
				if (r == NULL) {
					/* Fallback to "ge" prefix (general engine, e.g., geeng.sfx) */
					char gepath[260];
					snprintf(gepath, sizeof(gepath), "ge%s", filename);
					r = file_try_binary_with_exts(gepath, sfx_exts);
				}
				if (r == NULL) {
					/* Last resort: try unprefixed */
					r = file_try_binary_with_exts(filename, sfx_exts);
				}
				return r;
			}
		case 7:
			{
				void* r = file_decomp_nofatal(filename);
				if (r == NULL) {
					r = file_load_binary_nofatal(filename);
					return r;
				}
				return r;
			}
		case 8:
			#ifdef _WIN32
			{
				void* shape = file_load_shape2d_nofatal_thunk(filename);
				if (shape != NULL) {
					return shape;
				}
				return file_try_binary_with_exts(filename, shape_exts);
			}
			#endif
			{
				void* shape = file_load_shape2d_nofatal(filename);
				return shape;
			}
		default:
			break;
	}

	return NULL;
}
void unload_resource(void* resptr) { mmgr_release((char*)resptr); }
/** @brief File load audiores.
 * @param songfile Parameter `songfile`.
 * @param voicefile Parameter `voicefile`.
 * @param name Parameter `name`.
 */
void file_load_audiores(const char* songfile, const char* voicefile, const char* name)
{
	void* audiores;
	voicefileptr = file_load_resource(5, voicefile);
	if (voicefileptr == NULL) {
		is_audioloaded = 0;
		return;
	}

	songfileptr = file_load_resource(4, songfile);
	if (songfileptr == NULL) {
		is_audioloaded = 0;
		return;
	}

	audiores = init_audio_resources(songfileptr, voicefileptr, name);
	if (audiores == NULL) {
		is_audioloaded = 0;
		return;
	}

	load_audio_finalize(audiores);
	is_audioloaded = 1;
}
/** @brief File load 3dres.
 * @param filename Parameter `filename`.
 * @return Function result.
 */
void* file_load_3dres(const char* filename)
{
	char name[260];
	void* result;

	if (filename == NULL) {
		return NULL;
	}

	snprintf(name, sizeof(name), "%s.p3s", filename);
	result = file_load_resource(7, name);
	if (result != NULL) {
		return result;
	}

	snprintf(name, sizeof(name), "%s.3sh", filename);
	result = file_load_resource(1, name);
	if (result != NULL) {
		return result;
	}

	return NULL;
}

/** @brief File load replay.
 * @param dir Parameter `dir`.
 * @param name Parameter `name`.
 * @return Function result.
 */
short file_load_replay(const char* dir, const char* name)
{
	/* Match restunts reference: file_build_path + file_read_fatal into replay_header.
	   The RPL file layout is: 26-byte GAMEINFO header + 901-byte elem map +
	   901-byte terrain map + replay input bytes.
	   td13/td14/td15/td16 are contiguous in memory, so a single read fills
	   all four buffers correctly. */
	file_build_path(dir, name, ".rpl", g_path_buf);

	g_is_busy = 1;
	file_read_fatal(g_path_buf, replay_header);
	gameconfig = *(struct GAMEINFO *)replay_header;
	g_is_busy = 0;

	return 0;
}

/** @brief File write replay.
 * @param filename Parameter `filename`.
 * @return Function result.
 */
short file_write_replay(const char* filename)
{
	/* Match restunts reference: copy gameconfig back into replay_header,
	   then write the contiguous block (header + elem map + terrain map +
	   replay data) as a single write.  The original writes
	   sizeof(GAMEINFO) + game_recordedframes bytes from replay_header,
	   but that's only 26 + frames (missing the 1802 bytes of maps).
	   We write the full correct size: 26 + 901 + 901 + frames. */
	short ret;

	if (filename == NULL) {
		return -1;
	}

	*(struct GAMEINFO *)replay_header = gameconfig;

	g_is_busy = 1;
	ret = file_write_fatal(filename, replay_header,
		(unsigned long)(26 + 901 + 901) + gameconfig.game_recordedframes);
	g_is_busy = 0;

	return ret;
}

/** @brief Parse filepath separators dosptr.
 * @param src_path Parameter `src_path`.
 * @param dst_path_buffer Parameter `dst_path_buffer`.
 */
void parse_filepath_separators_dosptr(const char* src_path, void* dst_path_buffer)
{
	const char* src = src_path;
	const char* base;
	char* dest = (char*)dst_path_buffer;
	int remaining = 12;
	if (dest == NULL || src == NULL) {
		return;
	}

	/* Find last path separator (/ \ or :) */
	base = src;
	while (*src != '\0') {
		if (*src == '/' || *src == '\\' || *src == ':') {
			base = src + 1;
		}
		src++;
	}
	src = base;

	/* Copy characters, converting to uppercase, stopping at '.' or '\0'.
	   The original ASM (seg008:parse_filepath_separators) strips the
	   extension by stopping at the dot character. */
	while (*src != '\0' && *src != '.' && remaining > 0) {
		unsigned char ch = (unsigned char)*src;
		if (ch == '/') {
			ch = '\\';
		}
		*dest++ = (char)toupper(ch);
		src++;
		remaining--;
	}
	*dest = '\0';
}
