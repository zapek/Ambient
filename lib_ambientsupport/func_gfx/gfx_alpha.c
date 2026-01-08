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
 * $Id: gfx_alpha.c,v 1.2 2017/08/02 16:44:47 piru Exp $
 */

#include "../config.h"
#include "../macros.h"
#include "../library.h"

#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <workbench/ambientsupport.h>
#define USE_INLINE_STDARG
#include <proto/ambientsupport.h>
#undef USE_INLINE_STDARG

/*
 * Sets the alpha channel values within a bitmap.
 */

void LIB_gfx_AlphaSet(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val, struct AmbientSupportBase *AmbientSupportBase UNUSED )
{
	CHECKARGB32(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);

#if USE_ALTIVEC
	if ( AmbientSupportBase->UseAltiVec )
	{
		ULONG i, j;
		ULONG rw;
		union {
			ULONG alphaval;
			VECTOR_ULONG dummy; /* just for force alignment by 16 */
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

					vec_st(v1, 0, ptr);

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
	else
#endif
	{
		ULONG i, j;
		ULONG rw;

		rw = gfx_bitmap_modulo(bm);

		for (i = y; i < y + height; i++)
		{
			UBYTE *array = gfx_bitmap_array(bm) + ( i * rw + x ) * 4;

			j = width;
			do
			{

				*array = val;
				array += 4;

			} while( --j );
		}
	}
}

/*
 * Check's bitmap for presence of alphachannel information.
 */
ULONG LIB_gfx_AlphaIsPresent(APTR bm, struct AmbientSupportBase *AmbientSupportBase UNUSED )
{
	ASSERT(bm);
	CHECKARGB32(gfx_bitmap_bm(bm));
	{

		ULONG i, j;
		ULONG rw;
		ULONG width = gfx_bitmap_width( bm );
		ULONG height = gfx_bitmap_height( bm );

		rw = gfx_bitmap_modulo(bm);

		for (i = 0; i < height; i++)
		{
			UBYTE *array = gfx_bitmap_array(bm) + i * rw * 4;
			j = width;

			do
			{
				if ( *array != 255 )
					return TRUE;

				array += 4;

			} while( --j );
		}
	}

	return FALSE;

}

static void alpha_div(UBYTE * p, ULONG val)
{
	ULONG t;

	t = *p * val;
	*p = t / 256; /* XXX: or 255? hm.. */
}

static void alpha_div2(UBYTE * p, ULONG val UNUSED)
{
	*p >>= 1;
}

static void alpha_div4(UBYTE * p, ULONG val UNUSED)
{
	*p >>= 2;
}

static void alpha_div8(UBYTE * p, ULONG val UNUSED)
{
	*p >>= 3;
}

static void alpha_div16(UBYTE * p, ULONG val UNUSED)
{
	*p >>= 4;
}

#define SET_ALPHAFUNC(_f, _v) \
    switch (_v)               \
    {                         \
        case 0x10:            \
            _f = alpha_div16; \
            break;            \
                              \
        case 0x20:            \
            _f = alpha_div8;  \
            break;            \
                              \
        case 0x40:            \
            _f = alpha_div4;  \
            break;            \
                              \
        case 0x80:            \
            _f = alpha_div2;  \
            break;            \
                              \
        default:              \
            _f = alpha_div;   \
            break;            \
    }


/*
 * Composes the alpha channel value within a bitmap (ARGB32).
 */
void LIB_gfx_AlphaCompose(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val, struct AmbientSupportBase *AmbientSupportBase UNUSED )
{
	ULONG i, j;
	ULONG rw;
	void (*fp) (UBYTE * p, ULONG val);

	CHECKARGB32(gfx_bitmap_bm(bm));

	ASSERT(width);
	ASSERT(height);


	SET_ALPHAFUNC(fp, val);

#if USE_ALTIVEC
	if (use_altivec && (val == 0x10 || val == 0x20 || val == 0x40 || val == 0x80))
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
	else
#endif
	{
		UBYTE *p;

		rw = gfx_bitmap_modulo(bm);

		for (i = y; i < y + height; i++)
		{
			for (j = x; j < x + width; j++)
			{
				p = (gfx_bitmap_array(bm) + (i * rw + j) * 4);

				fp(p, val);
			}
		}
	}
}


/*
 * Sets the alpha channel values within a bitmap using a mask.
 * Clears it to 0 otherwise and if clear is TRUE.
 */

void LIB_gfx_AlphaSetMask(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, APTR mask, UBYTE val, ULONG mode, struct AmbientSupportBase *AmbientSupportBase UNUSED )
{
	ULONG i, j, jm, im;
	UBYTE *mp;
	UBYTE m;
	ULONG rw;

	CHECKARGB32(gfx_bitmap_bm(bm));
	CHECKMASK(gfx_bitmap_bm(mask));
	ASSERT(width);
	ASSERT(height);

	rw = gfx_bitmap_modulo(bm);

	for (i = y, im = 0; i < y + height; i++, im++)
	{
		mp = gfx_bitmap_array(mask) + im * gfx_bitmap_modulo(mask);
		m = 1 << 7;

		for (j = x, jm = 0; j < x + width; j++, jm++, (m == 1) ? (m = 1 << 7) : (m >>= 1))
		{
			if (*(mp + ((jm & ~7) >> 3)) & m)
			{
				if (mode == ACM_COMPOSE)
				{
					*(gfx_bitmap_array(bm) + (i * rw + j) * 4) += val;
				}
				else
				{
					*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = val;
				}
			}
			else
			{
				if (mode == ACM_SET_AND_CLEAR || mode == ACM_COMPOSE_AND_CLEAR)
				{
					*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = 0x0;
				}
			}
		}
	}
}


/*
 * Sets the alpha channel values within a bitmap using an array
 * with alpha as source (ARGB32). Compute the value between 'val' and
 * 0xff and adds 'val' to it.
 */
void LIB_gfx_AlphaSetArrayAdd(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE * a, UBYTE val, struct AmbientSupportBase *AmbientSupportBase UNUSED )
{
	ULONG i, j;
	ULONG ia = 0;
	ULONG rw;

	CHECKARGB32(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);
	ASSERT(a);

	rw = gfx_bitmap_modulo(bm);

	for (i = y; i < y + height; i++)
	{
		for (j = x; j < x + width; j++, ia++)
		{
			/* XXX: sucks.. it doesn't take the source's alpha map into account and overwrites it */
			*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = *(a + ia * 4) ? val : 1 + *(a + ia * 4) * val / 0xff - 1;	/* XXX: disallow 0xff as input and 0.. this routine needs work.. that -1 is painful too */
		}
	}
}


/*
 * Sets the alpha value low on the borders and
 * high near the center. Takes vertical and horizontal
 * distance into account. 'val' is the maximum alpha value.
 * Only pixels with an existing alpha value are modified.
 */

void LIB_gfx_AlphaSetRadial(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val, ULONG radius, struct AmbientSupportBase *AmbientSupportBase UNUSED )
{
	LONG i, j;
	ULONG rw;
	LONG alphaval, topval;
	LONG invmaxdist;

	CHECKARGB32(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);

	rw = gfx_bitmap_modulo(bm);
	invmaxdist = 65536 / radius;

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			//dprintf("val: %p\n", math_sqrt(i * i + j * j));
			if ((topval = *(gfx_bitmap_array(bm) + (i * rw + j) * 4)))
			{
				//*(bm->Planes[0] + (i * rw + j) * 4) = *(bm->Planes[0] + (i * rw + j) * 4) * (math_sqrt((y - i) * (y - i) + (x - j) * (x - j))) / max(width, height);
				alphaval = ((*(gfx_bitmap_array(bm) + (i * rw + j) * 4) * val) >> 8) - (math_sqrt((y - i) * (y - i) + (x - j) * (x - j)) * invmaxdist / 256);  /* XXX: a bit simplistic */
				if (alphaval < 0)
					alphaval = 0;

				if (alphaval > topval)
					*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = 0;
				else
					*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = alphaval;
			}
		}
	}
}


/*
 * Function made for rendering of alphashadow labels. It's moving
 * certain component into alpha channel. All other components are
 * set to given value.
 */

void LIB_gfx_AlphaTransfer(APTR bm, ULONG w, ULONG h, LONG component, ULONG fill, struct AmbientSupportBase *AmbientSupportBase UNUSED )
{
//	  GETCYBERGFXBASE( AmbientSupportBase );

	ULONG i, j;
	LONG mod;

	ASSERT(bm);

	/* only works for ARGB32 bitmaps */

	{
		struct BitMap *cgxbm = gfx_bitmap_bm(bm);

		if (GetCyberMapAttr(cgxbm, CYBRMATTR_PIXFMT) != PIXFMT_ARGB32)
			return;
	}

	fill = fill & 0x00ffffff;

	mod = gfx_bitmap_modulo(bm) * 4;

	/* here we transfer from color->alpha and fill */

	for (j = 0; j < h; j++)
	{
		UBYTE *row = (UBYTE *) (gfx_bitmap_array(bm) + j * mod);

		i = w;
		do
		{
			LONG pn = row[component];
			*(ULONG *) row = fill | (pn << 24);
			row += 4;
		} while( --i );
	}
}
