/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: methodstack.c,v 1.14 2022/01/01 00:01:15 piru Exp $
 */

#include "ambient.h"

/* public */
#include <exec/memory.h>
#include <proto/exec.h>
#include <dos/dosextens.h>

/* private */
#include "methodstack.h"
#include "mui_func.h"
#include "taskdata.h"

static struct SignalSemaphore mssem;
static struct Task *mstask;
struct MinList methodlist;
static APTR methodpool;

#define PMB_SYNC   1 /* sync method */

#define PMF_SYNC   (1UL << PMB_SYNC)

struct pushedmethod {
	struct MinNode n;
	ULONG size;
	APTR obj;
	struct Message msg;
	ULONG flags; /* flag + pad */
	ULONG result;
	ULONG m[0];
};


ULONG methodstack_init(void)
{
	methodpool = CreatePool(MEMF_ANY, 2048, 1024);
	if (methodpool)
	{
		InitSemaphore(&mssem);
		mstask = FindTask(NULL);
		NEWLIST(&methodlist);
		return TRUE;
	}
	return FALSE;
}


void methodstack_cleanup(void)
{
	if (methodpool)
	{
		/*
		 * I would argue this ObtainSemaphore() is useless. If something else
		 * is still there calling ObtainSemaphore() on this, we're screwed
		 * anyway. - piru
		 */

		/* Semaphore is kept till shutdown */
		ObtainSemaphore(&mssem);
		DeletePool(methodpool);
		mstask = NULL;
	}
}


/*
 * Pushes a method asynchronously just like MUI, except
 * we have more control and there's no stupid static limit.
 */
#if USE_ASYNC_PUSHMETHOD
void methodstack_push(APTR obj, ULONG cnt, ...)
{
	struct pushedmethod *pm;
	va_list va;
#warning using DEBUG does not really work with a non debug MUI	
	#ifdef MUI_DEBUG

	CHECKOBJECT(obj);
#endif
	ASSERT(cnt);
	ASSERT(mstask);

	va_start(va, cnt);

	ObtainSemaphore(&mssem);

	if ( (pm = AllocPooled(methodpool, sizeof(*pm) + cnt * sizeof(ULONG))) )
	{
		ULONG i = 0;
		pm->obj = obj;
		DoMethod(obj, OM_RETAIN);
		pm->size = cnt;
		pm->flags = 0;

		while (cnt--)
		{
			pm->m[i] = va_arg(va, ULONG);
			i++;
		}
		ADDTAIL(&methodlist, pm);
	}

	ReleaseSemaphore(&mssem);

	va_end(va);
}
#endif /* USE_ASYNC_PUSHMETHOD */


/*
 * Pushes a method synchronously. Completes all the
 * waiting methods in the stack before ourself. Can
 * be used to synchronise previous pushmethods and
 * get greater efficiency by minimizing context switches.
 */
ULONG methodstack_push_sync(APTR obj, ULONG cnt, ...)
{
	struct pushedmethod *pm;
	struct Process *thisproc = (APTR)FindTask(NULL);
	struct MsgPort *replyport = NULL;
	ULONG res;
	va_list va;

	ASSERT(mstask);
	ASSERT(cnt);
#warning using DEBUG does not really work with a non debug MUI	
	#ifdef MUI_DEBUG
	if (obj)
	{
		if (!_ISOBJ(obj))
		{
			PDB(("*** not an object: %p, %s\n", obj, (ODH(obj)->magic == 0x77777777) ? "disposed object" : "unknown"));
			if (thisproc == (APTR)mstask)
			{
				PDB(("method sent from main task\n"));
			}
			else
			{
				PDB(("method sent from thread %s\n", thisproc->pr_Task.tc_Node.ln_Name));
			}
			va_start(va, cnt);
			PDB(("the method is: 0x%lx (cnt: %lu)\n", va_arg(va, ULONG), cnt));
			va_end(va);
		}
	}
	else
	{
		PDB(("*** not an object: NULL object\n"));
	}
	#endif

	va_start(va, cnt);

	if ( (pm = AllocTaskPooled(sizeof(*pm) + cnt * sizeof(ULONG))) )
	{
		ULONG i = 0;
		pm->size = cnt;
		pm->flags = 0;

		while (cnt--)
		{
			pm->m[i] = va_arg(va, ULONG);
			i++;
		}

		if (thisproc == (APTR)mstask)
		{
			//PDB(("*** called from main thread\n"));
			/*
			 * Empty the methodlist, that way
			 * we sort of "sync" everything.
			 */
			methodstack_check(TRUE);
			res = DoMethodA(obj, (Msg)&pm->m[0]);

			FreeTaskPooled(pm, pm->size * sizeof(LONG) + sizeof(*pm));

			va_end(va);

			return (res);
		}

		replyport = ((struct taskdata *)thisproc->pr_Task.tc_UserData)->msport;

		pm->obj = DoMethod(obj, OM_RETAIN);

		pm->flags |= PMF_SYNC;

		pm->msg.mn_ReplyPort = replyport;

		ObtainSemaphore(&mssem);
		ADDTAIL(&methodlist, pm);
		ReleaseSemaphore(&mssem);
	}

	va_end(va);

	if (!pm)
	{
		return 0;
	}

	Signal(mstask, SIGBREAKF_CTRL_E);
	WaitPort(replyport);
	GetMsg(replyport);

	res = pm->result;

	FreeTaskPooled(pm, pm->size * sizeof(ULONG) + sizeof(*pm));

	return (res);
}


