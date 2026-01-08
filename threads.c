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
 * $Id: threads.c,v 1.24 2022/02/01 10:27:28 geit Exp $
 */

#include "ambient.h"

/* public */
#include <exec/execbase.h>
#include <proto/alib.h>
#include <proto/exec.h>
#include <dos/dostags.h>
#include <dos/dosextens.h>
#include <proto/dos.h>
#include <utility/tagitem.h>
#include <proto/utility.h>

/* private */
#include "threads.h"
#include "methodstack.h"
#include "mui_func.h"
#include "threadtags.h"
#include "taskdata.h"
#include "dosreq.h"
#include "atomic.h"
#include "smartreq.h"
#include "debug.h"
#include "qport.h"
#include "thread_dispatcher.h"

#if !USE_THREADPOOL
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

#define ID_MSG      0
#define ID_STARTMSG 1

struct thread_startmsg {
	struct Message msg;
	ULONG id;
	struct thread_node *tn;
};

/* signaling node */
struct signal_node {
	struct MinNode n;
	APTR obj;
	ULONG abort[THREAD_MAXABORT + 1];
};

/* Threads are added and removed only from MAINTASK, so
 *  1. ObtainSemaphore() when removing and adding thread nodes
 *  2. ObtainSemaphoreShared() when iterating the list in threads
 */
static struct SignalSemaphore threadsem;
static struct MinList threadlist;
static struct MinList signalthreadlist;   /* list objects to notify (MM_Thread_Finished) are put into */

static struct MsgPort *threadport;
static ULONG thread_num;

static void ambient_thread(void);
static ULONG create_thread(ULONG active);

ULONG threadsig;
ULONG threadcnt;

struct Task *maintask;

/*
 * Formats thread name according to action ID.
 */

#define PROCESS_THREAD_NAME "Ambient Thread "
#define PROCESS_THREAD_ACTION_LEN 10
#define PROCESS_THREAD_ACTION_DEF "     "

static STRPTR thread_build_name( STRPTR buf, ULONG bufsize, LONG num, LONG action )
{
	TEXT name[sizeof(PROCESS_THREAD_NAME) + PROCESS_THREAD_ACTION_LEN + 16 ];

	if ( action > 0 )
		snprintf( name, sizeof( name ), "%s%lu [%lu]  ", PROCESS_THREAD_NAME, num, action );
	else
		snprintf( name, sizeof( name ), "%s%lu     ", PROCESS_THREAD_NAME, num );


	if ( !buf )
	{
		bufsize = strlen( name ) + 1;
		buf = malloc( bufsize );
	}

	if ( buf )
	{
		LONG len = min( bufsize - 1, strlen( name ) );
		memcpy( buf, name, len );
		buf[ len ] = 0;
	}

	return buf;
}

/*
 * Initializes the whole thread system.
 */
ULONG threads_init(void)
{
	InitSemaphore(&threadsem);

	NEWLIST(&threadlist);
	NEWLIST(&signalthreadlist);

	D(PROC, bug("creating threads support..\n"));

	maintask = FindTask(NULL);

	if ( (threadport = CreateMsgPort()) )
	{
		ULONG i;
		ULONG rc = 0;

		threadsig = 1L << threadport->mp_SigBit;

		D(PROC, bug("launching..\n"));

		for (i = 0; i < THREAD_MIN; i++)
		{
			if (!(rc = create_thread(FALSE)))
				break;
		}
		return (rc);
	}
	return (FALSE);
}


/*
 * Cleanups the whole thread system.
 */
