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

#include "stunts.h"
#include "math.h"
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Variables moved from data_game.c */
static struct MATRIX mat_rot_temp;
static struct MATRIX mat_x_rot;

/* Variables moved from data_game.c (private to this translation unit) */
static struct MATRIX mat_planetmp = { 0 };
static struct MATRIX mat_y_rot = { 0 };
static struct MATRIX mat_yaw_step = { 0 };
static struct MATRIX mat_z_rot = { 0 };


/* file-local data (moved from data_global.c) */
static unsigned short f36f40_whlData = 9999;
static unsigned short sine_lookup_cache_key = 9999;


/** @brief Data.
 * @param end Parameter `end`.
 * @return Function result.
 */
/* file-local data (moved from data_global.c) */

/** @brief Sift-down a node in a min-heap to restore heap order.
 *
 * @param heap   Key array used for ordering.
 * @param data   Satellite data array reordered alongside keys.
 * @param start  Index of the node to sift down.
 * @param end    Last valid index in the heap.
 */
static void heapify(int* heap, int* data, int start, int end)
{
	int last_parent = ((end + 1) >> 1);
	while (start < last_parent) {
		int left_child  = (start << 1) + 1;
		int right_child = left_child + 1;
		int smallest;

		if (right_child <= end) {
			smallest = (heap[left_child] >= heap[right_child]) ? right_child : left_child;
		} else {
			smallest = left_child;
		}

		if (heap[smallest] <= heap[start]) {
			int temp    = heap[start];
			heap[start] = heap[smallest];
			heap[smallest] = temp;

			temp         = data[start];
			data[start]  = data[smallest];
			data[smallest] = temp;
			start = smallest;
		} else {
			break;
		}
	}
}

/** @brief Sort parallel key/data arrays in ascending order via heapsort.
 *
 * @param n     Number of elements.
/** @brief Array.
 * @param data Parameter `data`.
 * @return Function result.
 */
void heapsort_by_order(int n, int* heap, int* data)
{
	int i;
	for (i = (n - 1) / 2; i >= 0; i--) {
		heapify(heap, data, i, n - 1);
	}

	for (i = n - 1; i > 0; i--) {
		int temp = heap[0];
		heap[0] = heap[i];
		heap[i] = temp;

		temp = data[0];
		data[0] = data[i];
		data[i] = temp;
		heapify(heap, data, 0, i - 1);
	}
}

static unsigned short mat_y_rot_angle = 65535;

/** @brief Load a 32-bit integer from an unaligned byte pointer.
 *
 * @param src  Source byte pointer.
 * @return The loaded 32-bit value.
 */
static int32_t load_i32(const unsigned char* src)
{
	int32_t value;
	memcpy(&value, src, sizeof(value));
	return value;
}

#define FIXED_POINT_SHIFT 14
#define FIXED_POINT_ONE 16384

#define ANGLE_BYTE_MASK 255
#define ANGLE_OCTANT 128
#define ANGLE_QUADRANT 256
#define ANGLE_HALF_TURN 512
#define ANGLE_FULL_TURN 1024
#define ANGLE_ROUNDING_THRESHOLD 128

#define DIRECTION_BUCKET_UP_NEGATIVE 30
#define DIRECTION_BUCKET_UP_POSITIVE 31
#define DIRECTION_BUCKET_Y_OFFSET 15

#define PROJECTED_CLAMP_MIN ((short)32000)
#define PROJECTED_CLAMP_MAX ((short)33536)
#define PROJECTED_INVALID ((short)32768)

#define SCALE_MUL_ROUND_MASK 32768
#define SCALE_MUL_SHIFT 16
#define SCALE_MUL_BIAS_SHIFT 15
#define NORMAL_DOT_DIVISOR 8192
#define SHAPE2D_LOW_NIBBLE_MASK 15

short sintab[] = {
	0, 101, 201, 302, 402, 503, 603, 704, 804, 904, 1005, 1105, 1205, 1306, 1406, 1506, 1606, 1706, 1806, 1906, 2006, 2105, 2205, 2305, 2404, 2503, 2603, 2702, 2801, 2900, 2999, 3098, 3196, 3295, 3393, 3492, 3590, 3688, 3786, 3883, 3981, 4078, 4176, 4273, 4370, 4467, 4563, 4660, 4756, 4852, 4948, 5044, 5139, 5235, 5330, 5425, 5520, 5614, 5708, 5803, 5897, 5990, 6084, 6177, 6270, 6363, 6455, 6547, 6639, 6731, 6823, 6914, 7005, 7096, 7186, 7276, 7366, 7456, 7545, 7635, 7723, 7812, 7900, 7988, 8076, 8163, 8250, 8337, 8423, 8509, 8595, 8680, 8765, 8850, 8935, 9019, 9102, 9186, 9269, 9352, 9434, 9516, 9598, 9679, 9760, 9841, 9921, 10001, 10080, 10159, 10238, 10316, 10394, 10471, 10549, 10625, 10702, 10778, 10853, 10928, 11003, 11077, 11151, 11224, 11297, 11370, 11442, 11514, 11585, 11656, 11727, 11797, 11866, 11935, 12004, 12072, 12140, 12207, 12274, 12340, 12406, 12472, 12537, 12601, 12665, 12729, 12792, 12854, 12916, 12978, 13039, 13100, 13160, 13219, 13279, 13337, 13395, 13453, 13510, 13567, 13623, 13678, 13733, 13788, 13842, 13896, 13949, 14001, 14053, 14104, 14155, 14206, 14256, 14305, 14354, 14402, 14449, 14497, 14543, 14589, 14635, 14680, 14724, 14768, 14811, 14854, 14896, 14937, 14978, 15019, 15059, 15098, 15137, 15175, 15213, 15250, 15286, 15322, 15357, 15392, 15426, 15460, 15493, 15525, 15557, 15588, 15619, 15649, 15679, 15707, 15736, 15763, 15791, 15817, 15843, 15868, 15893, 15917, 15941, 15964, 15986, 16008, 16029, 16049, 16069, 16088, 16107, 16125, 16143, 16160, 16176, 16192, 16207, 16221, 16235, 16248, 16261, 16273, 16284, 16295, 16305, 16315, 16324, 16332, 16340, 16347, 16353, 16359, 16364, 16369, 16373, 16376, 16379, 16381, 16383, 16384, 16384
};

