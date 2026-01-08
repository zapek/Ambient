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
 * $Id: gfx_mask.c,v 1.6 2006/08/08 13:31:34 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/graphics.h>

/* private */
#include "gfx.h"
#include "gfx_mask.h"
#include "gfx_bitmap.h"


/*
 * Creates a mask from a planar bitmap.
 */
APTR gfx_mask_create_planar(APTR bm, ULONG width, ULONG height, ULONG depth)
{
	APTR mbm;

	CHECKPLANAR(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);
	ASSERT(depth);

	if ((mbm = gfx_bitmap_create(width, height, 1, BITMAPTAG_Clear, TRUE, TAG_DONE)))
	{
		ULONG i, j;
		UBYTE *p;

		struct BitMap tmpbm;

		InitBitMap(&tmpbm, depth, width, height);

		/*
		 * Stuff all the planes and BltBitMapRastPort()
		 * with an ORing minterm to get the resulting
		 * mask.
		 */
		for (i = 0; i < depth; i++)
		{
			tmpbm.Planes[i] = gfx_bitmap_array(mbm);
		}

		BltBitMap(gfx_bitmap_bm(bm), 0, 0, &tmpbm, 0, 0, width, height, 0xe0, 0xff, NULL);

		p = gfx_bitmap_array(mbm);

		#ifdef DEBUG
		if (db_a[DB_DUMPIMAGE].active)
		{
			PDB(("width: %ld, height: %ld, depth: %ld, dumping mask:\n", width, height, depth));
			dump_image(p, width * height / 8, 7);
		}
		#endif

		/*
		 * If the icon has more than 2 colors, chances are it
		 * needs some special handling to have to be displayed
		 * properly.
		 * We compute 2 markers which give the first and
		 * last bits of a line which are relevant. Bits between
		 * them are filled in. Although this algo seems dodgy,
		 * it fits perfectly for real world icons and is fast.
		 */
		if (depth > 1)
		{
			ULONG s, e;
			ULONG prevbits, prevbytes;
			UBYTE m;

			for (i = 0; i < height; i++)
			{
				prevbits = i * gfx_bitmap_modulo(bm) * 8;
				prevbytes = (prevbits & ~7) >> 3;

				m = 1 << (7 - (prevbits % 8));

				/* beginning */
				for (s = 0; s < width; s++, (m == 1) ? (m = 1 << 7) : (m >>= 1))
				{
					if (*(p + prevbytes + ((s & ~7) >> 3)) & m)
					{
						break;
					}
				}

				m = 1 << (7 - ((prevbits + width) % 8));

				/* end */
				for (e = width; e; e--, (m == (1 << 7)) ? (m = 1) : (m <<= 1))
				{
					if (*(p + prevbytes + ((e & ~7) >> 3)) & m)
						break;
				}

				m = 1 << (7 - ((prevbits + s) % 8));

				for (j = s; j < e; j++, (m == 1) ? (m = 1 << 7) : (m >>= 1))
				{
					*(p + prevbytes + ((j & ~7) >> 3)) |= m;
				}
			}
		}
	}
	return (mbm);
}


#if 0
/*
 * This one takes a *mask* as input and creates an inverted mask.
 */
APTR gfx_mask_create_inverted(APTR bm, ULONG width, ULONG height)
{
	APTR mbm;

	CHECKMASK(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);

	if (mbm = gfx_bitmap_create(width, height, 1, BITMAPVAL_Clear, TRUE, TAG_DONE))
	{
		gfx_blit(bm, mbm, BLITTAG_Minterm, 0x30, TAG_DONE);
	}
	return (mbm);
}
#endif


/*
 * Creates a mask from a CLUT chunky source.
 * 'bgvalue' is the value in chunky which means transparent (usually 0).
 */
APTR gfx_mask_create_chunky8(UBYTE * chunky, ULONG width, ULONG height, ULONG bgvalue)
{
	APTR mbm;

	ASSERT(chunky);
	ASSERT(width);
	ASSERT(height);

	if ((mbm = gfx_bitmap_create(width, height, 1, BITMAPTAG_Clear, TRUE, TAG_DONE)))
	{
		ULONG i, j;
		UBYTE shift;
		UBYTE *p = gfx_bitmap_array(mbm);
		UBYTE *maskline;

		maskline = p;

		for (j = 0; j < height; j++)
		{
			p = maskline;
			shift = (1L << 7);

			for (i = 0; i < width; i++)
			{
				if (!shift)
				{
					shift = (1L << 7);
					p++;
				}

				if (*chunky++ != bgvalue)
				{
					*p |= shift;
				}
				shift >>= 1;
			}
			maskline += gfx_bitmap_modulo(mbm);
		}
	}
	return (mbm);
}


#if USE_SOLIDDRAG
/*
 * Only for non planar CGX bitmaps in fastram.
 */
APTR gfx_mask_create_cgx_fastram(APTR bm, ULONG width, ULONG height, ULONG bgvalue)
{
	APTR mbm;

	CHECKCYBERMAP(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);

	if (mbm = gfx_bitmap_create(width, height, 1, BITMAPTAG_Clear, TRUE, TAG_DONE))
	{
		ULONG i, j;
		UBYTE shift;
		UBYTE *p = gfx_bitmap_array(mbm);
		UBYTE *maskline;
		UBYTE *c = gfx_bitmap_array(bm);
		ULONG dispval;
		ULONG tst;
		ULONG rw;

		maskline = p;

		dispval = (gfx_bitmap_depth(bm) + 1) / 8;

		rw = gfx_bitmap_modulo(bm);

		for (j = 0; j < height; j++)
		{
			p = maskline;
			shift = (1L << 7);

			c = gfx_bitmap_array(bm) + j * rw * dispval;

			for (i = 0; i < width; i++)
			{
				if (!shift)
				{
					shift = (1L << 7);
					p++;
				}

				switch (dispval)
				{
					case 1:
						tst = *c;
						break;

					case 2:
						tst = *c | *(c + 1);
						break;

					case 3:
						tst = *c | *(c + 1) | *(c + 2);
						break;

					case 4:
						tst = *c | *(c + 1) | *(c + 2) | *(c + 3);
						break;

					default:
						PDB(("unsupported format.. huh ?!?\n"));
						tst = 0;
						break;
				}

				if (tst != bgvalue)
				{
					*p |= shift;
				}
				c += dispval;
				shift >>= 1;
			}
			maskline += gfx_bitmap_modulo(mbm);
		}
	}
	return (mbm);
}
#endif
