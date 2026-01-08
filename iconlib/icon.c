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
 * $Id: icon.c,v 1.6 2021/01/31 21:48:53 piru Exp $
 */

#include "globals.h"

/* public */
#include <proto/exec.h>
#include <proto/intuition.h>
#include <exec/system.h>

/* private */
#include "lib.h"

struct ExecBase *SysBase;
struct Library *WorkbenchBase;
struct DosLibrary *DOSBase;
#if USE_ICONLIB_PNGLIB
struct Library *UtilityBase;
#endif

#define DB_LIB 0

LONG libnix_altivec; /* for setjmp/longjmp */

int lib_init(struct ExecBase *SBase)
{
	SysBase = SBase;

	D(LIB,bug("initializing..\n"));

	if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 50)))
	{
		#if USE_ICONLIB_PNGLIB
		if ((UtilityBase = OpenLibrary("utility.library", 50)))
		#endif
		{
			NewGetSystemAttrsA(&libnix_altivec, sizeof(libnix_altivec), SYSTEMINFOTYPE_PPC_ALTIVEC, NULL);
			D(LIB,bug("init finished\n"));
			return (TRUE);
		}
	}
	return (FALSE);
}

struct MsgPort *wbport;

int lib_open(void)
{
	if ((WorkbenchBase = OpenLibrary("workbench.library", 37))) /* Grmbl */
	{
		return (TRUE);
	}
	return (FALSE);
}


void lib_cleanup(void)
{
	CloseLibrary(WorkbenchBase);
	#if USE_ICONLIB_PNGLIB
	CloseLibrary(UtilityBase);
	#endif
	CloseLibrary((struct Library *)DOSBase);
}

