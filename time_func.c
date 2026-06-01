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
 * $Id: time_func.c,v 1.12 2023/08/09 01:20:14 piru Exp $
 */

#include "ambient.h"

/* public */
#include <dos/datetime.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/dos.h>

/* private */
#include "time_func.h"

#define UnixTimeOffset     (252482400-(6*3600))
#define SECONDS_PER_MINUTE 60
#define SECONDS_PER_DAY    (24 * 60 * SECONDS_PER_MINUTE)

struct Library *TimerBase;
ULONG cpu_clock;

static struct timerequest treq;


/*
 * Timer initialization
 */
static UQUAD start_timed;

ULONG timer_init(void)
{
	treq.tr_node.io_Message.mn_ReplyPort = CreateMsgPort();
	if (OpenDevice(TIMERNAME, UNIT_VBLANK, (struct IORequest *)&treq, 0))
	{
		return (FALSE);
	}
	TimerBase =(struct Library *)treq.tr_node.io_Device;

	/*
	 * Set the start time for timed()
	 */
	cpu_clock = ReadCPUClock(&start_timed);

	return (TRUE);
}


/*
 * Timer cleanup
 */
void timer_cleanup(void)
{
	if (TimerBase)
	{
		CloseDevice((struct IORequest *)&treq);
		//DeleteMsgPort(treq.tr_node.io_Message.mn_ReplyPort); // bitRocky
	}
	if (treq.tr_node.io_Message.mn_ReplyPort)
		DeleteMsgPort(treq.tr_node.io_Message.mn_ReplyPort); // bitRocky: shouldn't it be called here? If OpenDevice() fails!?
}


/*
 * Creates a timer. 'mp' is optional.
 */
struct timerequest * timer_create(ULONG unit, struct MsgPort *mp)
{
	struct IORequest *io;

	if (mp)
	{
		mp->mp_Node.ln_Name = (STRPTR)0xff; /* nasty hack */
	}
	else
	{
		mp = CreateMsgPort();
	}

	if (mp)
	{
		if ( (io = (struct IORequest *)CreateIORequest(mp, sizeof(struct timerequest))) )
		{
			if (!OpenDevice(TIMERNAME, unit, (struct IORequest *)io, 0))
			{
				return ((struct timerequest *)io);
			}
			DeleteIORequest(io);
		}
		if (mp->mp_Node.ln_Name != (char *)0xff) /* still a nasty hack */
		{
			DeleteMsgPort(mp);
		}
	}
	return (NULL);
}


ULONG timer_addreq_sync(struct timerequest *tr, struct TimeVal *tv)
{
	tr->tr_node.io_Command = TR_ADDREQUEST;
	tr->tr_time.tv_secs    = tv->tv_secs;
	tr->tr_time.tv_micro   = tv->tv_micro;

	return (DoIO((struct IORequest *)tr));
}


VOID timer_addreq_async(struct timerequest *tr, struct TimeVal *tv)
{
	tr->tr_node.io_Command = TR_ADDREQUEST;
	tr->tr_time.tv_secs    = tv->tv_secs;
	tr->tr_time.tv_micro   = tv->tv_micro;

	SendIO((struct IORequest *)tr);
}


ULONG timer_addclock_sync(struct timerequest *tr, UQUAD clk)
{
	/* XXX: hm.. add some assertion for the unit ? */
	tr->tr_node.io_Command = TR_ADDREQUEST;
	tr->tr_time.tv_secs    = clk >> 32ULL;
	tr->tr_time.tv_micro   = clk & 0xffffffffULL;

	return (DoIO((struct IORequest *)tr));
}


VOID timer_abort(struct timerequest *tr)
{
	if (!CheckIO(&tr->tr_node))
	{
		AbortIO(&tr->tr_node);
		WaitIO(&tr->tr_node);
	}
}


void timer_delete(struct timerequest *tr)
{
	if (tr)
	{
		struct MsgPort *mp;

		if (tr->tr_node.io_Device)
		{
			CloseDevice((struct IORequest *)tr);
		}
		mp = ((struct IORequest *)tr)->io_Message.mn_ReplyPort;
		DeleteIORequest(tr);

		if (mp && mp->mp_Node.ln_Name != (char *)0xff) /* still a nasty hack */
		{
			DeleteMsgPort(mp);
		}
	}
}


/*
 * Gets the current time
 */
time_t time(time_t *tp)
{
	struct TimeVal tv;

	if (TimerBase->lib_Version >= 52)
		GetUTCSysTime((APTR)&tv);
	else
		GetSysTime((APTR)&tv);
	tv.tv_secs += UnixTimeOffset;

	if (tp)
	{
		*tp = tv.tv_secs;
	}
	return ((time_t)tv.tv_secs);
}


/*
 * Gets the current time
 */
time_t timev(void)
{
	struct TimeVal tv;

	if (TimerBase->lib_Version >= 52)
		GetUTCSysTime((APTR)&tv);
	else
		GetSysTime((APTR)&tv);
	return ((time_t)tv.tv_secs + UnixTimeOffset);
}


/*
 * Works like timev() but doesn't depend on the system time
 */
time_t timed(void)
{
	UQUAD clk;

	ReadCPUClock(&clk);

	return ((time_t)((clk - start_timed) / cpu_clock));
}


/*
 * Same as timed() but returns the milliseconds for
 * increased precision.
 */
ULONG timedm(void)
{
	UQUAD clk;

	ReadCPUClock(&clk);

	return ((time_t)((clk - start_timed) / (cpu_clock / 1000)));
}


/*
 * Just like GetSysTime() but not dependent on the system
 * clock and the timeval starts from when the program was
 * started.
 */
void getlocaltime(struct TimeVal *dest)
{
	UQUAD clk, ret;

	ASSERT(dest);

	ReadCPUClock(&clk);

	ret = clk - start_timed;
	dest->tv_secs  = ret / cpu_clock;
	dest->tv_micro = ret % cpu_clock * 1000000 / cpu_clock;
}


/* XXX: this function name is wrong.. clock ticks are NOT for
 * the start fo the program.. fix that and add a new function
 */
void getlocalclock(UQUAD *dest)
{
	ASSERT(dest);

	ReadCPUClock(dest);
}


/*
 * Converts a DateStamp structure
 * to a string.
 */
ULONG datestamp_to_str(struct DateStamp *ds, STRPTR datestr, STRPTR timestr)
{
	struct DateTime dt;

	ASSERT(ds);

	memcpy(&dt.dat_Stamp, ds, sizeof(*ds));

	dt.dat_Format  = FORMAT_DOS;
	dt.dat_Flags   = DTF_SUBST;
	dt.dat_StrDay  = NULL;
	dt.dat_StrDate = datestr;
	dt.dat_StrTime = timestr;

	return (DateToStr(&dt));
}


ULONG datestamp_to_seconds(struct DateStamp *ds)
{
	return (ds->ds_Days * SECONDS_PER_DAY + ds->ds_Minute * SECONDS_PER_MINUTE + ds->ds_Tick / TICKS_PER_SECOND);
}

void seconds_to_datestamp(ULONG seconds, struct DateStamp *ds)
{
	ds->ds_Days   = seconds / SECONDS_PER_DAY;
	ds->ds_Minute = (seconds - ds->ds_Days * SECONDS_PER_DAY) / SECONDS_PER_MINUTE;
	ds->ds_Tick   = (seconds - ds->ds_Minute * SECONDS_PER_MINUTE - ds->ds_Days * SECONDS_PER_DAY) * TICKS_PER_SECOND;
}
