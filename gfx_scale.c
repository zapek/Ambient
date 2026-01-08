/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: gfx_scale.c,v 1.12 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/graphics.h>

/* private */
#include "gfx.h"
#include "gfx_scale.h"
#include "gfx_scale_vec.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "mui_func.h" /* FORTAG, etc.. */
#include "cpu.h"
#include "ambient_altivec.h"

#define USE_INTERNAL_SCALER 1

/*
 * Some reference material:
 * - http://www.compuphase.com/graphic/scale2.htm
 * - http://www.compuphase.com/graphic/scale3.htm
 * - http://www.stereopsis.com/doubleblend.html
 * - http://astronomy.swin.edu.au/~pbourke/colour/bicubic/
 */

#if USE_INTERNAL_SCALER

static ULONG gsi_scale_nearest_neighbour(struct gsi_info *gsii UNUSED, CONST ULONG *s, ULONG x UNUSED, ULONG y UNUSED)
{
	return (*s);
}

#define PIXEL_A(x) ((x) >> 24)
#define PIXEL_R(x) (((x) >> 16) & 0xff)
#define PIXEL_G(x) (((x) >> 8) & 0xff)
#define PIXEL_B(x) ((x) & 0xff)


#define GSIMUL 4096
#define GSISHIFT 12


/*
 * v00|     |v01
 *   -+-----+-
 *    |     |      For 'x' we get the 4 corners with their intensity depending on the 'x' placement.
 *    |     |      We sum up the whole then divide. The resulting pixel has the correct value of
 *    |   x |      the 4 pixels surrounding it.
 *    |     |
 *   -+-----+-
 * v10|     |v11
 */
static ULONG gsi_scale_bilinear(struct gsi_info *gsii, CONST ULONG *s, ULONG x, ULONG y)
{
	ULONG v00m, v10m, v01m, v11m; /* multipliers */
	ULONG smp;
	ULONG d;
	ULONG advx, advy;
	ULONG xfrac = gsii->xfrac, yfrac = gsii->yfrac;
	ULONG p1, p2, pp1, pp2;

	/* limit borders */
	if (x + 1 >= gsii->swidth)
	{
		advx = 0;
	}
	else
	{
		advx = 1;
	}

	if (y + 1 >= gsii->sheight)
	{
		advy = 0;
	}
	else
	{
		advy = gsii->smod;
	}


	/* weights */
	v00m = ((GSIMUL - xfrac) * (GSIMUL - yfrac)) >> (GSISHIFT + GSISHIFT - 7);
	v01m = (xfrac * (GSIMUL - yfrac)) >> (GSISHIFT + GSISHIFT - 7);
	v10m = ((GSIMUL - xfrac) * yfrac) >> (GSISHIFT + GSISHIFT - 7);
	v11m = (xfrac * yfrac) >> (GSISHIFT + GSISHIFT - 7);

	/* apply weights */
	smp = *s;
	p2 = smp & 0x00ff00ff;
	p1 = (smp & 0xff00ff00)>>8;
	pp1 = p1 * v00m;
	pp2 = p2 * v00m;

	smp = *(s + advx);
	p2 = smp & 0x00ff00ff;
	p1 = (smp & 0xff00ff00)>>8;
	pp1 += p1 * v01m;
	pp2 += p2 * v01m;

	smp = *(s + advy);
	p2 = smp & 0x00ff00ff;
	p1 = (smp & 0xff00ff00)>>8;
	pp1 += p1 * v10m;
	pp2 += p2 * v10m;

	smp = *(s + advy + advx);
	p2 = smp & 0x00ff00ff;
	p1 = (smp & 0xff00ff00)>>8;
	pp1 += p1 * v11m;
	pp2 += p2 * v11m;

	/* merge RB and GA */

	d = (pp1 <<1) & 0xff00ff00;
	d |= (pp2 >>7) & 0x00ff00ff;

	return (d);
}