static unsigned char atantable[258] = { 0, 1, 1, 2, 3, 3, 4, 4, 5, 6, 6, 7, 8, 8, 9, 10, 10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 16, 17, 18, 18, 19, 20, 20, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28, 28, 29, 30, 30, 31, 31, 32, 33, 33, 34, 34, 35, 36, 36, 37, 38, 38, 39, 39, 40, 41, 41, 42, 42, 43, 44, 44, 45, 45, 46, 46, 47, 48, 48, 49, 49, 50, 51, 51, 52, 52, 53, 53, 54, 55, 55, 56, 56, 57, 57, 58, 58, 59, 60, 60, 61, 61, 62, 62, 63, 63, 64, 65, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 73, 74, 74, 75, 75, 76, 76, 77, 77, 78, 78, 79, 79, 80, 80, 81, 81, 82, 82, 83, 83, 84, 84, 84, 85, 85, 86, 86, 87, 87, 88, 88, 89, 89, 90, 90, 91, 91, 91, 92, 92, 93, 93, 94, 94, 95, 95, 96, 96, 96, 97, 97, 98, 98, 99, 99, 99, 100, 100, 101, 101, 102, 102, 102, 103, 103, 104, 104, 104, 105, 105, 106, 106, 106, 107, 107, 108, 108, 108, 109, 109, 110, 110, 110, 111, 111, 112, 112, 112, 113, 113, 113, 114, 114, 115, 115, 115, 116, 116, 116, 117, 117, 118, 118, 118, 119, 119, 119, 120, 120, 120, 121, 121, 121, 122, 122, 122, 123, 123, 123, 124, 124, 124, 125, 125, 125, 126, 126, 126, 127, 127, 127, 128, 128, 0 };

/** @brief Fast sine lookup using the pre-computed table.
 *
/** @brief Unit.
 * @param s Parameter `s`.
 * @return Function result.
 */
short sin_fast(unsigned short s) {
	unsigned char c = s & ANGLE_BYTE_MASK;
	switch ((s >> 8) & 3) {
		case 0:
			return sintab[c];
		case 1:
			return sintab[ANGLE_QUADRANT - c];
		case 2:
			return -sintab[c];
		case 3:
			return -sintab[ANGLE_QUADRANT - c];
	}
	return 0;
}

/** @brief Lookup.
 * @param s Parameter `s`.
 * @return Function result.
 */
/** @brief Fast cosine lookup (delegates to sin_fast with a quarter-turn offset).
 *
 * @param s  Angle in the engine's fixed-point unit.
 * @return Cosine value in 14-bit fixed point.
 */
short cos_fast(unsigned short s) {
	return sin_fast(s + ANGLE_QUADRANT);
}

/** @brief Angle.
 * @param y Parameter `y`.
 * @return Function result.
 */
/** @brief Compute the polar angle (atan2-equivalent) of a 2-D vector.
 *
 * @param z  Horizontal component.
 * @param y  Vertical component.
 * @return Angle in the engine's fixed-point unit.
 */
int polarAngle(int z, int y) {
	
	unsigned flag;
	int temp, result = 0;
	unsigned long index;
	
	flag = 0;
	
	if (z < 0) {
		flag |= 4;
		z = -z;
	}
	
	if (y < 0) {
		flag |= 2;
		y = -y;
	}
	
	if (z == y) {
		if (z == 0) return result; // orig code has undefined return value here as well!
		result = ANGLE_OCTANT;
	} else {
		if (z > y) {
			temp = z;
			z = y;
			y = temp;
			flag |= 1;
		}
		index = (((unsigned long)z << 16) / y);
		if ((index & ANGLE_BYTE_MASK) >= ANGLE_ROUNDING_THRESHOLD) // round upwards
			index += ANGLE_QUADRANT;
		result = atantable[index >> 8];
	}
	
	switch (flag) {
		case 0:
			return result;
		case 1:
			return -result + ANGLE_QUADRANT;
		case 2:
			return -result + ANGLE_HALF_TURN;
		case 3:
			return result + ANGLE_QUADRANT;
		case 4:
			return -result;
		case 5:
			return result - ANGLE_QUADRANT;
		case 6:
			return result - ANGLE_HALF_TURN;
		case 7:
			return -(result + ANGLE_QUADRANT);
	}
	return result;
}

/** @brief Radius.
 * @param y Parameter `y`.
 * @return Function result.
 */
/** @brief Compute the 2-D polar radius (magnitude) of a vector.
 *
 * @param z  Horizontal component.
 * @param y  Vertical component.
 * @return Length of the vector as an integer.
 */
int polarRadius2D(int z, int y) {
	long result;
	
	result = polarAngle(z, y);
	
	if (result < 0) {
		result = -result;
	}
	
	if (result >= ANGLE_QUADRANT) {
		result = -(result - ANGLE_HALF_TURN);
	}
	
	if (result <= ANGLE_OCTANT) {
		result = cos_fast(result);
		if (y < 0)
			y = -y;
		return (((unsigned long)y) << FIXED_POINT_SHIFT) / result;
	} else {
		result = sin_fast(result);
		if (z < 0)
			z = -z;
		return (((unsigned long)z) << FIXED_POINT_SHIFT) / result;
	}
}

/** @brief Radius.
 * @param vec Parameter `vec`.
 * @return Function result.
 */
/** @brief Compute the 3-D polar radius (magnitude) of a vector.
 *
 * @param vec  Pointer to the input vector.
 * @return Length of the vector as an integer.
 */
int polarRadius3D(struct VECTOR* vec) {
	return polarRadius2D( polarRadius2D(vec->x, vec->y), vec->z );
}

