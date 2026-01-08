/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005 Ambient Open Source Team
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
 * $Id: gfx_linedraw.c,v 1.1 2006/02/22 14:48:20 fab Exp $
 */

#include "ambient.h"
#include "gfx_linedraw.h"

#include <graphics/rastport.h>
#include <proto/cybergraphics.h>

#define	Int(x) ((x)>>8)
#define	Fix(x) ((x)<<8)
#define	Ceil(x) ((((x)>>8)+1)<<8)
#define	CeilInt(x) (((x)>>8)+1)
#define	Floor(x) (((x)>>8)<<8)
#define ALP(x) (((x) & 0xff000000) >> 24)
#define RED(x) (((x) & 0x00ff0000) >> 16)
#define GRN(x) (((x) & 0x0000ff00) >> 8)
#define BLU(x) ((x) & 0x000000ff)
#define	SWAP_COORDS {int t; \
t = x1; x1 = x2; x2 = t; \
t = y1; y1 = y2; y2 = t; \
xd = x2 - x1; yd = y2 - y1;}

static inline int frac(int x)
{
	return (x & 0x000000ff);
}

static inline int invfrac(int x)
{
	return 256 - frac(x);
}

static inline unsigned int blend32(unsigned int v1, unsigned int v2, int bl)
{
	int a,r,g,b;

	r = RED(v1);
	g = GRN(v1);
	b = BLU(v1);
	a = ALP(v1);
	r = r + (((RED(v2) - r) * bl) >> 8);
	g = g + (((GRN(v2) - g) * bl) >> 8);
	b = b + (((BLU(v2) - b) * bl) >> 8);
	a = a + (((ALP(v2) - b) * bl) >> 8);

	return (a << 24) | (r << 16) | (g << 8) | b;
}

void gfx_linedraw_aa(struct RastPort *rp, int x1, int y1, int x2, int y2, unsigned int color)
{
	int grad;
	int xd, yd, xf, yf;
	int bright1, bright2;
	int x, y, ix1, ix2, iy1, iy2;

	x1 <<= 8; y1 <<= 8;
	x2 <<= 8; y2 <<= 8;
	xd = x2 - x1; /* width and height of the line */
	yd = y2 - y1;

	if (abs(xd) > abs(yd)) /* check line gradient */
	{
		/* horizontal(ish) lines*/
		if (x1 > x2) /* if the line is back to front  */
		{
			SWAP_COORDS
		}

		if (xd)
		{
			grad = (yd << 12) / xd; /* gradient of the line (24:8) */
			grad = grad << 4;
		}
		else grad = 0;

		ix1 = Int(x1);
		ix2 = Int(x2);
		yf = y1 << 8;

		/* main loop */
		for (x = ix1; x <= ix2; x++)
		{
			bright1 = invfrac(yf >> 8);
			bright2 = 255 - bright1;
			WriteRGBPixel(rp, x, Int(yf >> 8), blend32(ReadRGBPixel(rp, x, Int(yf >> 8)), color, bright1));
			WriteRGBPixel(rp, x, (Int(yf >> 8) +1), blend32(ReadRGBPixel(rp, x, (Int(yf >> 8) + 1)), color, bright2));
			yf += grad;
		}
	}
	else
	{
		/* vertical(ish) lines */
		if (y1 > y2) /* if the line is bottom to top */
		{
			SWAP_COORDS
		}

		if (yd)
		{
			grad = (xd << 12) / yd; /* gradient of the line (24:8) */
			grad = grad << 4;
		}
		else grad = 0;

		iy1 = Int(y1);
		iy2 = Int(y2);
		xf = x1 << 8;

		/* main loop */
		for (y = iy1; y <= iy2; y++)
		{
			bright1 = invfrac(xf >> 8);
			bright2 = 255 - bright1;
			WriteRGBPixel(rp, Int(xf >> 8), y, blend32(ReadRGBPixel(rp, Int(xf >> 8), y), color, bright1));
			WriteRGBPixel(rp, Int(xf >> 8) + 1, y, blend32(ReadRGBPixel(rp, Int(xf >> 8) + 1, y), color, bright2));
			xf += grad;
		}
	}
}

#undef SWAP_COORDS