static ULONG gsi_scale_bicubic(struct gsi_info *gsii, CONST ULONG *s, ULONG x, ULONG y)
{
	#define BC_GSIMUL (GSIMUL>>2)
	//LONG p1x=x;
	LONG normx = gsii->xfrac >> 2;
	//LONG p1y=y;
	LONG normy = gsii->yfrac >> 2;

	/* coefficients for interpolation */
	LONG s0, s1, s2, s3;      // in the x-direction
	LONG t0, t1, t2, t3;      // in the y-direction

	LONG r1, g1, b1, a1;
	LONG r2, g2, b2, a2;
	LONG r3, g3, b3, a3;
	LONG r4, g4, b4, a4;
	ULONG pxl;
	LONG advx_n1 = -1, advx_1 = 1, advx_2 = 2;
	LONG advy_n1 = -1, advy_1 = 1, advy_2 = 2;

	/* limit borders (this sucks for performance) */
	if (x == 0 )
		advx_n1 = 0;

	if (y == 0 )
		advy_n1 = 0;

	if (x + 1 >= gsii->swidth)
		advx_1 = 0;

	if (y + 1 >= gsii->sheight)
		advy_1 = 0;

	if (x + 2 >= gsii->swidth)
		advx_2 = 0;

	if (y + 2 >= gsii->sheight)
		advy_2 = 0;

	s0 = ((2*BC_GSIMUL-normx)*normx/BC_GSIMUL-1*BC_GSIMUL)*normx/BC_GSIMUL;    // -1
	s1 = (3*normx-5*BC_GSIMUL)*normx*normx/(BC_GSIMUL*BC_GSIMUL)+2*BC_GSIMUL;  //  0
	s2 = ((4*BC_GSIMUL-3*normx)*normx/BC_GSIMUL+1*BC_GSIMUL)*normx/BC_GSIMUL;  // +1
	s3 = (normx-1*BC_GSIMUL)*normx*normx/(BC_GSIMUL*BC_GSIMUL);                // +2

	t0 = ((2*BC_GSIMUL-normy)*normy/BC_GSIMUL-1*BC_GSIMUL)*normy/BC_GSIMUL;    // -1
	t1 = (3*normy-5*BC_GSIMUL)*normy*normy/(BC_GSIMUL*BC_GSIMUL)+2*BC_GSIMUL;  //  0
	t2 = ((4*BC_GSIMUL-3*normy)*normy/BC_GSIMUL+1*BC_GSIMUL)*normy/BC_GSIMUL;  // +1
	t3 = (normy-1*BC_GSIMUL)*normy*normy/(BC_GSIMUL*BC_GSIMUL);                // +2

#define pxl_get(dx,dy) (s[(dx)+(dy)*gsii->smod])
#define pxl_scale_add(r, g, b, a, pixel, scale ) \
	r += scale * PIXEL_R( pixel ); \
	g += scale * PIXEL_G( pixel ); \
	b += scale * PIXEL_B( pixel ); \
	a += scale * PIXEL_A( pixel );

#define pxl_scale(r, g, b, a, pixel, scale ) \
	r = scale * PIXEL_R( pixel ); \
	g = scale * PIXEL_G( pixel ); \
	b = scale * PIXEL_B( pixel ); \
	a = scale * PIXEL_A( pixel );

	pxl = pxl_get(advx_n1, advy_n1);
	pxl_scale(r1, g1, b1, a1, pxl, s0 );
	pxl = pxl_get(0, advy_n1);
	pxl_scale_add(r1, g1, b1, a1, pxl, s1 );
	pxl = pxl_get(advx_1, advy_n1);
	pxl_scale_add(r1, g1, b1, a1, pxl, s2 );
	pxl = pxl_get(advx_2, advy_n1);
	pxl_scale_add(r1, g1, b1, a1, pxl, s3 );

	pxl = pxl_get(advx_n1,0);
	pxl_scale(r2, g2, b2, a2, pxl, s0 );
	pxl = pxl_get(0,0);
	pxl_scale_add(r2, g2, b2, a2, pxl, s1 );
	pxl = pxl_get(advx_1,0);
	pxl_scale_add(r2, g2, b2, a2, pxl, s2 );
	pxl = pxl_get(advx_2,0);
	pxl_scale_add(r2, g2, b2, a2, pxl, s3 );

	pxl = pxl_get(advx_n1, advy_1);
	pxl_scale(r3, g3, b3, a3, pxl, s0 );
	pxl = pxl_get(0, advy_1);
	pxl_scale_add(r3, g3, b3, a3, pxl, s1 );
	pxl = pxl_get(advx_1, advy_1);
	pxl_scale_add(r3, g3, b3, a3, pxl, s2 );
	pxl = pxl_get(advx_2, advy_1);
	pxl_scale_add(r3, g3, b3, a3, pxl, s3 );

	pxl = pxl_get(advx_n1, advy_2);
	pxl_scale(r4, g4, b4, a4, pxl, s0 );
	pxl = pxl_get(0, advy_2);
	pxl_scale_add(r4, g4, b4, a4, pxl, s1 );
	pxl = pxl_get(advx_1, advy_2);
	pxl_scale_add(r4, g4, b4, a4, pxl, s2 );
	pxl = pxl_get(advx_2, advy_2);
	pxl_scale_add(r4, g4, b4, a4, pxl, s3 );

#undef pxl_get
#undef pxl_scale
#undef pxl_scale_add

	r1 = ( r1 * t0 + r2 *t1 + r3 * t2 + r3 * t3 ) / (BC_GSIMUL * BC_GSIMUL * 4);
	g1 = ( g1 * t0 + g2 *t1 + g3 * t2 + g3 * t3 ) / (BC_GSIMUL * BC_GSIMUL * 4);
	b1 = ( b1 * t0 + b2 *t1 + b3 * t2 + b3 * t3 ) / (BC_GSIMUL * BC_GSIMUL * 4);
	a1 = ( a1 * t0 + a2 *t1 + a3 * t2 + a3 * t3 ) / (BC_GSIMUL * BC_GSIMUL * 4);

	if ( r1 < 0 ) r1 = 0;
	if ( g1 < 0 ) g1 = 0;
	if ( b1 < 0 ) b1 = 0;
	if ( a1 < 0 ) a1 = 0;
	if ( r1 > 255 ) r1 = 255;
	if ( g1 > 255 ) g1 = 255;
	if ( b1 > 255 ) b1 = 255;
	if ( a1 > 255 ) a1 = 255;

	return ( a1 << 24 ) | ( r1 << 16 ) | ( g1 << 8 ) | b1;
}