/** @brief Rectangle.
 * @param pt Parameter `pt`.
 * @return Function result.
 */
/** @brief Classify a point relative to the selection rectangle (Cohen-Sutherland style).
 *
 * @param pt  Pointer to the point to test.
 * @return Bit-flags indicating which edges the point lies outside.
 */
unsigned rect_compare_point(struct POINT2D* pt) {
	unsigned char flag;
	if (pt->py < select_rect_rc.top)
		flag = 1;
	else if (pt->py > select_rect_rc.bottom)
		flag = 2; 
	else
		flag = 0;
	
	if (pt->px < select_rect_rc.left)
		flag |= 4;
	else if (pt->px > select_rect_rc.right)
		flag |= 8;
	return flag;
}

/** @brief Matrix.
 * @param outvec Parameter `outvec`.
 * @return Function result.
 */
/** @brief Multiply a vector by a matrix (outvec = mat * invec).
 *
 * Zero-element short-circuit avoids unnecessary multiplications.
 *
 * @param invec   Input vector.
 * @param mat     Transformation matrix.
 * @param outvec  Output vector receiving the result.
 */
void mat_mul_vector(struct VECTOR* invec, struct MATRIX* mat, struct VECTOR* outvec) {

	if (mat->m._11 != 0 && invec->x != 0)
		outvec->x = ((long)mat->m._11 * invec->x) >> FIXED_POINT_SHIFT;
	else
		outvec->x = 0;

	if (mat->m._12 != 0 && invec->y != 0)
		outvec->x += ((long)mat->m._12 * invec->y) >> FIXED_POINT_SHIFT;

	if (mat->m._13 != 0 && invec->z != 0)
		outvec->x += ((long)mat->m._13 * invec->z) >> FIXED_POINT_SHIFT;


	if (mat->m._21 != 0 && invec->x != 0)
		outvec->y = ((long)mat->m._21 * invec->x) >> FIXED_POINT_SHIFT;
	else
		outvec->y = 0;

	if (mat->m._22 != 0 && invec->y != 0)
		outvec->y += ((long)mat->m._22 * invec->y) >> FIXED_POINT_SHIFT;

	if (mat->m._23 != 0 && invec->z != 0)
		outvec->y += ((long)mat->m._23 * invec->z) >> FIXED_POINT_SHIFT;


	if (mat->m._31 != 0 && invec->x != 0)
		outvec->z = ((long)mat->m._31 * invec->x) >> FIXED_POINT_SHIFT;
	else
		outvec->z = 0;

	if (mat->m._32 != 0 && invec->y != 0)
		outvec->z += ((long)mat->m._32 * invec->y) >> FIXED_POINT_SHIFT;

	if (mat->m._33 != 0 && invec->z != 0)
		outvec->z += ((long)mat->m._33 * invec->z) >> FIXED_POINT_SHIFT;

}

/** @brief Matrix.
 * @param outvec Parameter `outvec`.
 * @return Function result.
 */
/** @brief Multiply a vector by a matrix (safe copy variant).
 *
 * Copies the matrix first so that mat and outvec may alias.
 *
 * @param invec   Input vector.
 * @param mat     Transformation matrix.
 * @param outvec  Output vector receiving the result.
 */
void mat_mul_vector2(struct VECTOR* invec, struct MATRIX * mat, struct VECTOR* outvec)
{
	struct MATRIX tmpmat = *mat;
	
	mat_mul_vector(invec, &tmpmat, outvec);
}

/** @brief Matrices.
 * @param outmat Parameter `outmat`.
 * @return Function result.
 */
/** @brief Multiply two 3x3 matrices (outmat = rmat * lmat).
 *
 * @param rmat    Right-hand matrix.
 * @param lmat    Left-hand matrix.
 * @param outmat  Output matrix receiving the product.
 */
void mat_multiply(struct MATRIX* rmat, struct MATRIX* lmat, struct MATRIX* outmat) {
	int counter;
	int16_t* rmatvals = rmat->vals;
	int16_t* lmatvals = lmat->vals;
	int16_t* outmatvals = outmat->vals;
	
	counter = 9;
	while (counter > 0) {
		if (rmatvals[0] != 0 && lmatvals[0] != 0)
			outmatvals[0] = ((long)rmatvals[0] * lmatvals[0]) >> FIXED_POINT_SHIFT; else
			outmatvals[0] = 0;

		if (rmatvals[1] != 0 && lmatvals[3] != 0)
			outmatvals[0] += ((long)rmatvals[1] * lmatvals[3]) >> FIXED_POINT_SHIFT;

		if (rmatvals[2] != 0 && lmatvals[6] != 0)
			outmatvals[0] += ((long)rmatvals[2] * lmatvals[6]) >> FIXED_POINT_SHIFT;
		
		outmatvals++;
		if (counter != 7 && counter != 4) {
			lmatvals++;
		} else {
			lmatvals -= 2;
			rmatvals += 3;
		}
		counter--;
	}
	
}

/** @brief Transpose.
 * @param outmat Parameter `outmat`.
 * @return Function result.
 */
/** @brief Transpose (invert) an orthonormal 3x3 matrix.
 *
 * Supports in-place operation when inmat == outmat.
 *
 * @param inmat   Input matrix.
 * @param outmat  Output matrix receiving the transpose.
 */
void mat_invert(struct MATRIX* inmat, struct MATRIX* outmat) {
	int temp;
	if (inmat == outmat) {
		temp = outmat->m._21;
		outmat->m._21 = outmat->m._12;
		outmat->m._12 = temp;

		temp = outmat->m._31;
		outmat->m._31 = outmat->m._13;
		outmat->m._13 = temp;

		temp = outmat->m._32;
		outmat->m._32 = outmat->m._23;
		outmat->m._23 = temp;
	} else {
		outmat->m._11 = inmat->m._11;
		outmat->m._12 = inmat->m._21;
		outmat->m._13 = inmat->m._31;

		outmat->m._21 = inmat->m._12;
		outmat->m._22 = inmat->m._22;
		outmat->m._23 = inmat->m._32;

		outmat->m._31 = inmat->m._13;
		outmat->m._32 = inmat->m._23;
		outmat->m._33 = inmat->m._33;
	}
}


