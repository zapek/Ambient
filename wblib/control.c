/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: control.c,v 1.8 2017/07/25 20:23:12 piru Exp $
 */

#include "globals.h"

/* public */
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>
#include <string.h>

/* private */
#include "clib/wb_protos.h"
#include "../ipc.h"
#include "qport.h"
#include "sendrexx.h"


#define DB_WBCONTROL 0
#define DB_PATH      0


struct pathentry {
	BPTR pe_next;
	BPTR pe_lock;
};


/****** workbench.library/ManageDesktopObjectA() **********************
*
* NAME
*   ManageDesktopObjectA -- Manage a desktop object
*
* SYNOPSIS
*   Success = ManageDesktopObjectA(target, action, taglist)
*
*   BOOL ManageDesktopObjectA(CONST_STRPTR, LONG, struct TagItem *);
*
* FUNCTION
*   Manage desktop objects.
*
* INPUTS
*   target   - target file or directory
*   action   - an action to take on object
*   taglist  - parameters
*
* ACTIONS
*   DESKACTION_AddShortcut    - add target as a desktop shortcut
*   DESKACTION_RemoveShortcut - remove desktop shortcut
*
* RESULT
*   Success  - TRUE if request was successfully passed to Ambient.
*
*****************************************************************************
*
*/
BOOL LIB_ManageDesktopObjectA(CONST_STRPTR name, LONG action, const struct TagItem *tags)
{
	BOOL rc = FALSE;

	switch (action)
	{
		case DESKACTION_AddShortcut:
		case DESKACTION_RemoveShortcut:
			if (name && *name)
			{
				int len = strlen(name) + sizeof("shortcut remove") + 3;
				STRPTR cmd = AllocMem(len, MEMF_ANY);

				if (cmd)
				{
					NewRawDoFmt("Shortcut \"%s\" %s", NULL, cmd, name, action == DESKACTION_AddShortcut ? "ADD" : "REMOVE");
					rc = sendrexx(cmd);
					FreeMem(cmd, len);
				}
			}
			break;
	}

	return rc;
}


BOOL WorkbenchControlA(CONST_STRPTR name, struct TagItem *tags)
{
	BOOL rc = FALSE;

	D(WBCONTROL,bug("called\n"));

	FORTAG(tags)
	{
		case WBCTRLA_DuplicateSearchPath:
		{
			struct Task *me = FindTask(NULL); /* we must not wait on ourself (eg, MUI config window is an appwindow as well) */
			struct ipcmessage gmsg;
			struct ipc_clonepath msg;
			struct MsgPort rport;
			struct MsgPort *amp;

			CreateQPort(&rport);

			msg.path = (BPTR *)tag->ti_Data;

			gmsg.pool             = NULL;
			gmsg.msg.mn_ReplyPort = &rport;
			gmsg.msg.mn_Length    = sizeof(gmsg);
			gmsg.type             = IPC_CLONEPATH;
			gmsg.msgtype          = &msg;

			Forbid();
			if ( (amp = FindPort("Ambient IPC")) )
			{
				if (me != amp->mp_SigTask)
				{
					PutMsg(amp, &gmsg.msg);
				}
			}
			Permit();

			if (amp && (me != amp->mp_SigTask))
			{
				WaitPort(&rport);
				(void)GetMsg(&rport);

				rc = msg.path ? TRUE : FALSE;
			}

			DeleteQPort(&rport);
			break;
		}

		case WBCTRLA_FreeSearchPath:
		{
			struct pathentry *pe = BADDR(tag->ti_Data), *peo;

			D(WBCONTROL,bug("freeing path..\n"));

			rc = TRUE;

			while (pe)
			{
				peo = pe;
				pe = BADDR(pe->pe_next);
				D(PATH,bug("freeing lock 0x%lx\n", (ULONG)peo->pe_lock));
				UnLock(peo->pe_lock);
				FreeVec(peo);
			}

			break;
		}
	}
	NEXTTAG

	return (rc);
}
