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
 * $Id: align_vec.c,v 1.8 2017/07/25 20:36:32 piru Exp $
 */

#include "ambient.h"

#if USE_ALTIVEC

/* public */
#if __GNUC__ > 3
#include <altivec.h>
#endif

/* private */
#include "align_vec.h"

#if 0 /* it's 'static inline' in align_vec.h now */

VECTOR_ULONG vec_ld_align(CONST_APTR ptr)
{
	VECTOR_ULONG v0;
	VECTOR_ULONG v1;
	VECTOR_UBYTE valign;
	ULONG *p;

	p = ptr;

	if ((ULONG)p & 15)
	{
		v0 = vec_ld(0, p);
		v1 = vec_ld(16, p);
		valign = vec_lvsl(0, p);

		return (vec_perm(v0, v1, valign));
	}
	else
	{
		return (vec_ld(0, p));
	}
}

#endif

#endif /* USE_ALTIVEC */
