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

/*
 * track.c - Track menu, setup, and raw decode
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "stunts.h"
#include "font.h"
#include "memmgr.h"
#include "shape2d.h"
#include "ressources.h"
#include "ui_widgets.h"

/* Variables moved from data_game.c */
static unsigned char game_security_flag = 0;
static unsigned char sprite_render_state = 0;

/* Variables moved from data_game.c (private to this translation unit) */
static unsigned short boundary_check_limits = 1;
static unsigned char lap_checkpoint_counter = 0;
static unsigned short obstacle_collision_table = 12;
static unsigned char * pboxshape = 0;
static unsigned short segment_height_lookup = 9;
static unsigned short track_curve_data = 11;


/* file-local data (moved from data_global.c) */
static unsigned char animation_state_advance_table[6] = { 0, 0, 1, 0, 1, 0 };
static unsigned char animation_frame_transition_table[6] = { 0, 1, 0, 0, 1, 0 };
static unsigned char terrConnDataEtoW[20] = { 0, 0, 0, 0, 0, 0, 1, 2, 1, 3, 0, 2, 3, 0, 0, 1, 1, 3, 2, 0 };
static unsigned char terrConnDataWtoE[20] = { 0, 0, 0, 0, 0, 0, 1, 2, 0, 3, 1, 0, 0, 3, 2, 2, 3, 1, 1, 0 };
static unsigned char terrConnDataNtoS[20] = { 0, 0, 0, 0, 0, 0, 1, 1, 5, 0, 4, 5, 0, 0, 4, 1, 5, 4, 1, 0 };
static unsigned char terrConnDataStoN[19] = { 0, 0, 0, 0, 0, 0, 1, 0, 5, 1, 4, 0, 5, 4, 0, 5, 1, 1, 4 };

/*
 * trkmenu.c - Track Menu Functions
 *
 * This module contains track editor menu functions ported from seg009.asm.
 *
 * Functions ported:
 *   track_cleanup_multitile_markers - Track map multi-tile cleanup
 *   track_validate_elements_for_terrain - Track element terrain validation
 *   (More functions to be added)
 */

/*
 * Legacy globals in dsegrep use byte arrays sized for 16-bit pointer width.
 * On 64-bit builds, writing void* entries into those arrays corrupts memory.
 * Keep track-menu shape pointer tables local with correct element counts.
 */
static char * tracksmenushape2dunk_local[186];
static char * tracksmenushape2dunk2_local[186];
static char * tracksmenushapes1_local[19];
static char * tracksmenushapes2_local[4];
static char * tracksmenushapes3_local[4];

#define tracksmenushape2dunk tracksmenushape2dunk_local
#define tracksmenushape2dunk2 tracksmenushape2dunk2_local
#define tracksmenushapes1 tracksmenushapes1_local
#define tracksmenushapes2 tracksmenushapes2_local
#define tracksmenushapes3 tracksmenushapes3_local

/* Track grid size is 30x30 */
#define TRACK_SIZE 30

enum {
	TRACK_TILE_COUNT = TRACK_SIZE * TRACK_SIZE,
	TRACK_LAST_INDEX = TRACK_SIZE - 1,
	TRACK_MARKER_CORNER = 253,
	TRACK_MARKER_VERTICAL = 254,
	TRACK_MARKER_HORIZONTAL = 255,
	TRACK_U8_INVALID = 255,
	TRACK_U16_INVALID = 65535,
	TRACK_PAGE_STRIDE = 36,
	TRACK_PICKER_COLS = 6,
	TRACK_PICKER_ROWS = 6,
	TRACK_TILE_PIXELS = 16,
	TRACK_MAP_TILE_OFFSET_X = 8,
	TRACK_MAP_TILE_OFFSET_Y = 4,
	TRACK_MAP_VISIBLE_COLS = 12,
	TRACK_MAP_VISIBLE_ROWS = 11,
	TRACK_VIEWPORT_MAX_X = 319,
	TRACK_VIEWPORT_MAX_Y = 199,
	TRACK_MENU_X = 220,
	TRACK_MENU_Y = 36,
	TRACK_EDITOR_WINDOW_W = 320,
	TRACK_EDITOR_WINDOW_H = 200,
	TRACK_DIALOG_AUTO_POS = UI_DIALOG_AUTO_POS,
	TRACK_TERRAIN_FLAT_MIN = 1,
	TRACK_TERRAIN_FLAT_MAX = 5,
	TRACK_TERRAIN_WATER = 6,
	TRACK_TERRAIN_HILL_MIN = 7,
	TRACK_TERRAIN_HILL_MAX = 10,
	TRACK_VALIDATE_ERR_FLAT = 12,
	TRACK_VALIDATE_ERR_HILL = 13,
	TRACK_VALIDATE_ERR_OTHER = 14,
	TRACK_ELEM_ROAD_A_MIN = 34,
	TRACK_ELEM_ROAD_A_MAX = 35,
	TRACK_ELEM_ROAD_B_MIN = 103,
	TRACK_ELEM_ROAD_B_MAX = 108,
	TRACK_ELEM_ROAD_C_MIN = 171,
	TRACK_ELEM_ROAD_C_MAX = 174,
	TRACK_ELEM_MAX_VALID = 182,
	TRACK_DIRTY_FLAG_COUNT = 132,
	TRACK_CURSOR_SHAPE_COUNT = 4,
	TRACK_ELEMENT_ICON_COUNT = 186,
	TRACK_MENU_PBOX_ROWS = 10,
	TRACK_MENU_SPECIAL_ROW_CAT = 6,
	TRACK_MENU_SPECIAL_ROW_SCENERY = 7,
	TRACK_MENU_SPECIAL_ROW_LOADSAVE = 8,
	TRACK_TRACKDATA_ELEM_BYTES = 900,
	TRACK_TRACKDATA_TOTAL_BYTES = 901,
	TRACK_FILE_SAVE_BYTES = 1802,
	TRACK_CURSOR_BLINK_START = 99,
	TRACK_CURSOR_BLINK_THRESHOLD = 15,
	TRACK_SETUP_TCOMP_BYTES = 896,
	TRACK_SETUP_DEPTH_MAX = 64,
	TRACK_CONN_CODE_SENTINEL = 99,
	TRACK_CHECKPOINT_LIMIT_MAX = 48,
	TRACK_HILL_HEIGHT_OFFSET = 450,
	TRACK_DIALOG_MOUSE_MASK = 3,
	TRACK_DIALOG_RELEASE_GUARD_MAX = 6000
};

/** @brief Track consume dialog click.
 * @return Function result.
 */
static void track_consume_dialog_click(void)
{
	unsigned short buttons;
	unsigned short mouse_x;
	unsigned short mouse_y;
	unsigned short guard = 0;

	check_input();
	mousebutinputcode = 0;

	do {
		(void)input_checking(1);
		mouse_get_state(&buttons, &mouse_x, &mouse_y);
		if (++guard >= TRACK_DIALOG_RELEASE_GUARD_MAX) {
			break;
		}
	} while ((buttons & TRACK_DIALOG_MOUSE_MASK) != 0);

	mousebutinputcode = 0;
}

/* trkObjectList is raw packed 14-byte records; access fields via offset */

enum { TRACKOBJECT_RAW_SIZE = 14, TRACKOBJECT_LIST_COUNT = 215, SCENESHAPES2_COUNT = 19, SCENESHAPES3_COUNT = 13 };

/** @brief Trkobj multi flag legacy.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
static unsigned char trkobj_multi_flag_legacy(unsigned elem)
{
	if (elem < TRACKOBJECT_LIST_COUNT) {
		return trkObjectList[elem * TRACKOBJECT_RAW_SIZE + 11u];
	}
	elem -= TRACKOBJECT_LIST_COUNT;
	if (elem < SCENESHAPES2_COUNT) {
		return sceneshapes2[elem * TRACKOBJECT_RAW_SIZE + 11u];
	}
	elem -= SCENESHAPES2_COUNT;
	if (elem < SCENESHAPES3_COUNT) {
		return sceneshapes3[elem * TRACKOBJECT_RAW_SIZE + 11u];
	}
	return 0;
}

#define TRKOBJ_MULTI_TILE_FLAG(elem) trkobj_multi_flag_legacy((unsigned)(elem))

/** @brief Track row ofs safe.
 * @param row Parameter `row`.
 * @return Function result.
 */
static int track_row_ofs_safe(int row)
{
	if (row < 0) {
		row = 0;
	} else if (row >= TRACK_SIZE) {
		row = TRACK_SIZE - 1;
	}
	return trackrows[row];
}


/** @brief Terrain row ofs safe.
 * @param row Parameter `row`.
 * @return Function result.
 */
static int terrain_row_ofs_safe(int row)
{
	if (row < 0) {
		row = 0;
	} else if (row >= TRACK_SIZE) {
		row = TRACK_SIZE - 1;
	}
	return terrainrows[row];
}


/** @brief Terrain at safe.
 * @param row Parameter `row`.
 * @param col Parameter `col`.
 * @return Function result.
 */
static unsigned char terrain_at_safe(int row, int col)
{
	if (row < 0) {
		row = 0;
	} else if (row >= TRACK_SIZE) {
		row = TRACK_SIZE - 1;
	}

	if (col < 0) {
		col = 0;
	} else if (col >= TRACK_SIZE) {
		col = TRACK_SIZE - 1;
	}

	return track_terrain_map[terrainrows[row] + col];
}

/*--------------------------------------------------------------
 * track_cleanup_multitile_markers - Track Map Multi-Tile Cleanup
 *
 * This function processes the track map to handle multi-tile
/** @brief Direction.
 * @param g Parameter `g`.
 * @param track_cleanup_multitile_markers Parameter `track_cleanup_multitile_markers`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
void track_cleanup_multitile_markers(void)
{
	unsigned char flags_384[TRACK_TILE_COUNT];	/* 30x30 flag buffer - var_384/var_383 */
	unsigned char row, col;			/* Loop indices - var_38C, var_388 */
	unsigned char elem_id;			/* Track element ID - var_38A */
	int i;
	int row_ofs, row_ofs_next;
	unsigned char multiflag;

	/* Initialize flag buffer to zero */
	for (i = 0; i < TRACK_TILE_COUNT; i++) {
		flags_384[i] = 0;
	}

/** @brief Order.
 * @param row Parameter `row`.
 * @return Function result.
 */
	/* Process rows in forward order (0→29), matching original behavior. */
	for (row = 0; row < TRACK_SIZE; row++) {
		row_ofs = trackrows[row];
		row_ofs_next = track_row_ofs_safe((int)row + 1);

		/* Process each column */
		for (col = 0; col < TRACK_SIZE; col++) {
			/* Get element at this position */
			elem_id = track_elem_map[row_ofs + col];

			/* Skip empty tiles */
			if (elem_id == 0) {
				continue;
			}

/** @brief Tiles.
 * @param TRACK_MARKER_CORNER Parameter `TRACK_MARKER_CORNER`.
 * @return Function result.
 */
			/* Skip special marker tiles (253, 254, 255) - they are secondary tiles */
			if (elem_id >= TRACK_MARKER_CORNER) {
				/* If not already flagged, clear it */
				if (flags_384[row_ofs + col] == 0) {
					track_elem_map[row_ofs + col] = 0;
				}
				continue;
			}

			/* Get multi-tile flag for this element type */
			multiflag = TRKOBJ_MULTI_TILE_FLAG(elem_id);

			if (multiflag == 1) {
				/* Type 1: secondary marker must be in next row */
				if (flags_384[row_ofs_next + col] != 0) {
					/* Clear the current row position */
					track_elem_map[row_ofs + col] = 0;
					continue;
				}

				/* Check if next row has 254 marker */
				if (track_elem_map[row_ofs_next + col] == TRACK_MARKER_VERTICAL) {
					/* Mark this position as processed */
					flags_384[row_ofs_next + col] = 1;
				} else {
					/* Clear in the current row array */
					track_elem_map[row_ofs + col] = 0;
				}
			}
			else if (multiflag == 2) {
				/* Type 2: Check next column flag */
				if (flags_384[row_ofs + col + 1] != 0) {
					track_elem_map[row_ofs + col] = 0;
					continue;
				}

				/* Check if next column has 255 marker */
				if (track_elem_map[row_ofs + col + 1] != TRACK_MARKER_HORIZONTAL) {
					track_elem_map[row_ofs + col] = 0;
				} else {
					/* Mark the next column position */
					flags_384[row_ofs + col + 1] = 1;
				}
			}
			else if (multiflag == 3) {
				/* Type 3: Corner piece - check multiple conditions */
				int sum = (int)flags_384[row_ofs_next + col + 1] +
				          (int)flags_384[row_ofs + col + 1] +
				          (int)flags_384[row_ofs_next + col];

				if (sum != 0) {
					/* Already processed - clear */
					track_elem_map[row_ofs + col] = 0;
					continue;
				}

				/* Check for valid multi-tile pattern:
				 * - Next col has 255 marker
				 * - Next row has 254 marker
				 * - Next row + next col has 253 marker
				 */
				if (track_elem_map[row_ofs + col + 1] == TRACK_MARKER_HORIZONTAL &&
				    track_elem_map[row_ofs_next + col] == TRACK_MARKER_VERTICAL &&
				    track_elem_map[row_ofs_next + col + 1] == TRACK_MARKER_CORNER) {
					/* Valid corner - mark all three secondary positions */
					flags_384[row_ofs + col + 1] = 1;
					flags_384[row_ofs_next + col] = 1;
					flags_384[row_ofs_next + col + 1] = 1;
				} else {
					/* Invalid - clear */
					track_elem_map[row_ofs + col] = 0;
				}
			}
			/* multiflag == 0 or other values: do nothing special */
		}
	}
}

/*--------------------------------------------------------------
 * track_validate_elements_for_terrain - Track Element Terrain Validation
 *
 * This function validates track elements against terrain types.
 * It iterates through the 30x30 track grid and:
 * 1. Calls track_cleanup_multitile_markers first to cleanup multi-tile markers
 * 2. For each cell, checks if the element is valid for the terrain
/** @brief Markers.
 * @param left Parameter `left`.
 * @param above Parameter `above`.
 * @param made Parameter `made`.
 * @param changes Parameter `changes`.
 * @param track_validate_elements_for_terrain Parameter `track_validate_elements_for_terrain`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
int track_validate_elements_for_terrain(void)
{
	unsigned char validation_result;	/* result/error code */
	unsigned char terrain_type;	/* terrain type */
	unsigned char row;	/* row counter */
	unsigned char element_type;	/* element type */
	unsigned char col;	/* column counter */
	int row_ofs;
	int row_ofs_prev;
	unsigned int elem_tmp;

	/* First call track_cleanup_multitile_markers to cleanup multi-tile markers */
	track_cleanup_multitile_markers();

	validation_result = 0;

	/* Process each row */
	for (row = 0; row < TRACK_SIZE; row++) {
		row_ofs = trackrows[row];
		row_ofs_prev = track_row_ofs_safe((int)row - 1);

		/* Process each column */
		for (col = 0; col < TRACK_SIZE; col++) {
			/* Get terrain type at this position */
			terrain_type = track_terrain_map[terrainrows[row] + col];

			/* Get element at this position */
			element_type = track_elem_map[row_ofs + col];

			/* Skip if element is 0 */
			if (element_type == 0) {
				continue;
			}

			/* Skip if terrain is 0 */
			if (terrain_type == 0) {
				continue;
			}

			/* Skip if terrain is 6 (water) */
			if (terrain_type == TRACK_TERRAIN_WATER) {
				continue;
			}

			/* Handle based on terrain type */
			if (terrain_type >= TRACK_TERRAIN_FLAT_MIN && terrain_type <= TRACK_TERRAIN_FLAT_MAX) {
				/* Terrain types 1-5: paved/dirt/ice roads */

				/* Resolve continuation markers */
				if (element_type == TRACK_MARKER_HORIZONTAL) {
					if (col == 0) {
						track_elem_map[row_ofs + col] = 0;
						validation_result = TRACK_VALIDATE_ERR_FLAT;
						continue;
					}
					/* 255: read element to the LEFT (col-1) */
					element_type = track_elem_map[row_ofs + col - 1];
				}
				else if (element_type == TRACK_MARKER_VERTICAL) {
					/* 254: read element ABOVE (row-1, same col) */
					element_type = track_elem_map[row_ofs_prev + col];
				}
				else if (element_type == TRACK_MARKER_CORNER) {
					if (col == 0) {
						track_elem_map[row_ofs + col] = 0;
						validation_result = TRACK_VALIDATE_ERR_FLAT;
						continue;
					}
					/* 253: read element ABOVE-LEFT (row-1, col-1) */
					element_type = track_elem_map[row_ofs_prev + col - 1];
				}

				/* Check if element is in valid ranges */
				elem_tmp = (unsigned int)element_type;

				if ((elem_tmp >= TRACK_ELEM_ROAD_A_MIN && elem_tmp <= TRACK_ELEM_ROAD_A_MAX) ||
				    (elem_tmp >= TRACK_ELEM_ROAD_B_MIN && elem_tmp <= TRACK_ELEM_ROAD_B_MAX) ||
				    (elem_tmp >= TRACK_ELEM_ROAD_C_MIN && elem_tmp <= TRACK_ELEM_ROAD_C_MAX)) {
					/* Valid element - do nothing */
				} else {
					/* Invalid element - clear it */
					track_elem_map[row_ofs + col] = 0;
					validation_result = TRACK_VALIDATE_ERR_FLAT;
				}
			}
			else if (terrain_type >= TRACK_TERRAIN_HILL_MIN && terrain_type <= TRACK_TERRAIN_HILL_MAX) {
				/* Terrain types 7-10: hills */

				/* Call subst_hillroad_track to check validity */
				if (subst_hillroad_track(terrain_type, element_type) == 0) {
					/* Invalid for this hill terrain - clear it */
					track_elem_map[row_ofs + col] = 0;
					validation_result = TRACK_VALIDATE_ERR_HILL;
				}
			}
			else {
				/* Other terrain types - element not allowed */
				track_elem_map[row_ofs + col] = 0;
				validation_result = TRACK_VALIDATE_ERR_OTHER;
			}
		}
	}

	/* If any changes were made, run cleanup again */
	if (validation_result != 0) {
		track_cleanup_multitile_markers();
	}

	return (int)(signed char)validation_result;
}

