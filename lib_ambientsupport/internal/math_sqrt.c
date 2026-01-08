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
 * $Id: math_sqrt.c,v 1.1 2015/03/02 13:50:55 geit Exp $
 */

#include "../config.h"
#include "../macros.h"
#include "../library.h"

#include "math_sqrt.h"


#if !USE_FPU
/*
 * Algo from Mark Borgerding. Not
 * very fast. There are others using
 * tables if it's too slow.
 */
LONG math_sqrt(LONG val)
{
	LONG guess = 0;
	LONG bit = 1 << 15;

	do
	{
		guess ^= bit;
		/* check to see if we can set this bit without going over sqrt(val)... */
		if (guess * guess > val)
		{
			guess ^= bit; /* it was too much, unset the bit */
		}
	}
	while (bit >>= 1);

	return (guess);
}
#endif

