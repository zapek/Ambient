/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * $Id: gfx_analyze.c,v 1.2 2012/08/02 15:37:39 geit Exp $
 */

#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

/* private */
#include "gfx_analyze.h"


/*  analyzes a limited area of a given rastport to calculate its average 
 *  brightness
 */
ULONG gfx_analyze_average_brightness (struct RastPort *rp, ULONG x, ULONG y, ULONG w, ULONG h)
{
	ULONG ix, iy, count = 0;
	ULONG colsum_r = 0, colsum_g = 0, colsum_b = 0;
	ULONG pixels[ 128 ];

	ASSERT( w <= 128 )

	if ( w > 128 )
	{
		w = 128;
	}

	for (iy = y; iy < h + y; iy++)
	{
		ReadPixelArray( pixels, 0, 0, w * 4, rp, x, iy, w, 1, RECTFMT_RGBA );

		for (ix = 0; ix < w; ix++)
		{
			ULONG pixel = pixels[ ix ];

			colsum_r += (pixel >> 24) & 0xff;
			colsum_g += (pixel >> 16) & 0xff;
			colsum_b += (pixel >> 8 ) & 0xff;
		}
	}

	count = w * h * 3;

	return ( colsum_r + colsum_g + colsum_b ) / count;
}

/*  analyzes a limited area of a given rastport to calculate its average
 *  alpha
 */
ULONG gfx_analyze_average_alpha (struct RastPort *rp, ULONG x, ULONG y, ULONG w, ULONG h)
{
	ULONG ix, iy, count = 0;
	ULONG colsum_a = 0;
	ULONG pixels[ 128 ];

	ASSERT( w <= 128 )

	if ( w > 128 )
	{
		w = 128;
	}

	for (iy = y; iy < h + y; iy++)
	{
		ReadPixelArray( pixels, 0, 0, w * 4, rp, x, iy, w, 1, RECTFMT_RGBA );

		for (ix = 0; ix < w; ix++)
		{
			ULONG pixel = pixels[ ix ];

			colsum_a += (pixel ) & 0xff;
		}
	}

	count = w * h;

	return ( colsum_a) / count;
}

