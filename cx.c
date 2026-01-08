/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2007 Ambient Open Source Team
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
 * $Id: cx.c,v 1.8 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <proto/commodities.h>

/* private */
#include "cx.h"
#include "mui_func.h"


struct MsgPort *cxport;
ULONG cxsig;
ULONG exsig;
CxObj *exbroker;

ULONG cx_init(void)
{
	if ((cxport = CreateMsgPort()))
	{
		cxsig = 1L << cxport->mp_SigBit;
		return (TRUE);
	}
	return (FALSE);
}


void cx_cleanup(void)
{
	if (cxport)
	{
		CxMsg *cxmsg;

		while ((cxmsg = (CxMsg *)GetMsg(cxport)))
			;

		DeleteMsgPort(cxport);
	}
}


void cx_handle(void)
{
	CxMsg *cxmsg;

	while ((cxmsg = (CxMsg *)GetMsg(cxport)))
	{
		free(cxmsg);
	}
}


/*
 * Exchange fake port handling
 * to get notifies.
 */
struct NewBroker newbroker = {
	NB_VERSION,
	"Exchange",
	"Commodities Exchange",
	"Controls System Commodities",
	NBU_UNIQUE | NBU_NOTIFY,
	0,
	0,
	NULL,
	0
};


ULONG exchange_create(void)
{
	if ((newbroker.nb_Port = CreateMsgPort()))
	{
		LONG err;

		if ((exbroker = CxBroker(&newbroker, &err)))
		{
			exsig = 1 << newbroker.nb_Port->mp_SigBit;
			return (CE_OK);
		}

		DeleteMsgPort(newbroker.nb_Port);

		if (err == CBERR_DUP)
		{
			return (CE_DUP);
		}
	}
	return (CE_FAILED);
}


void exchange_delete(void)
{
	CxMsg *msg;

	while ((msg = (CxMsg *)GetMsg(newbroker.nb_Port)))
	{
		ReplyMsg((struct Message *)msg);
	}

	DeleteCxObjAll(exbroker);

	DeleteMsgPort(newbroker.nb_Port);

	exsig = 0;
}


void exchange_handle(void)
{
	CxMsg *msg;

	while ((msg = (CxMsg *)GetMsg(newbroker.nb_Port)))
	{
		ULONG type, id;

		type = CxMsgType(msg);
		id = CxMsgID(msg);
		ReplyMsg((struct Message *)msg);

		switch (type)
		{
			case CXM_COMMAND:
			{
				switch (id)
				{
					case CXCMD_UNIQUE:
						set(cxwin, MUIA_Window_Open, TRUE);
						break;

					case CXCMD_LIST_CHG:
						DoMethod(cxwin, MM_Cxwin_Rescan);
						break;
				}
			}
			break;
		}
	}
}
