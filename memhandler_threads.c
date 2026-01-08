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
 * $Id: memhandler_threads.c,v 1.4 2006/08/08 13:31:35 fab Exp $
 */

#include "ambient.h"

/* public */
#include <exec/interrupts.h>
#include <exec/memory.h>
#include <exec/semaphores.h>
#include <emul/emulinterface.h>
#include <proto/exec.h>

/* private */
#include "memhandler_threads.h"
#include "methodstack.h"
#include "mui_func.h"


static LONG memhandlerfunc(void);

static struct {
	struct Interrupt Interrupt;
	struct ExecBase *SysBase;
} mosintdata;

static const struct EmulLibEntry InterruptFunc =
{
	TRAP_LIB,
	0,
	(void (*)(void))memhandlerfunc
};

static struct SignalSemaphore memhandlersem;
static ULONG memhandler_threads_active;


ULONG memhandler_threads_init(void)
{
	InitSemaphore(&memhandlersem);

	mosintdata.SysBase = SysBase;
	mosintdata.Interrupt.is_Node.ln_Type = NT_INTERRUPT;
	mosintdata.Interrupt.is_Node.ln_Pri  = 0;
	mosintdata.Interrupt.is_Node.ln_Name = "Ambient's Threads MemHandler";
	mosintdata.Interrupt.is_Data         = &mosintdata;
	mosintdata.Interrupt.is_Code         = (void(*)(void))&InterruptFunc;

	AddMemHandler(&mosintdata.Interrupt);
	memhandler_threads_active = TRUE;
	return (TRUE);
}


void memhandler_threads_cleanup(void)
{
	if (memhandler_threads_active)
	{
		RemMemHandler(&mosintdata.Interrupt);
	}
}


static LONG memhandlerfunc(void)
{
	struct ExecBase *SysBase = mosintdata.SysBase;

	if (AttemptSemaphore(&memhandlersem))
	{
		/*
		 * Unfortunately we can't do anything immediately.
		 * We hope that some other app will free memory for that request
		 * to succeed nevertheless we'll do our best to do out of
		 * a low memory situation.
		 */
		methodstack_push(app, 1, MM_Application_ReclaimThreads);
		ReleaseSemaphore(&memhandlersem);
	}
	return (MEM_DID_NOTHING);
}
