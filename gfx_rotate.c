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
 * $Id: gfx_rotate.c,v 1.3 2006/08/08 13:31:34 fab Exp $
 */

#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

/* private */
#include "gfx.h"
#include "gfx_rotate.h"
#include "gfx_bitmap.h"

APTR gfx_rotate( APTR bm, ULONG mode )
{
	ULONG width, height;
	ULONG nwidth, nheight;
	APTR newbm;

	CHECKARGB32(gfx_bitmap_bm(bm));
	ASSERT(bm);

	width  = gfx_bitmap_width( bm );
	height = gfx_bitmap_height( bm );

	if (!width || !height)
	{
		return NULL;
	}

	/*
	 * Calculate new dimensions.
	 */

	switch( mode )
	{
		case ROTATE_90:
		case ROTATE_270:
			nwidth  = height;
			nheight = width;
			break;

		case ROTATE_180:
			nwidth  = width;
			nheight = height;
			break;

		default:
			return NULL;
	}

	/*
	 * Allocate new bitmap.
	 */

	newbm = gfx_bitmap_create( nwidth, nheight, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE );

	if ( newbm )
	{
		/*
		 * Perform rotation.
		 */

		ULONG i, j;
		const ULONG srcmod = gfx_bitmap_modulo( bm );
		const ULONG dstmod = gfx_bitmap_modulo( newbm );
		const ULONG *src   = (ULONG*)gfx_bitmap_array( bm );
		ULONG * const dst  = (ULONG*)gfx_bitmap_array( newbm );

		switch ( mode )
		{
			case ROTATE_90:

				for (j = 0; j < height; j++)
				{
					ULONG *d = dst + ( nwidth - 1 - j );

					i = width;
					if (i >= 4)
					{
						do
						{
							d[0 * dstmod] = src[0];
							d[1 * dstmod] = src[1];
							d[2 * dstmod] = src[2];
							d[3 * dstmod] = src[3];

							src += 4;
							d   += dstmod * 4;

						} while ((i -= 4) >= 4);
					}
					while (i--)
					{
						*d = *src++;
						d += dstmod;
					}

					src += srcmod - width;
				}
				break;

			case ROTATE_270:

				for (j = 0; j < height; j++)
				{
					ULONG *d = dst + dstmod * ( nheight - 1 ) + j;

					i =  width;
					if (i >= 4)
					{
						do
						{
							d[ 0 * dstmod] = src[0];
							d[-1 * dstmod] = src[1];
							d[-2 * dstmod] = src[2];
							d[-3 * dstmod] = src[3];

							src += 4;
							d   -= dstmod * 4;

						} while ((i -= 4) >= 4);
					}
					while (i--)
					{
						*d = *src++;
						d -= dstmod;
					}

					src += srcmod - width;
				}
				break;

			case ROTATE_180:

				for (j = 0; j < height; j++)
				{
					ULONG *d = dst + dstmod * ( height - 1 - j ) + width - 1;

					i =  width;
					if (i >= 4)
					{
						do
						{
							d[ 0] = src[0];
							d[-1] = src[1];
							d[-2] = src[2];
							d[-3] = src[3];

							src += 4;
							d   -= 4;

						} while ((i -= 4) >= 4);
					}
					while (i--)
					{
						*d-- = *src++;
					}

					src += srcmod - width;
				}
				break;
		}
	}

	return newbm;
}
