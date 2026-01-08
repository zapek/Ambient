/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2004 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
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
 * $Id: appwindow.c,v 1.8 2016/06/19 16:33:06 itix Exp $
 */

#include "globals.h"

/* public */
#include <exec/memory.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/hashtable.h>

/* private */
#include "clib/wb_protos.h"
#include "../ipc.h"
#include "lib.h"


#define DB_ADDAPPWINDOW 0
#define DB_REMOVEAPPWINDOW 0

APTR LIB_AppWindowObtain(struct Window *win, struct LibBase *base)
{
	struct ipc_appwindow *appwin = NULL;
	
	ObtainSemaphore(&base->appwinsem);
	GetHashDataByKey(base->appwindows, (IPTR)win, (APTR)&appwin);

	return appwin;
}

VOID LIB_AppWindowRelease(struct LibBase *base)
{
	ReleaseSemaphore(&base->appwinsem);
}

/****** workbench.library/AddAppWindowA() *****************************
*
* NAME
*   AddAppWindowA -- Add an application window
*
* SYNOPSIS
*   AppWindow = AddAppWindowA(id, userdata, window, msgport, taglist)
*
*   APTR AddAppWindowA(ULONG, ULONG, struct Window *, struct MsgPort *,
*           struct TagItem *);
*
* FUNCTION
*   Add an application window. User may drag'n drop files to AppWindow
*   in which case Ambient sends an AppMessage to your message port.
*
*   Prior workbench.library V51, AppWindow could not be added if Ambient
*   was not running. In V51 (MorphOS 3.10) this limitation was removed.
*
* INPUTS
*   id        - private id
*   userdata  - private user data
*   window    - Intuition window pointer
*   msgport   - message port to receive AppMessage
*   taglist   - no tags defined yet (you may pass NULL)
*
* RESULT
*   AppWindow - A pointer to AppWindow handle or NULL on an error.
*
* SEE ALSO
*   RemoveAppWindow()
*
*****************************************************************************
*
*/
struct AppWindow *LIB_AddAppWindowA(void)
{
	struct ipc_appwindow *appwin = NULL;
	ULONG id = REG_D0;
	ULONG userdata = REG_D1;
	struct Window *window = (APTR)REG_A0;
	struct MsgPort *msgport = (APTR)REG_A1;
	//APTR tags = (APTR)REG_A2;
	struct LibBase *base = (APTR)REG_A6;

	if (window && msgport)
	{
		if ((appwin = AllocMem(sizeof(*appwin), MEMF_ANY)))
		{
			APTR dummy;

			appwin->id = id;
			appwin->userdata = userdata;
			appwin->userport = msgport;
			appwin->window = window;

			ObtainSemaphore(&base->appwinsem);

			if (!GetHashDataByKey(base->appwindows, (IPTR)window, &dummy))
			{
				if (!InsertHash(base->appwindows, (IPTR)window, appwin))
				{
					FreeMem(appwin, sizeof(*appwin));
					appwin = NULL;
				}
			}

			ReleaseSemaphore(&base->appwinsem);
		}
	}

	return appwin ? (APTR)window : NULL;
}


/****** workbench.library/RemoveAppWindowA() **************************
*
* NAME
*   RemoveAppWindow -- Remove an application window
*
* SYNOPSIS
*   Success = RemoveAppWindow(AppWindow)
*
*   BOOL RemoveAppWindow(APTR);
*
* FUNCTION
*   Remove an application window.
*
*   You MUST check the AppMessage port after removal.
*
* INPUTS
*   AppWindow - an AppWindow handle returned by AddAppWindowA().
*
* RESULT
*   Success   - TRUE if the AppWindow was removed or FALSE if an error occured.
*
* SEE ALSO
*   AddAppWindowA()
*
*****************************************************************************
*
*/
BOOL LIB_RemoveAppWindow(void)
{
	struct ipc_appwindow *msg = NULL;
	struct LibBase *base = (APTR)REG_A6;
	size_t appWindow = REG_A0;

	if (appWindow)
	{
		ObtainSemaphore(&base->appwinsem);

		if (RemoveHashByKey(base->appwindows, (IPTR)appWindow, (APTR)&msg))
		{
			FreeMem(msg, sizeof(*msg));
		}

		ReleaseSemaphore(&base->appwinsem);
	}

	return msg ? TRUE : FALSE;
}
