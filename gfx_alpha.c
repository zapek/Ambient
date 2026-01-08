/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2015 Ambient Open Source Team
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
 * $Id: gfx_alpha.c,v 1.12 2017/08/02 15:00:51 piru Exp $
 */

#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>

/* private */
#include "gfx.h"
#include "gfx_alpha.h"
#include "gfx_alpha_vec.h"
#include "gfx_bitmap.h"
#include "ambient_altivec.h"
#include "math_sqrt.h"


/*
 * Sets the alpha channel values within a bitmap.
 */
void gfx_alpha_set(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val)
{
	CHECKARGB32(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);

#if USE_ALTIVEC
	if (use_altivec && (x & 3) == 0)
	{
		gfx_alpha_set_vec(bm, x, y, width, height, val);
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
ULONG gfx_alpha_ispresent(APTR bm)
{
	ASSERT(bm);
	CHECKARGB32(gfx_bitmap_bm(bm));

	{
		ULONG i, j;
		ULONG width = gfx_bitmap_width( bm );
		ULONG height = gfx_bitmap_height( bm );

		ULONG bpr = gfx_bitmap_bpr(bm);

		for (i = 0; i < height; i++)
		{
			UBYTE *array = gfx_bitmap_array(bm) + i * bpr;
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
void gfx_alpha_compose(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val)
{
	ULONG i, j;
	ULONG rw;
	void (*fp) (UBYTE * p, ULONG val);

	CHECKARGB32(gfx_bitmap_bm(bm));

	ASSERT(width);
	ASSERT(height);


	SET_ALPHAFUNC(fp, val);

#if USE_ALTIVEC
	if (use_altivec && (x & 3) == 0 && (val == 0x10 || val == 0x20 || val == 0x40 || val == 0x80))
	{
		gfx_alpha_compose_vec(bm, x, y, width, height, val, fp);
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


#if 0
/*
 * Blends a source bitmap (ARBG32) into a target bitmap (ARGB32).
 * The source has to be smaller than the target. Copy is only is alpha
 * value is not 0. The highest alpha value of the result is kept.
 * This function is meant to group objects together.
 */
void gfx_alpha_blend(APTR srcbm, APTR dstbm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val)
{
	UBYTE a;
	UBYTE *t, *s;
	ULONG srw, drw;
	ULONG i, j, k, l;
	void (*fp) (UBYTE * p, ULONG val);

	CHECKARGB32(gfx_bitmap_bm(srcbm));
	CHECKARGB32(gfx_bitmap_bm(dstbm));
	ASSERT(width);
	ASSERT(height);

	srw = gfx_bitmap_modulo(srcbm);
	drw = gfx_bitmap_modulo(dstbm);

	SET_ALPHAFUNC(fp, val);

	for (i = y, k = 0; i < y + height; i++, k++)
	{
		for (j = x, l = 0; j < x + width; j++, l++)
		{
			s = gfx_bitmap_array(srcbm) + (k * srw + l) * 4;
			t = gfx_bitmap_array(dstbm) + (i * drw + j) * 4;

			if (a = *s)
			{
				fp(&a, val);

				*t = max(*t, a);

				#if 0
				if (a >= val)	/* XXX: not sure.. */
				#endif
				{
					t++;
					s++;
					*t++ = *s++;
					*t++ = *s++;
					*t = *s;
				}
				#if 0
				else
				{
					/*
					 * XXX: need to finish that color blending
					 */
					ULONG srcr, srcg, srcb;
					ULONG dstr, dstg, dstb;

					t++;	/* A */
					dstr = *t++ * (0xff - a);	/* R */
					dstg = *t++ * (0xff - a);	/* G */
					dstb = *t * (0xff - a);	/* B */

					s++;	/* A */
					srcr = *s++ * a;	/* R */
					srcg = *s++ * a;	/* G */
					srcb = *s * a;	/* B */

					dstr += srcr;
					dstg += srcg;
					dstb += srcb;

					t -= 3;
					*t++ = dstr >> 8;
					*t++ = dstg >> 8;
					*t = dstb >> 8;
				}
				#endif
			}
		}
	}
}
#endif

/*
 * Sets the alpha channel values within a bitmap using a mask.
 * Clears it to 0 otherwise and if clear is TRUE.
 */

void gfx_alpha_set_mask(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, APTR mask, UBYTE val, ULONG mode)
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


#if 0
/*
 * Sets the alpha channel values within a bitmap using an array
 * with alpha as source (ARGB32).
 */
void gfx_set_alpha_array(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE * a)
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
			*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = *(a + ia * 4);
		}
	}
}
#endif


/*
 * Sets the alpha channel values within a bitmap using an array
 * with alpha as source (ARGB32). Compute the value between 'val' and
 * 0xff and adds 'val' to it.
 */
void gfx_alpha_set_array_add(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE * a, UBYTE val)
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

void gfx_alpha_set_radial(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val, ULONG radius)
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


#if 0 /* XXX: that one is broken.. only does horiz/vert gradients at a time.. well, could be reused */
void gfx_set_alpha_radial(APTR bm, ULONG x, ULONG y, ULONG width, ULONG height, UBYTE val)
{
	ULONG i, j;
	ULONG rw;
	ULONG alpha, left, right, up, down;
	ULONG alphaval;

	CHECKARGB32(gfx_bitmap_bm(bm));
	ASSERT(width);
	ASSERT(height);

	rw = gfx_bitmap_modulo(bm);

	alpha = (val << 24) | (val << 16) | (val << 8) | val;

	#if 0 /* see below.. sigh */
	alpha /= 2;
	#endif

	left  = alpha / x;
	right = alpha / (width - x);
	up    = alpha / y;
	down  = alpha / (height - y);

	for (i = 0; i < height; i++)
	{
		for (j = 0; j < width; j++)
		{
			if (j < x)
			{
				alphaval = alpha - left * (x - j);
			}
			else
			{
				alphaval = alpha - right * (j - x);
			}

			#if 0 /* XXX: this sucks.. I guess I have to check if alphaval is not too small or so.. lame */
			if (i < y)
			{
				alphaval += alpha - up * (y - i);
			}
			else
			{
				alphaval += alpha - down * (i - y);
			}
			#endif

			if (*(gfx_bitmap_array(bm) + (i * rw + j) * 4))
			{
				*(gfx_bitmap_array(bm) + (i * rw + j) * 4) = *(gfx_bitmap_array(bm) + (i * rw + j) * 4) * (alphaval >> 24) >> 8;
			}

			#if 0
			if (i == 10 && j == 10)
			{
				dprintf("alpha is %p -> left is %p -> attenuation left is %p\n", alpha, left, left * (x - 10));
				dprintf("alphaval for 10/10: %p, final: %p\n", alphaval, alphaval >> 24);
			}
			#endif
		}
	}
}
#endif

/*
 * Function made for rendering of alphashadow labels. It's moving
 * certain component into alpha channel. All other components are
 * set to given value.
 */

void gfx_alpha_transfer(APTR bm, ULONG w, ULONG h, LONG component, ULONG fill)
{
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

	mod = gfx_bitmap_bpr(bm);

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
