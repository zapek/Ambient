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
 * $Id: byterun1.c,v 1.6 2006/04/12 14:01:53 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "byterun1.h"


#define GETBITS(x) \
	if (bitshift < depth) \
	{ \
		if (--srclen < 0) \
		{ \
			return (FALSE); \
		} \
		shift |= (UWORD)*in++ << (8 - bitshift); \
		bitshift += 8; \
	} \
	(x) = shift >> (16 - depth); \
	shift <<= depth; \
	bitshift -= depth; \

#define GETBYTE(x) \
	if (bitshift < 8) \
	{ \
		if (--srclen < 0) \
		{ \
			return (FALSE); \
		} \
		shift |= (UWORD)*in++ << (8 - bitshift); \
		bitshift += 8; \
	} \
	(x) = (WORD)shift >> 8; \
	shift <<= 8; \
	bitshift -= 8; \

ULONG byterun1_decode(UBYTE *in, LONG srclen, ULONG depth, UBYTE *out, LONG dstlen)
{
	LONG n, bitshift = 0;
	UWORD shift = 0;
	UBYTE b;

	ASSERT(in);
	ASSERT(srclen);
	ASSERT(depth);
	ASSERT(out);
	ASSERT(dstlen);

	while (dstlen > 0)
	{
		GETBYTE(n);
		if (n < 0)
		{
			/*
			 * Copy the next *in + 1 bytes literally.
			 */
			n = -n + 1;
			dstlen -= n;

			if (dstlen < 0)
			{
				return (FALSE);
			}
			GETBITS(b);
			memset(out, b, n);
			out	+= n;
		}
		else
		{
			/*
			 * Replicate the next byte 256 - *in + 1 times.
			 * *in as 255 should be a NOP but PhotoShop didn't
			 * care and no known app uses 255 as NOP so there we go.
			 */
			n += 1;
			dstlen -= n;

			if (dstlen < 0)
			{
				return (FALSE);
			}

			while (n-- > 0)
			{
				GETBITS(b);
				*out++ = b;
			}
		}
	}
	return (TRUE);
}

