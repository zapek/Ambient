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
 * $Id: gfx_scale_vec.c,v 1.6 2025/07/23 22:53:31 geit Exp $
 */

#include "ambient.h"

#if USE_ALTIVEC

/* public */
#if __GNUC__ > 3
#include <altivec.h>
#endif

/* private */
#include "gfx.h"
#include "gfx_scale.h"
#include "gfx_scale_vec.h"
#include "ambient_altivec.h"
#include "align_vec.h"


ULONG gfx_scale_vec_average(struct gsi_info *gsii, CONST ULONG *s, ULONG x UNUSED, ULONG y UNUSED)
{
	VECTOR_UBYTE v1, v2, v3, v4;
	ULONG r;

	v1 = (VECTOR_UBYTE)vec_ld_align((s - 1 - gsii->smod));
	v2 = (VECTOR_UBYTE)vec_ld_align((s - 1));
	v3 = (VECTOR_UBYTE)vec_ld_align((s - 1 + gsii->smod));
	v4 = (VECTOR_UBYTE)vec_ld_align((s - 1 + gsii->smod * 2));

	/* average the rows */
	v1 = vec_avg(v1, v2);
	v3 = vec_avg(v3, v4);
	v1 = vec_avg(v1, v3);

	/* prepare to sum the 4 pixels in v1 */
	v2 = (VECTOR_UBYTE)vec_splat((VECTOR_ULONG)v1, 1);
	v3 = (VECTOR_UBYTE)vec_splat((VECTOR_ULONG)v1, 2);
	v4 = (VECTOR_UBYTE)vec_splat((VECTOR_ULONG)v1, 3);
	v1 = (VECTOR_UBYTE)vec_splat((VECTOR_ULONG)v1, 0);

	/* average */
	v1 = vec_avg(v1, v2);
	v3 = vec_avg(v3, v4);
	v1 = vec_avg(v1, v3);

	vec_ste((VECTOR_ULONG)v1, 0, (unsigned int *) &r);

	return (r);
}

#endif /* USE_ALTIVEC */
