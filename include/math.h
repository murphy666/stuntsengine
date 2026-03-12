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

#ifndef MATH_H
#define MATH_H

#include <stdint.h>

struct RECTANGLE {
	int left, right;
	int top, bottom;
};

struct VECTOR {
	short x, y, z;
};

struct VECTORLONG {
	int32_t lx, ly, lz;
};

struct POINT2D {
	int px, py;
};

struct POINT2D_SIMD {
	int16_t px, py;
};

struct MATRIX {
	union {
		int16_t vals[9];
		struct {
			int16_t _11, _21, _31;
			int16_t _12, _22, _32;
			int16_t _13, _23, _33;
		} m;
	};
};

struct PLANE {
	short plane_yz;
	short plane_xy;
	struct VECTOR plane_origin;
	struct VECTOR plane_normal;
	struct MATRIX plane_rotation;
};

short sin_fast(unsigned short s);
short cos_fast(unsigned short s);

int polarAngle(int z, int y);
int polarRadius2D(int z, int y);
int polarRadius3D(struct VECTOR* vec);

unsigned rect_compare_point(struct POINT2D* pt);

void mat_mul_vector(struct VECTOR* invec, struct MATRIX* mat, struct VECTOR* outvec);
void mat_mul_vector2(struct VECTOR* invec, struct MATRIX * mat, struct VECTOR* outvec);
void plane_apply_rotation_matrix(void);
int plane_get_collision_point(int plane_index, int x, int y, int z);
int vec_normalInnerProduct(int x, int y, int z, struct VECTOR * normal);
void mat_multiply(struct MATRIX* rmat, struct MATRIX* lmat, struct MATRIX* outmat);
void mat_invert(struct MATRIX* inmat, struct MATRIX* outmat);
void mat_rot_x(struct MATRIX* outmat, int angle);
void mat_rot_y(struct MATRIX* outmat, int angle);
void mat_rot_z(struct MATRIX* outmat, int angle);
struct MATRIX* mat_rot_zxy(int z, int x, int y, int use_alt_mult_order);

void rect_adjust_from_point(struct POINT2D* pt, struct RECTANGLE* rc);

void rectlist_add_rects(char rect_count, char* rect_source_flags,
	struct RECTANGLE* rect_array_a, struct RECTANGLE* rect_array_b,
	struct RECTANGLE* clip_rect, char* rect_count_ptr, struct RECTANGLE* rect_array_ptr);
void rect_array_sort_by_top(char array_length, struct RECTANGLE* rect_array, short* array_indices);

int vector_direction_bucket32(struct VECTOR* vec);
void vector_to_point(struct VECTOR* vec, struct POINT2D* outpt);
void vector_lerp_at_z(struct VECTOR* vec1, struct VECTOR* vec2, struct VECTOR* outvec, short i);

short multiply_and_scale(short a1, short a2);

void rect_union(struct RECTANGLE* r1, struct RECTANGLE* r2, struct RECTANGLE* outrc);
int rect_intersect(struct RECTANGLE* r1, struct RECTANGLE* r2);

void plane_apply_rotation_matrix(void);
int plane_get_collision_point(int index, int b, int c, int d);

#endif