/*--------------------------------------------------------------
 * preRender_icons - Pre-render track editor icons
 *
 * This function renders the track element icons in the track editor
/** @brief Grid.
 * @param elements Parameter `elements`.
 * @param icon_page Parameter `icon_page`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
void preRender_icons(unsigned char icon_page)
{
	unsigned char element_id;	/* element ID */
	unsigned char col;	/* column counter */
	unsigned char row;	/* row counter */
	unsigned int x, y;

	/* Pages 1+: clear picker grid once to base terrain to avoid marker-cell overdraw
	 * erasing multi-tile (e.g. 2x2) icon quadrants drawn from top-left anchors. */
	if (icon_page != 0) {
		for (row = 0; row < TRACK_PICKER_ROWS; row++) {
			for (col = 0; col < TRACK_PICKER_COLS; col++) {
				x = (unsigned int)col * TRACK_TILE_PIXELS + TRACK_MENU_X;
				y = (unsigned int)row * TRACK_TILE_PIXELS + TRACK_MENU_Y;
				sprite_shape_to_1((struct SHAPE2D *)tracksmenushapes1[0], x, y);
			}
		}
	}

	/* Iterate through 6x6 grid */
	for (row = 0; row < TRACK_PICKER_ROWS; row++) {
		for (col = 0; col < TRACK_PICKER_COLS; col++) {
			/* Look up element from pboxshape */
			/* pboxshape[icon_page * 36 + row * 6 + col] */
			element_id = pboxshape[(unsigned int)icon_page * TRACK_PAGE_STRIDE + (unsigned int)row * TRACK_PICKER_COLS + col];

			/* Calculate screen position */
			/* x = col * 16 + 220 */
			/* y = row * 16 + 36 */
			x = (unsigned int)col * TRACK_TILE_PIXELS + TRACK_MENU_X;
			y = (unsigned int)row * TRACK_TILE_PIXELS + TRACK_MENU_Y;

			if (icon_page != 0) {
				/* Pages 1+: full icon rendering */

				/* Skip overlays for special markers */
				if (element_id >= TRACK_MARKER_CORNER) {
					continue;
				}

				/* Render icon mask (surroundings) */
				putpixel_iconMask(tracksmenushape2dunk2[element_id], x, y);

				/* Render icon filling */
				putpixel_iconFillings(tracksmenushape2dunk[element_id], x, y);
			}
			else {
				/* Page 0: simpler rendering */
				sprite_shape_to_1((struct SHAPE2D *)tracksmenushapes1[element_id], x, y);
			}
		}
	}
}

/*--------------------------------------------------------------
 * draw_2DtrackMap - Draw 2D track map in track editor
 *
 * This function renders the 2D track map overview in the track editor.
 * It processes an 11x12 grid of tiles centered at the given position.
 *
 * Parameters:
 *   col_offset - Column offset for map lookup
 *   row_offset - Row offset for map lookup
 *   render_flags_1 - Pointer to render flag buffer 1
 *   render_flags_2 - Pointer to render flag buffer 2
 *
 * The function:
 * 1. Iterates through 11 rows x 12 columns
 * 2. For each cell, looks up element and terrain from maps
/** @brief Markers.
 * @param render_flags_2 Parameter `render_flags_2`.
 * @return Function result.
 */
 /*--------------------------------------------------------------*/
void draw_2DtrackMap(unsigned char col_offset, unsigned char row_offset,
				   unsigned char * render_flags_1, unsigned char * render_flags_2)
{
	signed char row;		/* row counter (0-10) */
	signed char col;		/* column counter (0-11) */
	unsigned char terrain_type;	/* terrain type */
	unsigned char element_type;	/* element type */
	int buffer_row_ofs;		/* row * 12 for buffer index */
	int buffer_index;		/* cell index in buffer */
	int map_row_twice;		/* row offset calculation */
	int si, di;				/* temp values for coords */
	int terrIdx, elemIdx;
	int map_row, map_col;
	unsigned char terrVal, elemVal;
	unsigned char multiflag;
	int screenX, screenY;
	unsigned char max_col_ofs;
	unsigned char max_row_ofs;

	max_col_ofs = (TRACK_SIZE > TRACK_MAP_VISIBLE_COLS) ? (TRACK_SIZE - TRACK_MAP_VISIBLE_COLS) : 0;
	max_row_ofs = (TRACK_SIZE > TRACK_MAP_VISIBLE_ROWS) ? (TRACK_SIZE - TRACK_MAP_VISIBLE_ROWS) : 0;
	if (col_offset > max_col_ofs) {
		col_offset = max_col_ofs;
	}
	if (row_offset > max_row_ofs) {
		row_offset = max_row_ofs;
	}

	/* Iterate through 11 rows */
	for (row = 0; row < TRACK_MAP_VISIBLE_ROWS; row++) {
		/* buffer_row_ofs = row * 12 (buffer row offset) */
		buffer_row_ofs = (int)row * TRACK_MAP_VISIBLE_COLS;

		/* Iterate through 12 columns */
		for (col = 0; col < TRACK_MAP_VISIBLE_COLS; col++) {
			/* Calculate indices */
			si = (int)col;
			di = (int)col_offset;
			map_row_twice = ((int)row + (int)row_offset) * 2;
			map_row = (int)row + (int)row_offset;
			map_col = (int)col + (int)col_offset;

			/* Get element from track_elem_map */
			elemIdx = track_row_ofs_safe(map_row_twice / 2) + si + di;
			element_type = track_elem_map[elemIdx];

			/* Get terrain from track_terrain_map */
			terrIdx = terrain_row_ofs_safe(map_row_twice / 2) + si + di;
			terrain_type = track_terrain_map[terrIdx];

			/* Calculate buffer position */
			buffer_index = buffer_row_ofs + si;

/** @brief Markers.
 * @param TRACK_MARKER_CORNER Parameter `TRACK_MARKER_CORNER`.
 * @return Function result.
 */
			/* Check for special markers (multi-tile) */
			if (element_type >= TRACK_MARKER_CORNER) {
				/* Interior marker tiles are continuation cells; do not render
				 * them as regular elements. Just mark buffers like DOS code. */
				if (row != 0 && col != 0) {
					render_flags_1[buffer_index] = TRACK_U8_INVALID;
					render_flags_2[buffer_index] = TRACK_U8_INVALID;
					continue;
				}

				/* Mark buffer position */
				render_flags_1[buffer_index] = TRACK_U8_INVALID;

/** @brief Marker.
 * @param TRACK_MARKER_HORIZONTAL Parameter `TRACK_MARKER_HORIZONTAL`.
 * @return Function result.
 */
				/* Handle 255 marker (horizontal extension) */
				if (element_type == TRACK_MARKER_HORIZONTAL) {
					int elemSrc;

					if (col != 0) {
						render_flags_2[buffer_index] = TRACK_U8_INVALID;
						continue;
					}

					/* Terrain at current cell and one cell below. */
					screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
					screenX = si * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
					terrVal = terrain_at_safe((int)row + (int)row_offset, si + (int)col_offset);
					sprite_putimage_and_alt(tracksmenushapes1[terrVal], screenX, screenY);
					if (map_row + 1 < TRACK_SIZE) {
						screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y + TRACK_TILE_PIXELS;
						terrVal = terrain_at_safe((int)row + (int)row_offset + 1, si + (int)col_offset);
						sprite_putimage_and_alt(tracksmenushapes1[terrVal], screenX, screenY);
					}

					/* Source element is current row, previous column. */
					elemSrc = track_row_ofs_safe((int)row + (int)row_offset) + si + (int)col_offset - 1;
					if (elemSrc >= 0 && elemSrc < TRACK_TILE_COUNT) {
						elemVal = track_elem_map[elemSrc];
						screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
						screenX = si * TRACK_TILE_PIXELS - TRACK_MAP_TILE_OFFSET_X;
						sprite_putimage_and((struct SHAPE2D *)tracksmenushape2dunk2[elemVal], screenX, screenY);
						sprite_putimage_or((struct SHAPE2D *)tracksmenushape2dunk[elemVal], screenX, screenY);
					}
					continue;
				}

/** @brief Marker.
 * @param TRACK_MARKER_VERTICAL Parameter `TRACK_MARKER_VERTICAL`.
 * @return Function result.
 */
				/* Handle 254 marker (vertical extension) */
				if (element_type == TRACK_MARKER_VERTICAL) {
					int elemSrc;

					if (row != 0) {
						render_flags_2[buffer_index] = TRACK_U8_INVALID;
						continue;
					}

					/* Terrain at current cell and one cell to the right. */
					screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
					screenX = si * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
					terrVal = terrain_at_safe((int)row + (int)row_offset, si + (int)col_offset);
					sprite_putimage_and_alt(tracksmenushapes1[terrVal], screenX, screenY);
					if (map_col + 1 < TRACK_SIZE) {
						screenX = si * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X + TRACK_TILE_PIXELS;
						terrVal = terrain_at_safe((int)row + (int)row_offset, si + (int)col_offset + 1);
						sprite_putimage_and_alt(tracksmenushapes1[terrVal], screenX, screenY);
					}

					/* Source element is previous row, same column. */
					elemSrc = track_row_ofs_safe((int)row + (int)row_offset - 1) + si + (int)col_offset;
					if (elemSrc >= 0 && elemSrc < TRACK_TILE_COUNT) {
						elemVal = track_elem_map[elemSrc];
						screenY = (int)row * TRACK_TILE_PIXELS - (TRACK_TILE_PIXELS - TRACK_MAP_TILE_OFFSET_Y);
						screenX = si * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
						sprite_putimage_and((struct SHAPE2D *)tracksmenushape2dunk2[elemVal], screenX, screenY);
						sprite_putimage_or((struct SHAPE2D *)tracksmenushape2dunk[elemVal], screenX, screenY);
					}
					continue;
				}

/** @brief Marker.
 * @param TRACK_MARKER_CORNER Parameter `TRACK_MARKER_CORNER`.
 * @return Function result.
 */
				/* Handle 253 marker (corner extension) */
				if (element_type == TRACK_MARKER_CORNER) {
					int elemSrc;

					if (row != 0 || col != 0) {
						render_flags_2[buffer_index] = TRACK_U8_INVALID;
						continue;
					}

					/* Only terrain at current corner cell. */
					screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
					screenX = si * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
					terrVal = terrain_at_safe((int)row + (int)row_offset, si + (int)col_offset);
					sprite_putimage_and_alt(tracksmenushapes1[terrVal], screenX, screenY);

					/* Source element is previous row, previous column. */
					elemSrc = track_row_ofs_safe((int)row + (int)row_offset - 1) + si + (int)col_offset - 1;
					if (elemSrc >= 0 && elemSrc < TRACK_TILE_COUNT) {
						elemVal = track_elem_map[elemSrc];
						screenY = (int)row * TRACK_TILE_PIXELS - (TRACK_TILE_PIXELS - TRACK_MAP_TILE_OFFSET_Y);
						screenX = si * TRACK_TILE_PIXELS - TRACK_MAP_TILE_OFFSET_X;
						sprite_putimage_and((struct SHAPE2D *)tracksmenushape2dunk2[elemVal], screenX, screenY);
						sprite_putimage_or((struct SHAPE2D *)tracksmenushape2dunk[elemVal], screenX, screenY);
					}
					continue;
				}

				/* Remaining marker cases on top/left edges are buffer-only. */
				render_flags_1[buffer_index] = TRACK_U8_INVALID;
				render_flags_2[buffer_index] = TRACK_U8_INVALID;
				continue;
			}

			/* Regular tile rendering */

			/* Mark both buffers */
			render_flags_1[buffer_index] = TRACK_U8_INVALID;
			render_flags_2[buffer_index] = TRACK_U8_INVALID;

			/* Render terrain sprite */
			screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
			screenX = (int)col * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
				sprite_shape_to_1((struct SHAPE2D *)tracksmenushapes1[terrain_type], screenX, screenY);

/** @brief Only.
 * @param element_type Parameter `element_type`.
 * @return Function result.
 */
			/* Empty cell: terrain only (no element mask/fill overlay). */
			if (element_type == 0) {
				continue;
			}

			/* Get multi-tile flag */
			multiflag = TRKOBJ_MULTI_TILE_FLAG(element_type);

			if (multiflag == 0) {
				/* Single tile element */
				putpixel_iconMask(tracksmenushape2dunk2[element_type], screenX, screenY);
				putpixel_iconFillings(tracksmenushape2dunk[element_type], screenX, screenY);
			}
			else if (multiflag == 1) {
				/* Vertical 2-tile element */
				/* Render terrain for next row */
				screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y + TRACK_TILE_PIXELS;
				terrVal = terrain_at_safe((int)row + (int)row_offset + 1, (int)col + (int)col_offset);
				sprite_shape_to_1((struct SHAPE2D *)tracksmenushapes1[terrVal], screenX, screenY);

				/* Render element mask and filling */
				screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
				putpixel_iconMask(tracksmenushape2dunk2[element_type], screenX, screenY);
				putpixel_iconFillings(tracksmenushape2dunk[element_type], screenX, screenY);
			}
			else if (multiflag == 2) {
				/* Horizontal 2-tile element */
				/* Render terrain for next column */
				screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
				screenX = (int)col * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X + TRACK_TILE_PIXELS;
				terrVal = terrain_at_safe((int)row + (int)row_offset, (int)col + (int)col_offset + 1);
				sprite_shape_to_1((struct SHAPE2D *)tracksmenushapes1[terrVal], screenX, screenY);

				/* Render element mask and filling */
				screenX = (int)col * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
				putpixel_iconMask(tracksmenushape2dunk2[element_type], screenX, screenY);
				putpixel_iconFillings(tracksmenushape2dunk[element_type], screenX, screenY);
			}
			else if (multiflag == 3) {
				/* Corner 4-tile element */
				/* Render terrain for additional tiles */
				screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
				screenX = (int)col * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X + TRACK_TILE_PIXELS;
				terrVal = terrain_at_safe((int)row + (int)row_offset, (int)col + (int)col_offset + 1);
				sprite_shape_to_1((struct SHAPE2D *)tracksmenushapes1[terrVal], screenX, screenY);

				screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y + TRACK_TILE_PIXELS;
				screenX = (int)col * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
				terrVal = terrain_at_safe((int)row + (int)row_offset + 1, (int)col + (int)col_offset);
				sprite_shape_to_1((struct SHAPE2D *)tracksmenushapes1[terrVal], screenX, screenY);

				screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y + TRACK_TILE_PIXELS;
				screenX = (int)col * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X + TRACK_TILE_PIXELS;
				terrVal = terrain_at_safe((int)row + (int)row_offset + 1, (int)col + (int)col_offset + 1);
				sprite_shape_to_1((struct SHAPE2D *)tracksmenushapes1[terrVal], screenX, screenY);

				/* Render element mask and filling */
				screenY = (int)row * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
				screenX = (int)col * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
				putpixel_iconMask(tracksmenushape2dunk2[element_type], screenX, screenY);
				putpixel_iconFillings(tracksmenushape2dunk[element_type], screenX, screenY);
			}
		}
	}
}

