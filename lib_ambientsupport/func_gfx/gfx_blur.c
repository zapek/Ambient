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
 * $Id: gfx_blur.c,v 1.1 2015/03/02 13:50:54 geit Exp $
 */

#include "../config.h"
#include "../macros.h"
#include "../library.h"

#include <cybergraphx/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

#include <workbench/ambientsupport.h>


/*
 * Does a:
 *
 *      111
 *      121
 *      111
 *
 */
void LIB_gfx_BlurAlpha(APTR bm, ULONG w, ULONG h, struct AmbientSupportBase *AmbientSupportBase UNUSED)
{
	ULONG i, j;
	int mod;

	ASSERT( bm );

	/* only works for ARGB32 bitmaps */

	{
		struct BitMap *cgxbm = gfx_bitmap_bm( bm );

		if ( GetCyberMapAttr(cgxbm, CYBRMATTR_PIXFMT) != PIXFMT_ARGB32 )
			return;
	}

	/* simple 3x3 box blur (gaussian approximation with 1 2 1 kernel) */

	mod = gfx_bitmap_modulo( bm ) * 4;

	/* horizontal pass */

	for (j=0; j < h; j++)
	{
		UBYTE	*row = (UBYTE*)( gfx_bitmap_array( bm )+ j * mod );
		int pp = row[ 0 ];		/* previous and current pixel */
		int pc = row[ 0 ];

		i = w - 1;
		do
		{
			int pn = row[ 4 ];	/* load next */
			row[ 0 ] = ( pc * 2 + pp + pn ) >> 2;	/* sum + average */
			pp = pc;			/* previous<-current */
			pc = pn;			/* current<-next */

			row += 4;			/* next pixel */
		} while ( --i );
	}

	/* vertical pass. have to be optimized (process 4 pixels at once for better cache performance) */

	for (j=0; j < w; j++)
	{
		UBYTE	*col = (UBYTE*)( gfx_bitmap_array( bm )+ j * 4 );
		int pp = col[ 0 ];
		int pc = col[ 0 ];

		i = h - 1;
		do
		{
			int pn = col[ mod ];
			col[ 0 ] = ( pc * 2 + pp + pn ) >> 2;
			pp = pc;
			pc = pn;
			col	+= mod;
		} while( --i );
	}

}

/*
 * Function made for rendering of alphashadow labels. It's smoothing selected
 * component and moves result into alpha channel. All other components are
 * set to given value.
 */

void LIB_gfx_BlurAlphaTransfer(APTR bm, ULONG w, ULONG h, LONG component, ULONG fill, struct AmbientSupportBase *AmbientSupportBase UNUSED )
{
	ULONG i, j;
	LONG mod;

	ASSERT( bm );

	/* only works for ARGB32 bitmaps */

	{
		struct BitMap *cgxbm = gfx_bitmap_bm( bm );

		if ( GetCyberMapAttr(cgxbm, CYBRMATTR_PIXFMT) != PIXFMT_ARGB32 )
			return;
	}

	fill = fill & 0x00ffffff;

	/* simple 3x3 box blur (gaussian approximation with 1 2 1 kernel) */

	mod = gfx_bitmap_modulo( bm ) * 4;

	/* horizontal pass (here we transfer from color->alpha and fill) */

	for (j=0; j < h; j++)
	{
		UBYTE	*row = (UBYTE*)( gfx_bitmap_array( bm )+ j * mod );
		int pp = row[ component ];		/* previous and current pixel */
		int pc = row[ component ];

		i = w - 1;
		do
		{
			LONG pn = row[ component + 4 ];	 /* load next */
			*(ULONG*)row = fill | ( ( ( pc * 2 + pp + pn ) >> 2 ) << 24 );	 /* sum + average */
			pp = pc;			/* previous<-current */
			pc = pn;			/* current<-next */

			row += 4;			/* next pixel */
		} while( --i );
	}

	/* vertical pass. have to be optimized (process 4 pixels at once for better cache performance) */

	for (j=0; j < w; j++)
	{
		UBYTE	*col = (UBYTE*)( gfx_bitmap_array( bm )+ j * 4 );
		int pp = col[ 0 ];
		int pc = col[ 0 ];

		i = h - 1;
		do
		{
			int pn = col[ mod ];
			col[ 0 ] = ( pc * 2 + pp + pn ) >> 2;
			pp = pc;
			pc = pn;
			col	+= mod;
		} while( --i );
	}

}
