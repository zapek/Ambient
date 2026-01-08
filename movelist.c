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
 * $Id: movelist.c,v 1.12 2018/07/24 09:48:52 itix Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <proto/dos.h>

/* private */
#include "getdirsize.h"
#include "file_func.h"
#include "movelist.h"
#include "mui_func.h"
#include "movedir.h"
#include "movefile.h"
#include "dragdrop.h"
#include "smartreq.h"
#include "notify.h"
#include "time_func.h"
#include "fastcopy.h"
#include "methodstack.h"
#include "examine64.h"

#define QUIET_COPY_THRESHOLD 0

static ULONG tr_movelist(APTR obj, APTR winobj, struct MinList *ml, CONST_STRPTR path, ULONG copy, APTR progressobj, ULONG * filecount, QUAD * totaldone, ULONG noicon, ULONG rename)
{
	struct dragdropnode *ddn, *nextddn;
	ULONG rc = MOVE_OK;
	ULONG res = TRUE;
	BOOL  skipall = FALSE;
	ULONG reqflags = 0;
	ULONG entrycount = 0;
	TEXT fullpath[PATH_SIZE];
	TEXT firstfile[PATH_SIZE];

	THREAD;
	ASSERT(ml);

	ITERATELIST(ddn, ml)
	{
		entrycount++;
	}

	ddn = FIRSTNODE(ml);

	/* XXX: rather than using this threshold, better get time since
	 *      last copy in the loop and enable update if a given amount of time has elapsed
	 */

	if(entrycount > QUIET_COPY_THRESHOLD)
	{
		stccpy(fullpath, path, sizeof(fullpath));
		AddPart(fullpath, FilePart(ddn->path), sizeof(fullpath));
		notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, FALSE);
		if(!copy)
		{
			stccpy(firstfile, ddn->path, sizeof(firstfile));
			notify_action(firstfile, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, FALSE);
		}
	}

		ITERATELIST(ddn, ml)
		{
retry:
			if (ddn->type == MV_Icon_FileType_Directory)
			{
				rc = tr_movedir(obj, winobj, ddn->path, path, copy, progressobj, filecount, totaldone, noicon, rename, &reqflags);
			}
			else if (ddn->type == MV_Icon_FileType_File)
			{
				rc = tr_movefile(obj, winobj, ddn->path, path, copy, progressobj, filecount, totaldone, noicon, rename, &reqflags);
			}

			if (res && rc == MOVE_FAILED && !skipall)
			{
				set( progressobj, MUIA_Window_Sleep, TRUE );
				ULONG but = smartreq_request_sync(NULL, GSI(MSG_MOVELIST_TITLE), GSI(MSG_MOVELIST_BUTS_RETRYSKIPSKIPALLCANCEL), MV_Notification_Error, GSI(MSG_MOVELIST_ERRORWHILEMOVING), ddn->path);
				set( progressobj, MUIA_Window_Sleep, FALSE );
				switch(but)
				{
					case 1:  /* retry */
						rc = MOVE_OK;
						goto retry;
					case 2:  /* skip */
						break;
					case 3:  /* skip all */
						skipall = TRUE;
						break;
					case 0:
						res = FALSE;
						break;
				}
			}

			if (!res || rc == MOVE_ABORTED)
				break;
		}

	ITERATELISTSAFE(ddn, nextddn, ml)
	{
		free(ddn);
	}
	free(ml);

	if(entrycount > QUIET_COPY_THRESHOLD)
	{
		notify_action(fullpath, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, TRUE);
		if(!copy)
		{
			notify_action(firstfile, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_Enable, TRUE);
		}
	}

	return TRUE;
}


ULONG tr_move(APTR obj, APTR refwin, struct MinList * l, CONST_STRPTR srcpath, CONST_STRPTR dstpath, ULONG copy, ULONG noicon, ULONG rename)
{
	APTR  progressobj;
	ULONG filecount = 0;
	QUAD  totaldone = 0;
	TEXT from[PATH_SIZE];
	TEXT to[PATH_SIZE];
	CONST_STRPTR ptr = dstpath;
	ULONG len, rc, numfiles;
	QUAD totalsize;

	progressobj = (APTR)methodstack_push_sync(app, 4, MM_Application_CreateProgresswin, copy ? MV_Progresswin_Look_CopyMany : MV_Progresswin_Look_MoveMany, (ULONG)FindTask(NULL), refwin);

	if(l)
	{
		struct dragdropnode *ddn = FIRSTNODE(l);
		strcpy(from, ddn->path);

		totalsize = 0;
		numfiles = 0;

		ITERATELIST(ddn, l)
		{
			/* ddn->type doesnt work here. Why? */
			/* probably because it's not always set in the first place at list construction (fab1) */

			if (isdir(ddn->path))
			{
				struct dirsize ds;
				getdirsize(ddn->path, &ds);
				totalsize += ds.totalsize;
				numfiles += ds.numfiles;
			}
			else
			{
				/* We should count info files too but maybe it is not important... */

				struct fileinfo64 finfo;

				if (examine64(ddn->path, &finfo))
				{
					totalsize += finfo.fi_Size;
					numfiles += 1;
				}
			}
		}
	}
	else
	{
		struct dirsize ds;
		strcpy(from, srcpath);
		getdirsize(srcpath, &ds);
		totalsize = ds.totalsize;
		numfiles = ds.numfiles;
	}

	methodstack_push(progressobj, 3, MM_Progresswin_InitProgress, &totalsize, numfiles);

	/* Supress DOS Notify for source and destination views */
	if (!copy)
		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, from, FALSE );

	/* EnableDOSNotify makes parent from it, so we have to trick it. */
	stccpy( to, ptr, sizeof(to));

	len = strlen( to );
	if ( !len || ( to[ len - 1 ] != ':' && to[ len - 1 ] != '/' ) )
	{
		if (len + 1 < sizeof(to))
		{
			to[len++] = '/';
			to[len] = '\0';
		}
	}

	methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, to, FALSE );

	/* Let's copy now */
	if(progressobj)
	{
		if (l)
		{
			rc = tr_movelist(obj, refwin, l, dstpath, copy, progressobj, &filecount, &totaldone, noicon, rename);
		}
		else
		{
			ULONG reqflags = 0;

			rc = tr_movedir(obj, refwin, srcpath, dstpath, copy, progressobj, &filecount, &totaldone, noicon, rename, &reqflags);
		}

		methodstack_push(progressobj, 5, MM_Progresswin_Update, NULL, NULL, 1, NULL);
	}
	else
	{
		rc = TRUE;
	}

	/* Enable DosNotify back */
	if (!copy)
		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, from, TRUE);

	methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, to, TRUE );

	return (rc);
}
