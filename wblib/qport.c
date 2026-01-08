/*
 * Ambient - the ultimate desktop
 * ------------------------------
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
 * $Id: qport.c,v 1.1 2006/04/12 17:51:47 fab Exp $
 */

#include "qport.h"

/* public */
#include <exec/ports.h>
#include <proto/exec.h>

/* private */

void CreateQPort(struct MsgPort *port)
{
	port->mp_Node.ln_Type = NT_MSGPORT;
	port->mp_Flags        = PA_SIGNAL;
	if ((BYTE) (port->mp_SigBit = AllocSignal(-1)) == -1)
	{
		port->mp_SigBit = SIGB_SINGLE;
		SetSignal(0, SIGF_SINGLE);
	}
	port->mp_SigTask      = FindTask(NULL);
	NEWLIST(&port->mp_MsgList);
}

void DeleteQPort(struct MsgPort *port)
{
	if (port->mp_SigBit == SIGB_SINGLE)
	{
		SetSignal(0, SIGF_SINGLE);
	}
	else
	{
		FreeSignal(port->mp_SigBit);
	}
}