/*
 *    |   |
 * v00|v01|
 * ---+---+
 * v10|v11|  For an 'x' in v11, we get the 2x2 grid surrounding then
 * ---+---+  use the average of them.
 */
static ULONG gsi_scale_average2(struct gsi_info *gsii, CONST ULONG *s, ULONG x, ULONG y)
{
	ULONG v00, v01;
	ULONG v10, v11;

	ULONG s00, s01;
	ULONG s10, s11;

	ULONG d;
	ULONG bckx;
	ULONG bcky;

	/* limit borders */
	if ((LONG)x - 1 < 0)
	{
		bckx = 0;
	}
	else
	{
		bckx = 1;
	}

	if ((LONG)y - 1 < 0)
	{
		bcky = 0;
	}
	else
	{
		bcky = gsii->smod;
	}

	/* fetch pixels */
	s00 = *(s - bcky - bckx);
	s01 = *(s - bcky);
	s10 = *(s - bckx);
	s11 = *(s);

	/* A */
	v00 = PIXEL_A(s00);
	v01 = PIXEL_A(s01);
	v10 = PIXEL_A(s10);
	v11 = PIXEL_A(s11);
	d = ((v00 + v01 + v10 + v11) / 4) << 24;

	/* R */
	v00 = PIXEL_R(s00);
	v01 = PIXEL_R(s01);
	v10 = PIXEL_R(s10);
	v11 = PIXEL_R(s11);
	d |= ((v00 + v01 + v10 + v11) / 4) << 16;

	/* G */
	v00 = PIXEL_G(s00);
	v01 = PIXEL_G(s01);
	v10 = PIXEL_G(s10);
	v11 = PIXEL_G(s11);
	d |= ((v00 + v01 + v10 + v11) / 4) << 8;

	/* B */
	v00 = PIXEL_B(s00);
	v01 = PIXEL_B(s01);
	v10 = PIXEL_B(s10);
	v11 = PIXEL_B(s11);
	d |= ((v00 + v01 + v10 + v11) / 4) & 0xff;

	return (d);
}

/*
 *    |   |   |
 * v00|v01|v02|v03
 * ---+---+---+---
 * v10|v11|v12|v13  For an 'x' in v11, we get the 4x4 grid surrounding then
 * ---+---+---+---  use the average of them.
 * v20|v21|v22|v23
 * ---+---+---+---
 * v30|v31|v32|v33
 *    |   |   |
 */
