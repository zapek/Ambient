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
 * $Id: clipboarddevice.c,v 1.1 2006/09/18 23:17:27 fab Exp $
 */

#include "ambient.h"

/* public */

#include <exec/exec.h>
#include <proto/exec.h>
#include <libraries/iffparse.h>
#include <devices/clipboard.h>

#include <string.h>

/* private */

#include "clipboarddevice.h"

struct IOClipReq * clipboard_open(ULONG unit)
{
	struct MsgPort *mp;
	struct IOClipReq *ior;

	if ((mp = CreateMsgPort()))
	{
		if ((ior=(struct IOClipReq *)CreateIORequest(mp,sizeof(struct IOClipReq))))
		{
			if (!(OpenDevice("clipboard.device", unit, (struct IORequest *) ior, 0L)))
			{
				return(ior);
			}
			DeleteIORequest((struct IORequest *) ior);
		}
		DeleteMsgPort(mp);
	}
	return(NULL);
}

void clipboard_close(struct IOClipReq *ior)
{
	if (ior)
	{
		struct MsgPort *mp;

		mp = ior->io_Message.mn_ReplyPort;

		CloseDevice((struct IORequest *)ior);
		DeleteIORequest((struct IORequest *)ior);
		DeleteMsgPort(mp);
	}
}

static LONG writedata(struct IOClipReq *ior, CONST_APTR data, ULONG datalen)
{
	ior->io_Command = CMD_WRITE;
	ior->io_Data    = (APTR)data;
	ior->io_Length  = datalen;
	DoIO( (struct IORequest *) ior);
	if (ior->io_Error)
	{
		return(FALSE);
	}
	if (ior->io_Actual != datalen)
	{
		ior->io_Error = 1;
		return(FALSE);
	}
	return(TRUE);
}

ULONG clipboard_write_ftxt(struct IOClipReq *ior, CONST_STRPTR string)
{
	ULONG length, slen;
	BOOL odd;
	ULONG success;
	struct
	{
		ULONG form;
		ULONG totalsize;
		ULONG ftxt;
		ULONG chrs;
		ULONG strlen;
	} iffheader;

	slen = strlen(string);
	odd = (slen & 1);               /* pad byte flag */

	length = (odd) ? slen+1 : slen;

	/* initial set-up for Offset, Error, and ClipID */
	ior->io_Offset = 0;
	ior->io_Error  = 0;
	ior->io_ClipID = 0;

	/* Create the IFF header information */
	iffheader.form      = ID_FORM;
	iffheader.totalsize = length + sizeof(ULONG) * 3;
	iffheader.ftxt      = MAKE_ID('F','T','X','T');
	iffheader.chrs      = MAKE_ID('C','H','R','S');
	iffheader.strlen    = slen;

	/* Write header */
	if (!writedata(ior, &iffheader, sizeof(iffheader))) goto error;

	/* Write string */
	if (!writedata(ior, string, slen)) goto error;

	/* Pad if needed */
	if (odd)
	{
		if (!writedata(ior, "", 1)) goto error;
	}

	ior->io_Command = CMD_UPDATE;
	DoIO( (struct IORequest *) ior);

	error:
	success = ior->io_Error ? FALSE : TRUE;

	return(success);
}
