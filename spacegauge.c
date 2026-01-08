/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
 * spacegauge.c, Copyright 2005 by Adam Waldenberg
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
 * $Id: spacegauge.c,v 1.2 2006/09/18 23:17:31 fab Exp $
 */

#include "ambient.h"

/* public */
#include <graphics/rastport.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>

/* private */
#include "gfx_bitmap.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "spacegauge.h"

#define SPACEGAUGE_COL 0xFFDDDDDD
#define SPACEGAUGE_X 9 /* X position in a 10x grid. */
#define SPACEGAUGE_Y 6 /* Y position in a 10y grid. */
#define SPACEGAUGE_W 6 /* 1/x of the total width.   */
#define SPACEGAUGE_H 3 /* 1/x of the total height.  */

static inline ULONG getgaugecol(LONG percent)
{
	ULONG outcol = 0xFF000000;
	UBYTE r = (SPACEGAUGE_COL >> 16) & 0xFF;
	UBYTE g = (SPACEGAUGE_COL >> 8) & 0xFF;
	UBYTE b = SPACEGAUGE_COL & 0xFF;

	if (percent < 90)
		return SPACEGAUGE_COL;
	else
	{
		outcol |= (r + (((0xDD - r) / 10) * (10 - (100 - percent)))) << 16;
		outcol |= (g - ((g / 10) * (10 - (100 - percent)))) << 8;
		outcol |= b - ((b / 10) * (10 - (100 - percent)));
	}

	return outcol;
}

void spacegauge_draw( struct RastPort *drawrp, LONG origin_x, LONG xw, LONG origin_y, LONG percent, ULONG alpha)
{
	LONG x = (origin_x / 10) * SPACEGAUGE_X - 1;
	LONG y = (origin_y / 10) * SPACEGAUGE_Y - 1;
	LONG w = xw / SPACEGAUGE_W;
	LONG h = origin_y / SPACEGAUGE_H;
	DOUBLE h2 = (((DOUBLE) (h - 4)) / 100);

	struct RastPort temprp;
	APTR tempbm = NULL;
	struct RastPort *rp = drawrp;

	/* Are we out-of-bounds? */
	x = x + w > origin_x ? origin_x - w : x;
	y = y + h > origin_y ? origin_y - h : y;

	/* We only draw if there is any sense in doing so... */
	if (w - 4 <= 0 || h - 4 <= 0 || percent == -1)
		return;

	if ( alpha != 0xffffffff )
	{
		tempbm = gfx_bitmap_create(w + 1, h + 1, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, BITMAPTAG_Clear, TRUE, TAG_DONE);

		if ( !tempbm )
			return;

		InitRastPort( &temprp );
		rp = &temprp;
		rp->BitMap = gfx_bitmap_bm(tempbm);
		x = 0;
		y = 0;
	}

	FillPixelArray(rp, x, y, w, h, SPACEGAUGE_COL);
	FillPixelArray(rp, x + 1, y + 1, w - 2, h - 2, 0xFF000000);
	FillPixelArray(rp, x + 2, y + 2, w - 4, h - 4, getgaugecol(percent));
	FillPixelArray(rp, x + 2, y + 2, w - 4, h2 * (100 - percent), 0xFF000000);

	if ( tempbm )
	{
		gfx_blit(tempbm, drawrp,
			BLITTAG_DstType, BLITVAL_DstType_RastPort,
			BLITTAG_DstX, (origin_x / 10) * SPACEGAUGE_X - 1,
			BLITTAG_DstY, (origin_y / 10) * SPACEGAUGE_Y - 1,
			BLITTAG_Alpha, alpha,
		TAG_DONE);

		gfx_bitmap_delete( tempbm );
	}
}
