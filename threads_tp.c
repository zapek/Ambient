/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2013 Ambient Open Source Team
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
 * $Id: threads_tp.c,v 1.3 2015/08/11 14:48:29 itix Exp $
 */

#include "ambient.h"

/* public */
#include <exec/execbase.h>
#include <clib/alib_protos.h>
#include <proto/exec.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <libraries/threadpool.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <proto/threadpool.h>
#include <proto/utility.h>

/* private */
#include "threads.h"
#include "methodstack.h"
#include "mui_func.h"
#include "threadtags.h"
#include "taskdata.h"
#include "dosreq.h"
#include "smartreq.h"
#include "debug.h"
#include "thread_dispatcher.h"

#if USE_THREADPOOL
#define THREAD_MIN 2                   /* minimum number of threads in the pool */
#define THREAD_MAX 9999                /* maximum, unlikely to be reached any day :) */
#define THREAD_STACK STACKSIZE_THREAD  /* stack per thread */
#define THREAD_WAITCLEANUP 5           /* time in seconds to wait for the thread cleanup on exit */
#define THREAD_MAXABORT 8UL            /* maximum number of aborting tags */

#define THREAD_PRIORITY 0

/*
 * Define if you want to have info on the action ID the thread performa.
 */

#define THREAD_ACTION_INFO_HACK

static struct SignalSemaphore semaphore;
static struct MinList threadlist;
static struct MinList finishlist;

static BYTE threadsigbit = -1;
static APTR threadpool;

static void ambient_thread(struct thread_msg *msg, struct MsgPort *port);

ULONG threadsig;

struct Task *maintask;

/*
 * Formats thread name according to action ID.
 */

#define PROCESS_THREAD_NAME "Ambient Thread "
#define PROCESS_THREAD_ACTION_LEN 10
#define PROCESS_THREAD_ACTION_DEF "     "

static void thread_build_name( STRPTR buf, ULONG bufsize, LONG action )
{
	switch (action)
	{
		case TA_Devices_UpdateInfo:
			snprintf( buf, bufsize, "%s [%lu] (%s)", PROCESS_THREAD_NAME, action, "UpdateInfo" );
			break;

		default:
			snprintf( buf, bufsize, "%s [%lu]", PROCESS_THREAD_NAME, action );
			break;
	}
}

/*
 * Initializes the whole thread system.
 */
ULONG threads_init(void)
{
	InitSemaphore(&semaphore);

	NEWLIST(&threadlist);
	NEWLIST(&finishlist);

	D(PROC, bug("creating threads support..\n"));

	maintask = FindTask(NULL);

	if ((threadsigbit = AllocSignal(-1)) >= 0)
	{
		threadsig = 1L << threadsigbit;

		threadpool = CreateThreadPoolTagList(THREAD_MAX, NULL);

		if (threadpool)
			return (TRUE);
	}

	return (FALSE);
}


/*
 * Cleanups the whole thread system.
 */
ULONG threads_finish(ULONG loop)
{
	DB(("Aborting threads..:%d.\n", loop))

	if (threadpool)
	{
		struct thread_msg *msg;

		/*
		 * Tell every process to die
		 */
		D(PROC, bug("telling processes to die..\n"));

		ObtainSemaphoreShared(&semaphore);
		ITERATELIST(msg, &threadlist)
		{
			D(PROC, bug("signaling %p (%d)..\n", msg->proc, msg->action ));
			SignalWorkItem(threadpool, msg->work_id, SIGBREAKF_CTRL_D);
		}
		ReleaseSemaphore(&semaphore);

		D(PROC, bug("all processes told to die, waiting %lu seconds for them to finish\n", (ULONG)THREAD_WAITCLEANUP));

		if (!ISLISTEMPTY(&threadlist))
		{
			if (loop)
			{
				ULONG trynum = THREAD_WAITCLEANUP * 5 + 1;

				while (!ISLISTEMPTY(&threadlist))
				{
					D(PROC, bug("processes still alive.. returning to try again\n"));
					Delay(10);

					if (trynum)
					{
						trynum--;

						if (!trynum)
						{
							/*
							 * This case never happens actually.
							 * threads_finish() is called from mainloop and
							 * relies on mui ticks.
							 */
							smartreq_info("Ambient Threads", MV_Notification_Warning, "%lu remaining thread(s) are preventing the cleanup sequence.\nWaiting more..", trynum);
						}
					}
				}
			}
			else
			{
				return (FALSE);
			}
		}

		D(PROC,bug("all processed dead\n"));
		threads_handle();

		D(PROC,bug("remove threadpool\n"));
		DeleteThreadPool(threadpool);
		threadpool = NULL;
		D(PROC, bug("threadpool removed\n"));
	}

	FreeSignal(threadsigbit);

	threadsigbit = -1;

	return (TRUE);
}


