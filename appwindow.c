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
 * $Id: appwindow.c,v 1.6 2006/04/12 14:01:53 fab Exp $
 */

#include "ambient.h"

/* public */
#include <workbench/workbench.h>
#include <proto/exec.h>

/* private */
#include "appwindow.h"
#include "ipc.h"


static struct MinList appwinlist;
static struct SignalSemaphore appwinsem;


struct ipc_appwindow * appwindow_obtain(struct Window *win)
{
	struct ipc_appwindow *appwin;
	
	ObtainSemaphore(&appwinsem);

	ITERATELIST(appwin, &appwinlist)
	{
		if (appwin->window == win)
		{
			return (appwin);
		}
	}
	return (NULL);
}


void appwindow_release(void)
{
	ReleaseSemaphore(&appwinsem);
}


ULONG appwindows_init(void)
{
	NEWLIST(&appwinlist);
	InitSemaphore(&appwinsem);

	return (TRUE);
}


void appwindows_cleanup(void)
{
	/* nothing to do */
}


ULONG appwindow_add(struct ipc_appwindow *msg)
{
	ULONG retval = FALSE;
	ASSERT(msg);

	if (!appwindow_obtain(msg->window))
	{
		ADDTAIL(&appwinlist, msg);
		retval = TRUE;
	}

	appwindow_release();

	if (!retval)
	{
		PDB(("window %p already added\n", msg->window));
	}
	return (retval);
}


ULONG appwindow_remove(struct ipc_appwindow *msg)
{
	ULONG retval = FALSE;
	ASSERT(msg);

	if (appwindow_obtain(msg->window))
	{
		REMOVE(msg);
		retval = TRUE;
	}

	appwindow_release();

	if (!retval)
	{
		PDB(("window %p doesn't exist\n", msg->window));
	}
	return (retval);
}