/** @brief Build a rotation matrix around the X axis.
 *
 * @param outmat  Output matrix.
 * @param angle   Rotation angle in engine units.
 */
void mat_rot_x(struct MATRIX* outmat, int angle) {
	int c, s;
	
	c = cos_fast(angle);
	s = sin_fast(angle);
	outmat->m._11 = FIXED_POINT_ONE;
	outmat->m._21 = 0;
	outmat->m._31 = 0;
	outmat->m._12 = 0;
	outmat->m._22 = c;
	outmat->m._32 = s;
	outmat->m._13 = 0;
	outmat->m._23 = -s;
	outmat->m._33 = c;
}

/** @brief Build a rotation matrix around the Y axis.
 *
 * @param outmat  Output matrix.
 * @param angle   Rotation angle in engine units.
 */
void mat_rot_y(struct MATRIX* outmat, int angle) {
	int c, s;
	
	c = cos_fast(angle);
	s = sin_fast(angle);
	outmat->m._11 = c;
	outmat->m._21 = 0;
	outmat->m._31 = -s;
	outmat->m._12 = 0;
	outmat->m._22 = FIXED_POINT_ONE;
	outmat->m._32 = 0;
	outmat->m._13 = s;
	outmat->m._23 = 0;
	outmat->m._33 = c;
}

/** @brief Build a rotation matrix around the Z axis.
 *
 * @param outmat  Output matrix.
 * @param angle   Rotation angle in engine units.
 */
void mat_rot_z(struct MATRIX* outmat, int angle) {
	int c, s;
	
	c = cos_fast(angle);
	s = sin_fast(angle);
	outmat->m._11 = c;
	outmat->m._21 = s;
	outmat->m._31 = 0;
	outmat->m._12 = -s;
	outmat->m._22 = c;
	outmat->m._32 = 0;
	outmat->m._13 = 0;
	outmat->m._23 = 0;
	outmat->m._33 = FIXED_POINT_ONE;
}

// mat_rot_zxy was originally optimized, using pre-calced y-matrices and only 
// multiplying the non-zero axes. currently not optimized except for the y cache:

/** @brief Build a combined ZXY rotation matrix.
 *
 * @param z                   Z-axis rotation angle.
 * @param x                   X-axis rotation angle.
 * @param y                   Y-axis rotation angle.
 * @param use_alt_mult_order  Non-zero selects YXZ multiplication order.
 * @return Pointer to one of the static rotation matrices containing the result.
 */
struct MATRIX* mat_rot_zxy(int z, int x, int y, int use_alt_mult_order) {
	mat_rot_z(&mat_z_rot, z);
	mat_rot_x(&mat_x_rot, x);
	
	// y rotation matrix cache
	/*if (mat_y_rot_angle != y) {
		mat_rot_y(&mat_y_rot, y);
		mat_y_rot_angle = y;
	}*/
	mat_y_rot_angle = y; // dont forget this!!
	mat_rot_y(&mat_y_rot, y);
	
	if ((use_alt_mult_order & 1) != 0) {
		mat_multiply(&mat_y_rot, &mat_x_rot, &mat_rot_temp);
		mat_multiply(&mat_rot_temp, &mat_z_rot, &mat_x_rot);
		return &mat_x_rot;
	} else {
		mat_multiply(&mat_z_rot, &mat_x_rot, &mat_rot_temp);
		mat_multiply(&mat_rot_temp, &mat_y_rot, &mat_z_rot);
		return &mat_z_rot;
	}
}

/** @brief Expand a rectangle so that it includes a given point.
 *
 * @param pt  Point to include.
 * @param rc  Rectangle to adjust in place.
 */
void rect_adjust_from_point(struct POINT2D* pt, struct RECTANGLE* rc) {
	int temp;
	
	if (rc->left > pt->px) {
		rc->left = pt->px;
	}
	
	temp = pt->px + 1;
	if (rc->right < temp) {
		rc->right = temp;
	}
	
	if (rc->top > pt->py) {
		rc->top = pt->py;
	}
	
	temp = pt->py + 1;
	if (rc->bottom < temp) {
		rc->bottom = temp;
	}
}

/** @brief Union.
 * @param outrc Parameter `outrc`.
 * @return Function result.
 */
/** @brief Compute the union (bounding box) of two rectangles.
 *
 * @param r1     First rectangle.
 * @param r2     Second rectangle.
 * @param outrc  Output rectangle receiving the union.
 */
void rect_union(struct RECTANGLE* r1, struct RECTANGLE* r2, struct RECTANGLE* outrc) {
	if (r1->left <= r2->left) {
		outrc->left = r1->left;
	} else {
		outrc->left = r2->left;
	}

	if (r1->right >= r2->right) {
		outrc->right = r1->right;
	} else {
		outrc->right = r2->right;
	}
	
	if (r1->top <= r2->top) {
		outrc->top = r1->top;
	} else {
		outrc->top = r2->top;
	}

	if (r1->bottom >= r2->bottom) {
		outrc->bottom = r1->bottom;
	} else {
		outrc->bottom = r2->bottom;
	}
	
	if (video_flag2_is1 == 1) {
		return ;
	}

	fatal_error("rect_union: unexpected code path");
}

/** @brief Clip r1 to the intersection with r2.
 *
/** @brief Clip.
 * @param unchanged Parameter `unchanged`.
 * @param r2 Parameter `r2`.
 * @return Function result.
 */
int rect_intersect(struct RECTANGLE* r1, struct RECTANGLE* r2) {
	if (r1->right <= r1->left) return 1;
	if (r2->right <= r1->left) return 1;
	if (r1->right <= r2->left) return 1;
	if (r1->top >= r2->bottom) return 1;
	if (r1->bottom <= r2->top) return 1;
	
	if (r1->left < r2->left) {
		r1->left = r2->left;
	}
	
	if (r1->right > r2->right) {
		r1->right = r2->right;
	}
	
	if (r1->top < r2->top) {
		r1->top = r2->top;
	}
	
	if (r1->bottom > r2->bottom) {
		r1->bottom = r2->bottom;
	}
	return 0;
}