#ifdef ENABLE_METHODSTACK_PUSHSYNCSAFE
/* USE ONLY WHEN CALLED IN ALIEN THREAD CONTEXT
 * (Currently unused)
 *
 * Pushes a method synchronously. Completes all the
 * waiting methods in the stack before ourself. Can
 * be used to synchronise previous pushmethods and
 * get greater efficiency by minimizing context switches.
*/
ULONG methodstack_push_sync_safe(APTR obj, ULONG cnt, ...)
{
	struct pushedmethod *pm;
	struct Process *thisproc = (APTR)FindTask(NULL);
	struct MsgPort *replyport = &(thisproc->pr_MsgPort);
	ULONG res, size;
	va_list va;

	ASSERT(mstask);
	ASSERT(cnt);
	#warning using DEBUG does not really work with a non debug MUI	
	#ifdef MUI_DEBUG
	if (obj)
	{
		if (!_ISOBJ(obj))
		{
			PDB(("*** not an object: %p, %s\n", obj, (ODH(obj)->magic == 0x77777777) ? "disposed object" : "unknown"));
			if (thisproc == (APTR)mstask)
			{
				PDB(("method sent from main task\n"));
			}
			else
			{
				PDB(("method sent from thread\n"));
			}
			va_start(va, cnt);
			PDB(("the method is: 0x%lx (cnt: %lu)\n", va_arg(va, ULONG), cnt));
		}
	}
	else
	{
		PDB(("*** not an object: NULL object\n"));
	}
	#endif

	va_start(va, cnt);

	size = sizeof(*pm) + cnt * sizeof(ULONG);

	if ((pm = AllocTaskPooled(size)))
	{
		ULONG i = 0;
		pm->size = cnt;
		pm->flags = 0;

		while (cnt--)
		{
			pm->m[i] = va_arg(va, ULONG);
			i++;
		}

		if (thisproc == (APTR)mstask)
		{
			PDB(("You moron!\n"));

			/*
			 * Empty the methodlist, that way
			 * we sort of "sync" everything.
			 */
			methodstack_check(TRUE);
			res = DoMethodA(obj, (Msg)&pm->m[0]);

			FreeTaskPooled(pm, size);

			va_end(va);

			return (res);
		}

		pm->obj = DoMethod(obj, OM_RETAIN);

		pm->flags |= PMF_SYNC;

		pm->msg.mn_ReplyPort = replyport;

		ObtainSemaphore(&mssem);
		ADDTAIL(&methodlist, pm);
		ReleaseSemaphore(&mssem);
	}
	va_end(va);

	Signal(mstask, SIGBREAKF_CTRL_E);
	WaitPort(replyport);
	GetMsg(replyport);

	res = pm->result;

	FreeTaskPooled(pm, size);

	return (res);
}
#endif


void methodstack_kill_methods(APTR obj)
{
	struct pushedmethod *pm, *next;

	CHECKOBJECT(obj);

	ObtainSemaphore(&mssem);

	for ( (pm = FIRSTNODE(&methodlist)); (next = NEXTNODE(pm)) ; (pm = next) )
	{
		if (pm->obj == obj)
		{
			DoMethod(pm->obj, OM_RELEASE);
			pm->obj = NULL;
		}
	}

	ReleaseSemaphore(&mssem);
}


void methodstack_check(ULONG nobreak)
{
	struct pushedmethod *pm = NULL;
	ULONG isempty = FALSE;

	for (;;)
	{
		ObtainSemaphore(&mssem);
		pm = REMHEAD(&methodlist);
		isempty = pm == NULL;
		ReleaseSemaphore(&mssem);
		if (!pm)
			break;

		if (pm->flags & PMF_SYNC) /* syncmethod ? */
		{
			if (pm->obj)
			{
				pm->result = DoMethodA(pm->obj, (Msg)&pm->m[0]);
			}
			else
			{
				pm->result = -1;
			}
			DoMethod(pm->obj, OM_RELEASE);
			ReplyMsg(&pm->msg);
		}
		else
		{
			/* normal (newobj is not async) */
			#ifdef DEBUG
			if (pm->flags)
			{
				SDB(("eek, error\n"));
			}
			#endif
			DoMethodA(pm->obj, (Msg)&pm->m[0]);
			DoMethod(pm->obj, OM_RELEASE);
			ObtainSemaphore(&mssem);
			FreePooled(methodpool, pm, pm->size * sizeof(ULONG) + sizeof(*pm));
			ReleaseSemaphore(&mssem);
		}

		if (!nobreak)
		{
			break;
		}
	}

	if (!isempty)
	{
		Signal(mstask, SIGBREAKF_CTRL_E);
	}
}