ULONG threads_finish(ULONG loop)
{
	struct thread_node *tn;

	DB(("Aborting threads..:%d.\n", loop))

	if (threadport)
	{
		/*
		 * Tell every process to die
		 */
		D(PROC, bug("telling processes to die..\n"));
		ITERATELIST(tn, &threadlist)
		{
			D(PROC, bug("signaling %p (%d)..\n", tn->proc, tn->action ));
			Signal((struct Task *)tn->proc, SIGBREAKF_CTRL_C);
		}

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
					threads_handle();
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

		DeleteMsgPort(threadport);
		threadport = NULL;
		D(PROC, bug("threadport removed\n"));
	}
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
	struct thread_node *tn;
	ULONG cnt = threadcnt;
	MAINTASK;

	ITERATELIST(tn, &threadlist)
	{
		if (cnt < THREAD_MIN) break;

		if (tn->state == TS_IDLE)
		{
			Signal((struct Task *)tn->proc, SIGBREAKF_CTRL_C);
			cnt--;
		}
	}
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
	struct thread_node *tn;
	va_list va;
	ULONG specific;

	MAINTASK;

	#ifdef DEBUG
	{
		ULONG count = 0;
		ULONG t;

		va_start(va, obj);
		while ( (t = va_arg(va, ULONG)) )
		{
			count++;
			if (count >= THREAD_MAXABORT)
			{
				PDB(("maximum argument of %lu exceeded! undefined behaviour..\n", THREAD_MAXABORT));
				break;
			}
		}
		va_end(va);
	}
	#endif

	va_start(va, obj);
	specific = va_arg(va, ULONG);
	va_end(va);

	/*
	 * Check if there's something to signal at
	 * all.
	 */
	ITERATELIST(tn, &threadlist)
	{
		if (tn->destobj == obj && tn->state == TS_BUSY)
		{
			if (specific)
			{
				ULONG t;

				va_start(va, obj);
				while ( (t = va_arg(va, ULONG)) )
				{
					if (t == tn->action)
					{
						D(PROC,bug("signaling process %p (action %d) to stop..\n", tn->proc, tn->action));
						Signal((struct Task *)tn->proc, SIGBREAKF_CTRL_D);
						found = TRUE;
						break;
					}
				}
				va_end(va);
			}
			else
			{
				D(PROC, bug("signaling process %p (action %d) to stop..\n", tn->proc, tn->action));
				Signal((struct Task *)tn->proc, SIGBREAKF_CTRL_D);
				found = TRUE;
			}
		}
	}

	if (found)
	{
		struct signal_node *sn;
		ULONG skip[THREAD_MAXABORT];

		memset(skip, 0, sizeof(skip));

		found = FALSE;

		/*
		 * Check if the object is already
		 * to be signaled.
		 */
		ITERATELIST(sn, &signalthreadlist)
		{
			if (obj == sn->obj)
			{
				found = TRUE;

				if (specific)
				{
					ULONG t;
					ULONG i;
					ULONG j = 0;

					found = FALSE;

					va_start(va, obj);
					while ( (t = va_arg(va, ULONG)) )
					{
						for (i = 0; i < THREAD_MAXABORT; i++)
						{
							if (sn->abort[i])
							{
								if (sn->abort[i] == t)
								{
									skip[j] = TRUE; /* already scheduled to be signaled, so skip it */
								}
							}
							else
							{
								break;
							}
						}
						j++;
					}
					va_end(va);
				}
				break;
			}
		}

		if (found)
		{
			/*
			 * Merge the events.
			 */
			ULONG i = 0;
			ULONG j = 0;
			ULONG t;

			D(PROC,bug("merging the events for obj %p..\n", obj));

			while (sn->abort[i]) i++;

			va_start(va, obj);
			while ( (t = va_arg(va, ULONG)) )
			{
				if (!skip[j])
				{
					sn->abort[i] = t;
					i++;
				}
				j++;
			}
			va_end(va);

			/* XXX: we should check if the merging exceeded the limit.. add a warning! */
			sn->abort[i] = 0; /* terminate the array */
		}
		else
		{
			/*
			 * Create a new one.
			 */
			if ( (sn = malloc(sizeof(*sn))) )
			{
				ULONG i = 0;
				ULONG j = 0;
				ULONG t;

				D(PROC, bug("adding %p to signaling list..\n", obj));
				sn->obj = obj;

				va_start(va, obj);
				while ( (t = va_arg(va, ULONG)) )
				{
					if (!skip[j])
					{
						sn->abort[i] = t;
						i++;
					}
					j++;
				}
				va_end(va);

				sn->abort[i] = 0; /* terminate the array */
				ADDTAIL(&signalthreadlist, sn);
			}
			/* XXX: argh */
		}
	}
	else
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
	struct thread_node *tn;
	struct Process *me = ( struct Process* )SysBase->ThisTask;

	THREAD;

	ObtainSemaphoreShared(&threadsem);
	ITERATELIST(tn, &threadlist)
	{
		if ( tn->proc == me )
		{
			tn->state = TS_WAITING;

			ReleaseSemaphore(&threadsem);
			Wait( SIGBREAKF_CTRL_D );
			return;
		}
	}
	ReleaseSemaphore(&threadsem);
}