/** @brief Test whether r1 is entirely contained within r2.
 *
 * @param r1  Inner rectangle to test.
 * @param r2  Outer bounding rectangle.
 * @return 1 if r1 is inside r2, 0 otherwise.
 */
int rect_is_inside(struct RECTANGLE* r1, struct RECTANGLE* r2) {
	if (r1->right > r2->right) {
		return 0;
	}
	
	if (r1->left < r2->left) {
		return 0;
	}
	
	if (r1->top < r2->top) {
		return 0;
	}
	
	if (r1->bottom > r2->bottom) {
		return 0;
	}
	
	return 1;
}

/** @brief Test whether two rectangles overlap.
 *
 * @param r1  First rectangle.
 * @param r2  Second rectangle.
 * @return 1 if overlapping, 0 otherwise.
 */
int rect_is_overlapping(struct RECTANGLE* r1, struct RECTANGLE* r2) {
	if (r1->right <= r2->left) {
		return 0;
	}
	
	if (r2->right <= r1->left) {
		return 0;
	}
	
	if (r1->top >= r2->bottom) {
		return 0;
	}
	
	if (r1->bottom <= r2->top) {
		return 0;
	}
	
	return 1;
}

/** @brief Adjacent.
 * @param adjacent Parameter `adjacent`.
 * @param r2 Parameter `r2`.
 * @return Function result.
 */
/** @brief Test whether two rectangles are adjacent (share an edge).
 *
 * @param r1  First rectangle.
 * @param r2  Second rectangle.
 * @return 1 if adjacent, 0 otherwise.
 */
int rect_is_adjacent(struct RECTANGLE* r1, struct RECTANGLE* r2) {
	if (r1->bottom == r2->top || r1->top == r2->bottom) {
		if (r1->left == r2->left && r1->right == r2->right)
			return 1;
		return 0;
	}

	if (r1->right == r2->left || r2->right == r2->left) {
		if (r1->top == r2->top && r1->bottom == r2->bottom)
			return 1;
	}
	return 0;
}

/** @brief Insert a rectangle into a managed rect-list, merging overlaps.
 *
 * Recursively splits and merges until no overlapping or adjacent entries remain.
 *
 * @param rect_count_ptr  Pointer to the current list length.
 * @param rect_array_ptr         Array of rectangles forming the list.
 * @param rect                       Rectangle to insert.
 */
void rectlist_add_rect(char* rect_count_ptr, struct RECTANGLE* rect_array_ptr, struct RECTANGLE* rect) {	
	int i;
	struct RECTANGLE merged_rect;
	struct RECTANGLE top_rect;
	struct RECTANGLE bottom_rect;
	struct RECTANGLE* existing;
	int has_bottom_extra, has_top_extra, shift_idx;

	if (video_flag2_is1 != 1) {
		fatal_error("rectlist_add_rect: unexpected code path");
	}
	
	for (i = 0; i < *rect_count_ptr; i++) {
		existing = &rect_array_ptr[i];
		if (rect_is_overlapping(rect, existing) == 0)
			continue;
		if (rect_is_inside(rect, existing) != 0)
			return ;

		if (rect_is_inside(existing, rect) != 0) {
			shift_idx = i;

			while (((*rect_count_ptr) - 1) > shift_idx) {
				rect_array_ptr[shift_idx] = rect_array_ptr[shift_idx + 1];
				shift_idx++;
			}
			(*rect_count_ptr)--;
			continue;
		}

		merged_rect = *existing;
		if (existing->top >= rect->top) {
			if (rect->top < existing->top) {
				top_rect = *rect;
				top_rect.bottom = existing->top;
				has_top_extra = 1;
			} else {
				has_top_extra = 0;
			}
		} else {	
			top_rect = *existing;
			top_rect.bottom = rect->top;
			merged_rect.top = rect->top;
			has_top_extra = 1;
		}

		if (existing->bottom <= rect->bottom) {
			if (rect->bottom > existing->bottom) {
				bottom_rect = *rect;
				bottom_rect.top = existing->bottom;
				has_bottom_extra = 1;
			} else {
				has_bottom_extra = 0;
			}
		} else {
			bottom_rect = *existing;
			bottom_rect.top = rect->bottom;
			merged_rect.bottom = rect->bottom;
			has_bottom_extra = 1;
		}

		if (rect->left <= existing->left)
			merged_rect.left = rect->left;
		else
			merged_rect.left = existing->left;

		if (rect->right >= existing->right)
			merged_rect.right = rect->right;
		else
			merged_rect.right = existing->right;

		shift_idx = i;

		while (((*rect_count_ptr) - 1) > shift_idx) {
			rect_array_ptr[shift_idx] = rect_array_ptr[shift_idx + 1];
			shift_idx++;
		}
		(*rect_count_ptr)--;
		if (has_top_extra != 0) {
			rectlist_add_rect(rect_count_ptr, rect_array_ptr, &top_rect);
		}

		rectlist_add_rect(rect_count_ptr, rect_array_ptr, &merged_rect);
		if (has_bottom_extra != 0) {
			rectlist_add_rect(rect_count_ptr, rect_array_ptr, &bottom_rect);
			return ;
		}
		return ;
	}

	for (i = 0; i < *rect_count_ptr; i++) {
		existing = &rect_array_ptr[i];

		if (rect_is_adjacent(existing, rect) == 0) {
			continue;
		}
		if (existing->left <= rect->left)
			merged_rect.left = existing->left;
		else
			merged_rect.left = rect->left;

		if (existing->right >= rect->right)
			merged_rect.right = existing->right;
		else
			merged_rect.right = rect->right;

		if (existing->top <= rect->top)
			merged_rect.top = existing->top;
		else
			merged_rect.top = rect->top;
		
		if (existing->bottom >= rect->bottom)
			merged_rect.bottom = existing->bottom;
		else
			merged_rect.bottom = rect->bottom;

		shift_idx = i;

		while (((*rect_count_ptr) - 1) > shift_idx) {
			rect_array_ptr[shift_idx] = rect_array_ptr[shift_idx + 1];
			shift_idx++;
		}
		(*rect_count_ptr)--;
		rectlist_add_rect(rect_count_ptr, rect_array_ptr, &merged_rect);
		return ;
	}

	{
		unsigned char index = (unsigned char)(*rect_count_ptr);
		rect_array_ptr[index] = *rect;
		*rect_count_ptr = (char)(index + 1);
	}
}