/*--------------------------------------------------------------
 * Additional external declarations for load_tracks_menu_shapes
 /*--------------------------------------------------------------*/

/* Track menu button arrays (moved from data_global.c) */
static unsigned short trackmenu2_buttons_x1[] = { 9, 202, 220, 8, 220 };
static unsigned short trackmenu2_buttons_x2[] = { 199, 206, 315, 199, 315 };
static unsigned short trackmenu2_buttons_y1[] = { 181, 4, 132, 4, 36 };
static unsigned short trackmenu2_buttons_y2[] = { 187, 179, 139, 179, 187 };

/* Track data tables (moved from data_global.c) */
static unsigned short track_segment_offset_table[] = { 15104, 15360, 15616, 15872, 16128, 16384, 16640, 16896, 17152, 17408, 0, 0 };
static unsigned char track_wall_flags[2] = { 30, 6 };
static unsigned char collision_response_code[2] = { 29, 9 };

/* String constant for terrain templates */
static char aTer0[] = "ter0";

/*--------------------------------------------------------------
 * load_tracks_menu_shapes - Main Track Editor Menu Function
 /*--------------------------------------------------------------*/
/** @brief Load tracks menu shapes.
 */
void load_tracks_menu_shapes(void)
{
	/* Far pointers */
	void * sdtedit_shapes;
	void * tedit_res;
	void * shape_ptr;
	void * snam_ptr;
	void * mnam_ptr;
	void * tnam_ptr;
	void * terrain_template_ptr;
	struct SHAPE2D * shape;

	/* Cursor window pointers stored as offset:segment pairs */
	void * crs_wnd[4];

	/* State variables */
	unsigned char last_place_col;		  /* last cursor col for swap detection */
	unsigned char last_place_row;		  /* last cursor row for swap detection */
	unsigned char prev_category;		  /* previous category */
	unsigned char redraw_map;		  /* redraw map flag */
	unsigned char redraw_scrollbars;		  /* redraw scrollbars flag */
	unsigned char redraw_cursor;		  /* redraw cursor flag */
	unsigned char blit_state;		  /* blit flag */
	unsigned char keep_running;		  /* keep running flag */
	unsigned char current_category;		  /* current category (0=terrain, 1-10=elements) */
	unsigned char needs_validation;		  /* validate track flag */
	unsigned char picker_mode;		  /* picker mode (0=map, 1=picker) */
	unsigned char selected_element;		  /* selected element ID */
	unsigned short anim_index;		  /* animation counter */
	unsigned char picker_col;		  /* picker column */
	unsigned char map_scroll_x;		  /* map scroll X */
	unsigned char map_scroll_y;		  /* map scroll Y */
	unsigned char last_named_element;		  /* last element name displayed */
	unsigned char validation_result;		  /* track validation result */
	unsigned char cursor_col;		  /* cursor col position */
	unsigned char cursor_row;		  /* cursor row position */
	unsigned char picker_row;		  /* picker row */
	unsigned char cursor_height_tiles = 1;		  /* cursor height (tiles) */
	unsigned char cursor_width_tiles = 1;		  /* cursor width (tiles) */
	unsigned char cursor_shape_index = 0;		  /* cursor shape index */
	unsigned char cursor_element;		  /* current element at cursor */
	unsigned char track_modified;		  /* track modified flag */
	unsigned char swap_element;		  /* swap element holder */
	unsigned char anim_saved_element;		  /* animation element holder */
	unsigned char swap_temp_element;		  /* temp element holder */
	unsigned char last_scroll_x;		  /* last scroll X */
	unsigned char last_scroll_y;		  /* last scroll Y */
	unsigned char temp_col;		  /* temp picker col */
	unsigned char temp_row;		  /* temp picker row */
	unsigned char temp_pbox_value;		  /* temp pbox value */
	unsigned char mouse_track_value;		  /* mouse track result */
	unsigned char mouse_hit_index;		  /* mouse hit index */
	unsigned char cursor_mode_for_blink;		  /* cursor blink state */
	unsigned char dialog_result;		  /* dialog result */

	/* Working variables */
	unsigned short last_mouse_x, last_mouse_y, unused_u16;
	unsigned short cursor_blink_toggle;		  /* cursor blink toggle */
	unsigned short blink_timer;		  /* timer accumulator */
	unsigned short input_code;		  /* input code */
	unsigned short cursor_screen_x;		  /* cursor screen X */
	unsigned short cursor_screen_y;		  /* cursor screen Y */
	unsigned char cursor_draw_height;		  /* cursor draw height */
	unsigned char cursor_draw_width;		  /* cursor draw width */

	/* Arrays for element display state */
	unsigned char elem_dirty_flags[132];	/* element dirty flags - 132 bytes */
	unsigned char terr_dirty_flags[132];	/* terrain dirty flags - 132 bytes */

	int i, si;
	unsigned char * elem_map;
	unsigned char * terr_map;
	unsigned char multiflag;

	(void)shape_ptr;
	(void)tnam_ptr;
	(void)blit_state;
	(void)validation_result;
	(void)temp_pbox_value;
	(void)unused_u16;
	(void)cursor_draw_height;
	(void)cursor_draw_width;

	/*========================================
	 * SECTION 1: Load shape resources
	 *========================================*/

	/* Load sdtedit shapes */
	sdtedit_shapes = file_load_shape2d_fatal_thunk("sdtedit");

	/* Locate terrain shapes (16 terrain types) */
	locate_many_resources(sdtedit_shapes,
		"flatlakelak1lak2lak3lak4highgoungouwgousgouegou1gou2gou3gou4gou5gou6gou7gou8",
		tracksmenushapes1);

	/* Locate cursor shapes (4 sizes) */
	locate_many_resources(sdtedit_shapes, "crs0crs1crs2crs3", tracksmenushapes2);

	/* Locate highlighted cursor shapes */
	locate_many_resources(sdtedit_shapes, "ucr0ucr1ucr2ucr3", tracksmenushapes3);

	/* Create cursor sprite windows */
	for (i = 0; i < TRACK_CURSOR_SHAPE_COUNT; i++) {
		shape = (struct SHAPE2D *)tracksmenushapes2[i];
		crs_wnd[i] = sprite_make_wnd(
			shape->s2d_width * video_flag1_is1,
			shape->s2d_height, 15);
	}

	/* Load tedit resource file */
	tedit_res = file_load_resfile("tedit");

	/* Create main window sprite (320x200) */
	wndsprite = sprite_make_wnd(TRACK_EDITOR_WINDOW_W, TRACK_EDITOR_WINDOW_H, 15);

	/* Locate UI shapes */
	pboxshape = (unsigned char *)locate_shape_alt(tedit_res, "pbox");
	snam_ptr = locate_shape_alt(tedit_res, "snam");
	mnam_ptr = locate_shape_alt(tedit_res, "mnam");
	tnam_ptr = locate_shape_alt(tedit_res, "tnam");

	/* Initialize dirty flag arrays */
	track_modified = 0;
	for (i = 0; i < TRACK_DIRTY_FLAG_COUNT; i++) {
		elem_dirty_flags[i] = TRACK_U8_INVALID;
		terr_dirty_flags[i] = TRACK_U8_INVALID;
	}

/** @brief Shapes.
 * @param i Parameter `i`.
 * @return Function result.
 */
	/* Load element icon shapes (186 elements) */
	for (i = 0; i < TRACK_ELEMENT_ICON_COUNT; i++) {
		/* Get 4-char name from snam and load mask shape */
		resID_byte1[0] = *((char *)snam_ptr + i * 4);
		resID_byte1[1] = *((char *)snam_ptr + i * 4 + 1);
		resID_byte1[2] = *((char *)snam_ptr + i * 4 + 2);
		resID_byte1[3] = *((char *)snam_ptr + i * 4 + 3);
		tracksmenushape2dunk[i] = locate_shape_fatal(sdtedit_shapes, resID_byte1);

		/* Get 4-char name from mnam and load filling shape */
		resID_byte1[0] = *((char *)mnam_ptr + i * 4);
		resID_byte1[1] = *((char *)mnam_ptr + i * 4 + 1);
		resID_byte1[2] = *((char *)mnam_ptr + i * 4 + 2);
		resID_byte1[3] = *((char *)mnam_ptr + i * 4 + 3);
		tracksmenushape2dunk2[i] = locate_shape_fatal(sdtedit_shapes, resID_byte1);
	}

	/*========================================
	 * SECTION 2: Initialize state variables
	 *========================================*/
	last_place_col = TRACK_U8_INVALID;
	prev_category = TRACK_U8_INVALID;
	redraw_map = 1;
	redraw_scrollbars = 1;
	redraw_cursor = 1;
	blit_state = TRACK_U8_INVALID;
	keep_running = 1;
	current_category = 1;
	needs_validation = 1;
	picker_mode = 0;
	selected_element = 0;
	anim_index = 0;
	picker_col = 0;
	map_scroll_x = 0;
	map_scroll_y = 0;
	last_scroll_x = TRACK_U8_INVALID;	   /* Initialize to invalid value to force first update */
	last_scroll_y = TRACK_U8_INVALID;	   /* Initialize to invalid value to force first update */
	last_named_element = 0;
	validation_result = 0;
	cursor_col = sprite_render_state;	/* Start cursor X */
	cursor_row = game_security_flag;	/* Start cursor Y */
	picker_row = 7;
	cursor_blink_toggle = 0;
	cursor_mode_for_blink = 0;
	blink_timer = TRACK_CURSOR_BLINK_START;
	last_place_row = TRACK_U8_INVALID;
	swap_element = 0;
	last_mouse_x = TRACK_U16_INVALID;
	last_mouse_y = TRACK_U16_INVALID;

	/*========================================
	 * SECTION 3: Draw initial UI
	 *========================================*/
	sprite_select_wnd_as_sprite1_and_clear();

	/* Draw INFO button */
	draw_button(locate_text_res(tedit_res, "bti"),
		217, 3, 102, 22,
		button_text_color, button_shadow_color, button_highlight_color, 0);

	/* Draw line decorations */
	draw_beveled_border(5, 0, 206, 190,
		track_curve_data, segment_height_lookup, boundary_check_limits);
	draw_beveled_border(217, 32, 102, 158,
		track_curve_data, segment_height_lookup, boundary_check_limits);

	/* Draw SCENERY button */
	draw_button(locate_text_res(tedit_res, "bsc"),
		221, 140, 94, 14,
		button_text_color, button_shadow_color, button_highlight_color, 0);

	/* Draw LOAD button */
	draw_button(locate_text_res(tedit_res, "blo"),
		221, 156, 46, 14,
		button_text_color, button_shadow_color, button_highlight_color, 0);

	/* Draw SAVE button */
	draw_button(locate_text_res(tedit_res, "bsa"),
		221, 172, 46, 14,
		button_text_color, button_shadow_color, button_highlight_color, 0);

	/* Draw CLEAR button */
	draw_button(locate_text_res(tedit_res, "bcl"),
		269, 156, 46, 14,
		button_text_color, button_shadow_color, button_highlight_color, 0);

	/* Draw EXIT button */
	draw_button(locate_text_res(tedit_res, "bex"),
		269, 172, 46, 14,
		button_text_color, button_shadow_color, button_highlight_color, 0);

	/*========================================
	 * SECTION 4: Main event loop
	 *========================================*/

	while (keep_running != 0) {

		/*----------------------------------------
		 * Update cursor size based on element type
		 /*--------------------------------------------------------------*/
		if (redraw_cursor != 0 || prev_category != current_category) {
			cursor_height_tiles = 1;	/* cursor height */
			cursor_width_tiles = 1;	/* cursor width */
			cursor_shape_index = 0;	/* cursor shape index */

			if (current_category != 0) {
				/* Check multi-tile flag for selected element */
				multiflag = TRKOBJ_MULTI_TILE_FLAG(selected_element);
				if (multiflag == 1) {
					/* 2x1 element (wide) */
					cursor_width_tiles = 2;
					cursor_shape_index = 1;
				} else if (multiflag == 2) {
					/* 1x2 element (tall) */
					cursor_height_tiles = 2;
					cursor_shape_index = 2;
				} else if (multiflag == 3) {
					/* 2x2 element */
					cursor_height_tiles = 2;
					cursor_width_tiles = 2;
					cursor_shape_index = 3;
				}
			}
		}

		/*----------------------------------------
		 * Adjust cursor position for multi-tile elements
		 /*--------------------------------------------------------------*/
		if (picker_mode == 0) {
			if (cursor_col >= TRACK_SIZE) {
				cursor_col = TRACK_SIZE - 1;
			}
			if (cursor_row >= TRACK_SIZE) {
				cursor_row = TRACK_SIZE - 1;
			}

			/* Map mode - ensure cursor doesn't go past edge */
			if (cursor_col == TRACK_LAST_INDEX && cursor_height_tiles == 2) {
				cursor_col--;
			}
			if (cursor_row == TRACK_LAST_INDEX && cursor_width_tiles == 2) {
				cursor_row--;
			}

			/* Adjust viewport to keep cursor visible */
			while ((int)cursor_col - (int)map_scroll_x + (int)cursor_height_tiles > TRACK_MAP_VISIBLE_COLS) {
				map_scroll_x++;
			}
			while ((int)cursor_col < (int)map_scroll_x) {
				map_scroll_x--;
			}
			while ((int)cursor_row - (int)map_scroll_y + (int)cursor_width_tiles > TRACK_MAP_VISIBLE_ROWS) {
				map_scroll_y++;
			}
			while ((int)cursor_row < (int)map_scroll_y) {
				map_scroll_y--;
			}

			if (map_scroll_x > (TRACK_SIZE - TRACK_MAP_VISIBLE_COLS)) {
				map_scroll_x = TRACK_SIZE - TRACK_MAP_VISIBLE_COLS;
			}
			if (map_scroll_y > (TRACK_SIZE - TRACK_MAP_VISIBLE_ROWS)) {
				map_scroll_y = TRACK_SIZE - TRACK_MAP_VISIBLE_ROWS;
			}

			/* Check if viewport changed */
			if (last_scroll_x != map_scroll_x || last_scroll_y != map_scroll_y) {
				last_scroll_x = map_scroll_x;
				last_scroll_y = map_scroll_y;
				redraw_map = 1;
				redraw_scrollbars = 1;
			}
		}

		/*----------------------------------------
		 * Handle category change
		 /*--------------------------------------------------------------*/
		if (prev_category != current_category) {
			redraw_cursor = 1;
			prev_category = current_category;

			/* Adjust picker position to valid element */
			do {
				unsigned char pbox_val;
				pbox_val = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col];
				if (pbox_val == TRACK_MARKER_HORIZONTAL) {
					picker_col--;
				} else if (pbox_val == TRACK_MARKER_VERTICAL) {
					picker_row--;
				} else {
					break;
				}
			} while (1);

			/* Redraw element picker */
			sprite_select_wnd_as_sprite1();
			preRender_icons(current_category);

			/* Set up mouse tracking for picker area */
			if (current_category == 0) {
				mouse_track_op(0, 221, 95, 133, 5, 0, 1, 1);
			} else {
				mouse_track_op(0, 221, 95, 133, 5, current_category - 1, 1, TRACK_MENU_PBOX_ROWS);
			}
		}

		/*----------------------------------------
		 * Handle track validation
		 /*--------------------------------------------------------------*/
		if (needs_validation != 0) {
			needs_validation = 0;
			validation_result = track_validate_elements_for_terrain();
		}

		/*----------------------------------------
		 * Render map if needed
		 /*--------------------------------------------------------------*/
		if (redraw_map != 0 || redraw_cursor != 0) {
			sprite_select_wnd_as_sprite1();

			if (redraw_map != 0) {
				redraw_map = 0;

				if (redraw_scrollbars != 0) {
					redraw_scrollbars = 0;
					/* Set up scrollbar mouse tracking */
					mouse_track_op(0, 9, 192, 181, 5, map_scroll_x, TRACK_MAP_VISIBLE_COLS, TRACK_SIZE);
					mouse_track_op(0, 202, 5, 4, 176, map_scroll_y, TRACK_MAP_VISIBLE_ROWS, TRACK_SIZE);
				}

				/* Render 2D track map */
				sprite_set_1_size(8, 200, 4, 179);
				draw_2DtrackMap(map_scroll_x, map_scroll_y, elem_dirty_flags, terr_dirty_flags);
				sprite_set_1_size(0, 320, 0, 200);
			}

			/*----------------------------------------
			 * Render cursor
			 /*--------------------------------------------------------------*/
			if (redraw_cursor != 0) {
				redraw_cursor = 0;

				/* Set cursor sprite */
				sprite_set_1_from_argptr(crs_wnd[cursor_shape_index]);

				if (current_category == 0) {
					/* Terrain mode - draw terrain shape */
					sprite_shape_to_1((void *)tracksmenushapes3[cursor_shape_index], 0, 0);
					sprite_shape_to_1((void *)tracksmenushapes1[selected_element], 0, 0);
					/* Draw border */
					preRender_line(1, 0, 15, 0, performGraphColor);
					preRender_line(1, 14, 15, 14, performGraphColor);
					preRender_line(1, 0, 1, 14, performGraphColor);
					preRender_line(15, 0, 15, 14, performGraphColor);
				} else {
					/* Element mode - draw cursor and element icon */
					sprite_shape_to_1((void *)tracksmenushapes3[cursor_shape_index], 0, 0);

					if (selected_element != 0) {
						putpixel_iconMask(tracksmenushape2dunk2[selected_element], 0, 0);
						putpixel_iconFillings(tracksmenushape2dunk[selected_element], 0, 0);
					}
				}
					sprite_set_1_from_argptr((struct SPRITE *)wndsprite);
			}
		}

		/*----------------------------------------
		 * Calculate cursor screen position
		 /*--------------------------------------------------------------*/
		if (picker_mode == 0) {
			/* Map mode cursor position */
			cursor_draw_width = TRACK_TILE_PIXELS;
			cursor_draw_height = TRACK_TILE_PIXELS;
			{
				int cursor_x;
				int cursor_y;

			if (cursor_col >= TRACK_SIZE) {
				cursor_col = TRACK_SIZE - 1;
			}
			if (cursor_row >= TRACK_SIZE) {
				cursor_row = TRACK_SIZE - 1;
			}
			if (map_scroll_x >= TRACK_SIZE) {
				map_scroll_x = TRACK_SIZE - 1;
			}
			if (map_scroll_y >= TRACK_SIZE) {
				map_scroll_y = TRACK_SIZE - 1;
			}

				cursor_x = ((int)cursor_col - (int)map_scroll_x) * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_X;
				cursor_y = ((int)cursor_row - (int)map_scroll_y) * TRACK_TILE_PIXELS + TRACK_MAP_TILE_OFFSET_Y;
				if (cursor_x < 0) {
					cursor_x = 0;
				} else if (cursor_x > TRACK_VIEWPORT_MAX_X) {
					cursor_x = TRACK_VIEWPORT_MAX_X;
				}
				if (cursor_y < 0) {
					cursor_y = 0;
				} else if (cursor_y > TRACK_VIEWPORT_MAX_Y) {
					cursor_y = TRACK_VIEWPORT_MAX_Y;
				}
				cursor_screen_x = (unsigned short)cursor_x;
				cursor_screen_y = (unsigned short)cursor_y;
			}

			/* Get element at cursor position */
			elem_map = (unsigned char *)track_elem_map;
			cursor_element = elem_map[trackrows[cursor_row] + cursor_col];

			/* Handle multi-tile markers */
			if (cursor_element == TRACK_MARKER_CORNER) {
				if (cursor_row > 0 && cursor_col > 0) {
					cursor_element = elem_map[trackrows[cursor_row - 1] + cursor_col - 1];
				} else {
					cursor_element = 0;
				}
			} else if (cursor_element == TRACK_MARKER_VERTICAL) {
				if (cursor_row > 0) {
					cursor_element = elem_map[trackrows[cursor_row - 1] + cursor_col];
				} else {
					cursor_element = 0;
				}
			} else if (cursor_element == TRACK_MARKER_HORIZONTAL) {
				if (cursor_col > 0) {
					cursor_element = elem_map[trackrows[cursor_row] + cursor_col - 1];
				} else {
					cursor_element = 0;
				}
			}
		} else {
			/* Picker mode cursor position */
			cursor_draw_width = TRACK_TILE_PIXELS;
			cursor_draw_height = TRACK_TILE_PIXELS;
			cursor_screen_y = (unsigned short)picker_row * TRACK_TILE_PIXELS + TRACK_MENU_Y;

			if (picker_row == TRACK_MENU_SPECIAL_ROW_CAT) {
				cursor_screen_x = TRACK_MENU_X;
				cursor_draw_height = 8;
				cursor_draw_width = 96;
			} else if (picker_row == TRACK_MENU_SPECIAL_ROW_SCENERY) {
				cursor_screen_y -= 8;
				picker_col = 0;
				cursor_screen_x = TRACK_MENU_X;
				cursor_draw_width = 96;
			} else if (picker_row > TRACK_MENU_SPECIAL_ROW_SCENERY) {
				cursor_screen_y -= 8;
				if (picker_col < 3) picker_col = 0;
				else picker_col = 3;
				cursor_screen_x = (unsigned short)picker_col * TRACK_TILE_PIXELS + TRACK_MENU_X;
				cursor_draw_width = 48;
			} else {
				cursor_screen_x = (unsigned short)picker_col * TRACK_TILE_PIXELS + TRACK_MENU_X;

				/* Check for wide elements */
				if (picker_row < 5) {
					unsigned char pbox_val = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col + TRACK_PICKER_COLS];
					if (pbox_val == TRACK_MARKER_VERTICAL) {
						cursor_draw_height = 32;
					}
				}
				if (picker_col < 5) {
					unsigned char pbox_val = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col + 1];
					if (pbox_val == TRACK_MARKER_HORIZONTAL) {
						cursor_draw_width = 32;
					}
				}

				/* Get selected element from picker */
				cursor_element = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col];
				if (cursor_element >= TRACK_MARKER_CORNER) {
					cursor_element = 0;
				}
			}

			if (current_category == 0) {
				cursor_element = 0;
			}
		}

		/*----------------------------------------
		 * Update element name display
		 /*--------------------------------------------------------------*/
		if (cursor_element != last_named_element) {
			mouse_draw_opaque_check();
			font_set_colors(dialog_fnt_colour, 0);

			/* Get element name from tnam resource */
			/* (Element names are 3 chars each) */
			last_named_element = cursor_element;

			/* Draw element name at fixed position */
			/* sprite_draw_text(...) - simplified for now */

			mouse_draw_transparent_check();
		}

		/*----------------------------------------
		 * Handle cursor blinking
		 /*--------------------------------------------------------------*/
		mouse_draw_opaque_check();
		cursor_mode_for_blink = picker_mode;
		if (cursor_mode_for_blink == 0) {
			sprite_clear_shape_alt((void *)tracksmenushapes3[cursor_shape_index], cursor_screen_x, cursor_screen_y);
		}