static ULONG gsi_scale_average4(struct gsi_info *gsii, CONST ULONG *s, ULONG x, ULONG y)
{
	ULONG v00, v01, v02, v03;
	ULONG v10, v11, v12, v13;
	ULONG v20, v21, v22, v23;
	ULONG v30, v31, v32, v33;

	ULONG s00, s01, s02, s03;
	ULONG s10, s11, s12, s13;
	ULONG s20, s21, s22, s23;
	ULONG s30, s31, s32, s33;

	ULONG d;
	ULONG bckx;
	ULONG bcky;

	/* limit borders */
	if ((LONG)x - 1 < 0)
	{
		bckx = 0;
	}
	else
	{
		bckx = 1;
	}

	if ((LONG)y - 1 < 0)
	{
		bcky = 0;
	}
	else
	{
		bcky = gsii->smod;
	}

	/* fetch pixels */
	s00 = *(s - bcky - bckx);
	s01 = *(s - bcky);
	s02 = *(s - bcky + 1);
	s03 = *(s - bcky + 2);
	s10 = *(s - bckx);
	s11 = *(s);
	s12 = *(s + 1);
	s13 = *(s + 2);
	s20 = *(s + gsii->smod - bckx);
	s21 = *(s + gsii->smod);
	s22 = *(s + gsii->smod + 1);
	s23 = *(s + gsii->smod + 2);
	s30 = *(s + gsii->smod * 2 - bckx);
	s31 = *(s + gsii->smod * 2);
	s32 = *(s + gsii->smod * 2 + 1);
	s33 = *(s + gsii->smod * 2 + 2);

	/* A */
	v00 = PIXEL_A(s00);
	v01 = PIXEL_A(s01);
	v02 = PIXEL_A(s02);
	v03 = PIXEL_A(s03);
	v10 = PIXEL_A(s10);
	v11 = PIXEL_A(s11);
	v12 = PIXEL_A(s12);
	v13 = PIXEL_A(s13);
	v20 = PIXEL_A(s20);
	v21 = PIXEL_A(s21);
	v22 = PIXEL_A(s22);
	v23 = PIXEL_A(s23);
	v30 = PIXEL_A(s30);
	v31 = PIXEL_A(s31);
	v32 = PIXEL_A(s32);
	v33 = PIXEL_A(s33);
	d = ((v00 + v01 + v02 + v03 + v10 + v11 + v12 + v13 + v20 + v21 + v22 + v23 + v30 + v31 + v32 + v33) / 16) << 24;

	/* R */
	v00 = PIXEL_R(s00);
	v01 = PIXEL_R(s01);
	v02 = PIXEL_R(s02);
	v03 = PIXEL_R(s03);
	v10 = PIXEL_R(s10);
	v11 = PIXEL_R(s11);
	v12 = PIXEL_R(s12);
	v13 = PIXEL_R(s13);
	v20 = PIXEL_R(s20);
	v21 = PIXEL_R(s21);
	v22 = PIXEL_R(s22);
	v23 = PIXEL_R(s23);
	v30 = PIXEL_R(s30);
	v31 = PIXEL_R(s31);
	v32 = PIXEL_R(s32);
	v33 = PIXEL_R(s33);
	d |= ((v00 + v01 + v02 + v03 + v10 + v11 + v12 + v13 + v20 + v21 + v22 + v23 + v30 + v31 + v32 + v33) / 16) << 16;

	/* G */
	v00 = PIXEL_G(s00);
	v01 = PIXEL_G(s01);
	v02 = PIXEL_G(s02);
	v03 = PIXEL_G(s03);
	v10 = PIXEL_G(s10);
	v11 = PIXEL_G(s11);
	v12 = PIXEL_G(s12);
	v13 = PIXEL_G(s13);
	v20 = PIXEL_G(s20);
	v21 = PIXEL_G(s21);
	v22 = PIXEL_G(s22);
	v23 = PIXEL_G(s23);
	v30 = PIXEL_G(s30);
	v31 = PIXEL_G(s31);
	v32 = PIXEL_G(s32);
	v33 = PIXEL_G(s33);
	d |= ((v00 + v01 + v02 + v03 + v10 + v11 + v12 + v13 + v20 + v21 + v22 + v23 + v30 + v31 + v32 + v33) / 16) << 8;

	/* B */
	v00 = PIXEL_B(s00);
	v01 = PIXEL_B(s01);
	v02 = PIXEL_B(s02);
	v03 = PIXEL_B(s03);
	v10 = PIXEL_B(s10);
	v11 = PIXEL_B(s11);
	v12 = PIXEL_B(s12);
	v13 = PIXEL_B(s13);
	v20 = PIXEL_B(s20);
	v21 = PIXEL_B(s21);
	v22 = PIXEL_B(s22);
	v23 = PIXEL_B(s23);
	v30 = PIXEL_B(s30);
	v31 = PIXEL_B(s31);
	v32 = PIXEL_B(s32);
	v33 = PIXEL_B(s33);
	d |= ((v00 + v01 + v02 + v03 + v10 + v11 + v12 + v13 + v20 + v21 + v22 + v23 + v30 + v31 + v32 + v33) / 16) & 0xff;

	return (d);
}

/*
 *    |   |
 * v00|v01|v02
 * ---+---+---
 * v10|v11|v12  For an 'x' in v11, we get the 3x3 grid surrounding then
 * ---+---+---  use the average of them.
 * v20|v21|v22
 *    |   |
 */

#define DIVIDER 7281

