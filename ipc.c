/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2015 Ambient Open Source Team
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
 * $Id: ipc.c,v 1.11 2025/08/09 16:51:41 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <exec/execbase.h>
#include <proto/exec.h>

/* private */
#include "ipc.h"
#include "appicon.h"
#include "path.h"
#include "wbstart.h"
#include "mui_func.h"
#include "args.h"
#include "threads.h"

/*
 * IPC is mostly used for workbench.library <-> Ambient communications.
 */

struct IpcData ipcdata;
static struct MsgPort *ipcport;

ULONG ipcsig;

ULONG ipc_init(void)
{
	if ( (ipcport = AddExecNode(NULL,
	                            SAL_Type, NT_MSGPORT,
	                            SAL_Name, (ULONG) "Ambient IPC",
	                            TAG_DONE)) )
	{
		ipcsig = 1L << ipcport->mp_SigBit;

		/* install global semaphore. used to locate shared appicons list */

		ipcdata.sema.ss_Link.ln_Name = "Ambient IPC";
		ipcdata.sema.ss_Link.ln_Pri  = -127;
		ipcdata.task = FindTask(NULL);
		ipcdata.sigbit = ipcport->mp_SigBit;

		NEWLIST(&ipcdata.appicon_list);
		AddSemaphore(&ipcdata.sema);

		return (TRUE);
	}
	return (FALSE);
}

void ipc_cleanup(void)
{
	if (ipcport)
	{
		struct ipcmessage *msg;

		RemSemaphore(&ipcdata.sema);
		ObtainSemaphore(&ipcdata.sema);

		{
			struct IpcAppiconNode *n, *nn;

			ObtainSemaphore(&ipcdata.sema);

			ITERATELISTSAFE(n, nn, &ipcdata.appicon_list)
			{
				REMOVE(n);
				DeletePool(n->entry->pool);
			}
		}

		ReleaseSemaphore(&ipcdata.sema);

		RemPort(ipcport);

		while ( (msg = (struct ipcmessage *)GetMsg(ipcport)) )
		{
			switch (msg->type)
			{
				case IPC_WBSTARTLIB:
					{
						struct ipc_wbstartlib *wbsm = msg->msgtype;

						/* indicate failure */
						wbsm->status = FALSE;
					}
					/* fall thru to ReplyMsg() */

				default:
					ReplyMsg(&msg->msg);
					break;
			}
		}

		DeleteMsgPort(ipcport);
	}
}


void ipc_handle(void)
{
	struct ipcmessage *msg;
	ULONG msgreply = TRUE;

	/* we synchronize appicons state on each ipc message. shouldn't introduce much of an overhead */

	appicon_synchronize();

	while ( (msg = (struct ipcmessage *)GetMsg(ipcport)) )
	{
		/* XXX: retval of all of that should be checked and sent in the reply! */
		switch (msg->type)
		{
			case IPC_NEWPATH:
				path_update(msg->msgtype);
				break;

			case IPC_WBSTARTLIB:
				msgreply = FALSE;
				launch_get_icon_properties(msg);
				break;

			case IPC_WINCOUNT:
				((struct ipc_wincount *)msg->msgtype)->wincount = DoMethod(app, MM_Application_CountWindows);
				((struct ipc_wincount *)msg->msgtype)->wbr = args.wbstartup;
				break;

			case IPC_CLONEPATH:
				path_clone(msg->msgtype);
				break;
		}

		if (msgreply)
		{
			ReplyMsg(&msg->msg);
		}
	}
}