loc_2AE73:
		if (blink_timer > TRACK_CURSOR_BLINK_THRESHOLD) {
			mouse_draw_opaque_check();
			sprite_set_1_from_argptr((struct SPRITE *)wndsprite);
			if (picker_mode == 0) {
				if (cursor_blink_toggle != 0) {
					sprite_shape_to_1((void *)tracksmenushapes3[cursor_shape_index], cursor_screen_x, cursor_screen_y);
				} else {
					sprite_shape_to_1(crs_wnd[cursor_shape_index], cursor_screen_x, cursor_screen_y);
				}
			} else {
				sprite_draw_rect_outline(cursor_screen_x,
					(unsigned short)(cursor_screen_y - 1),
					(unsigned short)(cursor_screen_x + cursor_draw_width),
					(unsigned short)(cursor_screen_y + cursor_draw_height - 1),
					obstacle_collision_table);
			}

			mouse_draw_transparent_check();
			cursor_blink_toggle ^= 1;
			blink_timer = 0;
		}

		/* Present composed track-menu frame */
		sprite_copy_2_to_1();
		mouse_draw_opaque_check();
		sprite_putimage(wndsprite->sprite_bitmapptr);
		mouse_draw_transparent_check();
		video_refresh();

		/*----------------------------------------
		 * Input loop
		 /*--------------------------------------------------------------*/
		{
			unsigned long delta = timer_get_delta_alt();
			blink_timer += (unsigned short)delta;
			input_code = input_checking((unsigned short)delta);
			if (input_code == 0 && (((unsigned short)mouse_xpos != last_mouse_x) || ((unsigned short)mouse_ypos != last_mouse_y))) {
				input_code = 1;
			}
			last_mouse_x = (unsigned short)mouse_xpos;
			last_mouse_y = (unsigned short)mouse_ypos;
		}

		/* Mouse button hit testing */
		mouse_hit_index = mouse_multi_hittest(5,
			trackmenu2_buttons_x1, trackmenu2_buttons_x2,
			trackmenu2_buttons_y1, trackmenu2_buttons_y2);

		if (mouse_hit_index != TRACK_U8_INVALID) {
			if (mouse_hit_index == 0) {
				if ((mouse_butstate & 3) != 0) {
					picker_mode = 0;
					mouse_track_value = mouse_track_op(1, 9, 192, 181, 5, map_scroll_x, TRACK_MAP_VISIBLE_COLS, TRACK_SIZE);
					cursor_col += mouse_track_value - map_scroll_x;
					map_scroll_x = mouse_track_value;
					if (map_scroll_x > (TRACK_SIZE - TRACK_MAP_VISIBLE_COLS)) map_scroll_x = TRACK_SIZE - TRACK_MAP_VISIBLE_COLS;
					input_code = 1;
				}
			} else if (mouse_hit_index == 1) {
				if ((mouse_butstate & 3) != 0) {
					picker_mode = 0;
					mouse_track_value = mouse_track_op(1, 202, 5, 4, 176, map_scroll_y, TRACK_MAP_VISIBLE_ROWS, TRACK_SIZE);
					cursor_row += mouse_track_value - map_scroll_y;
					map_scroll_y = mouse_track_value;
					if (map_scroll_y > (TRACK_SIZE - TRACK_MAP_VISIBLE_ROWS)) map_scroll_y = TRACK_SIZE - TRACK_MAP_VISIBLE_ROWS;
					input_code = 1;
				}
			} else if (mouse_hit_index == 2) {
				if (picker_mode != 1 || picker_row != 6) {
					picker_mode = 1;
					picker_row = 6;
					input_code = 1;
				}
				if ((mouse_butstate & 3) != 0) {
					unsigned short cat_track_value = (current_category == 0) ? 0 : (unsigned short)(current_category - 1);
					mouse_track_value = mouse_track_op(1, 221, 95, 133, 5, cat_track_value, 1, TRACK_MENU_PBOX_ROWS);
					current_category = (unsigned char)(mouse_track_value + 1);
					input_code = 1;
				}
			} else if (mouse_hit_index == 3) {
				temp_col = (mouse_xpos - TRACK_MAP_TILE_OFFSET_X) / TRACK_TILE_PIXELS;
				temp_row = (mouse_ypos - TRACK_MAP_TILE_OFFSET_Y) / TRACK_TILE_PIXELS;
				if (current_category != 0) {
					multiflag = TRKOBJ_MULTI_TILE_FLAG(selected_element);
					if (temp_row == (TRACK_MAP_VISIBLE_ROWS - 1) && (multiflag & 1) != 0) temp_row--;
					if (temp_col == (TRACK_MAP_VISIBLE_COLS - 1) && (multiflag & 2) != 0) temp_col--;
				}
				temp_col += map_scroll_x;
				temp_row += map_scroll_y;
				if (picker_mode != 0 || cursor_col != temp_col || cursor_row != temp_row) {
					picker_mode = 0;
					cursor_col = temp_col;
					cursor_row = temp_row;
					input_code = 1;
				}
				if (input_code == 32) input_code = 13;
			} else if (mouse_hit_index == 4) {
				temp_col = (mouse_xpos - TRACK_MENU_X) / TRACK_TILE_PIXELS;
				temp_row = (mouse_ypos - TRACK_MENU_Y) / TRACK_TILE_PIXELS;
				if (temp_row < 6) {
					unsigned int pbox_idx;
					unsigned char pbox_val;

					pbox_idx = (unsigned int)temp_row * TRACK_PICKER_COLS + (unsigned int)current_category * TRACK_PAGE_STRIDE + (unsigned int)temp_col;
					pbox_val = pboxshape[pbox_idx];
					if (pbox_val == TRACK_MARKER_CORNER) {
						if (temp_row > 0) temp_row--;
						if (temp_col > 0) temp_col--;
					} else if (pbox_val == TRACK_MARKER_VERTICAL) {
						if (temp_row > 0) temp_row--;
					} else if (pbox_val == TRACK_MARKER_HORIZONTAL) {
						if (temp_col > 0) temp_col--;
					}

					/* Re-check once after adjustment for mixed markers. */
					pbox_idx = (unsigned int)temp_row * TRACK_PICKER_COLS + (unsigned int)current_category * TRACK_PAGE_STRIDE + (unsigned int)temp_col;
					pbox_val = pboxshape[pbox_idx];
					if (pbox_val == TRACK_MARKER_VERTICAL && temp_row > 0) temp_row--;
					if (pbox_val == TRACK_MARKER_HORIZONTAL && temp_col > 0) temp_col--;
				} else {
					temp_row = (mouse_ypos - 28) / TRACK_TILE_PIXELS;
					if (temp_row == 7) temp_col = 0;
					else if (temp_col >= 3) temp_col = 3;
					else temp_col = 0;
				}
				picker_col = temp_col;
				picker_row = temp_row;
				picker_mode = 1;
				if (input_code == 32) {
					input_code = 13;
				} else {
					input_code = 1;
				}
			}

			/* Restore draw target to the main track-editor window. */
			sprite_set_1_from_argptr((struct SPRITE *)wndsprite);
		}

		if (input_code == 1) {
			last_place_col = TRACK_U8_INVALID;
			redraw_map = 1;
			redraw_cursor = 1;
		}

		/* If no input and animation active, continue animation */
		if (input_code == 0 && anim_index != 0) {
			input_code = 1;
		}

		if (input_code == 0) {
			/* No input - keep polling and blink cursor */
			goto loc_2AE73;
		}

		/* Delay handling for key repeat */
		if (anim_index != 0) {
			timer_wait_ticks_and_get_counter(10);
		}

		/*----------------------------------------
		 * Render cursor at current position
		 /*--------------------------------------------------------------*/
		if (cursor_blink_toggle != 0 || cursor_mode_for_blink == 0) {
			mouse_draw_opaque_check();
			sprite_set_1_from_argptr((struct SPRITE *)wndsprite);

			if (cursor_screen_x > TRACK_VIEWPORT_MAX_X) {
				cursor_screen_x = TRACK_VIEWPORT_MAX_X;
			}
			if (cursor_screen_y > TRACK_VIEWPORT_MAX_Y) {
				cursor_screen_y = TRACK_VIEWPORT_MAX_Y;
			}

			if (cursor_mode_for_blink == 0) {
				if (cursor_blink_toggle != 0) {
					sprite_shape_to_1((void *)tracksmenushapes3[cursor_shape_index], cursor_screen_x, cursor_screen_y);
				} else {
					sprite_shape_to_1(crs_wnd[cursor_shape_index], cursor_screen_x, cursor_screen_y);
				}
			} else if (cursor_blink_toggle != 0) {
				sprite_draw_rect_outline(cursor_screen_x,
					(unsigned short)(cursor_screen_y - 1),
					(unsigned short)(cursor_screen_x + cursor_draw_width),
					(unsigned short)(cursor_screen_y + cursor_draw_height - 1),
					obstacle_collision_table);
			}

			mouse_draw_transparent_check();
		}

		/*----------------------------------------
/** @brief Mode.
 * @param anim_index Parameter `anim_index`.
 * @return Function result.
 */
		 /*--------------------------------------------------------------*/
		if (anim_index != 0) {
			if (input_code == 1 && picker_mode == 0) {
				/* Continue animation */
			} else {
				anim_index = track_pieces_counter - 1;
			}

			/* Follow track path */
			cursor_col = ((unsigned char *)path_col)[anim_index];
			cursor_row = ((unsigned char *)path_row)[anim_index];
			elem_map = (unsigned char *)track_elem_map;
			selected_element = elem_map[trackrows[cursor_row] + cursor_col];
			redraw_map = 1;
			redraw_cursor = 1;
			anim_index++;

			if (anim_index >= track_pieces_counter) {
				selected_element = anim_saved_element;
				anim_index = 0;
			}
			continue;
		}

		anim_index = 0;

		/*----------------------------------------
/** @brief Keys.
 * @param si Parameter `si`.
 * @return Function result.
 */
		 /*--------------------------------------------------------------*/
		for (si = 0; si < 10; si++) {
			if (track_segment_offset_table[si] == input_code) {
				current_category = si + 1;
				input_code = 0;
				break;
			}
		}

		/*----------------------------------------
		 * Handle keyboard input
		 /*--------------------------------------------------------------*/
		if (input_code == 99 || input_code == 67) {
			/* 'C' or 'c' - Check track */
			goto loc_2B3EA;
		}

		if (input_code <= 99) {
			if (input_code == 13) {
				/* Enter - Place tile or select from picker */
				goto loc_2B49A;
			}
			if (input_code == 32) {
				/* Space - Toggle picker mode */
				picker_mode ^= 1;
				continue;
			}
			if (input_code == 43) {
				/* '+' - Next category */
				if (current_category < 10) current_category++;
				continue;
			}
			if (input_code == 45) {
				/* '-' - Previous category */
				if (current_category > 1) current_category--;
				continue;
			}
		}

		/* Extended keys */
		if (input_code == 19200) {
			/* Left arrow */
			goto loc_2BC8A;
		}
		if (input_code == 18176) {
			/* Home */
			goto loc_2BB46;
		}
		if (input_code == 18432) {
			/* Up arrow */
			goto loc_2BB82;
		}
		if (input_code == 19712) {
			/* Right arrow */
			goto loc_2BD20;
		}
		if (input_code == 20480) {
			/* Down arrow */
			goto loc_2BC06;
		}
		if (input_code == 20992) {
			/* Insert - Toggle picker mode */
			picker_mode ^= 1;
			continue;
		}
		if (input_code == 21504) {
			/* Ctrl+PgUp - Switch to terrain mode */
			current_category = 0;
			selected_element = 0;
			continue;
		}
		if (input_code == 283) {
			/* ESC - Exit (with confirmation if modified) */
			if (track_modified != 0) {
				/* Show exit confirmation dialog */
				track_consume_dialog_click();
				dialog_result = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 1,
					locate_text_res(tedit_res, "chx"),
					TRACK_DIALOG_AUTO_POS, TRACK_DIALOG_AUTO_POS, performGraphColor, 0, 0);
				if (dialog_result == 0 || dialog_result == TRACK_U8_INVALID) {
					continue;
				}
			}
			keep_running = 0;
			continue;
		}

		/* If no handler matched, continue loop */
		continue;

		/*----------------------------------------
		 * Track validation check (C key)
		 /*--------------------------------------------------------------*/