void threads_cleanup(void)
{
	threads_finish(TRUE);
}


/*
 * Remove the threads in IDLE state.
 */
void threads_reclaim(void)
{
}


/*
 * Signal all the threads dealing with 'obj' (MM_Thread_Finished) to
 * piss off. MM_Thread_Finished
 * with a msg->id of 0 is sent when they are aborted or
 * if there's nothing. msg->status tells if there were aborts.
 * obj - is the TT_Object by default, can be changed using TT_Object in special cases
 */
void threads_abort(APTR obj, ...)
{
	ULONG found = FALSE;
	struct thread_msg *msg;
	va_list va;
	ULONG specific;

	MAINTASK;

	va_start(va, obj);
	specific = va_arg(va, ULONG);
	va_end(va);

	ObtainSemaphoreShared(&semaphore);
	/*
	 * Check if there's something to signal at
	 * all.
	 */
	ITERATELIST(msg, &threadlist)
	{
		if (msg->destobj == obj && msg->state == TS_BUSY)
		{
			if (specific)
			{
				ULONG t;

				va_start(va, obj);
				while ( (t = va_arg(va, ULONG)) )
				{
					if (t == msg->action)
					{
						D(PROC,bug("signaling process %p (action %d) to stop..\n", msg->proc, msg->action));

						SignalWorkItem(threadpool, msg->work_id, SIGBREAKF_CTRL_D);
						found = TRUE;
						break;
					}
				}
				va_end(va);
			}
			else
			{
				D(PROC, bug("signaling process %p (action %d) to stop..\n", msg->proc, msg->action));

				SignalWorkItem(threadpool, msg->work_id, SIGBREAKF_CTRL_D);
				found = TRUE;
			}
		}
	}
	ReleaseSemaphore(&semaphore);

	if (!found)
	{
		D(PROC, bug("no thread found, sending finished method to obj %p directly..\n", obj));
		/*
		 * Following is always sent when there's nothing.
		 */
		if (specific)
		{
			ULONG t;

			va_start(va, obj);
			while ( (t = va_arg(va, ULONG)) )
			{
				methodstack_push_sync(obj, 4, MM_Thread_Finished, t, MV_Thread_Finished_Abort, NULL);
			}
			va_end(va);
		}
		else
		{
			methodstack_push_sync(obj, 4, MM_Thread_Finished, NULL, MV_Thread_Finished_Abort, NULL);
		}
	}
}

/*
 * Puts current (running) thread to sleep.
 */

void thread_wait( void )
{
	struct thread_msg *msg;
	struct Process *me = (APTR)SysBase->ThisTask;

	THREAD;

	ObtainSemaphoreShared(&semaphore);

	ITERATELIST(msg, &threadlist)
	{
		if ( msg->proc == me )
		{
			msg->state = TS_WAITING;
			ReleaseSemaphore(&semaphore);
			Wait( SIGBREAKF_CTRL_D );
			msg->state = TS_BUSY;
			return;
		}
	}

	ReleaseSemaphore(&semaphore);
}

ssize_t thread_get( void )
{
	THREAD;

	if ( IS_MAINTASK )
		return WORKITEM_INVALID;
	else
		return GetCurrentWorkItem(threadpool);

}

/*
 * Try to awake sleeping thread. If the thread is not
 * in 'sleeping' state then the function does nothing.
 */

void thread_signal(APTR thread, BOOL force)
{
	struct thread_msg *msg;

	MAINTASK;

	ObtainSemaphoreShared(&semaphore);

	/*
	 * Check if there's something to signal at
	 * all.
	 */

	ITERATELIST(msg, &threadlist)
	{
		if (msg->work_id == (ssize_t)thread && (force || msg->state == TS_WAITING))
		{
			D(PROC,bug("signaling process %p..\n", msg->proc));
			SignalWorkItem(threadpool, (ssize_t)thread, SIGBREAKF_CTRL_D);
			break;
		}
	}

	ReleaseSemaphore(&semaphore);
}

/*
 * Do an action. Spawns a thread if necessary.
 */