static ULONG gsi_scale_average3(struct gsi_info *gsii, CONST ULONG *s, ULONG x, ULONG y)
{
	ULONG v00, v01, v02;
	ULONG v10, v11, v12;
	ULONG v20, v21, v22;

	ULONG s00, s01, s02;
	ULONG s10, s11, s12;
	ULONG s20, s21, s22;

	ULONG d;
	ULONG bckx;
	ULONG bcky;

	/* limit borders */
	if ((LONG)x - 1 < 0)
	{
		bckx = 0;
	}
	else
	{
		bckx = 1;
	}

	if ((LONG)y - 1 < 0)
	{
		bcky = 0;
	}
	else
	{
		bcky = gsii->smod;
	}

	/* fetch pixels */
	s00 = *(s - bcky - bckx);
	s01 = *(s - bcky);
	s02 = *(s - bcky + 1);
	s10 = *(s - bckx);
	s11 = *(s);
	s12 = *(s + 1);
	s20 = *(s + gsii->smod - bckx);
	s21 = *(s + gsii->smod);
	s22 = *(s + gsii->smod + 1);

	/* A */
	v00 = PIXEL_A(s00);
	v01 = PIXEL_A(s01);
	v02 = PIXEL_A(s02);
	v10 = PIXEL_A(s10);
	v11 = PIXEL_A(s11);
	v12 = PIXEL_A(s12);
	v20 = PIXEL_A(s20);
	v21 = PIXEL_A(s21);
	v22 = PIXEL_A(s22);
	d = ((v00 + v01 + v02 + v10 + v11 + v12 + v20 + v21 + v22) * DIVIDER / 65536) << 24;

	/* R */
	v00 = PIXEL_R(s00);
	v01 = PIXEL_R(s01);
	v02 = PIXEL_R(s02);
	v10 = PIXEL_R(s10);
	v11 = PIXEL_R(s11);
	v12 = PIXEL_R(s12);
	v20 = PIXEL_R(s20);
	v21 = PIXEL_R(s21);
	v22 = PIXEL_R(s22);
	d |= ((v00 + v01 + v02 + v10 + v11 + v12 + v20 + v21 + v22) * DIVIDER / 65536) << 16;

	/* G */
	v00 = PIXEL_G(s00);
	v01 = PIXEL_G(s01);
	v02 = PIXEL_G(s02);
	v10 = PIXEL_G(s10);
	v11 = PIXEL_G(s11);
	v12 = PIXEL_G(s12);
	v20 = PIXEL_G(s20);
	v21 = PIXEL_G(s21);
	v22 = PIXEL_G(s22);
	d |= ((v00 + v01 + v02 + v10 + v11 + v12 + v20 + v21 + v22) * DIVIDER / 65536) << 8;

	/* B */
	v00 = PIXEL_B(s00);
	v01 = PIXEL_B(s01);
	v02 = PIXEL_B(s02);
	v10 = PIXEL_B(s10);
	v11 = PIXEL_B(s11);
	v12 = PIXEL_B(s12);
	v20 = PIXEL_B(s20);
	v21 = PIXEL_B(s21);
	v22 = PIXEL_B(s22);
	d |= ((v00 + v01 + v02 + v10 + v11 + v12 + v20 + v21 + v22) * DIVIDER / 65536) & 0xff;

	return (d);
}

#undef DIVIDER

#if USE_ALTIVEC
static ULONG gsi_scale_average4_vec(struct gsi_info *gsii, CONST ULONG *s, ULONG x, ULONG y)
{
	if (((LONG)x - 1 >= 0) && ((LONG)y - 1 >= 0))
	{
		return (gfx_scale_vec_average(gsii, s, x, y));
	}
	else
	{
		return (gsi_scale_average4(gsii, s, x, y));
	}
}
#endif


#define SUB_MODULO (q ? -rws : 0)
#define ADD_MODULO ((q < height - 1) ? rws : 0)
#define SUB_ONE (r ? -1 : 0)
#define ADD_ONE ((r < width - 1) ? 1 : 0)

/* XXX: should add scale3x too so that I could have all the sizes.. but it's boring (see below though) */

