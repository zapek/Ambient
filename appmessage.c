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
 * $Id: appmessage.c,v 1.10 2017/07/29 00:26:06 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <workbench/workbench.h>
#include <workbench/startup.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/wb.h>
#include <proto/dos.h>

/* private */
#include "appmessage.h"
#include "ipc.h"


/*
 * Sends a AppMessage. Called from a thread.
 */
ULONG tr_appmessage_send(ULONG type, struct MsgPort *tmp, struct Window *win, CONST_STRPTR path, ULONG cl, ULONG id, ULONG userdata, ULONG numargs, struct WBArg *arglist, ULONG mousex, ULONG mousey)
{
	THREAD;

	if (type)
	{
		struct MsgPort mp;

		if ((BYTE) (mp.mp_SigBit = AllocSignal(-1)) != -1)
		{
			struct WBArg *wba;
			BPTR l = (BPTR)NULL;
			struct AppMessage appmsg;

			mp.mp_Node.ln_Type = NT_MSGPORT;
			mp.mp_Flags        = PA_SIGNAL;
			mp.mp_SigTask      = FindTask(NULL);
			NEWLIST(&mp.mp_MsgList);

			memset(&appmsg, 0, sizeof(appmsg));

			/* common */
			appmsg.am_Message.mn_ReplyPort = &mp;
			appmsg.am_Message.mn_Length = sizeof(appmsg); /* not really needed, but lets set it up anyway */
			appmsg.am_Type = type;
			appmsg.am_Class = cl;
			appmsg.am_NumArgs = numargs;
			appmsg.am_ArgList = arglist;
			appmsg.am_Version = AM_VERSION;
			appmsg.am_MouseX = mousex;
			appmsg.am_MouseY = mousey;

			CurrentTime(&appmsg.am_Seconds, &appmsg.am_Micros);

			if (path)
			{
				ULONG sec, mic, i;

				ASSERT(arglist)
				ASSERT(numargs)
				/*
				 * Try to lock the parent dir of the
				 * icons. If !path[0] it means that
				 * it's a multidrag of device icons
				 * which have no common basepath and
				 * needs to be handled differently.
				 */
				if (path[0])
				{
					l = Lock(path, ACCESS_READ);
				}

				CurrentTime(&sec, &mic);

				/*
				 * If it took too long to Lock(),
				 * drop the request
				 */
				if (appmsg.am_Seconds - sec > 2)
				{
					PDB(("device was too slow to respond, dropping appmessage..\n"));
					goto cleanup;
				}

				/*
				 * Copy the lock to every wa_Lock
				 * member, except for the multidevice
				 * case.
				 */
				i = numargs;
				wba = arglist;

				if (path[0])
				{
					while (i--)
					{
						wba->wa_Lock = l;
						wba++;
					}
				}
				else if (numargs > 1)
				{
					while (i--)
					{
						wba->wa_Lock = Lock(wba->wa_Name, ACCESS_READ);
						wba++;
					}
				}
			}

			switch (type)
			{
				case AMTYPE_APPICON:
					{
						ASSERT(tmp);
						appmsg.am_UserData = userdata;
						appmsg.am_ID = id;
					}
					break;

				case AMTYPE_APPWINDOW:
					{
						struct ipc_appwindow *msg;
						ASSERT(win);

						if ((msg = (APTR)AppWindowObtain(win)))
						{
							appmsg.am_UserData = msg->userdata;
							appmsg.am_ID = msg->id;

							/*
							 * Gnah. It's supposed to be Window coordinates.
							 */
							appmsg.am_MouseX -= win->LeftEdge;
							appmsg.am_MouseY -= win->TopEdge;

							tmp = msg->userport;

							AppWindowRelease();
						}
						else
						{
							AppWindowRelease();
							goto cleanup;
						}
					}
					break;

				default:
					PDB(("wrong type\n"));
					break;
			}

			PutMsg(tmp, &appmsg.am_Message);
			WaitPort(&mp);
			(void) GetMsg(&mp);

			cleanup:

			FreeSignal(mp.mp_SigBit);

			if (path && path[0])
			{
				if (l)
				{
					UnLock(l);
				}
			}
			else if (numargs > 1)
			{
				ULONG j = numargs;

				wba = arglist;

				while (j--)
				{
					if (wba[j].wa_Lock)
					{
						UnLock(wba[j].wa_Lock);
					}
				}
			}
			return (TRUE);
		}
	}
	else
	{
		PDB(("gnah. type is NULL\n"));
	}
	return (FALSE);
}
