/*
 * Ambient - the ultimate desktop
 * ------------------------------
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
 * $Id: sendrexx.c,v 1.5 2022/01/06 16:53:16 piru Exp $
 */

/* public */
#include <string.h>
#include <exec/execbase.h>
#include <dos/dostags.h>
#include <rexx/errors.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/rexxsyslib.h>

/* private */
#include "qport.h"
#include "sendrexx.h"

static ULONG _sendrexxarg(CONST_STRPTR cmd, STRPTR *argstring)
{
	BOOL rc = FALSE;

	{
		struct RexxMsg *rexxmsg;
		struct MsgPort rport;

		CreateQPort(&rport);

		if ((rexxmsg = CreateRexxMsg(&rport, NULL, "AMBIENT")))
		{
			rexxmsg->rm_Args[0] = (STRPTR) CreateArgstring(cmd, strlen(cmd) + 1);

			if (rexxmsg->rm_Args[0])
			{
				struct MsgPort *amp;
				struct Task *caller = FindTask(NULL);

				rexxmsg->rm_Action = RXCOMM | (argstring ? RXFF_RESULT : 0);

				Forbid();
				amp = (struct MsgPort *)FindExecNode(EXECLIST_PORT, "AMBIENT");

				// protect against this being called from the ambient thread!
				if (amp && amp->mp_SigTask != caller)
				{
					STRPTR retstr = NULL;

					PutMsg(amp, &rexxmsg->rm_Node);
					Permit();
					WaitPort(&rport);
					(void)GetMsg(&rport);

					rc = rexxmsg->rm_Result1 == RC_OK ? TRUE : FALSE;
					retstr = (STRPTR)rexxmsg->rm_Result2;

					if (argstring)
					{
						*argstring = rc ? retstr : NULL;
					}

					if (retstr)
					{
						if (!rc || !argstring)
							DeleteArgstring(retstr);
					}
				}
				else
				{
					Permit();
				}

				DeleteArgstring(rexxmsg->rm_Args[0]);
			}

			DeleteRexxMsg(rexxmsg);
		}

		DeleteQPort(&rport);
	}

	return (rc);
}

ULONG childrenbusy;

static VOID sendrexxproc(STRPTR cmd)
{
	(void) _sendrexxarg(cmd, NULL);

	FreeVec(cmd);

	/*
	 * There is no Permit() here since else the segment could get
	 * unloaded while we're executing. See libexpunge in lib.c
	 */
	Forbid();
	childrenbusy--;
}

ULONG sendrexxarg(CONST_STRPTR cmd, STRPTR *argstring)
{
	/*
	 * If there is no requirement for response for the command, also support
	 * sending from Ambient itself.
	 */
	if (argstring == NULL)
	{
		struct MsgPort *amp;
		struct Task *caller = FindTask(NULL);
		Forbid();
		amp = (struct MsgPort *)FindExecNode(EXECLIST_PORT, "AMBIENT");
		if (amp && amp->mp_SigTask == caller)
		{
			STRPTR cmdcopy;

			Permit();

			cmdcopy = AllocVec(strlen(cmd) + 1, MEMF_ANY);
			if (cmdcopy)
			{
				strcpy(cmdcopy, cmd);

				Forbid();
				childrenbusy++;
				Permit();
				if (CreateNewProcTags(NP_Entry,    (IPTR) sendrexxproc,
				                      NP_CodeType, CODETYPE_PPC,
				                      NP_PPC_Arg1, (IPTR) cmdcopy,
				                      NP_Name,     (IPTR) "workbench.library ARexx Helper",
				                      NP_Priority, (IPTR) 1,
				                      TAG_DONE))
				{
					/* We can only assume success here. */
					return TRUE;
				}
				Forbid();
				childrenbusy--;
				Permit();

				FreeVec(cmdcopy);
			}
		}
		else
		{
			Permit();
		}
	}
	return _sendrexxarg(cmd, argstring);
}

ULONG sendrexx(CONST_STRPTR cmd)
{
	return sendrexxarg(cmd, NULL);
}