loc_2B3EA:
		si = track_setup();
		track_consume_dialog_click();
		ui_dialog_show_restext(UI_DIALOG_CONFIRM, 1,
			locate_text_res(tedit_res, "eok"),	/* Error/OK messages */
			TRACK_DIALOG_AUTO_POS, TRACK_DIALOG_AUTO_POS, performGraphColor, 0, 0);

		if (si > 1) {
			picker_mode = 0;
			if (track_pieces_counter == 0) {
				cursor_col = sprite_render_state;
				cursor_row = game_security_flag;
			} else {
				cursor_col = ((unsigned char *)path_col)[0];
				cursor_row = ((unsigned char *)path_row)[0];
				anim_saved_element = selected_element;
				elem_map = (unsigned char *)track_elem_map;
				selected_element = elem_map[trackrows[cursor_row] + cursor_col];
				anim_index = 1;
				redraw_cursor = 1;
			}
		}
		check_input();
		continue;

		/*----------------------------------------
		 * Enter key handler - Place tile or select element
		 /*--------------------------------------------------------------*/
loc_2B49A:
		if (picker_mode != 0) {
			/* Picker mode - select element */
			if (picker_row < TRACK_PICKER_ROWS) {
				/* Normalize marker cells (FD/FE/FF) to the piece's top-left anchor. */
				{
					unsigned int pbox_idx;
					unsigned char pbox_val;

					pbox_idx = (unsigned int)picker_row * TRACK_PICKER_COLS + (unsigned int)current_category * TRACK_PAGE_STRIDE + (unsigned int)picker_col;
					pbox_val = pboxshape[pbox_idx];
					if (pbox_val == TRACK_MARKER_CORNER) {
						if (picker_row > 0) picker_row--;
						if (picker_col > 0) picker_col--;
					} else if (pbox_val == TRACK_MARKER_VERTICAL) {
						if (picker_row > 0) picker_row--;
					} else if (pbox_val == TRACK_MARKER_HORIZONTAL) {
						if (picker_col > 0) picker_col--;
					}

					pbox_idx = (unsigned int)picker_row * TRACK_PICKER_COLS + (unsigned int)current_category * TRACK_PAGE_STRIDE + (unsigned int)picker_col;
					pbox_val = pboxshape[pbox_idx];
					if (pbox_val == TRACK_MARKER_VERTICAL && picker_row > 0) picker_row--;
					if (pbox_val == TRACK_MARKER_HORIZONTAL && picker_col > 0) picker_col--;
				}

				/* Get element from picker */
				selected_element = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col];

				/* Adjust cursor for multi-tile elements */
				if (current_category != 0) {
					multiflag = TRKOBJ_MULTI_TILE_FLAG(selected_element);
					if ((multiflag & 1) && (int)cursor_row - (int)map_scroll_y == 10) {
						cursor_row--;
					}
					if ((multiflag & 2) && (int)cursor_col - (int)map_scroll_x == 11) {
						cursor_col--;
					}
				}
				redraw_cursor++;
				picker_mode = 0;
			} else if (picker_row == TRACK_MENU_SPECIAL_ROW_CAT) {
				/* Category row - cycle category */
				needs_validation = 1;
				current_category++;
				if (current_category > 10) current_category = 1;
			} else if (picker_row == TRACK_MENU_SPECIAL_ROW_SCENERY) {
				if (picker_col == 0) {
					/* Scenery selection dialog */
					elem_map = (unsigned char *)track_elem_map;
					track_consume_dialog_click();
					dialog_result = ui_dialog_confirm_restext(
						locate_text_res(tedit_res, "mss"),
						elem_map[TRACK_TRACKDATA_ELEM_BYTES]);
					if (dialog_result != TRACK_U8_INVALID && dialog_result != 5) {
						elem_map[TRACK_TRACKDATA_ELEM_BYTES] = dialog_result;
						redraw_map++;
						track_modified = 1;
					}
				} else {
					/* Clear/New track dialog (right-side button) */
					track_consume_dialog_click();
					dialog_result = ui_dialog_confirm_restext(
						locate_text_res(tedit_res, "men"), 0);
					if (dialog_result != TRACK_U8_INVALID && dialog_result != 5) {
						/* Clear track */
						elem_map = (unsigned char *)track_elem_map;
						for (si = 0; si < TRACK_TRACKDATA_ELEM_BYTES; si++) {
							elem_map[si] = 0;
						}
						/* Load terrain template */
						aTer0[3] = '0' + dialog_result;
						terrain_template_ptr = locate_shape_alt(tedit_res, aTer0);
						terr_map = (unsigned char *)track_terrain_map;
						for (si = 0; si < TRACK_TRACKDATA_TOTAL_BYTES; si++) {
							terr_map[si] = ((unsigned char *)terrain_template_ptr)[si];
						}
						gameconfig.game_trackname[0] = 0;
						redraw_map++;
						track_modified = 1;
					}
				}
			} else if (picker_row == TRACK_MENU_SPECIAL_ROW_LOADSAVE) {
				if (picker_col == 0) {
					/* picker_row==8, picker_col==0: LOAD track */
					sprite_copy_2_to_1();

					if (track_modified != 0) {
						/* Changes warning */
						track_consume_dialog_click();
						dialog_result = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 1,
							locate_text_res(tedit_res, "chl"),
							TRACK_DIALOG_AUTO_POS, TRACK_DIALOG_AUTO_POS, performGraphColor, 0, 0);
						if (dialog_result == 0) {
							goto loc_done_load;
						}
					}

					si = 1;
					g_is_busy = 1;
					redraw_map++;
					{
						char track_dir_backup[82];
						strncpy(track_dir_backup, track_highscore_path_buffer, sizeof(track_dir_backup) - 1);
						track_dir_backup[sizeof(track_dir_backup) - 1] = '\0';

					/* File select dialog */
					track_consume_dialog_click();
					si = do_fileselect_dialog(track_highscore_path_buffer,
						gameconfig.game_trackname, ".trk",
						locate_text_res(mainresptr, "trk"));
						/* do_fileselect_dialog already wrote the selected base name (no extension)
						   into gameconfig.game_trackname — nothing more to parse here. */
						strncpy(track_highscore_path_buffer, track_dir_backup, 81);
						track_highscore_path_buffer[81] = '\0';
					}

					file_build_path(track_highscore_path_buffer, gameconfig.game_trackname, ".trk", g_path_buf);

					if (si > 0) {
						/* Load track file */
						file_read_fatal(g_path_buf, (char *)track_elem_map);
						track_setup();
						picker_mode = 0;
						cursor_row = game_security_flag;
						cursor_col = sprite_render_state;
						track_modified = 0;
						redraw_map++;
					}
				} else {
					/* picker_row==8, picker_col!=0: CLEAR/NEW track (right-side button) */
					track_consume_dialog_click();
					dialog_result = ui_dialog_confirm_restext(
						locate_text_res(tedit_res, "men"), 0);
					if (dialog_result != TRACK_U8_INVALID && dialog_result != 5) {
						/* Clear track elements */
						elem_map = (unsigned char *)track_elem_map;
						for (si = 0; si < TRACK_TRACKDATA_ELEM_BYTES; si++) {
							elem_map[si] = 0;
						}
						/* Load terrain template */
						aTer0[3] = '0' + dialog_result;
						terrain_template_ptr = locate_shape_alt(tedit_res, aTer0);
						terr_map = (unsigned char *)track_terrain_map;
						for (si = 0; si < TRACK_TRACKDATA_TOTAL_BYTES; si++) {
							terr_map[si] = ((unsigned char *)terrain_template_ptr)[si];
						}
						gameconfig.game_trackname[0] = 0;
						redraw_map++;
						track_modified = 1;
					}
				}
loc_done_load:
				;
			} else {
/** @brief Exit.
 * @param picker_col Parameter `picker_col`.
 * @return Function result.
 */
				/* picker_row > 8: SAVE (picker_col==0) or EXIT (picker_col!=0) */
				if (picker_col == 0) {
					/* SAVE track */
					char save_cancel = 0;
					sprite_copy_2_to_1();
					g_is_busy = 1;
					redraw_map++;
					track_consume_dialog_click();
					si = do_savefile_dialog(track_highscore_path_buffer, gameconfig.game_trackname,
						locate_text_res(mainresptr, "trk"));
					if (si != 0) {
						file_build_path(track_highscore_path_buffer, gameconfig.game_trackname, ".trk", g_path_buf);
						if (file_find(g_path_buf) != 0) {
							/* File exists — confirm overwrite */
							track_consume_dialog_click();
							dialog_result = (unsigned char)ui_dialog_show_restext(UI_DIALOG_CONFIRM, 1,
								locate_text_res(mainresptr, "fex"),
								TRACK_DIALOG_AUTO_POS, TRACK_DIALOG_AUTO_POS, performGraphColor, 0, 0);
							if ((short)(unsigned char)dialog_result == TRACK_U8_INVALID) {
								save_cancel = 1;
							}
						}
						if (!save_cancel) {
							si = file_write_fatal(g_path_buf, (char *)track_elem_map, TRACK_FILE_SAVE_BYTES);
							if (si != 0) {
								highscore_write_a_(0);
								track_modified = 0;
							} else {
								track_consume_dialog_click();
								ui_dialog_show_restext(UI_DIALOG_CONFIRM, 1,
									locate_text_res(mainresptr, "ser"),
									TRACK_DIALOG_AUTO_POS, TRACK_DIALOG_AUTO_POS, performGraphColor, 0, 0);
							}
						}
					}
					g_is_busy = 0;
				} else {
/** @brief Editor.
 * @param track_modified Parameter `track_modified`.
 * @return Function result.
 */
					/* EXIT editor (with confirmation if modified) */
					if (track_modified != 0) {
						track_consume_dialog_click();
						dialog_result = ui_dialog_show_restext(UI_DIALOG_CONFIRM, 1,
							locate_text_res(tedit_res, "chx"),
							TRACK_DIALOG_AUTO_POS, TRACK_DIALOG_AUTO_POS, performGraphColor, 0, 0);
						if (dialog_result == 0 || dialog_result == TRACK_U8_INVALID) {
							goto loc_done_exit;
						}
					}
					keep_running = 0;
				}
loc_done_exit:
				;
			}
			check_input();
			continue;
		}

		/* Map mode - place tile */
		if (current_category == 0) {
			/* Terrain mode */
			terr_map = (unsigned char *)track_terrain_map;
			si = terrainrows[cursor_row + 1] + cursor_col + 1;

			/* Swap if placing on same position */
			if (cursor_col == last_place_col && cursor_row == last_place_row) {
				swap_temp_element = selected_element;
				selected_element = swap_element;
				swap_element = swap_temp_element;
				redraw_cursor++;
			} else {
				swap_element = terr_map[si];
				if (swap_element >= TRACK_MARKER_CORNER) swap_element = 0;
				last_place_col = cursor_col;
				last_place_row = cursor_row;
			}

			/* Place terrain */
			terr_map[si] = selected_element;
			track_modified = 1;
			needs_validation = 1;
			redraw_map++;
		} else {
			/* Element mode */
			/* Check multi-tile boundary conditions */
			multiflag = TRKOBJ_MULTI_TILE_FLAG(selected_element);
			if ((multiflag & 1) && cursor_row > (TRACK_LAST_INDEX - 1)) {
				check_input();
				continue;
			}
			if ((multiflag & 2) && cursor_col > (TRACK_LAST_INDEX - 1)) {
				check_input();
				continue;
			}

			/* Swap if placing on same position */
			if (cursor_col == last_place_col && cursor_row == last_place_row) {
				swap_temp_element = selected_element;
				selected_element = swap_element;
				swap_element = swap_temp_element;
				redraw_cursor++;
			} else {
				elem_map = (unsigned char *)track_elem_map;
				swap_element = elem_map[trackrows[cursor_row] + cursor_col];
				if (swap_element >= TRACK_MARKER_CORNER) swap_element = 0;
				last_place_col = cursor_col;
				last_place_row = cursor_row;
			}

			/* Place element */
			elem_map = (unsigned char *)track_elem_map;
			elem_map[trackrows[last_place_row] + last_place_col] = selected_element;
			track_modified = 1;
			needs_validation = 1;
			redraw_map++;

			/* Handle multi-tile elements */
			if (multiflag == 1) {
				/* 2x1 - mark row below */
				elem_map[trackrows[last_place_row + 1] + last_place_col] = TRACK_MARKER_VERTICAL;
			} else if (multiflag == 2) {
				/* 1x2 - mark col right */
				elem_map[trackrows[last_place_row] + last_place_col + 1] = TRACK_MARKER_HORIZONTAL;
			} else if (multiflag == 3) {
				/* 2x2 - mark all adjacent */
				elem_map[trackrows[last_place_row] + last_place_col + 1] = TRACK_MARKER_HORIZONTAL;
				elem_map[trackrows[last_place_row + 1] + last_place_col] = TRACK_MARKER_VERTICAL;
				elem_map[trackrows[last_place_row + 1] + last_place_col + 1] = TRACK_MARKER_CORNER;
			}
		}
		check_input();
		continue;

		/*----------------------------------------
		 * Home key - Jump to top-left
		 /*--------------------------------------------------------------*/
