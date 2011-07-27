/*************************************************************************
 *                                                                       *
 * Copyright (c) 1992-2011 Ronald Joe Record                           *
 *                                                                       *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************/

#include "x.h"

#define ABS(a)	(((a)<0) ? (0-(a)) : (a) )
#define Min(x,y) ((x < y)?x:y)
#define Max(x,y) ((x > y)?x:y)

/* Useful mathematical constants that should have been defined in math.h 
 * M_LOG2E	- log2(e)
 * M_LN2        - ln(2)
 * M_PI		- pi
 */
#ifndef M_LOG2E
#define M_LOG2E	1.4426950408889634074
#endif
#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif
#ifndef M_LN2
#define M_LN2      6.9314718055994530942E-1 /*Hex  2^-1 * 1.62E42FEFA39EF */
#endif

/* Useful machine-dependent values that should have been defined in values.h
 * LN_MAXDOUBLE - the natural log of the largest double  -- log(MAXDOUBLE)
 * LN_MINDOUBLE - the natural log of the smallest double -- log(MINDOUBLE)
 */
#ifndef LN_MINDOUBLE
#define LN_MINDOUBLE (M_LN2 * (DMINEXP - 1))
#endif
#ifndef LN_MAXDOUBLE
#define LN_MAXDOUBLE (M_LN2 * DMAXEXP)
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define MAXFRAMES 4
#define DEF_WIDTH 4.0
#define DEF_HEIGHT 3.0
#define DEF_M_WIDTH 3.0
#define DEF_M_HEIGHT 3.0

#define MAXDIVS  12
#define STRIPE 7

typedef struct {
	int x, y;
} xy_t;

typedef struct {
	int start_x, start_y;
	int last_x, last_y;
	} rubber_band_data_t;

typedef struct {
	Cursor band_cursor;
	double p_min, p_max, q_min, q_max;
	rubber_band_data_t rubber_band;
	} image_data_t;

typedef int (*PFI)();
