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
 * $Id: gfx_alpha_vec.c,v 1.5 2017/08/02 16:38:55 piru Exp $
 */

#include "ambient.h"

#if USE_ALTIVEC

/* public */
#if __GNUC__ > 3
#include <altivec.h>
#endif

/* private */
#include "gfx.h"
#include "gfx_alpha.h"
#include "gfx_alpha_vec.h"
#include "gfx_bitmap.h"

void gfx_alpha_set_vec(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val)
{
	ULONG i, j;
	ULONG rw;
	union {
		ULONG alphaval;
		VECTOR_ULONG foo; /* just to enforce alignment of 16 */
	} align16;
	ULONG *ptr;
	VECTOR_ULONG vval;
	VECTOR_ULONG v1;

	align16.alphaval = val << 24;
	vval = vec_splat(vec_lde(0, &align16.alphaval), 0);

	rw = gfx_bitmap_modulo(bm);

	for (i = y; i < y + height; i++)
	{
		for (j = x; j < x + width;)
		{
			ptr	= (ULONG *)(gfx_bitmap_array(bm) + (i * rw + j) * 4);

			if (j + 3 < x + width)
			{
				CHECKALIGN(ptr, 16);
				v1 = vec_ld(0, ptr);
				v1 = vec_or(v1, vval);
				#if 1
				{ULONG l[4]; l[0] = l[1];/* don't touch that.. really (2.95.3vec workaround) */}
				#endif

				vec_st(v1, 0, (unsigned int *)ptr);

				j += 4;
			}
			else
			{
				*((UBYTE *)ptr) = val;
				j++;
			}
		}
	}
}


void gfx_alpha_compose_vec(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val, void (*fp)(UBYTE *p, ULONG val))
{
	ULONG i, j;
	ULONG rw;
	VECTOR_ULONG v1;
	VECTOR_ULONG vs = vs; /* shut up gcc */
	ULONG *ptr;

	rw = gfx_bitmap_modulo(bm);

	switch (val)
	{
		case 0x10:
			vs = vec_splat_u32(4);
			break;

		case 0x20:
			vs = vec_splat_u32(3);
			break;

		case 0x40:
			vs = vec_splat_u32(2);
			break;

		case 0x80:
			vs = vec_splat_u32(1);
			break;
	}

	for (i = y; i < y + height; i++)
	{
		for (j = x; j < x + width;)
		{
			ptr = (ULONG *)(gfx_bitmap_array(bm) + (i * rw + j) * 4);

			if (j + 3 < x + width)
			{
				CHECKALIGN(ptr, 16);

				v1 = vec_ld(0, ptr);
				v1 = vec_sr(v1, vs);

				vec_ste((VECTOR_UBYTE)v1, 0,  (UBYTE *)ptr);
				vec_ste((VECTOR_UBYTE)v1, 4,  (UBYTE *)ptr);
				vec_ste((VECTOR_UBYTE)v1, 8,  (UBYTE *)ptr);
				vec_ste((VECTOR_UBYTE)v1, 12, (UBYTE *)ptr);

				j += 4;
			}
			else
			{
				fp((UBYTE *)ptr, val);
				j++;
			}
		}
	}
}

#endif /* USE_ALTIVEC */
