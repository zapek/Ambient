/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: gfx_cmap.c,v 1.8 2013/10/28 11:15:22 geit Exp $
 */

#include "ambient.h"

/* public */
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#if USE_LEGACY
ULONG RemapMapColours(struct BitMap *src, struct BitMap *dst, UBYTE *ar1, UBYTE *ar2, struct ColorMap *cm, ULONG flags);
#else
#include <proto/cgxdither.h>
#endif

/* private */
#include "gfx.h"
#include "gfx_cmap.h"
#include "gfx_bitmap.h"


APTR gfx_cmap_create(CONST ULONG *cmap32, ULONG colornum)
{
	struct ColorMap *cm;

	ASSERT(cmap32);
	ASSERT(colornum);

	if ( (cm = GetColorMap(colornum)) )
	{
		ULONG *array;

		array = malloc(sizeof(*array) * (colornum * 3 + 2));
		if (array)
		{
			struct ViewPort fakevp;

			array[0] = (colornum << 16) | 0;
			memcpy(&array[1], cmap32, colornum * 3 * sizeof(*array));
			array[1 + colornum * 3] = (0 << 16) | 0;

			/* Neat trick to avoid using SetRGB32CM */
			InitVPort(&fakevp);
			fakevp.ColorMap = cm;
			LoadRGB32(&fakevp, array);

			free(array);
		}
		else
		{
			FreeColorMap(cm);
			cm = NULL;
		}
	}
	return (cm);
}


void gfx_cmap_delete(APTR ctx)
{
	struct ColorMap *cm = ctx;

	ASSERT(cm);
	FreeColorMap(cm);
}


/*
 * Remaps using a colormap.
 */
ULONG gfx_cmap_remap(APTR sbm, APTR tbm, APTR cm)
{
	ASSERT(sbm);
	ASSERT(tbm);
	ASSERT(cm);

	RemapMapColours(gfx_bitmap_bm(sbm), gfx_bitmap_bm(tbm), 0, 0, (struct ColorMap *)cm, 0);

	return (TRUE);
}
