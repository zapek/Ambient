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
 * $Id: cpu_vec.c,v 1.5 2017/07/25 20:36:32 piru Exp $
 */

#include "ambient.h"

#if USE_ALTIVEC

/* public */
#if __GNUC__ > 3
#include <altivec.h>
#endif

/* private */
#include "cpu_vec.h"


void cpu_vec_dst(CONST_APTR ptr, ULONG opts, ULONG channel)
{
	switch (channel)
	{
		case 0:
			vec_dst((VECTOR_UBYTE *)ptr, opts, 0);
			break;

		case 1:
			vec_dst((VECTOR_UBYTE *)ptr, opts, 1);
			break;

		case 2:
			vec_dst((VECTOR_UBYTE *)ptr, opts, 2);
			break;

		case 3:
			vec_dst((VECTOR_UBYTE *)ptr, opts, 3);
			break;
	}
}

void cpu_vec_dstt(CONST_APTR ptr, ULONG opts, ULONG channel)
{
	switch (channel)
	{
		case 0:
			vec_dstt((VECTOR_UBYTE *)ptr, opts, 0);
			break;

		case 1:
			vec_dstt((VECTOR_UBYTE *)ptr, opts, 1);
			break;

		case 2:
			vec_dstt((VECTOR_UBYTE *)ptr, opts, 2);
			break;

		case 3:
			vec_dstt((VECTOR_UBYTE *)ptr, opts, 3);
			break;
	}
}


void cpu_vec_dstst(CONST_APTR ptr, ULONG opts, ULONG channel)
{
	switch (channel)
	{
		case 0:
			vec_dstst((VECTOR_UBYTE *)ptr, opts, 0);
			break;

		case 1:
			vec_dstst((VECTOR_UBYTE *)ptr, opts, 1);
			break;

		case 2:
			vec_dstst((VECTOR_UBYTE *)ptr, opts, 2);
			break;

		case 3:
			vec_dstst((VECTOR_UBYTE *)ptr, opts, 3);
			break;
	}
}


void cpu_vec_dststt(CONST_APTR ptr, ULONG opts, ULONG channel)
{
	switch (channel)
	{
		case 0:
			vec_dststt((VECTOR_UBYTE *)ptr, opts, 0);
			break;

		case 1:
			vec_dststt((VECTOR_UBYTE *)ptr, opts, 1);
			break;

		case 2:
			vec_dststt((VECTOR_UBYTE *)ptr, opts, 2);
			break;

		case 3:
			vec_dststt((VECTOR_UBYTE *)ptr, opts, 3);
			break;
	}
}


void cpu_vec_dss(ULONG channel)
{
	switch (channel)
	{
		case 0:
			vec_dss(0);
			break;

		case 1:
			vec_dss(1);
			break;

		case 2:
			vec_dss(2);
			break;

		case 3:
			vec_dss(3);
			break;
	}
}

#endif /* USE_ALTIVEC */
