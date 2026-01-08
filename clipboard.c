/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2010 Ambient Open Source Team
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
 * $Id: clipboard.c,v 1.9 2013/10/29 22:30:36 geit Exp $
 */


#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "clipboard.h"
#include "mui_func.h"
#include "threads.h"
#include "file_func.h"
#include "methodstack.h"
#include "time_func.h"
#include "smartreq.h"
#include "movelist.h"
#include "dragdrop.h"
#include "appclass.h"
#include "viewapi.h"
#include "prefs.h"

#include "debug.h"


static struct SignalSemaphore clipboardsem;

#define ITERATECLIPBOARD(node) for(node=FIRSTNODE(&(clipctx.entries));NEXTNODE(node);node=NEXTNODE(node))

struct clipboard_context {
	clipboard_mode_t mode;
	struct timeval add_timestamp;
	struct MinList entries;
	ULONG count;
	ULONG handleicons;
};

static struct clipboard_context clipctx;

static BOOL clipboard_list_build(struct MinList ** copy, STRPTR destpath);
static int tv_substract(struct timeval *tv1, struct timeval *tv2);


void clipboard_reset_timer()
{
	getlocaltime(&(clipctx.add_timestamp));
}

BOOL clipboard_is_present(STRPTR filename)
{
	struct clipboard_node * cbnode;
	BOOL found = FALSE;

	ObtainSemaphore(&clipboardsem);

	ITERATECLIPBOARD(cbnode)
	{
		if(!stricmp(cbnode->filename, filename))
		{
			found = TRUE;
			break;
		}
	}

	ReleaseSemaphore(&clipboardsem);

	return found;
}

#if 0
// itix: not used atm
BOOL clipboard_is_empty(void)
{
	BOOL res;

	ObtainSemaphore(&clipboardsem);

	res = (ISLISTEMPTY( &(clipctx.entries) ))?TRUE:FALSE;

	ReleaseSemaphore(&clipboardsem);

	return res;
}
#endif

ULONG clipboard_init(void)
{
	InitSemaphore(&clipboardsem);

	clipctx.mode  = CLIPBOARD_VOID;
	clipctx.count = 0;
	clipctx.handleicons = FALSE;

	memset(&clipctx.add_timestamp, 0, sizeof(struct timeval));

	NEWLIST( &(clipctx.entries) );

	return TRUE;
}

void clipboard_cleanup(void)
{
	clipboard_clear();
}

void clipboard_clear(void)
{
	struct clipboard_node *n, *nextn;

	clipctx.mode = CLIPBOARD_VOID;
	clipctx.count = 0;
	memset(&clipctx.add_timestamp, 0, sizeof(struct timeval));

	ObtainSemaphore(&clipboardsem);

	ITERATELISTSAFE(n, nextn, &(clipctx.entries))
	{
		if(n->filename)
		{
			free(n->filename);
		}
		free(n);
	}
	NEWLIST(&(clipctx.entries));

	ReleaseSemaphore(&clipboardsem);
}


void clipboard_set_mode(clipboard_mode_t mode, ULONG handleicons)
{
	clipctx.mode = mode;
	clipctx.handleicons = handleicons;
}

clipboard_mode_t clipboard_get_mode(void)
{
	return clipctx.mode;
}

BOOL clipboard_add(STRPTR filename)
{
	BOOL ret = FALSE;

	if (filename != NULL && !clipboard_is_present(filename))
	{
		struct clipboard_node * cbnode;

		cbnode = (struct clipboard_node *) malloc(sizeof(*cbnode));

		if(cbnode)
		{
			int len = strlen(filename) + 1;
			cbnode->filename = (STRPTR) malloc(len);

			if(cbnode->filename)
			{
				strcpy(cbnode->filename, filename);
				ObtainSemaphore(&clipboardsem);
				ADDTAIL( &(clipctx.entries), cbnode);
				ReleaseSemaphore(&clipboardsem);

				clipctx.count++;
				getlocaltime(&(clipctx.add_timestamp));
				ret = TRUE;
			}
			else
			{
				free(cbnode);
			}
		}
	}

	return ret;
}