loc_2BB46:
		if (picker_mode != 0) {
			picker_row = 0;
		} else {
			if (cursor_row == map_scroll_y && cursor_col == map_scroll_x) {
				map_scroll_x = 0;
				map_scroll_y = 0;
			}
			cursor_row = map_scroll_y;
			cursor_col = map_scroll_x;
		}
		continue;

		/*----------------------------------------
		 * Up arrow
		 /*--------------------------------------------------------------*/
loc_2BB82:
		if (picker_mode) {
			if (cursor_row > 0) {
				last_place_col = TRACK_U8_INVALID;
				cursor_row--;
			}
			if (picker_mode && picker_row < 6) {
				/* Adjust picker position */
				do {
					unsigned char pbox_val = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col];
					if (pbox_val == TRACK_MARKER_HORIZONTAL) {
						picker_col--;
					} else if (pbox_val == TRACK_MARKER_VERTICAL) {
						picker_row--;
					} else {
						break;
					}
				} while (1);
			}
		} else {
			if (cursor_row > 0) {
				last_place_col = TRACK_U8_INVALID;
				cursor_row--;
			}
		}
		continue;

		/*----------------------------------------
		 * Down arrow
		 /*--------------------------------------------------------------*/
loc_2BC06:
		{
			unsigned char limit = picker_mode ? collision_response_code[picker_mode] : collision_response_code[0];
			if (picker_mode) {
				if (cursor_row < limit) {
					last_place_col = TRACK_U8_INVALID;
					cursor_row++;
				}
				if (picker_mode && picker_row < 6) {
					unsigned char pbox_val = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col];
					if (pbox_val == TRACK_MARKER_HORIZONTAL) {
						picker_col--;
					} else if (pbox_val == TRACK_MARKER_VERTICAL) {
						picker_row++;
					}
				}
			} else {
				if (cursor_row < limit) {
					last_place_col = TRACK_U8_INVALID;
					cursor_row++;
				}
			}
		}
		continue;

		/*----------------------------------------
		 * Left arrow
		 /*--------------------------------------------------------------*/
loc_2BC8A:
		if (picker_mode != 0 && picker_row == 6) {
			/* Category row - previous category */
			if (current_category > 1) current_category--;
		} else if (picker_mode) {
			if (cursor_col > 0) {
				last_place_col = TRACK_U8_INVALID;
				cursor_col--;
			}
			if (picker_row > 5) {
				picker_col = 0;
			} else {
				/* Adjust picker position */
				do {
					unsigned char pbox_val = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col];
					if (pbox_val == TRACK_MARKER_HORIZONTAL) {
						picker_col--;
					} else if (pbox_val == TRACK_MARKER_VERTICAL) {
						picker_row--;
					} else {
						break;
					}
				} while (1);
			}
		} else {
			if (cursor_col > 0) {
				last_place_col = TRACK_U8_INVALID;
				cursor_col--;
			}
		}
		continue;

		/*----------------------------------------
		 * Right arrow
		 /*--------------------------------------------------------------*/
loc_2BD20:
		if (picker_mode != 0 && picker_row == 6) {
			/* Category row - next category */
			if (current_category < 10) current_category++;
		} else {
			unsigned char step = 1;

			if (picker_mode && picker_row > 5) {
				step = 3;
			} else if (picker_mode && picker_row <= 5) {
				/* Check for wide elements in picker */
				do {
					unsigned char pbox_val = pboxshape[picker_row * TRACK_PICKER_COLS + current_category * TRACK_PAGE_STRIDE + picker_col + step];
					if (pbox_val >= TRACK_MARKER_VERTICAL) {
						if (pbox_val == TRACK_MARKER_HORIZONTAL) {
							step++;
						} else if (pbox_val == TRACK_MARKER_VERTICAL) {
							picker_row--;
						}
					} else {
						break;
					}
				} while ((int)cursor_col + step < track_wall_flags[picker_mode]);
			}

			if ((int)cursor_col + step < track_wall_flags[picker_mode]) {
				last_place_col = TRACK_U8_INVALID;
				cursor_col += step;
			}
		}
		continue;
	}

	/*========================================
	 * SECTION 5: Cleanup
	 *========================================*/
	sprite_free_wnd(wndsprite);
	sprite_free_wnd(crs_wnd[3]);
	sprite_free_wnd(crs_wnd[2]);
	sprite_free_wnd(crs_wnd[1]);
	sprite_free_wnd(crs_wnd[0]);
	unload_resource(tedit_res);
	mmgr_free(sdtedit_shapes);
}

/*
 * track_setup - Complete C translation from seg004.asm
 * 16-bit Borland C++ 5.2 medium model (int=short=16-bit)
 *
 * Validates track connectivity and builds path/checkpoint data.
 * Returns 0 on success, error code (1-11) on failure.
 */

/* Extern globals (declared in data_game.h) */


#pragma pack(push, 1)
struct TRKOBJINFO_RAW {
	uint8_t  si_noOfBlocks;
	uint8_t  si_entryPoint;
	uint8_t  si_exitPoint;
	uint8_t  si_entryType;
	uint8_t  si_exitType;
	uint8_t  si_arrowType;
	int16_t  si_arrowOrient;
	uint16_t si_cameraDataOffset;
	uint8_t  si_opp1;
	uint8_t  si_opp2;
	uint8_t  si_opp3;
	uint8_t  si_oppSpedCode;
};
#pragma pack(pop)

/** @brief Trkobject raw entry.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
static unsigned char* trkobject_raw_entry(unsigned char elem)
{
	return ((unsigned char*)trkObjectList) + ((unsigned int)elem * 14U);
}

/** @brief Trk resolve ofs.
 * @param ofs Parameter `ofs`.
 * @return Function result.
 */
static unsigned char* trk_resolve_ofs(unsigned short ofs)
{
	enum { TRKINFO_OFS_BASE = 6664, CAMERA_DATA_OFS_BASE = 3220 };
	unsigned int trkinfo_len = (unsigned int)1680u;
	unsigned int camera_data_len = (unsigned int)3444u;

	if ((unsigned int)ofs >= (unsigned int)TRKINFO_OFS_BASE &&
		(unsigned int)ofs < (unsigned int)(TRKINFO_OFS_BASE + trkinfo_len)) {
		return shapeinfos + ((unsigned int)ofs - (unsigned int)TRKINFO_OFS_BASE);
	}

	if ((unsigned int)ofs >= (unsigned int)CAMERA_DATA_OFS_BASE &&
		(unsigned int)ofs < (unsigned int)(CAMERA_DATA_OFS_BASE + camera_data_len)) {
		return track_camera_coords + ((unsigned int)ofs - (unsigned int)CAMERA_DATA_OFS_BASE);
	}

	return (unsigned char*)0;
}

/** @brief Trkobject info ptr.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
static unsigned char* trkobject_info_ptr(unsigned char elem)
{
	unsigned char* raw = trkobject_raw_entry(elem);
	unsigned short off = (unsigned short)raw[0] | ((unsigned short)raw[1] << 8);
	if (off == 0) {
		return (unsigned char*)0;
	}
	return trk_resolve_ofs(off);
}

/** @brief Trkobjinfo block.
 * @param base Parameter `base`.
 * @param block Parameter `block`.
 * @return Function result.
 */
static struct TRKOBJINFO_RAW* trkobjinfo_block(unsigned char* base, unsigned char block)
{
	return (struct TRKOBJINFO_RAW*)(base + ((unsigned int)block * 14u));
}

/** @brief Trkobjinfo camera ptr.
 * @param info Parameter `info`.
 * @return Function result.
 */
static short* trkobjinfo_camera_ptr(const struct TRKOBJINFO_RAW* info)
{
	return (short*)trk_resolve_ofs((unsigned short)info->si_cameraDataOffset);
}

/** @brief Trkobject multitile flag.
 * @param elem Parameter `elem`.
 * @return Function result.
 */
static unsigned char trkobject_multitile_flag(unsigned char elem)
{
	return trkobject_raw_entry(elem)[11];
}

#pragma pack(push, 1)
struct TCOMP_ENTRY {
	unsigned char tc_col;		 /* +0 */
	unsigned char tc_row;		 /* +1 */
	unsigned char tc_tileElem;	/* +2 */
	unsigned char tc_subBlock;	/* +3 */
	unsigned char tc_connStatus;	/* +4 */
	unsigned char tc_distCount;	/* +5 */
	unsigned char tc_prevCol;	/* +6 */
	unsigned char tc_prevRow;	/* +7 */
	unsigned char tc_prevElem;	/* +8 */
	unsigned char tc_prevSub;	/* +9 */
	unsigned char tc_prevConn;	/* +A */
	unsigned char tc_prevCode;	/* +B */
	short         tc_prevIdx;	/* +C */
};
#pragma pack(pop)

/** @brief Track setup.
 * @return Function result.
 */
