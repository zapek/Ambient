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
 * $Id
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "makelink.h"
#include "fastcopy.h"
#include "name.h"
#include "mui_func.h"
#include "notify.h"
#include "smartreq.h"


ULONG tr_makelink(CONST_STRPTR object, CONST_STRPTR link)
{
	THREAD;

	restart_makelink:

	if (MakeLink(link, (CPTR)object, TRUE))  /* FALSE for hard-links, non-zero for soft-links */
	{
		notify_action(link, NOTIFYTAG_Monitor_File, NOTIFYTAG_Monitor_File_Create);
	}
	else
	{
		if (smartreq_request_sync(NULL, GSI(MSG_MAKELINK_TITLE), GSI(MSG_MAKELINK_BUTS_RETRYCANCEL), MV_Notification_Error, GSI(MSG_MAKELINK_COULDNTCREATELINKDOSERROR), link) == 1)
		{
			goto restart_makelink;
		}
	}
	return (TRUE); /* XXX: in fact we never report failure.. but we should */
}
