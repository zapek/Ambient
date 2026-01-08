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
 * $Id: eject.c,v 1.3 2023/01/11 15:14:30 jacadcaps Exp $
 */

#include "ambient.h"

/* public */

#include <exec/memory.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <devices/scsidisk.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* private */
#include "eject.h"
#include "iostdreq.h"

/* local prototypes */

static BOOL send_command( struct IOStdReq *iorequest, const UBYTE *cmd);

/* our commands to send */

static const UBYTE SCSICMD_EJECT[]  = {6,SCSI_CMD_SSU,0x00,0x00,0x00,SSUF_EJECT,0x00};
static const UBYTE SCSICMD_LOAD[]   = {6,SCSI_CMD_SSU,0x00,0x00,0x00,SSUF_EJECT|SSUF_ON,0x00};

/* handy macros (tm) */

#define TICKS_PER_MINUTE (60 * TICKS_PER_SECOND)
#define TICKS_PER_DAY    (24 * 60 * TICKS_PER_MINUTE)
#define DSTOTICK(ds)     ((QUAD) (ds)->ds_Days * TICKS_PER_DAY + \
                          (ds)->ds_Minute * TICKS_PER_MINUTE + \
                          (ds)->ds_Tick)


/*
** Send Command
*/

static BOOL send_command( struct IOStdReq *iorequest, const UBYTE *cmd)
{
	struct SCSICmd scsicmd;
	UBYTE  sensedata[SENSE_SIZEOF];

	scsicmd.scsi_CmdLength    = (UWORD) *cmd++;
	scsicmd.scsi_Command      = (UBYTE *) cmd;
	scsicmd.scsi_Data         = 0;
	scsicmd.scsi_Length       = 0;
	scsicmd.scsi_SenseActual  = 0;

	scsicmd.scsi_Flags        = SCSIF_READ | SCSIF_AUTOSENSE;
	scsicmd.scsi_Status       = 0;

	scsicmd.scsi_SenseData    = sensedata;
	scsicmd.scsi_SenseLength  = (UWORD) SENSE_SIZEOF;

	iorequest->io_Data    = &scsicmd;
	iorequest->io_Length  = sizeof (scsicmd);
	iorequest->io_Command = HD_SCSICMD;

	DoIO( (struct IORequest *) iorequest);

	if ( iorequest->io_Error || scsicmd.scsi_SenseActual)
	{
		return (FALSE);
	}
	return (TRUE);
}

/*
** Eject
*/

void eject( STRPTR devname, ULONG unit, ULONG flags, ULONG mode )
{
	struct IOStdReq *ior;
	struct DateStamp start, stop;

	if ( (ior = iostd_open_device( devname, unit, flags )) )
	{
		switch ( mode )
		{
			case EJECT_LOAD:
				send_command( ior, SCSICMD_LOAD);
				break;

			case EJECT_EJECT:
				send_command( ior, SCSICMD_EJECT);
				break;

			default:
			case EJECT_TOGGLE:
				DateStamp(&start);

				send_command( ior, SCSICMD_EJECT);

				DateStamp(&stop);

				send_command( ior, SCSICMD_EJECT);

				if ( (EJECT_JIFFIES >= (DSTOTICK(&stop) - DSTOTICK(&start))) )
				{
					send_command( ior, SCSICMD_LOAD);
				}
				break;
		}

		iostd_close_device( ior );
	}
}

void unmount( STRPTR devname )
{
	if (!devname || !*devname)
		return;
	
	struct DosList *devices, *entry;
	char nameBuffer[64];
	
	stccpy(nameBuffer, devname, sizeof(nameBuffer));
	if (nameBuffer[strlen(nameBuffer) - 1] == ':')
		nameBuffer[strlen(nameBuffer) - 1] = 0; // trim the :

	devices = LockDosList(LDF_DEVICES | LDF_READ);

	entry = FindDosEntry(devices, nameBuffer, LDF_DEVICES | LDF_READ);

	// unsafe but will deadlock otherwise
	UnLockDosList(LDF_DEVICES | LDF_READ);

	if (entry && entry->dol_Task)
	{
		struct InfoData id;
		if (DoPkt1(entry->dol_Task, ACTION_DISK_INFO, MKBADDR(&id)) == DOSFALSE || id.id_InUse)
			return;
		
		if (DoPkt1(entry->dol_Task, ACTION_INHIBIT, DOSTRUE ) == DOSFALSE)
			return;

		if (DoPkt0(entry->dol_Task, ACTION_DIE) == DOSFALSE)
			DoPkt1(entry->dol_Task, ACTION_INHIBIT, DOSFALSE);
	}
}