/** @brief Add dirty rectangles from paired rect arrays into a rect-list.
 *
 * @param rect_count               Number of entries in the input arrays.
/** @brief Source.
 * @param rect_array_ptr Parameter `rect_array_ptr`.
 * @return Function result.
 */
void rectlist_add_rects(char rect_count, char* rect_source_flags, 
	struct RECTANGLE* rect_array_a, struct RECTANGLE* rect_array_b, 
	struct RECTANGLE* clip_rect, char* rect_count_ptr, struct RECTANGLE* rect_array_ptr) 
{
	struct RECTANGLE* rectptr_b;
	struct RECTANGLE* rectptr_a;
	struct RECTANGLE* best_rectptr;
	struct RECTANGLE clipped_rect;
	struct RECTANGLE union_rect;
	int has_valid_rect, i;
	int entry_flags;
/*
	return rect_clip_combined_(
		rect_count, rect_source_flags, rect_array_a, rect_array_b, clip_rect,
		rect_count_ptr, rect_array_ptr);
	*/
	for (i = 0; i < rect_count; i++) {

		entry_flags = rect_source_flags[i];
		if ((entry_flags & 1) != 0) {
			rectptr_a = &rect_array_a[i];
		}

		if ((entry_flags & 2) != 0) {
			rectptr_b = &rect_array_b[i];
		}

		if (((entry_flags & 1) == 0) || rectptr_a->right <= rectptr_a->left) {
			if (((entry_flags & 2) == 0) || rectptr_b->right <= rectptr_b->left) {
				has_valid_rect = 0;
			} else {
				best_rectptr = rectptr_b;
				has_valid_rect = 1;
			}
		} else if ((entry_flags & 2) == 0) {
			best_rectptr = rectptr_a;
			has_valid_rect = 1;
		} else if (rectptr_b->right <= rectptr_b->left) {
			best_rectptr = rectptr_a;
			has_valid_rect = 1;
		} else {
			rect_union(rectptr_a, rectptr_b, &union_rect);
			best_rectptr = &union_rect;
			has_valid_rect = 1;
		}

		if (has_valid_rect != 0) {
			clipped_rect = *best_rectptr;
			if (rect_intersect(&clipped_rect, clip_rect) == 0) {
				rectlist_add_rect(rect_count_ptr, rect_array_ptr, &clipped_rect);
			}
		}
	}

}

/** @brief Sort an array of rectangles by descending top coordinate via heapsort.
 *
 * @param array_length   Number of rectangles.
 * @param rect_array     Rectangle array.
 * @param array_indices  Output index array giving the sorted order.
 */
void rect_array_sort_by_top(char array_length, struct RECTANGLE* rect_array, short* array_indices) {
	int i;
	int intbuffer[256];
	int indexbuf[256];

	if (array_length > 1) {
		for (i = 0; i < array_length; i++) {
			intbuffer[i] = -rect_array[i].top;
			indexbuf[i] = i;
		}
		heapsort_by_order(array_length, intbuffer, indexbuf);
		for (i = 0; i < array_length; i++) {
			array_indices[i] = (short)indexbuf[i];
		}
	} else {
		array_indices[0] = 0;
	}
}


/** @brief Classify a 3-D vector into one of 32 discrete direction buckets.
 *
 * @param vec  Pointer to the input vector.
/** @brief Index.
 * @param vec Parameter `vec`.
 * @return Function result.
 */
int vector_direction_bucket32(struct VECTOR* vec) {
	long y;
	long temp;
	int flag;
	int result;
	long angle;
	int32_t sin80v;
	int32_t cos80v;
	
	y = abs(vec->y);
	
	temp = polarRadius2D(abs(vec->x), abs(vec->z));
	sin80v = load_i32(sin80);
	cos80v = load_i32(cos80);
	
	if (sin80v != cos80v) {
		//fatal_error("sin80 != cos80 - not observed");
		y = y * sin80v;
		temp = temp * cos80v;
	} 

	if (temp >= y) {
		flag = 0;
	} else {
		flag = 1;
	}
	
	if (vec->y < 0) {
		if (flag != 0) return DIRECTION_BUCKET_UP_NEGATIVE;
	} else
	if (vec->y > 0) {
		if (flag != 0) return DIRECTION_BUCKET_UP_POSITIVE;
	}

	if (vec->y > 0) {
		result = DIRECTION_BUCKET_Y_OFFSET;
	} else {
		result = 0;
	}
	
	angle = -polarAngle(vec->z, -vec->x);
	if (angle < 0) {
		angle += ANGLE_FULL_TURN;
	}
	
	result += (((angle << 4) - angle) >> 10);
	
	return result;
}


#define projectiondata5_raw projectiondata5
#define projectiondata8_raw projectiondata8
#define projectiondata9_raw projectiondata9
#define projectiondata10_raw projectiondata10

#define projectiondata5 (*(uint16_t*)(void*)projectiondata5_raw)
#define projectiondata8 (*(uint16_t*)(void*)projectiondata8_raw)
#define projectiondata9 (*(uint16_t*)(void*)projectiondata9_raw)
#define projectiondata10 (*(uint16_t*)(void*)projectiondata10_raw)


/** @brief Project a 3-D vector to a 2-D screen point using perspective projection.
 *
/** @brief Position.
 * @param outpt Parameter `outpt`.
 * @return Function result.
 */
