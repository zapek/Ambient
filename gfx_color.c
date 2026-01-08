/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: gfx_color.c,v 1.3 2006/02/22 14:48:19 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/graphics.h>

/* private */
#include "gfx.h"
#include "gfx_color.h"
#include "gfx_bitmap.h"


#if 0

/*
 * Transforms all the pixel having the color 'cf'
 * to 'ct'. Format is 00RRGGBB.
 */
void gfx_color_transform(APTR bm, ULONG width, ULONG height, ULONG cf, ULONG ct)
{
	APTR mbm;

	CHECKCYBERMAP(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);

	if (mbm = gfx_bitmap_create(width, height, 1, TAG_DONE))
	{
		struct RastPort rp;

		InitRastPort(&rp);
		rp.BitMap = gfx_bitmap_bm(bm);

		if (ExtractColor(&rp, gfx_bitmap_bm(mbm), cf, 0, 0, width, height))
		{
			APTR tbm;

			if (tbm = gfx_create_bitmap(width, height, gfx_bitmap_depth(bm), BITMAPTAG_Friend, bm, TAG_DONE))
			{
				rp.BitMap = gfx_bitmap_bm(tbm);
				FillPixelArray(&rp, 0, 0, width, height, ct);

				rp.BitMap = gfx_bitmap_bm(bm);
				gfx_blit(tbm, bm,
					BLITTAG_Minterm, 0xe0,
					BLITTAG_MaskPlane, gfx_bitmap_array(mbm),
				TAG_DONE);

				gfx_bitmap_delete(tbm);
			}
		}
		gfx_bitmap_delete(mbm);
	}
}


/*
 * Returns the value of one unit to change from the
 * least visible color component change (blue).
 */
ULONG gfx_color_unit(APTR bm)
{
	CHECKCYBERMAP(gfx_bitmap_bm(bm));

	if (gfx_bitmap_depth(bm) > 16)
	{
		return (1);
	}
	return (8);
}

#endif