int  track_setup(void)
{
	/* Snapshot td15 at entry to detect runtime overwrites */
	

	/* --- Local variable declarations matching ASM stack frame --- */
	unsigned char visited[904];		   /* var_738 */
	unsigned char subTOIBlockArr[904];	/* var_AD4..var_subTOIBlock area */
	unsigned char connStatusArr[904];	/* var_398 */

	unsigned char trkColIndex;		/* var_trkColIndex */
	unsigned char trkRowIndex;		/* var_trkRowIndex */
	unsigned char tileElem;			/* var_tileElem */
	unsigned char tileTerr;			/* var_tileTerr */
	unsigned char tileEntryPoint = 0;		/* var_tileEntryPoint */
	unsigned char prevConnCode;		/* var_prevConnCode */
	unsigned char sfCount;			/* var_sfCount */
	unsigned char sfPassCount;		/* var_4 */
	unsigned char matchCount;		/* var_2 */
	unsigned char trackErrorCode;		/* var_trackErrorCode */
	unsigned char loopFoundFlag;		/* var_AE8 */
	unsigned char subTOIBlock;		/* var_subTOIBlock */
	unsigned char MconnStatus;		/* var_MconnStatus */
	unsigned char MprevConnStatus = 0;		/* var_MprevConnStatus */
	unsigned char MprevColIndex = 0;		/* var_MprevColIndex */
	unsigned char MprevRowIndex = 0;		/* var_MprevRowIndex */
	unsigned char MprevTileElem = 0;		/* var_MprevTileElem */
	unsigned char prevSubBlock = 0;		/* var_3AA */
	unsigned char distCount;		/* var_3A8 */
	unsigned char McurrExitPoint;		/* var_McurrExitPoint */
	unsigned char opp3Val;			/* var_74C */
	unsigned char tcompDepth;		/* var_746 */
	signed char   connCheckFlag;		/* var_connCheckFlag */

	int trackDirection;			/* var_trackDirection */
	int prevPieceIdx;			/* var_3AC */
	int tempColExt;			/* var_AEA */
	int tempRowIdx2;			/* var_AEC */
	int tempSwap;			/* var_3A2 */

	int checkX, checkY, checkZ;		/* var_ADA, var_AD8, var_AD6 */

	/* Pointers */
	unsigned char* ptrTOInfo;		/* var_ptrTOInfo */
	struct TRKOBJINFO_RAW* ptrCurrTOInfo; /* var_ptrCurrTOInfo */
	short* cameraOfs;			/* var_ADE (near ptr) */

	/* tcomp buffer (-allocated) */
	struct TCOMP_ENTRY * tcompArr;	/* tcomp base pointer */
	struct TCOMP_ENTRY * tcompEntry;	/* current tcomp entry */

	/* Phase 10 temporaries */
	int chkPathIdx;				/* var_A */
	int chkTileElem;			/* var_C */

	int si, di;				/* register variables */

	/* ===== PHASE 1: Allocate tcomp buffer (64 * 14 = 896 bytes) ===== */
	tcompArr = (struct TCOMP_ENTRY *)mmgr_alloc_resbytes("tcomp", (long)TRACK_SETUP_TCOMP_BYTES);
	if (tcompArr == (struct TCOMP_ENTRY *)0L) {
		return 2;	/* ERR_INT_ERR */
	}

	/* ===== PHASE 2: Initialize ===== */
	sfCount = 0;
	sfPassCount = 0;
	track_pieces_counter = 0;

	/* Clear tile_obstacle_map */
	for (si = 0; si < TRACK_TRACKDATA_TOTAL_BYTES; si++) {
		tile_obstacle_map[si] = TRACK_U8_INVALID;
	}

	/* ===== PHASE 3: West-to-East terrain connectivity ===== */
	trkRowIndex = 0;
	while ((signed char)trkRowIndex < TRACK_SIZE) {
		prevConnCode = TRACK_CONN_CODE_SENTINEL;
		trkColIndex = 0;
		while ((signed char)trkColIndex < TRACK_SIZE) {
			tileTerr = track_terrain_map[
				terrainrows[(signed char)trkRowIndex] + (signed char)trkColIndex];
			if (terrConnDataEtoW[(unsigned char)tileTerr] != prevConnCode &&
				prevConnCode != TRACK_CONN_CODE_SENTINEL) {
				trackErrorCode = 11;	/* terr_mism */
				goto error_dispatch;
			}
			prevConnCode = terrConnDataWtoE[(unsigned char)tileTerr];
			trkColIndex++;
		}
		trkRowIndex++;
	}

	/* ===== PHASE 4: North-to-South terrain connectivity ===== */
	trkColIndex = 0;
	while ((signed char)trkColIndex < TRACK_SIZE) {
		prevConnCode = TRACK_CONN_CODE_SENTINEL;
		trkRowIndex = 0;
		while ((signed char)trkRowIndex < TRACK_SIZE) {
			tileTerr = track_terrain_map[
				terrainrows[(signed char)trkRowIndex] + (signed char)trkColIndex];
			if (terrConnDataNtoS[(unsigned char)tileTerr] != prevConnCode &&
				prevConnCode != TRACK_CONN_CODE_SENTINEL) {
				trackErrorCode = 11;	/* terr_mism */
				goto error_dispatch;
			}
			prevConnCode = terrConnDataStoN[(unsigned char)tileTerr];
			trkRowIndex++;
		}
		trkColIndex++;
	}

	/* ===== PHASE 5: Scan for start/finish ===== */
	trkRowIndex = 0;
	while ((signed char)trkRowIndex < TRACK_SIZE) {
		trkColIndex = 0;
		while ((signed char)trkColIndex < TRACK_SIZE) {
			tileElem = track_elem_map[
				trackrows[(signed char)trkRowIndex] + (signed char)trkColIndex];

			/* Clamp invalid elements */
			if (tileElem >= TRACK_MARKER_CORNER) {
				tileElem = 0;
			}

			/* Clamp high elements to 4 and patch map */
			if (tileElem >= TRACK_ELEM_MAX_VALID) {
				tileElem = 4;
				track_elem_map[
					trackrows[(signed char)trkRowIndex] + (signed char)trkColIndex] = 4;
			}

			/* Check for start/finish tile elements */
			switch ((unsigned int)tileElem) {
				case 1:
				case 134:
				case 147:
					track_angle = 0;
					goto found_sf;

				case 135:
				case 148:
				case 179:
					track_angle = 512;
					goto found_sf;

				case 136:
				case 149:
				case 180:
					track_angle = 256;
					goto found_sf;

				case 137:
				case 150:
				case 181:
					track_angle = 768;
					goto found_sf;

				default:
					goto not_sf;

				found_sf:
					if (sfCount != 0) {
						trackErrorCode = 3;	/* many_sf */
						goto error_dispatch;
					}
					startcol2 = trkColIndex;
					startrow2 = trkRowIndex;

					tileTerr = track_terrain_map[
						terrainrows[(signed char)trkRowIndex] + (signed char)trkColIndex];
					if (tileTerr == 6) {
						hillFlag = 1;
					} else {
						hillFlag = 0;
					}
					sfCount++;

				not_sf:
					;
			}

			trkColIndex++;
		}
		trkRowIndex++;
	}

	if (sfCount == 0) {
		trackErrorCode = 1;	/* no_sf */
		goto error_dispatch;
	}

	/* ===== PHASE 6: Initialize tracking arrays ===== */
	track_pieces_counter = 0;
	tcompDepth = 0;
	lap_checkpoint_counter = 0;
	game_exit_request_flag = 0;
	distCount = 0;
	loopFoundFlag = 0;

	for (si = 0; si < TRACK_TRACKDATA_TOTAL_BYTES; si++) {
		visited[si] = 0;
		track_waypoint_next[si] = -1;
		track_waypoint_alt[si] = -1;
	}

	/* Set start position */
	trkColIndex = startcol2;
	trkRowIndex = startrow2;
	trackDirection = track_angle;
	prevConnCode = 0;
	prevPieceIdx = -1;

	/* ===== PHASE 7: Main loop ===== */
main_loop:
	matchCount = 0;

	/* Bounds check */
	if ((signed char)trkColIndex < 0 ||
		(signed char)trkRowIndex < 0 ||
		(signed char)trkColIndex > TRACK_LAST_INDEX ||
		(signed char)trkRowIndex > TRACK_LAST_INDEX) {
		goto out_of_bounds;
	}
	goto process_tile;

out_of_bounds:
	/* Try backtracking */
	if (tcompDepth == 0) {
		goto track_complete;
	}

	tcompDepth--;
	tcompEntry = &tcompArr[tcompDepth];

	trkColIndex	 = tcompEntry->tc_col;
	trkRowIndex	 = tcompEntry->tc_row;
	tileElem	 = tcompEntry->tc_tileElem;
	subTOIBlock	 = tcompEntry->tc_subBlock;
	MconnStatus	 = tcompEntry->tc_connStatus;
	prevConnCode	 = tcompEntry->tc_prevCode;
	prevPieceIdx	 = tcompEntry->tc_prevIdx;
	distCount	 = tcompEntry->tc_distCount;
	MprevColIndex	 = tcompEntry->tc_prevCol;
	MprevRowIndex	 = tcompEntry->tc_prevRow;
	MprevTileElem	 = tcompEntry->tc_prevElem;
	prevSubBlock	 = tcompEntry->tc_prevSub;
	MprevConnStatus = tcompEntry->tc_prevConn;
	matchCount = 1;

check_match:
	if (matchCount == 0) {
		goto main_loop;
	}
	if (sfPassCount > 1) {
		trackErrorCode = 10;	/* long_jump */
		goto error_dispatch;
	}
	goto process_piece;

	/* --- Process tile --- */
process_tile:
	tempColExt = (signed char)trkColIndex;
	tempRowIdx2 = (signed char)trkRowIndex * 2;

	/* Read tileElem from map */
	tileElem = track_elem_map[
		trackrows[tempRowIdx2 / 2] + tempColExt];

	/* Wait: tempRowIdx2 = (signed char)trkRowIndex * 2 is used for word array index.
	   trackrows[tempRowIdx2 / 2] = trackrows[(signed char)trkRowIndex] */

	/* Read tileTerr */
	tileTerr = track_terrain_map[
		terrainrows[(signed char)trkRowIndex] + tempColExt];

/** @brief Hill.
 * @param tileTerr Parameter `tileTerr`.
 * @return Function result.
 */
	/* subst_hillroad_track if terrain is hill (7-10) and element is non-zero */
	if (tileElem != 0 && tileTerr != 0 &&
		tileTerr >= 7 && tileTerr < 11) {
		tileElem = subst_hillroad_track((unsigned char)tileTerr, (unsigned char)tileElem);
	}

/** @brief Elements.
 * @param TRACK_MARKER_CORNER Parameter `TRACK_MARKER_CORNER`.
 * @return Function result.
 */
	/* Handle filler elements (253, 254, 255) */
	if (tileElem >= TRACK_MARKER_CORNER) {
		switch ((unsigned int)tileElem) {
			case TRACK_MARKER_CORNER:
				/* NW corner filler - adjust both col and row */
				trkColIndex--;
				trkRowIndex--;
				switch (trackDirection) {
					case 0: tileEntryPoint = 12; break;
					case 256: tileEntryPoint = 0; break;
					case 512: tileEntryPoint = 0; break;
					case 768: tileEntryPoint = 9; break;
					default: break;
				}
				goto reread_tile;

			case TRACK_MARKER_VERTICAL:
				/* N side filler - adjust row only */
				trkRowIndex--;
				switch (trackDirection) {
					case 0: tileEntryPoint = 11; break;
					case 256: tileEntryPoint = 6; break;
					case 512: tileEntryPoint = 0; break;
					case 768: tileEntryPoint = 7; break;
					default: break;
				}
				goto reread_tile;

			case TRACK_MARKER_HORIZONTAL:
				/* W side filler - adjust col only */
				trkColIndex--;
				switch (trackDirection) {
					case 0: tileEntryPoint = 10; break;
					case 256: tileEntryPoint = 0; break;
					case 512: tileEntryPoint = 5; break;
					case 768: tileEntryPoint = 8; break;
					default: break;
				}
				goto reread_tile;

			default:
				goto reread_tile;
		}

	reread_tile:
		tileElem = track_elem_map[
			trackrows[(signed char)trkRowIndex] + (signed char)trkColIndex];
		goto after_entry_point;
	}

	/* Non-filler: compute entry point from trackDirection */
	switch (trackDirection) {
		case 0: tileEntryPoint = 2; break;
		case 256: tileEntryPoint = 4; break;
		case 512: tileEntryPoint = 1; break;
		case 768: tileEntryPoint = 3; break;
		default: break;
	}

after_entry_point:
	/* Validate: if sfPassCount==0 and entryPoint==0 -> int_err */
	if (sfPassCount == 0 && tileEntryPoint == 0) {
		trackErrorCode = 2;	/* int_err */
		goto error_dispatch;
	}

	/* ===== Search TRKOBJINFO blocks for matching entry/exit point ===== */
	matchCount = 0;
	ptrTOInfo = trkobject_info_ptr((unsigned char)tileElem);
	if (ptrTOInfo == (unsigned char*)0) {
		goto all_blocks_done;
	}

	for (si = 0; trkobjinfo_block(ptrTOInfo, 0)->si_noOfBlocks > (unsigned char)si; si++) {
		connCheckFlag = -1;	/* unmatched */
		ptrCurrTOInfo = trkobjinfo_block(ptrTOInfo, (unsigned char)si);

		/* Check entryPoint match */
		if (ptrCurrTOInfo->si_entryPoint == tileEntryPoint) {
			/* entryPoint matches, check entryType */
			if (ptrCurrTOInfo->si_entryType == prevConnCode) {
				connCheckFlag = 0;	/* normal match */
			} else {
				trackErrorCode = 4;	/* elem_mism */
				goto error_dispatch;
			}
		} else {
/** @brief Match.
 * @param tileEntryPoint Parameter `tileEntryPoint`.
 * @return Function result.
 */
			/* Try exit point match (reverse direction) */
			if (ptrCurrTOInfo->si_exitPoint == tileEntryPoint) {
				if (ptrCurrTOInfo->si_exitType == prevConnCode) {
					connCheckFlag = 1;	/* reverse match */
				} else {
					trackErrorCode = 4;	/* elem_mism */
					goto error_dispatch;
				}
			}
			/* else connCheckFlag stays -1 */
		}

		if (connCheckFlag < 0) {
			goto next_block;
		}

		/* Check if we've already visited this tile */
		if (visited[trackrows[(signed char)trkRowIndex] + (signed char)trkColIndex] != 0) {
			/* Already visited: scan existing pieces for a loop */
			for (di = 0; di < track_pieces_counter; di++) {
				/* Check col match */
				if (path_col[di] != trkColIndex)
					continue;
				/* Check row match */
				if (path_row[di] != trkRowIndex)
					continue;
				/* Check subTOIBlock match */
				if (subTOIBlockArr[di] != (unsigned char)si)
					continue;
				/* Check connStatus match */
				if (connStatusArr[di] != (unsigned char)connCheckFlag) {
					trackErrorCode = 5;	/* wrong_way */
					goto error_dispatch;
				}
				/* Match found - link it */
				connCheckFlag = -1;	/* mark as already linked */
				{
					short * linkPtr;
					linkPtr = &track_waypoint_next[prevPieceIdx];
					if (*linkPtr != -1) {
						/* td01 slot already used, use td02 instead */
						linkPtr = &track_waypoint_alt[prevPieceIdx];
					}
					*linkPtr = di;
				}
				if (di == 0) {
					loopFoundFlag = 1;
				}
				/* DON'T break - continue scanning for wrong_way errors */
			}
		}

	next_block:
		if (connCheckFlag < 0) {
			continue;	/* already linked or unmatched, skip to next block */
		}

		/* First match or additional match? */
		if (matchCount == 0) {
			/* First match: save it */
			subTOIBlock = (unsigned char)si;
			MconnStatus = (unsigned char)connCheckFlag;
		} else {
			/* Additional match: push to tcomp backtrack stack */
			if (tcompDepth >= TRACK_SETUP_DEPTH_MAX) {
				trackErrorCode = 8;	/* many_path */
				goto error_dispatch;
			}

			tcompEntry = &tcompArr[tcompDepth];
			tcompEntry->tc_col		 = trkColIndex;
			tcompEntry->tc_row		 = trkRowIndex;
			tcompEntry->tc_tileElem	 = tileElem;
			tcompEntry->tc_subBlock	 = (unsigned char)si;
			tcompEntry->tc_connStatus = (unsigned char)connCheckFlag;
			tcompEntry->tc_prevCode	 = prevConnCode;
			tcompEntry->tc_prevIdx	 = prevPieceIdx;
			tcompEntry->tc_distCount	 = distCount;
			tcompEntry->tc_prevCol	 = MprevColIndex;
			tcompEntry->tc_prevRow	 = MprevRowIndex;
			tcompEntry->tc_prevElem	 = MprevTileElem;
			tcompEntry->tc_prevSub	 = prevSubBlock;
			tcompEntry->tc_prevConn	 = MprevConnStatus;
			tcompDepth++;
		}
		matchCount++;
	}

all_blocks_done:
	/* All blocks checked */
	if (matchCount != 0) {
		goto check_match;
	}

	/* No match found */
	if (prevConnCode != 1) {
		goto out_of_bounds;	/* backtrack */
	}

	/* prevConnCode==1: runway extension logic */
	if (sfPassCount >= 2) {
		goto out_of_bounds;
	}
	if (distCount < 2) {
		trackErrorCode = 9;	/* no_runway */
		goto error_dispatch;
	}

	/* Extend runway */
	distCount++;
	sfPassCount++;

	switch (trackDirection) {
		case 0:	/* North */
			trkColIndex = MprevColIndex;
			trkRowIndex = MprevRowIndex - sfPassCount - 1;
			goto main_loop;

		case 512:	/* South */
			trkColIndex = MprevColIndex;
			trkRowIndex = MprevRowIndex + sfPassCount + 1;
			goto main_loop;

		case 256:	/* East */
			trkRowIndex = MprevRowIndex;
			trkColIndex = MprevColIndex + sfPassCount + 1;
			goto main_loop;

		case 768:	/* West */
			trkRowIndex = MprevRowIndex;
			trkColIndex = MprevColIndex - sfPassCount - 1;
			goto main_loop;

		default:
			goto main_loop;
	}

	/* ===== PHASE 8: Track complete ===== */
track_complete:
	if (!loopFoundFlag) {
		trackErrorCode = 7;	/* no_path */
		goto error_dispatch;
	}

	/* Set error position variables */
	sprite_render_state = startcol2;
	game_security_flag = startrow2;

	/* Compute checkpoint limit = min(track_pieces_counter / 3, 64) */
	si = track_pieces_counter / 3;
	if (si > 64) {
		si = 64;
	}
	game_exit_request_flag = (unsigned char)si;

	/* Clear subTOIBlockArr for checkpoint phase */
	for (si = 0; si < TRACK_TRACKDATA_TOTAL_BYTES; si++) {
		subTOIBlockArr[si] = 0;
	}

	/* Init checkpoint loop */
	di = 0;	/* checkpoint counter */
	si = 0;	/* iteration index */
	goto checkpoint_loop_entry;

	/* ===== PHASE 9: Process piece ===== */
process_piece:
	sfPassCount = 0;

	/* Mark tile as visited */
	visited[trackrows[(signed char)trkRowIndex] + (signed char)trkColIndex] = 1;

	/* Record subTOIBlock and connStatus for this piece */
	subTOIBlockArr[track_pieces_counter] = subTOIBlock;
	connStatusArr[track_pieces_counter] = MconnStatus;

	/* Link previous piece to current */
	if (prevPieceIdx != -1) {
		short * linkPtr;
		linkPtr = &track_waypoint_next[prevPieceIdx];
		if (*linkPtr != -1) {
			/* td01 already used, use td02 */
			linkPtr = &track_waypoint_alt[prevPieceIdx];
		}
		*linkPtr = track_pieces_counter;
	}

	/* Save current piece index as prevPieceIdx */
	prevPieceIdx = track_pieces_counter;

	/* Record col and row in path arrays */
	path_col[track_pieces_counter] = trkColIndex;
	path_row[track_pieces_counter] = trkRowIndex;

	/* Record connStatus and subTOIBlock in path_conn_flags (packed byte) */
	path_conn_flags[track_pieces_counter] = (MconnStatus << 4) + subTOIBlock;

	/* Record tileElem in td17 */
	track_elem_ordered[track_pieces_counter] = tileElem;

	/* Get ptrTOInfo and ptrCurrTOInfo for this tile/block */
	ptrTOInfo = trkobject_info_ptr((unsigned char)tileElem);
	ptrCurrTOInfo = trkobjinfo_block(ptrTOInfo, (unsigned char)subTOIBlock);

	/* Get opp3 value */
	opp3Val = ptrCurrTOInfo->si_opp3;

	if (opp3Val == 0) {
		/* opp3 == 0: increment distCount, skip checkpoint data */
		distCount++;
		goto after_checkpoint;
	}

	if (opp3Val == TRACK_U8_INVALID) {
		goto reset_dist;
	}

	if (distCount <= 3) {
		goto reset_dist;
	}

	if (lap_checkpoint_counter >= TRACK_CHECKPOINT_LIMIT_MAX) {
		goto reset_dist;
	}

	/* ---- Checkpoint data recording ---- */
	{
		struct TRKOBJINFO_RAW* currTOI;
		struct TRKOBJINFO_RAW* prevTOI;
		short* camSrc;
		short altCamOfs;

		/* Re-get ptrTOInfo/ptrCurrTOInfo for current tile */
		ptrTOInfo = trkobject_info_ptr((unsigned char)tileElem);
		currTOI = trkobjinfo_block(ptrTOInfo, (unsigned char)subTOIBlock);
		opp3Val = currTOI->si_opp3;

		/* Get ptrTOInfo/ptrCurrTOInfo for PREVIOUS tile */
		ptrTOInfo = trkobject_info_ptr((unsigned char)MprevTileElem);
		prevTOI = trkobjinfo_block(ptrTOInfo, (unsigned char)prevSubBlock);

		/* Get camera data pointer */
		if (MprevConnStatus != 0) {
			/* Check alternate camera data at offset +10 (si_opp1+si_opp2 as word) */
			altCamOfs = *(short*)(&prevTOI->si_opp1);
			if (altCamOfs != 0) {
				cameraOfs = (short*)trk_resolve_ofs((unsigned short)altCamOfs);
			} else {
				cameraOfs = trkobjinfo_camera_ptr(prevTOI);
			}
		} else {
			cameraOfs = trkobjinfo_camera_ptr(prevTOI);
		}

		/* Compute camera data entry address */
		if (MprevConnStatus != 0) {
			/* Reversed: cameraBase + arrowType * 12 + 12 */
			camSrc = (short*)((char*)cameraOfs +
					 (unsigned char)prevTOI->si_arrowType * 12 + 12);
		} else {
			/* Normal: cameraBase + arrowType * 12 + 6 */
			camSrc = (short*)((char*)cameraOfs +
					 (unsigned char)prevTOI->si_arrowType * 12 + 6);
		}

		/* Copy 3 words of camera data */
		checkX = camSrc[0];
		checkY = camSrc[1];
		checkZ = camSrc[2];

		/* Apply opp3 direction lookup */
		if (MconnStatus != 0) {
			opp3Val = animation_frame_transition_table[(signed char)opp3Val];
		} else {
			opp3Val = animation_state_advance_table[(signed char)opp3Val];
		}

		/* Get arrowOrient -> trackDirection */
		trackDirection = prevTOI->si_arrowOrient;

		/* Rotate camera data based on trackDirection */
		if (trackDirection == 768) {
			/* Rotate 270 deg: swap X<->Z, negate new X */
			tempSwap = checkX;
			checkX = -checkZ;
			checkZ = tempSwap;
		} else if (trackDirection == 512) {
			/* Rotate 180 deg: negate both X and Z */
			checkZ = -checkZ;
			checkX = -checkX;
		} else if (trackDirection == 256) {
			/* Rotate 90 deg: swap X<->Z, negate new Z */
			tempSwap = checkX;
			checkX = checkZ;
			checkZ = -tempSwap;
		}

		/* Store direction in td08 */
		if (MprevConnStatus != 0) {
			/* Reverse direction: XOR high byte with 2 */
			obstacle_rot_z[(signed char)lap_checkpoint_counter] =
				trackDirection ^ 512;
		} else {
			obstacle_rot_z[(signed char)lap_checkpoint_counter] =
				trackDirection;
		}

		/* Store opp3 in obstacle_scene_index */
		obstacle_scene_index[(signed char)lap_checkpoint_counter] = opp3Val;

		/* Get terrain for prev tile, add hill height if terrain==6 */
		if (track_terrain_map[
				terrainrows[(signed char)MprevRowIndex] +
				(signed char)MprevColIndex] == 6) {
			checkY += TRACK_HILL_HEIGHT_OFFSET;	/* hill height = 450 */
		}

		/* Store checkpoint Y in obstacle_world_pos[checkpointIdx*3 + 1] */
		{
			int chkIdx6 = (signed char)lap_checkpoint_counter * 3;

			obstacle_world_pos[chkIdx6 + 1] = checkY;

			/* Compute Z position */
			if (trkobject_multitile_flag((unsigned char)MprevTileElem) & 1) {
				/* Multi-tile (flag bit 0): use trackpos */
				checkZ += trackpos[(signed char)MprevRowIndex];
			} else {
				/* Single-tile: use trackcenterpos */
				checkZ += trackcenterpos[(signed char)MprevRowIndex];
			}
			obstacle_world_pos[chkIdx6 + 2] = checkZ;

			/* Compute X position */
			if (trkobject_multitile_flag((unsigned char)MprevTileElem) & 2) {
				/* Multi-tile (flag bit 1): use trackpos2+1 */
				checkX += trackpos2[(signed char)MprevColIndex + 1];
			} else {
				/* Single-tile: use trackcenterpos2 */
				checkX += trackcenterpos2[(signed char)MprevColIndex];
			}
			obstacle_world_pos[chkIdx6] = checkX;
		}

		/* Mark tile in tile_obstacle_map */
		tile_obstacle_map[trackrows[(signed char)MprevRowIndex] +
				(signed char)MprevColIndex] = lap_checkpoint_counter;

		lap_checkpoint_counter++;	/* checkpoint counter++ */
	}

reset_dist:
	distCount = 0;

after_checkpoint:
	/* Increment track_pieces_counter */
	track_pieces_counter++;
	if (track_pieces_counter >= TRACK_TRACKDATA_TOTAL_BYTES) {
		trackErrorCode = 6;	/* many_elem */
		goto error_dispatch;
	}

	/* Get exit direction for current piece */
	ptrTOInfo = trkobject_info_ptr((unsigned char)tileElem);
	ptrCurrTOInfo = trkobjinfo_block(ptrTOInfo, (unsigned char)subTOIBlock);

	/* Determine exit point and type based on connStatus */
	if (MconnStatus != 0) {
		/* Reversed: use entryPoint as exit */
		McurrExitPoint = ptrCurrTOInfo->si_entryPoint;
		prevConnCode = ptrCurrTOInfo->si_entryType;
	} else {
		/* Normal: use exitPoint */
		McurrExitPoint = ptrCurrTOInfo->si_exitPoint;
		prevConnCode = ptrCurrTOInfo->si_exitType;
	}

	/* Save current state as prev */
	MprevColIndex = trkColIndex;
	MprevRowIndex = trkRowIndex;
	MprevConnStatus = MconnStatus;
	prevSubBlock = subTOIBlock;
	MprevTileElem = tileElem;

/** @brief Point.
 * @param McurrExitPoint Parameter `McurrExitPoint`.
 * @return Function result.
 */
	/* Dispatch on exit point (1-12) */
	switch (McurrExitPoint) {
		case 1:	/* N */
			trkRowIndex--;
			trackDirection = 0;
			break;
		case 2:	/* S */
			trkRowIndex++;
			trackDirection = 512;
			break;
		case 3:	/* E */
			trkColIndex++;
			trackDirection = 256;
			break;
		case 4:	/* W */
			trkColIndex--;
			trackDirection = 768;
			break;
		case 5:	/* NE */
			trkRowIndex--;
			trkColIndex++;
			trackDirection = 0;
			break;
		case 6:	/* SW */
			trkRowIndex++;
			trkColIndex--;
			trackDirection = 768;
			break;
		case 7:	/* SE via E */
			trkColIndex++;
			trkRowIndex++;
			trackDirection = 256;
			break;
		case 8:	/* E+col=2 */
			trkColIndex += 2;
			trackDirection = 256;
			break;
		case 9:	/* E+col=2, S */
			trkColIndex += 2;
			trkRowIndex++;
			trackDirection = 256;
			break;
		case 10:	/* SE */
			trkColIndex++;
			trkRowIndex++;
			trackDirection = 512;
			break;
		case 11:	/* S, row+=2 */
			trkRowIndex += 2;
			trackDirection = 512;
			break;
		case 12:	/* SE, row+=2 */
			trkColIndex++;
			trkRowIndex += 2;
			trackDirection = 512;
			break;
		default:
			break;
	}
	goto main_loop;

	/* ===== PHASE 10: Checkpoint computation loop ===== */
checkpoint_loop_entry:
	while (si < (signed char)game_exit_request_flag) {
		int mapIdx;
		unsigned char* visitedPtr;
		struct TRKOBJINFO_RAW* subBlockInfo;

		/* Compute path index: si * track_pieces_counter / checkpointLimit */
		chkPathIdx = track_pieces_counter * si / (int)(signed char)game_exit_request_flag;

		/* Read col/row from path */
		trkColIndex = path_col[chkPathIdx];
		trkRowIndex = path_row[chkPathIdx];

		/* Check if already processed (subTOIBlockArr used as visited here) */
		mapIdx = terrainrows[(signed char)trkRowIndex] + (signed char)trkColIndex;
		visitedPtr = &subTOIBlockArr[mapIdx];
		if (*visitedPtr != 0) {
			si++;
			continue;	/* already processed, skip */
		}
		*visitedPtr = 1;	/* mark as processed */

		/* Read tile element from path */
		chkTileElem = (unsigned char)track_elem_ordered[chkPathIdx];

		/* Read packed path_conn_flags entry */
		subTOIBlock = path_conn_flags[chkPathIdx] & 15;
		connCheckFlag = path_conn_flags[chkPathIdx] & 16;

		/* Get ptrCurrTOInfo */
		ptrTOInfo = trkobject_info_ptr((unsigned char)chkTileElem);
		ptrCurrTOInfo = trkobjinfo_block(ptrTOInfo, 0);

		if (connCheckFlag != 0) {
			/* connCheckFlag!=0: check for alternate camera data */
			subBlockInfo = trkobjinfo_block(ptrTOInfo, (unsigned char)subTOIBlock);
			{
				short altCam = *(short*)(&subBlockInfo->si_opp1);
				if (altCam != 0) {
					cameraOfs = (short*)trk_resolve_ofs((unsigned short)altCam);
					goto use_camera_ofs;
				}
			}
			/* Fall through to normal camera data */
		}

		/* Normal camera data: get from sub-block */
		{
			subBlockInfo = trkobjinfo_block(ptrTOInfo, (unsigned char)subTOIBlock);
			cameraOfs = trkobjinfo_camera_ptr(subBlockInfo);
		}

	use_camera_ofs:
		{
			short* camSrc;
			int camDir;

			/* Get arrowType for camera lookup from sub-block */
			subBlockInfo = trkobjinfo_block(ptrTOInfo, (unsigned char)subTOIBlock);

			/* Camera data: cameraBase + arrowType * 12 (no extra offset in Phase 10) */
			camSrc = (short*)((char*)cameraOfs +
					 (unsigned char)subBlockInfo->si_arrowType * 12);

			/* Copy 3 words of camera data */
			checkX = camSrc[0];
			checkY = camSrc[1];
			checkZ = camSrc[2];

			/* Get arrowOrient */
			camDir = subBlockInfo->si_arrowOrient;
			trackDirection = camDir;

			/* Rotate camera data based on arrowOrient */
			if (camDir == 768) {
				tempSwap = checkX;
				checkX = -checkZ;
				checkZ = tempSwap;
			} else if (camDir == 512) {
				checkZ = -checkZ;
				checkX = -checkX;
			} else if (camDir == 256) {
				tempSwap = checkX;
				checkX = checkZ;
				checkZ = -tempSwap;
			}

			/* Check terrain for hill height */
			if (track_terrain_map[
					terrainrows[(signed char)trkRowIndex] +
					(signed char)trkColIndex] == 6) {
				/* Hill: set track_element_height_ofs to 450 */
				((short *)track_element_height_ofs)[di] = TRACK_HILL_HEIGHT_OFFSET;
			} else {
				/* Not hill: set track_element_height_ofs to 0 */
				((short *)track_element_height_ofs)[di] = 0;
			}

			/* Store in track_cam_height_base (always 0) */
			((short *)track_cam_height_base)[di] = 0;

			/* waypoint_world_pos[di*3 + 1] = track_element_height_ofs[di] + camData[1] */
			waypoint_world_pos[di * 3 + 1] =
				((short *)track_element_height_ofs)[di] + checkY;

/** @brief Position.
 * @param chkTileElem Parameter `chkTileElem`.
 * @return Function result.
 */
			/* Compute Z position (waypoint_world_pos[di*3 + 2]) */
			if (trkobject_multitile_flag((unsigned char)chkTileElem) & 1) {
				/* Multi-tile */
				waypoint_world_pos[di * 3 + 2] =
					trackpos[(signed char)trkRowIndex] + checkZ;
			} else {
				/* Single-tile */
				waypoint_world_pos[di * 3 + 2] =
					trackcenterpos[(signed char)trkRowIndex] + checkZ;
			}

/** @brief Position.
 * @param chkTileElem Parameter `chkTileElem`.
 * @return Function result.
 */
			/* Compute X position (waypoint_world_pos[di*3]) */
			if (trkobject_multitile_flag((unsigned char)chkTileElem) & 2) {
				/* Multi-tile */
				waypoint_world_pos[di * 3] =
					trackpos2[(signed char)trkColIndex + 1] + checkX;
			} else {
				/* Single-tile */
				waypoint_world_pos[di * 3] =
					trackcenterpos2[(signed char)trkColIndex] + checkX;
			}

			di++;	/* checkpoint counter++ */
		}

		si++;	/* iteration index++ */
	}

	/* ===== PHASE 11: Cleanup and return ===== */
	game_exit_request_flag = (unsigned char)di;
	trackErrorCode = 0;	/* success */
	goto cleanup;

	/* --- Error handlers --- */
error_dispatch:
	if (trkColIndex == TRACK_U8_INVALID) {
		trkColIndex = 0;
		goto error_handler_B;
	}
	/* error_handler_A */
	if (trkColIndex == TRACK_SIZE) {
		trkColIndex = TRACK_LAST_INDEX;
	}
error_handler_B:
	if (trkRowIndex == TRACK_U8_INVALID) {
		trkRowIndex = 0;
	} else if (trkRowIndex == TRACK_SIZE) {
		trkRowIndex = TRACK_LAST_INDEX;
	}
	sprite_render_state = trkColIndex;
	game_security_flag = trkRowIndex;

cleanup:
	/* Snapshot td15 before exiting track_setup */
	
	mmgr_release((char *)tcompArr);
	return (signed char)trackErrorCode;
}