static void gfx_scale_scale2x(APTR sbm, ULONG width, ULONG height, APTR tbm)
{
	if (width && height)
	{
		ULONG q;
		ULONG rws = gfx_bitmap_modulo(sbm);
		ULONG rwt = gfx_bitmap_modulo(tbm);
		CONST UBYTE * CONST sbma = gfx_bitmap_array(sbm);
		UBYTE * CONST tbma = gfx_bitmap_array(tbm);
		#if USE_64BIT_WRITES
		UBYTE _ba[sizeof(double) * 2 - 1];
		register struct {
			union {
				ULONG  ulong[2];
				double d64;
			} un;
		} *burst = (APTR) (((IPTR) _ba + sizeof(double) - 1) & -sizeof(double));
		#endif

		q = 0;
		do
		{
			register CONST ULONG *sp = (ULONG *)(sbma + (q * rws + 0) * 4);
			register ULONG *tp = (ULONG *)(tbma + (q * 2 * rwt + 0 * 2) * 4);
			register ULONG r;

			r = 0;
			do
			{
				//register ULONG a, b, c, d, e, f, g, h, i; // bitRocky: GCC5 says "a,c,g,i" are not used
				register ULONG b, d, e, f, h;

				//a = *(sp + SUB_MODULO + SUB_ONE); // bitRocky: GCC5 says, not used
				b = *(sp + SUB_MODULO);
				//c = *(sp + SUB_MODULO + ADD_ONE); // bitRocky: GCC5 says, not used
				d = *(sp + SUB_ONE);
				e = *sp;
				f = *(sp + ADD_ONE);
				//g = *(sp + ADD_MODULO + SUB_ONE); // bitRocky: GCC5 says, not used
				h = *(sp + ADD_MODULO);
				//i = *(sp + ADD_MODULO + ADD_ONE); // bitRocky: GCC5 says, not used

				#if USE_64BIT_WRITES

				burst->un.ulong[0] = d == b && b != f && d != h ? d : e;
				burst->un.ulong[1] = b == f && b != d && f != h ? f : e;
				*((double *)tp) = burst->un.d64;

				burst->un.ulong[0] = d == h && d != b && h != f ? d : e;
				burst->un.ulong[1] = h == f && d != h && b != f ? f : e;
				*((double *)(tp + rwt)) = burst->un.d64;

				#else

				*tp             = d == b && b != f && d != h ? d : e;
				*(tp + 1)       = b == f && b != d && f != h ? f : e;
				*(tp + rwt)     = d == h && d != b && h != f ? d : e;
				*(tp + rwt + 1) = h == f && d != h && b != f ? f : e;

				#endif

				sp++;
				tp += 2;

			} while (++r < width);

		} while (++q < height);
	}
}

#endif /* USE_INTERNAL_SCALER */


#define GSF_NEAREST  (1 << 0UL)
#define GSF_BILINEAR (1 << 1UL)
#define GSF_AVERAGE  (1 << 2UL)
#define GSF_SCALE2X  (1 << 3UL)
#define GSF_ASPECT   (1 << 4UL)
#define GSF_BICUBIC  (1 << 5UL)

/*
 * Scaling routine. If no Bilinear/Nearest/Average/Scale2x tag is specified it falls
 * back to the system's scaling.
 */
