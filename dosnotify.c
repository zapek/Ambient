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
 * $Id: dosnotify.c,v 1.7 2006/09/18 23:17:28 fab Exp $
 */

#include "ambient.h"

#if USE_DOSNOTIFY

/* public */
#include <dos/notify.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "dosnotify.h"
#include "mui_func.h"


static struct MsgPort *dosnotifyport;

ULONG dosnotifysig;


ULONG dosnotify_init(void)
{
	if ( (dosnotifyport = CreateMsgPort()) )
	{
		dosnotifysig = 1L << dosnotifyport->mp_SigBit;

		return (TRUE);
	}
	return (FALSE);
}


void dosnotify_cleanup(void)
{
	if (dosnotifyport)
	{
		struct NotifyMessage *msg;

		/* clean up any pending messages */
		while ( (msg = (struct NotifyMessage *)GetMsg(dosnotifyport)) )
		{
			ReplyMsg(&msg->nm_ExecMessage);
		}

		DeleteMsgPort(dosnotifyport);
	}
}


APTR dosnotify_start(CONST_STRPTR name, ULONG userdata)
{
	struct NotifyRequest *nr;
	ULONG namelen;

	ASSERT(name);

	namelen = strlen(name);

	if (namelen)
	{
		if ( (nr = malloc(sizeof(*nr) + namelen + 1)) )
		{
			nr->nr_Name                 = (char *) (nr + 1);
			memcpy(nr->nr_Name, name, namelen + 1);
			nr->nr_UserData             = userdata;
			nr->nr_stuff.nr_Msg.nr_Port = dosnotifyport;
			nr->nr_Flags                = NRF_SEND_MESSAGE;
			if (StartNotify(nr))
			{
				return (nr);
			}
			free(nr);
		}
	}
	return (NULL);
}


void dosnotify_stop(APTR ctx)
{
	struct NotifyRequest *nr = (struct NotifyRequest *)ctx;

	if (nr)
	{
		EndNotify(nr);
		free(nr);
	}
}


void dosnotify_handle(void)
{
	struct NotifyMessage *msg;

	while ( (msg = (struct NotifyMessage *)GetMsg(dosnotifyport)) )
	{
		ULONG userdata = msg->nm_NReq->nr_UserData;
		UBYTE *name = msg->nm_NReq->nr_Name;

		/*
		 * PFS3 (and possibly some other filesystems) can get
		 * confused if the NotifyMessage is replied after the
		 * notification has been terminated with EndNotify().
		 *
		 * So, first ReplyMsg(), then process the event. - Piru
		 */
		ReplyMsg(&msg->nm_ExecMessage);
		DoMethod(app, MM_Application_DOSNotify, userdata, name); /* XXX: userdata is wrong as we don't really use the 'id' anymore but the name */
	}
}

#else /* USE_DOSNOTIFY */

#include "dosnotify.h"

ULONG dosnotify_init(void) { return FALSE; }
void dosnotify_cleanup(void) {}
APTR dosnotify_start(CONST_STRPTR name, ULONG userdata) { return NULL; }
void dosnotify_stop(APTR ctx) {}
void dosnotify_handle(void);

#endif /* USE_DOSNOTIFY */
