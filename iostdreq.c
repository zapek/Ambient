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
 * $Id: iostdreq.c,v 1.1 2006/04/12 14:01:54 fab Exp $
 */

#include "ambient.h"

/* public */

#include <exec/io.h>
#include <exec/ports.h>
#include <exec/lists.h>
#include <proto/exec.h>

/* private */
#include "iostdreq.h"

/*
** Open Device
*/

struct IOStdReq *iostd_open_device(CONST_STRPTR devname, ULONG unit, ULONG flags)
{
	struct {
		struct IOStdReq ior;
		struct MsgPort  mp;
	} *clump;

	clump = malloc(sizeof(*clump));
	if (clump)
	{
		if ( (BYTE) (clump->mp.mp_SigBit = AllocSignal(-1)) != -1 )
		{
			clump->mp.mp_Node.ln_Type = NT_MSGPORT;
			clump->mp.mp_Flags        = PA_SIGNAL;
			clump->mp.mp_SigTask      = FindTask(NULL);
			NEWLIST(&clump->mp.mp_MsgList);

			clump->ior.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
			clump->ior.io_Message.mn_ReplyPort    = &clump->mp;
			clump->ior.io_Message.mn_Length       = sizeof(clump->ior);

			if (!OpenDevice(devname, unit, (struct IORequest *) &clump->ior, flags))
			{
				return (&clump->ior);
			}
			FreeSignal(clump->mp.mp_SigBit);
		}
		free(clump);
	}
	return (NULL);
}

/*
** Close Device
*/

void iostd_close_device(struct IOStdReq *ior)
{
	if (ior)
	{
		CloseDevice((struct IORequest *) ior);

		FreeSignal(ior->io_Message.mn_ReplyPort->mp_SigBit);

		free(ior);
	}
}
