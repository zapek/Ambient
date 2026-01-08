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
 * $Id: colormap.c,v 1.5 2006/04/12 14:01:53 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/graphics.h>

/* private */
#include "colormap.h"


struct ColorMap *magicwb_cm;


/*
 * Allocates a MagicWB v2 (with RomIcons/TomIcons extension) ColorMap.
 * Original MagicWB v2 is 8 colors only.
 * We also support MagicWB v1 then.. colors from 16 to 251 will use an
 * arbitrary gradient (better than random colors)
 */

#if 1

static const ULONG magicwb2[] = {
	/* MagicWB v2 */
	0x959595,
	0x000000,
	0xffffff,
	0x3b67a2,
	0x7b7b7b,
	0xafafaf,
	0xaa907c,
	0xffa997
};
static const int magicwb2cnt = sizeof(magicwb2) / sizeof(magicwb2[0]);

static const ULONG romicons[] = {
	/* RomIcons/TomIcons */
	0x0000ff,
	0x283e5b,
	0x608060,
	0xe2d177,
	0xffd4cb,
	0x7a6048,
	0xd2d2d2,
	0xe55d5d
};
static const int romiconscnt = sizeof(romicons) / sizeof(romicons[0]);

static const ULONG magicwb1[] = {
	/* MagicWB v1 */
	0x7b7b7b,
	0xafafaf,
	0xaa907c,
	0xffa997
};
static const int magicwb1cnt = sizeof(magicwb1) / sizeof(magicwb1[0]);


#define UNPACK_32(x) (((x) << 24) | ((x) << 16) | ((x) << 8) | (x))

static inline void LoadRGBArray(const ULONG *from, int start, int cnt, ULONG *array)
{
	const int end = start + cnt;
	int i;

	for (i = start; i < end; i++)
	{
		ULONG rgb;
		UBYTE r, g, b;

		rgb = *from++;

		r = rgb >> 16;
		g = rgb >> 8;
		b = rgb >> 0;

		array[1 + i * 3 + 0] = UNPACK_32(r);
		array[1 + i * 3 + 1] = UNPACK_32(g);
		array[1 + i * 3 + 2] = UNPACK_32(b);
	}
}


#define NUMCOLOURS 256

ULONG magicwb_cm_init(void)
{
	if ((magicwb_cm = GetColorMap(NUMCOLOURS)))
	{
		ULONG *array;

		array = malloc(sizeof(*array) * (NUMCOLOURS * 3 + 2));
		if (array)
		{
			struct ViewPort fakevp;
			ULONG i;

			array[0] = (NUMCOLOURS << 16) | 0;

			LoadRGBArray(magicwb2, 0, magicwb2cnt, array);
			LoadRGBArray(romicons, magicwb2cnt, romiconscnt, array);

			/* Nothing there.. let's put a B&W gradient */
			for (i = magicwb2cnt + romiconscnt;
			     i < (NUMCOLOURS - magicwb1cnt);
			     i++)
			{
				array[1 + i * 3 + 0] = UNPACK_32(i);
				array[1 + i * 3 + 1] = UNPACK_32(i);
				array[1 + i * 3 + 2] = UNPACK_32(i);
			}

			/* MagicWB v1 */
			LoadRGBArray(magicwb1, NUMCOLOURS - magicwb1cnt, magicwb1cnt, array);

			array[1 + NUMCOLOURS * 3] = (0 << 16) | 0;

			/* Neat trick to avoid using SetRGB32CM */
			InitVPort(&fakevp);
			fakevp.ColorMap = magicwb_cm;
			LoadRGB32(&fakevp, array);

			free(array);
	
			return TRUE;
		}
	}

	return FALSE;
}

#else

#define UNPACK_32(x) (((x << 24) & 0xff000000) | ((x << 16) & 0xff0000) | ((x << 8) & 0xff00) | (x & 0xff))

ULONG magicwb_cm_init(void)
{
	if ((magicwb_cm = GetColorMap(256)))
	{
		ULONG i;

		/* MagicWB v2 */
		SetRGB32CM(magicwb_cm,   0, 0x95959595, 0x95959595, 0x95959595);
		SetRGB32CM(magicwb_cm,   1, 0x00000000, 0x00000000, 0x00000000);
		SetRGB32CM(magicwb_cm,   2, 0xffffffff, 0xffffffff, 0xffffffff);
		SetRGB32CM(magicwb_cm,   3, 0x3b3b3b3b, 0x67676767, 0xa2a2a2a2);
		SetRGB32CM(magicwb_cm,   4, 0x7b7b7b7b, 0x7b7b7b7b, 0x7b7b7b7b);
		SetRGB32CM(magicwb_cm,   5, 0xafafafaf, 0xafafafaf, 0xafafafaf);
		SetRGB32CM(magicwb_cm,   6, 0xaaaaaaaa, 0x90909090, 0x7c7c7c7c);
		SetRGB32CM(magicwb_cm,   7, 0xffffffff, 0xa9a9a9a9, 0x97979797);

		/* RomIcons/TomIcons */
		SetRGB32CM(magicwb_cm,   8, 0x00000000, 0x00000000, 0xffffffff);
		SetRGB32CM(magicwb_cm,   9, 0x28282828, 0x3e3e3e3e, 0x5b5b5b5b);
		SetRGB32CM(magicwb_cm,  10, 0x60606060, 0x80808080, 0x60606060);
		SetRGB32CM(magicwb_cm,  11, 0xe2e2e2e2, 0xd1d1d1d1, 0x77777777);
		SetRGB32CM(magicwb_cm,  12, 0xffffffff, 0xd4d4d4d4, 0xcbcbcbcb);
		SetRGB32CM(magicwb_cm,  13, 0x7a7a7a7a, 0x60606060, 0x48484848);
		SetRGB32CM(magicwb_cm,  14, 0xd2d2d2d2, 0xd2d2d2d2, 0xd2d2d2d2);
		SetRGB32CM(magicwb_cm,  15, 0xe5e5e5e5, 0x5d5d5d5d, 0x5d5d5d5d);

		/* Nothing there.. let's put a B&W gradient */
		for (i = 16; i < 252; i++)
		{
			SetRGB32CM(magicwb_cm, i, UNPACK_32(i), UNPACK_32(i), UNPACK_32(i));
		}

		/* MagicWB v1 */
		SetRGB32CM(magicwb_cm, 252, 0x7b7b7b7b, 0x7b7b7b7b, 0x7b7b7b7b);
		SetRGB32CM(magicwb_cm, 253, 0xafafafaf, 0xafafafaf, 0xafafafaf);
		SetRGB32CM(magicwb_cm, 254, 0xaaaaaaaa, 0x90909090, 0x7c7c7c7c);
		SetRGB32CM(magicwb_cm, 255, 0xffffffff, 0xa9a9a9a9, 0x97979797);
	
		return (TRUE);
	}
	return (FALSE);
}

#endif

/*
 * Frees a MagicWB v2 ColorMap.
 */
void magicwb_cm_cleanup(void)
{
	if (magicwb_cm)
	{
		FreeColorMap(magicwb_cm);
	}
}

