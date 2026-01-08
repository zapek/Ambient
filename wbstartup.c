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
 * $Id: wbstartup.c,v 1.13 2020/08/16 03:15:18 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <dos/exall.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <workbench/workbench.h>

/* private */
#include "ambient_cat.h"
#include "wbstartup.h"
#include "smartreq.h"
#include "wbstart.h"
#include "methodstack.h"
#include "mui_func.h"
#include "iconio.h"
#include "exdir.h"
#include "prefs.h"
#include "file_func.h"


struct wbnode {
	struct Node n;
	LONG wait;
	LONG toolpri;
	LONG wbstartdelay;
	ULONG stacksize;
	STRPTR defaulttool;
	TEXT name[0];
};

static const CONST_STRPTR tt[] = {
	"STARTPRI",
	"WAIT",
	"TOOLPRI",
	"MOSDONOTSTART",	/* old tooltype name */
	"DONOTSTART",
	"WBSTARTDELAY",

	/* Used by Workbench but not by Ambient */
	/* "DONOTWAIT", */

	NULL
};

static ULONG addwbfunc(APTR obj UNUSED, CONST_STRPTR path, struct ExAllData *ead, APTR userdata)
{
	ULONG retval = TRUE;
	Object *o;

	/*
	 * We don't execute directories.
	 */
	if (ead->ed_Type > 0 &&
	    ead->ed_Type != ST_LINKFILE)
	{
		return (TRUE); /* skip directories */
	}

	if ((o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, FALSE, MV_ViewID_Unknown)))
	{
		struct wbnode *wbn;
		ULONG len;

		D(WBSTARTUP, bug("got object, name: %s..\n", ead->ed_Name));

		len = strlen(path) + strlen(ead->ed_Name) + 2;

		if ((wbn = malloc(sizeof(*wbn) + len)))
		{
			STRPTR dt;
			ULONG runme, cut;

			runme = FALSE;

			strcpy(wbn->name, path);
			AddPart(wbn->name, ead->ed_Name, len);
			cut = strlen(wbn->name) - 5;
			wbn->name[cut] = '\0'; /* remove .info */

			if (!isdir(wbn->name))
			{
				LONG startpri, mosdonotstart, donotstart;

				wbn->name[cut] = '.'; /* put it back */
				icon_read(wbn->name, o,
					ICONTAG_FileSize, ead->ed_Size,
					ICONTAG_FileType, MV_Icon_FileType_File,
					ICONTAG_Position, 0,
					ICONTAG_End     , 0,
					ICONTAG_GetImage, 0,
					TAG_DONE); /* XXX */
				wbn->name[cut] = '\0'; /* and remove again */

				D(WBSTARTUP, bug("name: %s\n", wbn->name));

				methodstack_push(o, 3, OM_GET, MA_Icon_StackSize, &wbn->stacksize); /* XXX: glurk.. we have to find the Tool, not the project's stack.. sigh */
				methodstack_push(o, 3, OM_GET, MA_Icon_DefaultTool, &dt);
				methodstack_push_sync(o, 8, MM_Icon_GetToolTypes,
					tt,
					&startpri,
					&wbn->wait,
					&wbn->toolpri,
					&mosdonotstart,
					&donotstart,
					&wbn->wbstartdelay
				);

				if (!mosdonotstart && !donotstart)
				{
					if (startpri < -128 && startpri > 127)
						startpri = 0;

					wbn->n.ln_Pri = startpri;
					wbn->defaulttool = NULL;
					runme = TRUE;

					if (dt && *dt)
					{
						wbn->defaulttool = malloc(strlen(dt) + 1);

						if (wbn->defaulttool)
						{
							strcpy(wbn->defaulttool, dt);
						}
						else
						{
							runme = FALSE;
						}
					}
				}
			}

			if (runme)
			{
				ENQUEUE((struct MinList *)userdata, wbn);
			}
			else
			{
				D(WBSTARTUP, bug("%s is not runnable\n", wbn->name));
				free(wbn);
			}
		}
		else
		{
			retval = FALSE;
		}

		methodstack_push(o, 1, OM_RELEASE); /* XXX: no sync needed because local */
	}
	return (retval);
}


ULONG tr_wbstartup_execute(CONST_STRPTR path)
{
	struct MinList wbapps;

	THREAD;

	NEWLIST(&wbapps);

	if (path && *path)
	{
		D(WBSTARTUP, bug("handling path: %s\n", path));

		if (exdir(NULL, path, "~(disk|%).info", ED_SIZE, 0, addwbfunc, NULL, NULL, &wbapps))
		{
			struct wbnode *wbn, *nextwbn;

			ITERATELISTSAFE(wbn, nextwbn, &wbapps)
			{
				D(WBSTARTUP, bug("trying with %s\n", wbn->name));

				if (wbn->defaulttool)
				{
					D(WBSTARTUP, bug("okay.. tool: %s, name: %s\n", wbn->defaulttool, wbn->name));
					if (!wbstart(wbn->name,
						WBSTARTTAG_Priority, wbn->toolpri,
						WBSTARTTAG_Stack, wbn->stacksize,
					TAG_DONE))
					{
						smartreq_ok(MSG_WBSTARTUP_REQ, MV_Notification_Error, MSG_PROJECT_NOT_STARTED_REQ, wbn->name, wbn->defaulttool);
					}

					free(wbn->defaulttool);
				}
				else
				{
					D(WBSTARTUP, bug("okay.. name: %s\n", wbn->name));
					if (!wbstart(wbn->name,
						WBSTARTTAG_Priority, wbn->toolpri,
						WBSTARTTAG_Stack, wbn->stacksize,
					TAG_DONE))
					{
						smartreq_ok(MSG_WBSTARTUP_REQ, MV_Notification_Error, MSG_PROGRAM_NOT_STARTED_REQ, wbn->name);
					}
				}

				if (wbn->wbstartdelay > 0)
				{
					#if !USE_LEGACY /* XXX: 1.4 kludge */
					TEXT title[64]; /* XXX: localize that */

					snprintf(title, sizeof(title), "Starting %s...", FilePart(wbn->name));

					methodstack_push_sync(app, 3,
						MM_Application_ChangeScreenTitle,
						title,
						ISLISTEMPTY(&wbapps) ? MF_Application_ChangeScreenTitle_Delayed : MF_Application_ChangeScreenTitle_Temporary
					);
					#endif

					/* Wait max 60 secs */
					Delay((wbn->wbstartdelay > 60 ? 60 : wbn->wbstartdelay) * 50); /* XXX: not very smart.. who cares.. */
				}
				free(wbn);
			}
		}
	}
	else
	{
		smartreq_ok(MSG_WBSTARTUP_REQ, MV_Notification_Warning, MSG_WBSTARTUP_PATH_ERROR_REQ, NULL);
	}
	return (TRUE); /* works everytime (tm) */
}