void vector_to_point(struct VECTOR* vec, struct POINT2D* outpt) {
	unsigned short z;
	unsigned long proj;
	unsigned short ax;
	unsigned short bx;
	short value;
	int sum;

	if (vec->z <= 0) {
		outpt->px = PROJECTED_INVALID;
		outpt->py = PROJECTED_INVALID;
		return;
	}

	z = (unsigned short)vec->z;

	if (vec->x >= 0) {
		proj = (unsigned long)(unsigned short)vec->x * (unsigned long)(unsigned short)projectiondata9;
		ax = (unsigned short)proj;
		bx = (unsigned short)(proj >> 16);
		bx = (unsigned short)(bx << 1);
		if ((short)ax < 0) {
			bx++;
		}

		if (z <= bx) {
			outpt->px = PROJECTED_CLAMP_MIN;
		} else {
			value = (short)((unsigned short)(proj / z));
			sum = (int)value + (int)(short)projectiondata5;
			value = (short)sum;
			if (sum < -32768 || sum > 32767) {
				if (value >= 0) {
					outpt->px = PROJECTED_CLAMP_MAX;
				} else {
					outpt->px = PROJECTED_CLAMP_MIN;
				}
			} else {
				outpt->px = value;
			}
		}
	} else {
		proj = (unsigned long)(unsigned short)(-vec->x) * (unsigned long)(unsigned short)projectiondata9;
		ax = (unsigned short)proj;
		bx = (unsigned short)(proj >> 16);
		bx = (unsigned short)(bx << 1);
		if ((short)ax < 0) {
			bx++;
		}

		if (z <= bx) {
			outpt->px = PROJECTED_CLAMP_MAX;
		} else {
			value = (short)((unsigned short)(proj / z));
			value = (short)(-value);
			sum = (int)value + (int)(short)projectiondata5;
			value = (short)sum;
			if (sum < -32768 || sum > 32767) {
				if (value >= 0) {
					outpt->px = PROJECTED_CLAMP_MAX;
				} else {
					outpt->px = PROJECTED_CLAMP_MIN;
				}
			} else {
				outpt->px = value;
			}
		}
	}

	if (vec->y >= 0) {
		proj = (unsigned long)(unsigned short)vec->y * (unsigned long)(unsigned short)projectiondata10;
		ax = (unsigned short)proj;
		bx = (unsigned short)(proj >> 16);
		bx = (unsigned short)(bx << 1);
		if ((short)ax < 0) {
			bx++;
		}

		if (z <= bx) {
			outpt->py = PROJECTED_CLAMP_MAX;
		} else {
			value = (short)((unsigned short)(proj / z));
			value = (short)(-value);
			sum = (int)value + (int)(short)projectiondata8;
			value = (short)sum;
			if (sum < -32768 || sum > 32767) {
				if (value < 0) {
					outpt->py = PROJECTED_CLAMP_MIN;
				} else {
					outpt->py = PROJECTED_CLAMP_MAX;
				}
			} else {
				outpt->py = value;
			}
		}
	} else {
		proj = (unsigned long)(unsigned short)(-vec->y) * (unsigned long)(unsigned short)projectiondata10;
		ax = (unsigned short)proj;
		bx = (unsigned short)(proj >> 16);
		bx = (unsigned short)(bx << 1);
		if ((short)ax < 0) {
			bx++;
		}

		if (z <= bx) {
			outpt->py = PROJECTED_CLAMP_MIN;
		} else {
			value = (short)((unsigned short)(proj / z));
			sum = (int)value + (int)(short)projectiondata8;
			value = (short)sum;
			if (sum < -32768 || sum > 32767) {
				if (value < 0) {
					outpt->py = PROJECTED_CLAMP_MIN;
				} else {
					outpt->py = PROJECTED_CLAMP_MAX;
				}
			} else {
				outpt->py = value;
			}
		}
	}
}

/** @brief Linearly interpolate between two vectors at a given Z value.
 *
 * @param vec1    First endpoint.
 * @param vec2    Second endpoint.
 * @param outvec  Output vector at the interpolated position.
 * @param i       Target Z value for the interpolation.
 */
void vector_lerp_at_z(struct VECTOR* vec1, struct VECTOR* vec2, struct VECTOR* outvec, short i) {
	
	long z_offset, z_range;
	
	outvec->z = i;

	z_offset = outvec->z - vec2->z;
	z_range = vec1->z - vec2->z;
	if (z_range < 0) {
		z_offset = z_offset >> 1;
		z_range = z_range >> 1;
	}
	
	outvec->x = (vec1->x - vec2->x) * z_offset / z_range + vec2->x;
	outvec->y = (vec1->y - vec2->y) * z_offset / z_range + vec2->y;
}

/** @brief Multiply two 16-bit values and return a scaled 16-bit result.
 *
 * @param a1  First operand.
 * @param a2  Second operand.
 * @return Scaled product with rounding.
 */
short multiply_and_scale(short a1, short a2)
{
	long mul = (long)a1 * (long)a2 * 4L;
	return (mul >> SCALE_MUL_SHIFT) + ((mul & SCALE_MUL_ROUND_MASK) >> SCALE_MUL_BIAS_SHIFT);
}


/** @brief Of.
 * @param x Parameter `x`.
 * @param y Parameter `y`.
 * @param normal Parameter `normal`.
 * @param normal Parameter `normal`.
 * @return Function result.
 */
/** @brief Compute the dot product of (x,y,z) with a plane normal, scaled down.
 *
 * @param x       X component.
 * @param y       Y component.
 * @param z       Z component.
 * @param normal  Pointer to the normal vector.
 * @return Scaled dot product.
 */
int vec_normalInnerProduct(int x, int y, int z, struct VECTOR * normal) {
	return (
		((long)normal->x * x) + 
		((long)normal->z * z) +
		((long)normal->y * y)) / NORMAL_DOT_DIVISOR;
}

