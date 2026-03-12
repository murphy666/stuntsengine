#ifndef CARSTATE_H
#define CARSTATE_H

/*
 * Car physics lookup tables — defined in src/data_menu_track.c.
 * Include this header in any translation unit that reads these tables.
 */

/* Per-wheel grip coefficients (4 wheels × 2 bytes each, little-endian pairs) */

#ifdef DATA_MENU_TRACK_IMPL
#  define _CS_
#else
#  define _CS_ extern
#endif

_CS_ unsigned char  wheel_rating_coefficients[8];

/* Grass-surface deceleration divisors indexed by number of wheels on grass (0-4) */
_CS_ unsigned short grassDecelDivTab[];

/* Corner X-flip flags for collision position rotation (4 corners) */
_CS_ short          position_rotation_matrix[];

/* Camera reference position/direction vector (4 elements) */
_CS_ short          camera_position_vector[];

#undef _CS_

#endif /* CARSTATE_H */
