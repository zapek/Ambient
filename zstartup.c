/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: zstartup.c,v 1.9 2017/07/25 19:52:47 piru Exp $
 */

#include "ambient.h"

/* public */
#include <workbench/startup.h>
#include <dos/dosextens.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <proto/exec.h>
#include <proto/intuition.h>


struct Task     *MainTask;
struct ExecBase *SysBase;

const ULONG __abox__ __attribute__((section(".rodata"))) = 1;
__attribute__ ((weak)) ULONG __stack = 4096;


extern LONG callmain(STRPTR arg, ULONG arglen);

LONG __start(STRPTR arg, ULONG arglen, APTR elfobj);
LONG __start(STRPTR arg, ULONG arglen, APTR elfobj UNUSED)
{
	struct Process *pr;
	int rc = RETURN_FAIL;

	SysBase = *(void **)4L;
	pr = (struct Process *)(MainTask = FindTask(NULL));

	if (pr->pr_CLI)
	{
		ULONG stacksize;

		if (__stack &&
			NewGetTaskAttrsA(&pr->pr_Task, &stacksize, sizeof(stacksize), TASKINFOTYPE_STACKSIZE, NULL) &&
			stacksize < __stack)
		{
			struct StackSwapStruct stack;
			struct PPCStackSwapArgs args;

			if ( (stack.stk_Lower = AllocTaskPooled(__stack)) )
			{
				stack.stk_Upper = (ULONG)stack.stk_Lower + __stack;
				args.Args[0] = (ULONG)arg;     /* this is NOT argc/argv but just a string */
				args.Args[1] = arglen;

				rc = NewPPCStackSwap(&stack, callmain, &args);
			}
		}
		else
		{
			rc = callmain(arg, arglen);
		}
	}
	else
	{
		struct WBStartup *wbmsg;

		WaitPort(&pr->pr_MsgPort);
		wbmsg = (struct WBStartup *)GetMsg(&pr->pr_MsgPort);

		if( (IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 36)) )
		{
			static const struct EasyStruct req = {
				sizeof(struct EasyStruct),
				0,
				"Ambient startup",
				"Sorry but this program doesn't start from Workbench.\nI create my own world myself :)",
				"Oh well"
			};

			EasyRequest(NULL, &req, NULL, NULL);

			CloseLibrary((struct Library *)IntuitionBase);
		}

		/* Prevent unloading while we still execute our code. */
		Forbid();

		ReplyMsg(&wbmsg->sm_Message);
	}
	return (rc);
}