/** @brief Compute the signed distance from a point to a terrain plane.
 *
 * @param plane_index  Index of the plane in the global plane table.
 * @param x              World X coordinate.
 * @param y              World Y coordinate.
 * @param z              World Z coordinate.
/** @brief Distance.
 * @param z Parameter `z`.
 * @return Function result.
 */
int plane_get_collision_point(int plane_index, int x, int y, int z) {
	struct PLANE * curplane;
	struct VECTOR a;
	struct VECTOR b;
	
	if (plane_index == planindex) {
		curplane = current_planptr;
	} else {
		curplane = &planptr[plane_index];
	}

	b.y = curplane->plane_origin.y + terrainHeight;
	a.y = y - b.y;
	if (plane_index < 4) {
		return a.y;
	}
	b.x = curplane->plane_origin.x + elem_xCenter;
	b.z = curplane->plane_origin.z + elem_zCenter;
	a.x = x - b.x;
	a.z = z - b.z;
	{
		int _result = vec_normalInnerProduct(a.x, a.y, a.z, &curplane->plane_normal);
		return _result;
	}
}


/** @brief Apply the current plane rotation matrix to the physics vector.
 *
 * Transforms vec_movement_local through the appropriate rotation chain and
 * stores the result in vec_planerotopresult.
 */
void plane_apply_rotation_matrix(void) {
    struct PLANE *plane;
    struct VECTOR rotated_vec;
    struct MATRIX inv_rotation;
    struct MATRIX plane_rot_copy;
    struct VECTOR transformed_vec;
	int si;

	if (planindex_copy != -1) {
		plane = &planptr[planindex_copy];
		if (plane->plane_xy == pState_minusRotate_x_2 &&
			plane->plane_yz == pState_minusRotate_z_2) {
			si = pState_minusRotate_y_2;
		} else {
			mat_mul_vector(&vec_movement_local, &mat_car_orientation, &transformed_vec);
			plane_rot_copy = plane->plane_rotation;
			mat_invert(&plane_rot_copy, &inv_rotation);
			mat_mul_vector(&transformed_vec, &inv_rotation, &rotated_vec);
			si = polarAngle(-rotated_vec.x, rotated_vec.z);
		}

		si += pState_f36Mminf40sar2;
		if (si == 0) {
			mat_mul_vector2(&vec_movement_local, &plane->plane_rotation, &vec_planerotopresult);
			return;
		}

		if (sine_lookup_cache_key != si) {
			mat_rot_y(&mat_planetmp, -si);
			sine_lookup_cache_key = si;
		}

		mat_mul_vector(&vec_movement_local, &mat_planetmp, &rotated_vec);
		mat_mul_vector2(&rotated_vec, &plane->plane_rotation, &vec_planerotopresult);
		return;
	}

	if (pState_f36Mminf40sar2 == 0) {
		mat_mul_vector(&vec_movement_local, &mat_car_orientation, &vec_planerotopresult);
		return;
	}

	if (pState_f36Mminf40sar2 != f36f40_whlData) {
		mat_rot_y(&mat_yaw_step, -pState_f36Mminf40sar2);
		f36f40_whlData = pState_f36Mminf40sar2;
	}

	mat_mul_vector(&vec_movement_local, &mat_yaw_step, &rotated_vec);
	mat_mul_vector(&rotated_vec, &mat_car_orientation, &vec_planerotopresult);
}

/**
/** @brief Parse shape2d helper.
 * @param low Parameter `low`.
 * @param high Parameter `high`.
 * @return Function result.
 */
unsigned long parse_shape2d_helper(unsigned int low, unsigned int high) {
	unsigned long result;
	result = ((unsigned long)high << 4) + low;
	return result;
}

/**
/** @brief Parse shape2d helper2.
 * @param low Parameter `low`.
 * @param high Parameter `high`.
 * @return Function result.
 */
unsigned long parse_shape2d_helper2(unsigned int low, unsigned int high) {
	unsigned long combined;
	unsigned int shifted;
	unsigned int remainder;
	
	combined = ((unsigned long)high << 16) | low;
	shifted = (unsigned int)(combined >> 4);
	remainder = low & SHAPE2D_LOW_NIBBLE_MASK;
	
	/* Return DX in high word, AX in low word */
	return ((unsigned long)shifted << 16) | remainder;
}

/**
/** @brief Equal.
 * @param stub Parameter `stub`.
 * @param compare_ds_ss Parameter `compare_ds_ss`.
 * @return Function result.
 */
int  compare_ds_ss(void) {
	return 1;
}

/**
 * @brief Divide a 32-bit unsigned value by a 16-bit divisor.
 *
 * @param low   Low 16 bits of the dividend.
 * @param high   High 16 bits of the dividend.
 * @param divisor  Divisor.
 * @return Quotient as a 16-bit unsigned value.
 */
unsigned int  div_uint32(unsigned int low, unsigned int high, unsigned int divisor) {
	unsigned long combined = ((unsigned long)high << 16) | low;
	return (unsigned int)(combined / divisor);
}

/**
 * @brief Multiply a value by projectiondata9.
 *
 * @param arg  Operand.
 * @return 32-bit product.
 */
unsigned long  mul_by_proj_data9(unsigned int arg) {
	return (unsigned long)projectiondata9 * arg;
}

/**
 * @brief Multiply a value by projectiondata10.
 *
 * @param arg  Operand.
 * @return 32-bit product.
 */
unsigned long  mul_by_proj_data10(unsigned int arg) {
	return (unsigned long)projectiondata10 * arg;
}

/**
 * @brief Multiply by projectiondata10 then divide.
 *
 * @param multiplicand  Multiplicand.
 * @param divisor  Divisor.
/** @brief Mul div proj data10.
 * @param multiplicand Parameter `multiplicand`.
 * @param divisor Parameter `divisor`.
 * @return Function result.
 */
unsigned int  mul_div_proj_data10(unsigned int multiplicand, unsigned int divisor) {
	unsigned long product = (unsigned long)projectiondata10 * multiplicand;
	return (unsigned int)(product / divisor);
}