/** @brief Read u16.
 * @param p Parameter `p`.
 * @return Function result.
 */
static uint16_t read_u16(const unsigned char* p)
{
	return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/** @brief State trackobject raw decode.
 * @param table Parameter `table`.
 * @param index Parameter `index`.
 * @param out Parameter `out`.
 * @return Function result.
 */
int state_trackobject_raw_decode(const unsigned char* table, unsigned int index, state_trackobject_raw* out)
{
	const unsigned char* p;

	if (table == 0 || out == 0) {
		return 0;
	}

	p = table + ((unsigned int)STATE_TRACKOBJECT_RAW_SIZE * index);
	out->trkobj_info_ofs = read_u16(p + 0);
	out->rot_y = (int16_t)read_u16(p + 2);
	out->shape_ofs = read_u16(p + 4);
	out->lo_shape_ofs = read_u16(p + 6);
	out->overlay = p[8];
	out->surface_type = p[9];
	out->ignore_z_bias = p[10];
	out->multi_tile_flag = p[11];
	out->physical_model = p[12];
	out->scene_reserved5 = p[13];
	return 1;
}

/** @brief State trkobjinfo raw decode.
 * @param table Parameter `table`.
 * @param index Parameter `index`.
 * @param out Parameter `out`.
 * @return Function result.
 */
int state_trkobjinfo_raw_decode(const unsigned char* table, unsigned int index, state_trkobjinfo_raw* out)
{
	const unsigned char* p;

	if (table == 0 || out == 0) {
		return 0;
	}

	p = table + ((unsigned int)STATE_TRKOBJINFO_RAW_SIZE * index);
	out->no_of_blocks = p[0];
	out->entry_point = p[1];
	out->exit_point = p[2];
	out->entry_type = p[3];
	out->exit_type = p[4];
	out->arrow_type = p[5];
	out->arrow_orient = (int16_t)read_u16(p + 6);
	out->camera_data_ofs = read_u16(p + 8);
	out->opp1 = p[10];
	out->opp2 = p[11];
	out->opp3 = p[12];
	out->opp_sped_code = p[13];
	return 1;
}
