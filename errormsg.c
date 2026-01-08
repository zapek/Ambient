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
 * $Id: errormsg.c,v 1.4 2006/04/12 14:01:53 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/intuition.h>

/* private */
#include "errormsg.h"
#include "smartreq.h"
#include "classes.h"

void errormsg(ULONG type)
{
	switch (type)
	{
		case ERR_NOMEM:
			/* XXX: we could set Ambient's titlescreen here.. */
			DB(("memory allocation failure\n"));
			DisplayBeep(NULL);
			break;

		case ERR_READERROR:
			/* XXX: we should somehow prevent the stacking of those.. */
			smartreq_info(SRT_IOERROR, MV_Notification_Error, "Read error", NULL);
			break;

		case ERR_WRITEERROR:
			/* XXX: we should somehow prevent the stacking of those.. */
			smartreq_info(SRT_IOERROR, MV_Notification_Error, "Write error", NULL);
			break;
	}
}