static BOOL clipboard_paste(STRPTR destpath, ULONG sync, LONG viewid, ULONG rename)
{
	struct clipboard_node * cbnode;
	APTR wo = NULL;
	BOOL copy = (clipctx.mode == CLIPBOARD_CUT)?FALSE:TRUE;
	struct timeval timestamp;
	ULONG res = TRUE;
	struct MinList * entries_copy = NULL;

	if(destpath == NULL || clipctx.mode == CLIPBOARD_VOID ||
	   !isdir(destpath) || clipctx.count == 0)
	{
		return TRUE;
	}

restart_paste:

	/* let's popup a reminder requester if too much time has passed since last cut/copy operation */

	getlocaltime(&timestamp);

	if( tv_substract(&timestamp, &(clipctx.add_timestamp)) > CLIPBOARD_REMINDER_DELAY)
	{
		TEXT content[1024]; /* XXX: lazy... */
		int i=0;
		snprintf(content, sizeof(content), GSI(MSG_CLIPBOARD_FILESWILLBE), clipctx.count, copy ? GSI(MSG_CLIPBOARD_COPIED):GSI(MSG_CLIPBOARD_CUT), destpath);

		ObtainSemaphore(&clipboardsem);
		ITERATECLIPBOARD(cbnode)
		{
			strncat(content, "\033b - ", sizeof(content));
			strncat(content, cbnode->filename, sizeof(content));
			strncat(content, "\033n\n", sizeof(content));
			i++;
			if(i == CLIPBOARD_REMINDER_MAX_ENTRIES)
			{
				strncat(content, "\033b - ...\033n\n", sizeof(content));
				break;
			}
		}
		ReleaseSemaphore(&clipboardsem);

		if (smartreq_request_sync(NULL, GSI(MSG_CLIPBOARD_TITLE), GSI(MSG_CLIPBOARD_COPYCANCEL), MV_Notification_Help, content) == 1)
		{
			clipboard_reset_timer();
			goto restart_paste;
		}
		else
		{
			return TRUE;
		}
	}

	// kprintf("clipboard_paste to %s VIEWID %d copy = %d\n", destpath, viewid, copy);

	if ( viewid > 0 )
	{
		wo = (APTR)methodstack_push_sync( app, 2, MM_Application_FindWindowByID, viewid );
	}

	/* As copy process can take a long time, we don't want to lock clipboard list, so we copy it... */

	if(clipboard_list_build(&entries_copy, destpath) && !ISLISTEMPTY(entries_copy))
	{
		if(wo)
		{
			ULONG noicon = FALSE;

			if(!clipctx.handleicons)
			{
				noicon = TRUE;
			}

			if ( sync )
			{
				res = do_action_sync(NULL, TA_File_Move,					
					TT_File_Move_SrcList, entries_copy,
					TT_File_Move_DstPath, destpath,
					TT_File_Move_Refwin, wo,
					TT_File_Move_Copy, copy,
					TT_File_Move_NoIcon, noicon,
					TT_File_Move_Rename, rename,
				TAG_DONE);
			}
			else
			{
				res = do_action(NULL, TA_File_Move,
					TT_File_Move_SrcList, entries_copy,
					TT_File_Move_DstPath, destpath,
					TT_File_Move_Refwin, wo,
					TT_File_Move_Copy, copy,
					TT_File_Move_NoIcon, noicon,
					TT_File_Move_Rename, rename,
				TAG_DONE);
			}
		}
		else
		{
			/* XXX : copy/move without window progress */		 
		}

		if(!copy)
		{
			clipboard_clear();
		}
	}

	/* entries_copy is freed by tr_movelist(), for listviewlist compatibility */

	if(!res)
		return FALSE;
	else
		return TRUE;
}

ULONG tr_clipboard_paste(STRPTR destpath, LONG viewid, ULONG rename)
{
	THREAD;
	return clipboard_paste(destpath, 1, viewid, rename);
}


/*
BOOL clipboard_paste_as  (STRPTR destpath);   // paste the selection to destination. pops up a requester to give new names
BOOL clipboard_paste_into(STRPTR archive);    // paste into an archive (reading mime to check destination ?)
*/

/* not used, so disabled (geit) */
#if 0
static struct MinList * clipboard_names(void)
{  
	return &(clipctx.entries);
}
#endif

static int tv_substract(struct timeval *tv1, struct timeval *tv2)
{
	int result;
	result = (tv1->tv_secs - tv2->tv_secs);
	return result;
}

BOOL clipboard_list_build(struct MinList ** copy, STRPTR destpath)
{
	BOOL res = TRUE;
	struct clipboard_node * cbnode;
	struct dragdropnode * cbnode_copy = NULL;

	*copy = (struct MinList *) malloc(sizeof(**copy));

	if(*copy)
	{
		NEWLIST(*copy);

		ObtainSemaphore(&clipboardsem);
		ITERATECLIPBOARD(cbnode)
		{
			int len;
			ULONG skip = isdir(cbnode->filename) && is_path_contained(destpath, cbnode->filename);

			if(skip)
			{
				/* XXX : requester to notify user */
				DB(("Recursive copy/paste, skipping %s -> %s...\n", cbnode->filename, destpath));
			}
			else
			{
				len = strlen(cbnode->filename) + 1;

				cbnode_copy = (struct dragdropnode *) malloc(sizeof(*cbnode_copy)+ len);

				if(cbnode_copy)
				{
					strcpy(cbnode_copy->path, cbnode->filename);
					cbnode_copy->type = MV_Icon_FileType_Directory; /* doesn't matter, if it's a file, movefile will be called later */

					ADDTAIL(*copy, cbnode_copy);
				}
				else
				{
					res = FALSE;
				}
			}
		}
		ReleaseSemaphore(&clipboardsem);
	}
	else
	{
		res = FALSE;
	}

	return res;
}

BOOL clipboard_menu_paste_enable(void)
{
	//return !clipboard_is_empty();
	return clipctx.count == 0 ? FALSE : TRUE;
}

BOOL clipboard_menu_copy_enable(void)
{
	LONG retval = 0;
	ULONG msg[ sizeof( struct MP_Application_DoRexx ) / sizeof( ULONG ) ];

	msg[ 0 ] = MM_Application_DoRexx;
	msg[ 1 ] = TRUE;
	msg[ 2 ] = (ULONG)NULL;
	msg[ 3 ] = (ULONG)"GetSelectedNames";
	msg[ 4 ] = (ULONG)&retval;
	msg[ 5 ] = (ULONG)NULL;
	msg[ 6 ] = 0;
	msg[ 7 ] = (ULONG)NULL,
	msg[ 8 ] = FALSE;
	application_dorexx( OCLASS(app), app, ( struct MP_Application_DoRexx * )msg );

	return (retval == 0);
}