APTR thread_get( void )
{
	THREAD;

	if ( IS_MAINTASK )
		return NULL;
	else
		return SysBase->ThisTask;
}

/*
 * Try to awake sleeping thread. If the thread is not
 * in 'sleeping' state then the function does nothing.
 */

void thread_signal(APTR thread, BOOL force)
{
	struct thread_node *tn;

	MAINTASK;

	/*
	 * Check if there's something to signal at
	 * all.
	 */

	ITERATELIST(tn, &threadlist)
	{
		if (tn->proc == thread && (force || tn->state == TS_WAITING))
		{
			D(PROC,bug("signaling process %p..\n", tn->proc));
			Signal((struct Task *)tn->proc, SIGBREAKF_CTRL_D);
			tn->state = TS_BUSY;
			break;
		}
	}
}

/*
 * Do an action. Spawns a thread if necessary.
 */
ULONG v_do_action(APTR obj, int action, struct TagItem *tags)
{
	struct thread_node *tn;
	ULONG found = FALSE;

	MAINTASK;

	/*
	 * Find a free thread.
	 */
	ITERATELIST(tn, &threadlist)
	{
		if (tn->state == TS_IDLE)
		{
			D(PROC, bug("found %p for job\n", tn->proc));
			found = TRUE;
			tn->state = TS_BUSY;
			break;
		}
	}

	if (!found)
	{
		tn = (struct thread_node *)create_thread(TRUE);
	}

	if (tn)
	{
		struct thread_msg *msg;

		if ( (msg = malloc(sizeof(*msg))) )
		{
			struct TagItem *taglist;

			if ( (taglist = threadtags_clone(tags)) )
			{
				LONG pri;

				memset(msg, 0, sizeof(*msg));

				D(PROC, bug("sending message to %p (direct mode, action: %ld)\n", tn->proc, (LONG)action));

				msg->msg.mn_ReplyPort = threadport;
				msg->msg.mn_Length    = sizeof(*msg); /* not really needed */
				msg->obj              = (Object *)DoMethod(obj, OM_RETAIN);
				msg->tn               = tn;
				msg->taglist          = taglist;

				pri = GetTagData(TT_Priority, 0, msg->taglist);

				if (pri != THREAD_PRIORITY)
				{
					SetTaskPri((struct Task *)tn->proc, pri);
					tn->pri = pri;
				}

				#ifdef THREAD_ACTION_INFO_HACK
				/* XXX: HACK: Modify thread name to give info about action it performs */

				{
					STRPTR name = ((struct Task *)tn->proc)->tc_Node.ln_Name;
					ULONG namelen = strlen( name );

					/* +1 because there is trailing 0 there too */

					thread_build_name( name, namelen + 1, tn->index, action );

				}
				#endif

				tn->destobj = (APTR)DoMethod((Object *)GetTagData(TT_Object, (ULONG)obj, msg->taglist), OM_RETAIN);
				tn->action  = action;
				PutMsg(tn->mp, &msg->msg);
				return (TRUE); /* XXX ? */
			}
			free(msg);
			return (FALSE);
		}
		/* XXX: hum.. */
	}
	else
	{
		D(PROC, bug("failed to run thread\n"));
		/* XXX: couldn't run thread.. gasp */
	}
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

	msg.msg.mn_ReplyPort = NULL;   /* not really needed */
	msg.id               = 0;      /* not really needed */
	msg.tn               = NULL;
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
 * Internal function. Runs a thread.
 */
static ULONG create_thread(ULONG active)
{
	struct Process *proc;

	/*
	 * Process name
	 */
	thread_num++;

	if (thread_num <= THREAD_MAX)
	{
		struct thread_node *tn;

		if ( (tn = malloc(sizeof(*tn))) )
		{
			struct thread_startmsg *startmsg;

			if ( (startmsg = malloc(sizeof(*startmsg))) )
			{
				TEXT name[ 64 ];

				memset(startmsg, 0, sizeof(*startmsg));

				/*
				 * Prepare start message.
				 */
				startmsg->msg.mn_ReplyPort = threadport;
				startmsg->id = ID_STARTMSG;
				startmsg->tn = tn;

				D(PROC, bug("sending message, creating thread number %ld..\n", thread_num));

				tn->index = thread_num; /* not 'atomic' but it's only for information purposes anyway */
				thread_build_name( name, sizeof( name ), thread_num, 0 );

				if ( (proc = CreateNewProcTags(
					NP_CodeType, CODETYPE_PPC,
					NP_Entry, ambient_thread,
					NP_Cli, FALSE,
					NP_CopyVars, FALSE,
					NP_PPCStackSize, THREAD_STACK,
					NP_Name, (ULONG)name,
					NP_Priority, THREAD_PRIORITY,
					NP_StartupMsg, startmsg,
					NP_TaskMsgPort, &tn->mp,
					TAG_DONE
				)) )
				{
					ASSERT(tn->mp);

					atomic_add(&threadcnt, 1);

					tn->proc = proc;
					if (active)
					{
						tn->state = TS_BUSY;
					}
					else
					{
						tn->state = TS_IDLE;
					}
					/*
					 * Add process node into list.
					 */
					ObtainSemaphore(&threadsem);
					ADDTAIL(&threadlist, tn);
					ReleaseSemaphore(&threadsem);

					return ((ULONG)tn);
				}
				else
				{
					PDB(("couldn't create proc..\n")); /* XXX */
				}
				free(startmsg);
			}
			else
			{
				PDB(("not enough memory for startmsg\n")); /* XXX */
			}
			free(tn);
		}
	}
	return (FALSE);
}


/*
 * Handles the thread system after receiving a replymsg.
 */
void threads_handle(void)
{
	struct thread_msg *msg;
	struct thread_node *tn;
	struct signal_node *sn, *snn;
	ULONG found;
	MAINTASK;

	while ( (msg = (struct thread_msg *)GetMsg(threadport)) )
	{
		if (msg->id == ID_MSG) /* not a startup message ? */
		{
			if (msg->tn->destobj)
			{
				D(PROC, bug("sending MM_Thread_Finished (action %d)\n", msg->tn->action));
				methodstack_push_sync(msg->tn->destobj, 4, MM_Thread_Finished, msg->tn->action, msg->status, msg->taglist);

				/*
				 * Remove object from the signaling list then.
				 */
				ITERATELIST(sn, &signalthreadlist)
				{
					if (sn->obj == msg->tn->destobj)
					{
						REMOVE(sn);
						free(sn);
						break;
					}
				}
			}

			D(PROC, bug("setting process %p to idle mode (action %d)..\n", msg->tn, msg->tn->action));
			if (msg->tn->destobj)
				DoMethod(msg->tn->destobj, OM_RELEASE);
			msg->tn->destobj = NULL;
			msg->tn->state = TS_IDLE;
			DoMethod(msg->obj, OM_RELEASE);
			if (msg->tn->pri != THREAD_PRIORITY)
			{
				SetTaskPri((struct Task *)msg->tn->proc, THREAD_PRIORITY);
			}

			#ifdef THREAD_ACTION_INFO_HACK
			/* XXX: HACK: Modify thread name to give info about action it performs */

			{
				STRPTR name = ((struct Task *)msg->tn->proc)->tc_Node.ln_Name;
				LONG namelen = strlen( name );

				/* +1 because there is trailing 0 there too */

				thread_build_name( name, namelen + 1, msg->tn->index, 0 );

			}
			#endif
		}

		/*
		 * Signal objects who did a threads_abort().
		 */
		for ( (sn = FIRSTNODE(&signalthreadlist)); (snn = NEXTNODE(sn)) ; (sn = snn) )
		{
			D(PROC, bug("scanning the list for obj %p..\n", sn->obj));
			found = FALSE;

			/*
			 * Check if we sent the abort from above.
			 */
			ITERATELIST(tn, &threadlist)
			{
				D(PROC, bug("against %p\n", tn->destobj));
				if (sn->obj == tn->destobj && tn->state == TS_BUSY)
				{
					found = TRUE;
					D(PROC, bug("found %p (tn=%p)!!\n", sn, tn));
					break;
				}
			}

			if (!found)
			{
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
				D(PROC,bug("removing object node..\n"));
				REMOVE(sn);
				free(sn);
			}
		}

		if (msg->id == ID_STARTMSG)
		{
			atomic_sub(&threadcnt, 1);
			D(PROC, bug("removing startmsg..\n"));
			ObtainSemaphore(&threadsem);
			REMOVE(msg->tn);
			ReleaseSemaphore(&threadsem);
			free(msg->tn);
		}
		else
		{
			D(PROC,bug("Releasing thread args...\n"));
			threadtags_free(msg->taglist);
			D(PROC,bug("Done\n"));
		}
		free(msg);
	}
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
static void ambient_thread(void)
{
	ULONG sigs;
	struct thread_msg *msg;
	/*struct thread_startmsg *startmsg;*/
	struct Process *me = (struct Process *)FindTask(NULL);
	ULONG exitnow = FALSE;
	struct MsgPort *port;
	struct taskdata td;
	struct MsgPort lmsport;

	/*NewGetTaskAttrs(NULL, &startmsg, sizeof(startmsg), TASKINFOTYPE_STARTUPMSG, TAG_DONE);*/
	NewGetTaskAttrs(NULL, &port, sizeof(port), TASKINFOTYPE_TASKMSGPORT, TAG_DONE);

	/*ASSERT(startmsg);*/
	ASSERT(port);

	memset(&td, 0, sizeof(td));

	/*
	 * Create MethodStack msgport.
	 */
	CreateQPort(&lmsport);
	td.msport = &lmsport;

	me->pr_Task.tc_UserData = &td;

	sigs = (1L << port->mp_SigBit) | SIGBREAKF_CTRL_C; /* SIGBREAKF_CTRL_D is only used for task aborting */

	dosreq_disable();

	while (1)
	{
		D(PROC, bug("%p waiting..\n", me));
		SetSignal(0, SIGBREAKF_CTRL_D | /* Clear it.. in case one arrived too late */
		             SIGBREAKF_CTRL_C); /* Clear it too */

		if (Wait(sigs) & SIGBREAKF_CTRL_C)
		{
			D(PROC, bug("%p got ^C !\n", me));
			exitnow = TRUE;
		}

		while ( (msg = (struct thread_msg *)GetMsg(port)) )
		{
			D(PROC, bug("%p working..\n", me));
			if (exitnow)
			{
				msg->status = MV_Thread_Finished_Abort;
			}
			else
			{
				msg->status = thread_domsg(msg);
				#ifdef DEBUG
				if (msg->status == (ULONG)ABORTED)
				{
					D(PROC, bug("%p marking ABORTED\n", me));
				}
				#endif
			}
			D(PROC, bug("%p did its job number %ld\n", me, msg->tn->action));
			ReplyMsg(&msg->msg);
		}

		D(PROC, bug("%p done with loop\n", me));

		if (exitnow)
			break;
	}

	DeleteQPort(&lmsport);

	D(PROC, bug("%p exiting..\n", me));
}
#endif
