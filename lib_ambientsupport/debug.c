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
 * $Id: debug.c,v 1.1 2015/03/02 13:50:53 geit Exp $
 */

#include "macros.h"

#include "debug.h"

#ifdef DEBUG

/* public */
#include <intuition/classes.h>
#include <utility/hooks.h>
#include <proto/exec.h>
#include <proto/input.h>
#include <proto/dos.h>

/* private */
#include "debug.h"
#include "prefs.h"
#include "menus.h"
#include "input.h"


/*
 * How to add a debug var:
 *
 * - update debug.c
 * - update debug.h
 * - update menus.c
 *
 */

#define DBENT(_s,_x) {DSI_DEBUG + 1 + DB_##_s, _x, #_s }
#define ENVPATH "Ambient/"

/* add new entries at the end */
struct debug_flags db_a[] = {
	DBENT(ICONIO,       FALSE),
	DBENT(PROC,         FALSE),
	DBENT(DUMPIMAGE,    FALSE),
	DBENT(WBSTART,      FALSE),
	DBENT(WBSTARTUP,    FALSE),
	DBENT(REXX,         FALSE),
	DBENT(LAYOUT,       FALSE),
	DBENT(DEVICEIO,     FALSE),
	DBENT(DRAGDROP,     FALSE),
	DBENT(INIT,         FALSE),
	DBENT(ARGS,         FALSE),
	DBENT(CLASS,        FALSE),
	DBENT(LIB,          FALSE),
	DBENT(DOSLISTCACHE, FALSE),
	DBENT(PATH,         FALSE),
	DBENT(COPY,         FALSE),
	DBENT(DEFICON,      FALSE),
	DBENT(DEFICONPOOL,  FALSE),
	DBENT(EXDIR,        FALSE),
	DBENT(DOMETHOD,     FALSE),
	DBENT(RECURSE,      FALSE),
	DBENT(SCANDIR,      FALSE),
	DBENT(PREFSIO,      FALSE),
	DBENT(LABELSPLIT,   FALSE),
	DBENT(SHORTCUT,     FALSE),
	DBENT(MIMEURI,      FALSE),
	DBENT(RDARGS,       FALSE),
	DBENT(RECOG,        FALSE),
	DBENT(PREFSPOOL,    FALSE),
	DBENT(SNDDRV,       FALSE),
	DBENT(SOUND,        FALSE),
	DBENT(NOTIFY,       FALSE),
	DBENT(MIMEACTIONED, FALSE),
	DBENT(MIMETYPE,     FALSE),
	DBENT(ADVANCEDPREFS,FALSE),
	DBENT(ACTIONDISP,   FALSE),
	DBENT(APPICON,      FALSE),
	{NULL, NULL, NULL}
};


ULONG debug_init(void)
{
	ULONG i;

	if (check_qualifier(IEQUALIFIER_CAPSLOCK))
	{
		for (i = 0; db_a[i].prefs; i++)
		{
			db_a[i].active = TRUE;
		}
	}


	/*  check env:ambient/#? vars
	 */
	{
		struct DosLibrary *DOSBase;
		UBYTE buf[4];

		if ((DOSBase = (struct DosLibrary *)OpenLibrary("dos.library",50UL)))
		{

			if ((GetVar(ENVPATH "all", buf, sizeof(buf), 0 ) > 0) && (atoi(buf) == 1 ))
			{
	            for (i = 0; db_a[i].prefs; i++)
				{
					db_a[i].active = TRUE;
				}
			}
			else
			{
				for (i = 0; db_a[i].prefs; i++)
				{
					UBYTE temp[64];

					sprintf(temp, "%s%s", ENVPATH, db_a[i].envvar);

					if ((GetVar(temp , buf, sizeof(buf), 0 ) > 0) && (atoi(buf) == 1))
					{
						db_a[i].active = TRUE;
					}
				}
			}

			CloseLibrary((struct Library *)DOSBase);
		}
	}

	return (TRUE);
}


void debug_cleanup(void)
{
	/* nothing to cleanup */
}


void dump_image(UBYTE *p, ULONG size, ULONG width)
{
	ULONG i = 0;

	ASSERT(p);

	while (size--)
	{
		if (!(i % width))
		{
			SDB(("\n"));
		}
		SDB(("%02lx ", (ULONG)(*p++)));
		i++;
	}
	SDB(("\n"));
}


#if USE_INTERNAL_DOMETHOD
ULONG DoMethodA(Object *obj, Msg message); /* avoid the macros in libamiga */
ULONG DoMethodA(Object *obj, Msg message)
{
	struct Task *t = FindTask(NULL);
	D(DOMETHOD,bug("obj: %p, msg: %p (MethodID: 0x%lx)\n", obj, message, *((ULONG *)message)));

	if (t == MainTask)
	{
		if (obj)
		{
			Class *cl = (struct IClass *)(((ULONG *)obj)[-1]);

			REG_A0 = (ULONG)cl;
			REG_A1 = (ULONG)message;
			REG_A2 = (ULONG)obj;

			return ((*MyEmulHandle->EmulCallDirect68k)(cl->cl_Dispatcher.h_Entry));
		}
		else
		{
			PDB(("*** DoMethod(NULL, ..) detected! msg: %p (MethodID: 0x%lx)\n", message, *((ULONG *)message + 1)));
		}
	}
	else
	{
		PDB(("*** DoMethod() not done in the current task (%p)! Done in %p instead. msg: %p (MethodID: 0x%lx)\n", MainTask, t, message, *((ULONG *)message + 1)));
	}
	return (0);
}
#endif

#endif /* DEBUG */
