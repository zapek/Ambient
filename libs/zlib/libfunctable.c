/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: libfunctable.c,v 1.2 2005/07/04 23:06:12 laire Exp $
 */

#include "globals.h"

/* public */

/* private */
#include "lib.h"
#include "clib/z_protos.h"


ULONG LibFuncTable[] =
{
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
	(ULONG)&LIB_Open,
	(ULONG)&LIB_Close,
	(ULONG)&LIB_Expunge,
	(ULONG)&LIB_GetQueryAttr,
	0xffffffff,
	FUNCARRAY_32BIT_SYSTEMV,
	(ULONG)&deflateInit_,
	(ULONG)&deflate,
	(ULONG)&deflateEnd,
	(ULONG)&inflateInit_,
	(ULONG)&inflate,
	(ULONG)&inflateEnd,
	(ULONG)&deflateInit2_,
	(ULONG)&deflateSetDictionary,
	(ULONG)&deflateCopy,
	(ULONG)&deflateReset,
	(ULONG)&deflateParams,
	(ULONG)&inflateInit2_,
	(ULONG)&inflateSetDictionary,
	(ULONG)&inflateSync,
	(ULONG)&inflateReset,
	(ULONG)&compress,
	(ULONG)&compress2,
	(ULONG)&uncompress,
	(ULONG)&adler32,
	(ULONG)&crc32,
	0xffffffff,
	FUNCARRAY_END
};
