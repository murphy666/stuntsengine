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

#ifndef SHAPE3D_H
#define SHAPE3D_H

#include <stdint.h>
#include "math.h"

#pragma pack (push, 1)

struct SHAPE3D {
	unsigned short shape3d_numverts;
	struct VECTOR * shape3d_verts;
	unsigned short shape3d_numprimitives;
	unsigned short shape3d_numpaints;
	char * shape3d_primitives;
	char * shape3d_cull1;
	char * shape3d_cull2;
};

struct SHAPE3DHEADER {
	unsigned char header_numverts;
	unsigned char header_numprimitives;
	unsigned char header_numpaints;
	unsigned char header_reserved;
};

struct TRANSFORMEDSHAPE3D {
	struct VECTOR pos;
	struct SHAPE3D* shapeptr;
	struct RECTANGLE* rectptr;
	struct VECTOR rotvec;
	unsigned short shape_visibility_threshold;
	unsigned char ts_flags;
	unsigned char material;
};

/* TRANSFORMEDSHAPE3D::ts_flags bits */
#define TRANSFORM_FLAG_FORCE_UNSORTED          1
#define TRANSFORM_FLAG_SKIP_VIEW_CULL          2
#define TRANSFORM_FLAG_UPDATE_RECT             8
#define TRANSFORM_FLAG_TERRAIN_DOUBLE_SIDED    32

/* Binary layout constants for raw SHAPE3D buffers. */
#define SHAPE3D_HEADER_SIZE_BYTES      4
#define SHAPE3D_VERTEX_SIZE_BYTES      6
#define SHAPE3D_CULL_ENTRY_SIZE_BYTES  4
#define SHAPE3D_PRIMITIVE_SIZE_BYTES   8

/**
 * @brief Parse a raw shape buffer and populate a SHAPE3D struct.
 */
static inline void shape3d_init_shape_pure(char* shapeptr, struct SHAPE3D* gameshape)
{
	struct SHAPE3DHEADER* hdr = (struct SHAPE3DHEADER*)shapeptr;

	gameshape->shape3d_numverts = hdr->header_numverts;
	gameshape->shape3d_numprimitives = hdr->header_numprimitives;
	gameshape->shape3d_numpaints = hdr->header_numpaints;

	gameshape->shape3d_verts =
		(struct VECTOR*)(shapeptr + SHAPE3D_HEADER_SIZE_BYTES);

	gameshape->shape3d_cull1 =
		shapeptr +
		hdr->header_numverts * SHAPE3D_VERTEX_SIZE_BYTES +
		SHAPE3D_HEADER_SIZE_BYTES;

	gameshape->shape3d_cull2 =
		shapeptr +
		hdr->header_numprimitives * SHAPE3D_CULL_ENTRY_SIZE_BYTES +
		hdr->header_numverts * SHAPE3D_VERTEX_SIZE_BYTES +
		SHAPE3D_HEADER_SIZE_BYTES;

	gameshape->shape3d_primitives =
		shapeptr +
		hdr->header_numprimitives * SHAPE3D_PRIMITIVE_SIZE_BYTES +
		hdr->header_numverts * SHAPE3D_VERTEX_SIZE_BYTES +
		SHAPE3D_HEADER_SIZE_BYTES;
}

/**
 * @brief Project a 3-D radius into screen pixels.
 */
static inline unsigned shape3d_project_radius(uint16_t proj_scale,
											  unsigned radius_3d,
											  int depth_z)
{
	unsigned long numer;

	if (depth_z <= 0) {
		return 0;
	}

	numer = (unsigned long)proj_scale * (unsigned long)radius_3d;
	return (unsigned)(numer / (unsigned long)depth_z);
}

#pragma pack (pop)

int shape3d_load_all(void);
void shape3d_free_all(void);
void shape3d_init_shape(char * shapeptr, struct SHAPE3D* gameshape);
unsigned shape3d_render_transformed(struct TRANSFORMEDSHAPE3D* transformed_shape);
void set_projection(int i1, int i2, int i3, int i4);
void set_projection_raw(unsigned short ang1, unsigned short ang2, unsigned short i3, unsigned short i4);
int polarAngle(int z, int y);
unsigned select_cliprect_rotate(int angZ, int angX, int angY, struct RECTANGLE* cliprect, int use_scaled_preview);
void init_polyinfo(void);
void polyinfo_reset(void);
void get_a_poly_info(void);
void shape3d_update_car_wheel_vertices(struct VECTOR * wheel_vertices, int wheel_stride, short* radius_table, short* offset_table, struct VECTOR* source_vectors, struct VECTOR* destination_vector);

#endif
