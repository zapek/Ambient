/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
 * Copyright 2015 Ambient Open Source Team
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
 * $Id: wb.c,v 1.7 2016/08/11 17:02:38 itix Exp $
 */

#include "globals.h"

/* public */
#include <proto/exec.h>
#include <proto/hashtable.h>

/* private */
#include "../ipc.h"
#include "lib.h"
#include "wb.h"

struct ExecBase *SysBase;
struct Library *WorkbenchBase;
struct DosLibrary *DOSBase;
struct IntuitionBase *IntuitionBase;
struct Library *UtilityBase;
struct Library *HashTableBase;
struct Library *RexxSysBase;


#define DB_LIB 0


int lib_init(struct ExecBase *SBase, struct LibBase *base)
{
	SysBase = SBase;

	D(LIB,bug("initializing..\n"));

	if ( (DOSBase = (struct DosLibrary *)OpenLibrary("dos.library", 50)) )
	{
		if ( (IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 40)) )
		{
			if ( (UtilityBase = OpenLibrary("utility.library", 36)) )
			{
				D(LIB,bug("init finished\n"));
				return (TRUE);
			}
		}
	}
	return (FALSE);
}

struct MsgPort *wbport;

static BOOL iterator(CONST_APTR h, size_t key, APTR data, APTR userdata)
{
	struct ipc_appwindow *appwin = data;
	FreeMem(appwin, sizeof(*appwin));
	return TRUE;
}

void lib_close(struct LibBase *base)
{
	CloseLibrary(RexxSysBase);

	if (HashTableBase)
	{
		if (base->appwindows)
		{
			/* Free remaining (leaked) app windows. */
			IterateHashTable(base->appwindows, iterator, NULL);
			DeleteHashTable(base->appwindows);
		}

		CloseLibrary(HashTableBase);
	}

	HashTableBase = NULL;
	RexxSysBase = NULL;
}

int lib_open(struct LibBase *base)
{
	if ((HashTableBase = OpenLibrary("hashtable.library", 0)))
	{
		if ((base->appwindows = CreateHashTableTagList(NULL)))
		{
			if ((RexxSysBase = OpenLibrary("rexxsyslib.library", 0)))
				return (TRUE);
		}

		lib_close(base);
	}

	return (FALSE);
}

void lib_cleanup(struct LibBase *base)
{
	CloseLibrary((struct Library *)DOSBase);
	CloseLibrary((struct Library *)IntuitionBase);
	CloseLibrary(UtilityBase);
}
