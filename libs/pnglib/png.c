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
 * $Id: png.c,v 1.4 2006/04/12 14:01:59 fab Exp $
 */

#include "globals.h"

/* public */
#include <exec/memory.h>
#include <proto/exec.h>

/* private */
#include "png.h"
#include "pngmem.h"
#include "lib.h"

struct ExecBase *SysBase;
struct Library *ZLibBase;
APTR png_mempool;

int lib_init(struct ExecBase *SBase)
{
	SysBase = SBase;

	return (TRUE);
}

#define ZLIBVERSION 1

int lib_open(void)
{
	ZLibBase = OpenLibrary("PROGDIR:libs/z.alib", ZLIBVERSION);
	
	if (!ZLibBase)
	{
		ZLibBase = OpenLibrary("libs/z.alib", ZLIBVERSION);
	}

	if (!ZLibBase)
	{
		ZLibBase = OpenLibrary("/libs/z.alib", ZLIBVERSION);
	}

	if (!ZLibBase)
	{
		ZLibBase = OpenLibrary("mossys:ambient/libs/z.alib", ZLIBVERSION);
	}

	if (!ZLibBase)
	{
		ZLibBase = OpenLibrary("sys:system/ambient/libs/z.alib", ZLIBVERSION);
	}

	if (ZLibBase)
	{
		if ( (png_mempool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED, 16384, 8192)) )
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


void lib_cleanup(void)
{
	if (png_mempool)
	{
		DeletePool(png_mempool);
	}
	CloseLibrary(ZLibBase);
}

