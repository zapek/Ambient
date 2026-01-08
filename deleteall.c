/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
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
 * $Id: deleteall.c,v 1.10 2013/07/26 14:35:41 leif Exp $
 */

#include "ambient.h"

/* public */

#include <proto/dos.h>

/* private */
#include "deleteall.h"
#include "deletefile.h"
#include "deletedir.h"
#include "file_func.h"
#include "methodstack.h"
#include "mui_func.h"
#include "name.h"
#include "notify.h"


ULONG tr_deleteall(APTR obj, APTR refwin, STRPTR *path, BOOL noicon)
{
	ULONG retval = FALSE;
	STRPTR firstpath;
	APTR progressobj;
	ULONG filecount = 0;

	THREAD;

	/* Supress DOS Notify for source and destination views */

	firstpath = *path;
	methodstack_push( app, 3, MM_Application_EnableDOSNotify, firstpath , FALSE );

	notify_action(firstpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, FALSE);

	progressobj = (APTR)methodstack_push_sync(app, 4, MM_Application_CreateProgresswin, MV_Progresswin_Look_Delete, (ULONG)FindTask(NULL), refwin);

	if(progressobj)
	{
		ULONG delete_flags = 0;

		do
		{
			if (islink(*path))
			{
            /* delete links as files, damnit!!11 */
            retval = deletefile(obj, refwin, *path, progressobj, &filecount, noicon, &delete_flags);
			}
			else if (isdir(*path))
			{
				retval = deletedir(obj, refwin, *path, progressobj, &filecount, noicon, &delete_flags);
			}
			else
			{
				retval = deletefile(obj, refwin, *path, progressobj, &filecount, noicon, &delete_flags);
			}
			path++;
		}
		while (retval && *path);

		/* Enable DOS Notify for source and destination views */

		methodstack_push(progressobj, 5, MM_Progresswin_Update, NULL, NULL, filecount, NULL);
	}

	notify_action(firstpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, TRUE);

	methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, firstpath, TRUE );

	return retval;
}