ULONG v_do_action(APTR obj, int action, struct TagItem *tags)
{
	struct thread_msg *msg;

	MAINTASK;

	if ( (msg = malloc(sizeof(*msg))) )
	{
		struct TagItem *taglist;

		if ( (taglist = threadtags_clone(tags)) )
		{
			va_end(va);

			D(PROC, bug("sending message to thread (direct mode, action: %ld)\n", (LONG)action));

			msg->proc = NULL;
			msg->state = TS_BUSY;
			msg->obj              = obj;
			msg->taglist          = taglist;

			msg->pri = GetTagData(TT_Priority, 0, msg->taglist);
			msg->destobj = (APTR)GetTagData(TT_Object, (ULONG)obj, msg->taglist);
			msg->action  = action;

			ObtainSemaphore(&semaphore);
			ADDTAIL(&threadlist, msg);
			ReleaseSemaphore(&semaphore);

			msg->work_id = QueueWorkItem(threadpool, (THREADFUNC)ambient_thread, msg);

			return (TRUE); /* XXX ? */
		}
		free(msg);
	}

	/* XXX: hum.. */
	return (FALSE);
}

/*
 * Do an action. It's NOT spawning new thread, but have to be executed from
 * already existing one. It will return action code.
 */
ULONG v_do_action_sync(APTR obj, int action, struct TagItem *taglist)
{
	struct thread_msg msg;
	APTR destobj;
	ULONG status;

	THREAD;

	DB(("Executing synchronous action:%d\n", action ));

	destobj = (APTR)GetTagData(TT_Object, (ULONG)obj, taglist);

	msg.action           = action; /* thread_domsg() will use this one when msg->tn == NULL */
	msg.status           = 0;      /* not really needed */
	msg.obj              = obj;
	msg.taglist          = taglist;

	status               = thread_domsg(&msg);

	DB(("Action returned (%d)!\n", status ));

	D(PROC, bug("sending MM_Thread_Finished\n"));
	if ( destobj )
		methodstack_push_sync(destobj, 4, MM_Thread_Finished, action, status, taglist);

	return status;
}


/*
 * Handles the thread system after receiving a replymsg.
 */
void threads_handle(void)
{
	struct thread_msg *msg, *tmp;

	ObtainSemaphore(&semaphore);

	ITERATELISTSAFE(msg, tmp, &finishlist)
	{
		if (msg->destobj)
		{
			D(PROC, bug("sending MM_Thread_Finished (action %d)\n", msg->action));
			methodstack_push_sync(msg->destobj, 4, MM_Thread_Finished, msg->action, msg->status, msg->taglist);
		}

#if 0
				D(PROC, bug("not found, sending method to %p..\n", sn->obj));
				if (sn->abort[0])
				{
					ULONG i;

					for (i = 0; i < THREAD_MAXABORT; i++)
					{
						if (sn->abort[i])
						{
							methodstack_push_sync(sn->obj, 4, MM_Thread_Finished, sn->abort[i], MV_Thread_Finished_Abort, msg->taglist);
						}
					}
				}
				else
				{
					methodstack_push_sync(sn->obj, 4, MM_Thread_Finished, NULL, MV_Thread_Finished_Abort, msg->taglist);
				}
#endif

		D(PROC,bug("Releasing thread args...\n"));
		threadtags_free(msg->taglist);
		D(PROC,bug("Done\n"));
		free(msg);
	}

	NEWLIST(&finishlist);

	ReleaseSemaphore(&semaphore);
}


ULONG threads_check_abort(void)
{
	return (SetSignal(0, SIGBREAKF_CTRL_D) & (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D));
}


ULONG threads_waitsig(ULONG usersigs, ULONG *aborted)
{
	ULONG sigmask = Wait(SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_C | usersigs);

	*aborted = sigmask & (SIGBREAKF_CTRL_D | SIGBREAKF_CTRL_C);

	return sigmask;
}


/*
 * Actual threads.
 */
static void ambient_thread(struct thread_msg *msg, struct MsgPort *port)
{
	struct Process *me = (APTR)SysBase->ThisTask;
	ULONG status = ABORTED;

	msg->proc = me;

	if (!threads_check_abort())
	{
		struct taskdata td;
		TEXT buf[80];

		if (me->pr_Task.tc_Node.ln_Pri != msg->pri)
			SetTaskPri(&me->pr_Task, msg->pri);

		dosreq_disable();
		thread_build_name(buf, sizeof(buf), msg->action);

		td.msport = port;
		td.reqport = NULL;

		me->pr_Task.tc_Node.ln_Name = buf;
		me->pr_Task.tc_UserData = &td;

		D(PROC, bug("%p working..\n", me));
		status = thread_domsg(msg);
	}

	msg->status = status;

	#ifdef DEBUG
	if (status == (ULONG)ABORTED)
	{
		D(PROC, bug("%p marking ABORTED\n", me));
	}
	#endif

	D(PROC, bug("%p did its job number %ld\n", me, msg->action));

	ObtainSemaphore(&semaphore);
	REMOVE(msg);
	ADDTAIL(&finishlist, msg);
	ReleaseSemaphore(&semaphore);

	/* Release first, signal after. */
	Signal(maintask, threadsig);
}
#endif