ULONG v_gfx_scale(APTR sbm, APTR tbm, ULONG txs, ULONG tys, struct TagItem *tags)
{
	ULONG flags = 0;
	ULONG *rxs = NULL;
	ULONG *rys = NULL;
	ULONG sxs, sys;

	ASSERT(sbm);
	ASSERT(tbm);

	sxs = gfx_bitmap_width(sbm);
	sys = gfx_bitmap_height(sbm);

	FORTAG(tags)
	{
		case SCALETAG_Nearest:
			if (tag->ti_Data)
			{
				flags |= GSF_NEAREST;
			}
			break;

		case SCALETAG_Bilinear:
			if (tag->ti_Data)
			{
				flags |= GSF_BILINEAR;
			}
			break;

		case SCALETAG_Bicubic:
			if (tag->ti_Data)
			{
				flags |= GSF_BICUBIC;
			}
			break;

		case SCALETAG_Average:
			if (tag->ti_Data)
			{
				flags |= GSF_AVERAGE;
			}
			break;

		case SCALETAG_Scale2x:
			if (tag->ti_Data)
			{
				flags |= GSF_SCALE2X;
			}
			break;

		case SCALETAG_AspectX:
			flags |= GSF_ASPECT;
			rxs = (ULONG *)tag->ti_Data;
			break;

		case SCALETAG_AspectY:
			flags |= GSF_ASPECT;
			rys = (ULONG *)tag->ti_Data;
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown tag 0x%lx\n", tag->ti_Tag));
			break;
		#endif
	}
	NEXTTAG

	if (flags & GSF_ASPECT)
	{
		gfx_scale_calc_aspect(gfx_bitmap_width(sbm), gfx_bitmap_height(sbm), &txs, &tys);
	}

	if (sxs == txs && sys == tys)
	{
		/* same size, just blit */
		gfx_blit(sbm, tbm, TAG_DONE);
	}
	else if (txs && tys)
	{
		#if USE_INTERNAL_SCALER
		if (flags & (GSF_NEAREST | GSF_BILINEAR | GSF_BICUBIC | GSF_AVERAGE | GSF_SCALE2X))
		{
			ULONG (*fp)(struct gsi_info *gsii, CONST ULONG *s, ULONG x, ULONG y);
			register ULONG spx; /* scaled pixels */
			ULONG spy;
			register ULONG rws; /* real width */
			ULONG rwt;          /* real width */
			APTR rsbm = sbm;
			APTR bm2x = NULL;

			CHECKARGB32(gfx_bitmap_bm(sbm));
			CHECKARGB32(gfx_bitmap_bm(tbm));

			if ((flags & GSF_SCALE2X) && ((txs > sxs) || (tys > sys))) /* XXX: we could apply scale2x multiple times indeed */
			{
				/*
				 * Is it exact 2x match? if so, just scale2x to tbm and
				 * be done it with! - piru
				 */
				if (txs == sxs * 2 && tys == sys * 2)
				{
					gfx_scale_scale2x(sbm, sxs, sys, tbm);
					goto done;
				}

				/*
				 * No, not that lucky, so get a temporary bitmap, scale2x to
				 * it and then use this as new scale source.
				 */
				if ( (bm2x = gfx_bitmap_create(sxs * 2, sys * 2, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
				{
					gfx_scale_scale2x(sbm, sxs, sys, bm2x);

					rsbm = bm2x;

					sxs *= 2;
					sys *= 2;
				}
			}

			rws = gfx_bitmap_modulo(rsbm);
			rwt = gfx_bitmap_modulo(tbm);

			spx = (sxs * GSIMUL) / txs;
			spy = (sys * GSIMUL) / tys;

			if ((flags & GSF_AVERAGE) && ((txs <= sxs / 4) || (tys <= sys / 4)))
			{
				#if USE_ALTIVEC
				if (use_altivec)
				{
					fp = gsi_scale_average4_vec;
				}
				else
				#endif
				{
					fp = gsi_scale_average4;
				}
			}
			else if ((flags & GSF_AVERAGE) && ((txs <= sxs / 2.5) || (tys <= sys / 2.5)))
			{
				fp = gsi_scale_average3;
			}
			else if ((flags & GSF_AVERAGE) && ((txs <= sxs / 1.6) || (tys <= sys / 1.6)))
			{
				fp = gsi_scale_average2;
			}
			else if (flags & GSF_BILINEAR)
			{
				fp = gsi_scale_bilinear;

			}
			else if (flags & GSF_BICUBIC)
			{
				fp = gsi_scale_bicubic;
			}
			else
			{
				fp = gsi_scale_nearest_neighbour;
			}

			{
				struct gsi_info gsii;
				#if USE_64BIT_WRITES
				UBYTE _ba[sizeof(double) * 2 -1];
				register struct {
					union {
						ULONG  ulong[2];
						double d64;
					} un;
				} *burst = (APTR) (((IPTR) _ba + sizeof(double) - 1) & -sizeof(double));
				#endif
				register CONST UBYTE * CONST sbma = gfx_bitmap_array(rsbm);
				UBYTE * CONST tbma = gfx_bitmap_array(tbm);
				ULONG tpy; /* target pixels */
				ULONG i;
				#if USE_CPU_CACHEHINTS
				CONST ULONG tbma_cacheable = TypeOfMem((APTR)tbma);
				CONST ULONG sbma_cacheable = TypeOfMem((APTR)sbma);
				#endif

				gsii.smod    = rws;
				gsii.swidth  = sxs;
				gsii.sheight = sys;

				i = 0;
				do
				{
					register ULONG tpx;       /* target pixels */
					register ULONG curtpx;
					register	ULONG j;
					register ULONG *tp;
					register CONST ULONG *sp;

					tpy = spy * i;
					gsii.yfrac = tpy & (GSIMUL - 1);
					tpy /= GSIMUL;

					tp = (ULONG *)(tbma + (i * rwt + 0) * 4);
					curtpx = 0;
					j = txs;

					#if USE_CPU_CACHEHINTS
					if (tbma_cacheable)
					{
						cpu_cache_zero(tbma + (i * rwt) * 4, txs);
					}
					if (sbma_cacheable)
					{
						/*
						 * Actually this can be used on non-cacheable memory aswell,
						 * but since it has no effect (other than making one unit busy),
						 * why bother?
						 */
						cpu_cache_stream_start(sbma + (tpy * rws) * 4, 256, 16, 256, 0);
					}
					#endif

					#if USE_64BIT_WRITES

					if (j >= 3)
					{
						register ULONG numloops;

						/* If first pixel is not aligned, write it so we get aligned by 8 */
						if ((IPTR) tp & 4)
						{
							gsii.xfrac = 0;
							sp = (CONST ULONG *)(sbma + (tpy * rws + 0) * 4);
							curtpx += spx;
							*tp++ = fp(&gsii, sp, 0, tpy);
							--j;
						}
						numloops = j >> 1; /* at least 1, as j >= 3 */
						j &= 1;            /* 0 or 1, depending on width/alignment */

						do
						{
							gsii.xfrac = curtpx & (GSIMUL - 1);
							tpx = curtpx / GSIMUL;
							curtpx += spx;
							sp = (CONST ULONG *)(sbma + (tpy * rws + tpx) * 4);

							burst->un.ulong[0] = fp(&gsii, sp, tpx, tpy);

							gsii.xfrac = curtpx & (GSIMUL - 1);
							tpx = curtpx / GSIMUL;
							curtpx += spx;
							sp = (CONST ULONG *)(sbma + (tpy * rws + tpx) * 4);

							burst->un.ulong[1] = fp(&gsii, sp, tpx, tpy);

							((double *)tp)[0] = burst->un.d64;
							tp += (sizeof(double)/sizeof(*tp));

						} while (--numloops);
					}

					/* do any leftover pixels */
					while (j--)
					{
						gsii.xfrac = curtpx & (GSIMUL - 1);
						tpx = curtpx / GSIMUL;
						curtpx += spx;
						sp = (CONST ULONG *)(sbma + (tpy * rws + tpx) * 4);

						*tp++ = fp(&gsii, sp, tpx, tpy);
					}

					#else

					do
					{
						gsii.xfrac = curtpx & (GSIMUL - 1);
						tpx = curtpx / GSIMUL;
						curtpx += spx;
						sp = (CONST ULONG *)(sbma + (tpy * rws + tpx) * 4);

						*tp++ = fp(&gsii, sp, tpx, tpy);

					} while (--j);

					#endif

				} while (++i < tys);

				#if USE_CPU_CACHEHINTS
				if (sbma_cacheable)
				{
					cpu_cache_stream_stop(0);
				}
				#endif
			}

			if (bm2x)
			{
				gfx_bitmap_delete(bm2x);
			}
		}
		else
		#endif
		{
			struct BitScaleArgs bsa;

			CHECKCYBERMAP(gfx_bitmap_bm(sbm));
			CHECKCYBERMAP(gfx_bitmap_bm(tbm));

			bsa.bsa_SrcX = 0;
			bsa.bsa_SrcY = 0;

			bsa.bsa_SrcWidth = gfx_bitmap_width(sbm);
			bsa.bsa_SrcHeight = gfx_bitmap_height(sbm);

			bsa.bsa_DestX = 0;
			bsa.bsa_DestY = 0;

			if (flags & GSF_ASPECT)
			{
				if (gfx_bitmap_width(sbm) > gfx_bitmap_height(sbm))
				{
					bsa.bsa_XSrcFactor = gfx_bitmap_width(sbm);
					bsa.bsa_YSrcFactor = gfx_bitmap_width(sbm);
				}
				else
				{
					bsa.bsa_XSrcFactor = gfx_bitmap_height(sbm);
					bsa.bsa_YSrcFactor = gfx_bitmap_height(sbm);
				}
			}
			else
			{
				bsa.bsa_XSrcFactor = gfx_bitmap_width(sbm);
				bsa.bsa_YSrcFactor = gfx_bitmap_height(sbm);
			}

			bsa.bsa_XDestFactor = txs;
			bsa.bsa_YDestFactor = tys;

			bsa.bsa_SrcBitMap = gfx_bitmap_bm(sbm);
			bsa.bsa_DestBitMap = gfx_bitmap_bm(tbm);

			bsa.bsa_Flags = 0;

			BitMapScale(&bsa);

			txs = bsa.bsa_DestWidth;
			tys = bsa.bsa_DestHeight;
		}
	}

	#if USE_INTERNAL_SCALER
	done:
	#endif

	if (flags & GSF_ASPECT)
	{
		if (rxs)
		{
			*rxs = txs;
		}

		if (rys)
		{
			*rys = tys;
		}
	}
	return (TRUE); /* XXX */
}


static ULONG scale_round_fixed(ULONG val)
{
	if (val & (1 << 15))
	{
		val = (val >> 16) + 1;
	}
	else
	{
		val >>= 16;
	}
	return (val);
}

/*
 * Calculate with proper aspect ratio and constraints.
 */
void gfx_scale_calc_aspect_constraints(ULONG sxs, ULONG sys, ULONG *txs, ULONG *tys)
{
	if (sxs > sys)
	{
		/* horizontal image */
		*tys = min(*tys << 16, sys * ((*txs << 16) / sxs));
		*txs = sxs * (*tys / sys);
		*txs = scale_round_fixed(*txs);
		*tys = scale_round_fixed(*tys);
	}
	else
	{
		/* vertical image */
		*txs = min(*txs << 16, sxs * ((*tys << 16) / sys));
		*tys = sys * (*txs / sxs);
		*tys = scale_round_fixed(*tys);
		*txs = scale_round_fixed(*txs);
	}
}


/*
 * Same but without constraints.
 */
void gfx_scale_calc_aspect(ULONG sxs, ULONG sys, ULONG *txs, ULONG *tys)
{
	/* XXX: fix above needed too ? */
	if (sxs > sys)
	{
		/* horizontal image */
		*tys = (sys * ((*txs << 16) / sxs)) >> 16;
	}
	else
	{
		/* vertical image */
		*txs = (sxs * ((*tys << 16) / sys)) >> 16;
	}
}

