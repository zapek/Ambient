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
 * $Id: format.c,v 1.8 2017/08/09 23:26:04 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <exec/memory.h>
#include <exec/io.h>
#include <exec/ports.h>
#include <exec/errors.h>
#include <devices/trackdisk.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>

/* private */
#include "format.h"
#include "iostdreq.h"
#include "device_func.h"
#include "mui_func.h"
#include "methodstack.h"
#include "dostype.h"
#include "threads.h"


/*
 * SFS stuff
 */
#define SFS_PACKET_BASE    (0xf00000)
#define ACTION_SFS_FORMAT  (SFS_PACKET_BASE + 100 + 3)

#define ASFBASE            (TAG_USER)
#define ASF_NAME           (ASFBASE + 1)
#define ASF_NORECYCLED     (ASFBASE + 2)
#define ASF_CASESENSITIVE  (ASFBASE + 3)
#define ASF_SHOWRECYCLED   (ASFBASE + 4)


enum {
	FRV_OK = 1,
	FRV_NOMEM,
	FRV_IOERR,
	FRV_ABORTED,
	FRV_VERIFY_ERROR
};

enum {
	MODE_WRITE_0x00,
	MODE_WRITE_0xFF,
	MODE_WRITE_0xAA,
	MODE_WRITE_0x55,
	MODE_WRITE_STREAM
};


static ULONG write_tracks(APTR obj, ULONG mode, ULONG numtracks, ULONG writecmd, UQUAD off, UBYTE *trackbuf, ULONG tracksize, struct IOStdReq *ior)
{
	ULONG j;
	ULONG retval = FRV_OK;

	switch (mode)
	{
		case MODE_WRITE_0x00:
			memset(trackbuf, 0, tracksize);
			break;

		case MODE_WRITE_0xFF:
			memset(trackbuf, 0xff, tracksize);
			break;

		case MODE_WRITE_0xAA:
			memset(trackbuf, 0xaa, tracksize);
			break;

		case MODE_WRITE_0x55:
			memset(trackbuf, 0x55, tracksize);
			break;

		case MODE_WRITE_STREAM:
			for (j = 0; j < tracksize; j++)
			{
				trackbuf[j] = j % 256;
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("hum, no mode like that\n"));
			break;
		#endif
	}

	for (j = 1; j <= numtracks; j++)
	{
		ior->io_Command    = writecmd;
		ior->io_HighOffset = off >> 32;
		ior->io_LowOffset  = off & 0xffffffff;
		ior->io_Data       = trackbuf;
		ior->io_Length     = tracksize;

		if (DoIO((struct IORequest *)ior))
		{
			retval = FRV_IOERR;
			break;
		}

		off += tracksize;
		
		if (threads_check_abort())
		{
			retval = FRV_ABORTED;
			break;
		}
		methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, j);
	}
	return (retval);
}


static ULONG verify_tracks(APTR obj, ULONG mode, ULONG numtracks, ULONG verifycmd, UQUAD off, UBYTE *trackbuf, ULONG tracksize, struct IOStdReq *ior)
{
	ULONG i, j;
	ULONG retval = FRV_OK;
	UBYTE val = 0;

	switch (mode)
	{
		case MODE_WRITE_0x00:
			break;

		case MODE_WRITE_0xFF:
			val = 0xff;
			break;

		case MODE_WRITE_0xAA:
			val = 0xaa;
			break;

		case MODE_WRITE_0x55:
			val = 0x55;
			break;

		case MODE_WRITE_STREAM:
			break;

		#ifdef DEBUG
		default:
			PDB(("hum, no mode like that\n"));
			break;
		#endif
	}

	for (j = 1; j <= numtracks; j++)
	{
		ior->io_Command    = verifycmd;
		ior->io_HighOffset = off >> 32;
		ior->io_LowOffset  = off & 0xffffffff;
		ior->io_Data       = trackbuf;
		ior->io_Length     = tracksize;

		off += tracksize;

		if (DoIO((struct IORequest *)ior))
		{
			retval = FRV_IOERR;
			break;
		}

		if (threads_check_abort())
		{
			retval = FRV_ABORTED;
			break;
		}
		
		if (mode == MODE_WRITE_STREAM)
		{
			for (i = 0; i < tracksize; i++)
			{
				if (trackbuf[i] != i % 256)
				{
					retval = FRV_VERIFY_ERROR;
					goto outver;
				}
			}
		}
		else
		{
			for (i = 0; i < tracksize; i++)
			{
				if (trackbuf[i] != val)
				{
					retval = FRV_VERIFY_ERROR;
					goto outver;
				}
			}
		}
		
		methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, j);
	}
	outver:

	return (retval);
}



static ULONG full_format(APTR obj, struct device_info *di, struct IOStdReq *ior, ULONG verify)
{
	UQUAD maxsize;
	ULONG tracksize;
	UWORD /*verifycmd,*/ writecmd; // bitRocky: just outcommented nto removed, maybe it will be used later?
	UBYTE *trackbuf;
	ULONG retval = FRV_NOMEM;

	tracksize = di->blocksize * di->surfaces * di->blockspertrack;

	/*
	 * If we require TD64 access, check if
	 * the device supports it.
	 */
	maxsize = (UQUAD)(di->highcyl + 1) * tracksize;
	if (maxsize > 0xffffffffULL)
	{
		APTR tempbuf;
		LONG iores;

		if ( !(tempbuf = AllocMem(di->blocksize, di->bufmemtype)) )
		{
			/* Out of memory, can't do much anyway, certainly not put up a req. */
			return (FALSE);
		}

		ior->io_Command    = TD_READ64;
		ior->io_HighOffset = 0;
		ior->io_LowOffset  = 0;
		ior->io_Data       = tempbuf;
		ior->io_Length     = di->blocksize;

		iores = DoIO((struct IORequest *)ior);

		FreeMem(tempbuf, di->blocksize);

		if (iores == IOERR_NOCMD)
		{
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Device doesn't support needed 64-bit access.");
			return (FALSE);
		}
		//verifycmd = TD_READ64;
		writecmd  = TD_WRITE64;
	}
	else
	{
		//verifycmd = CMD_READ;
		writecmd  = CMD_WRITE;
	}

	if ( (trackbuf = AllocMem(tracksize, di->bufmemtype | MEMF_CLEAR)) )
	{
		ULONG numtracks;
		UQUAD off;

		numtracks = di->highcyl - di->lowcyl + 1;
		off = (UQUAD)di->lowcyl * tracksize;

		methodstack_push(obj, 2, MM_Formatwin_SetText, verify ? "Blank write..." : "Formatting...");

		retval = write_tracks(obj, MODE_WRITE_0x00, numtracks, writecmd, off, trackbuf, tracksize, ior);

		switch (retval)
		{
			case FRV_IOERR:
				methodstack_push(obj, 2, MM_Formatwin_SetText, verify ? "I/O error (blank write)." : "I/O error (format).");
				goto out;
				break;

			case FRV_ABORTED:
				methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
				goto out;
				break;

			default:
				break;
		}

		if (verify)
		{
			/*
			 * According to Zuikkis, modern drives have several tracks reserved for
			 * bad block replacements and they automatically detect dodgy blocks
			 * and remap them. This renders simply verification useless *BUT*
			 * everyone saw cases of data corruption with Articia chipsets.
			 * Therefore I decided to implement a hardcore verify which could
			 * also be useful for broken HDs.
			 */
			
			/* blank write (0x0) */
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Verifying blank write...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);

			retval = verify_tracks(obj, MODE_WRITE_0x00, numtracks, writecmd, off, trackbuf, tracksize, ior);

			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (blank write verify).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;

				case FRV_VERIFY_ERROR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Verification error (blank write).");
					goto out;
					break;
			}

			/* full write (0xff) */
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Full write...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);
			retval = write_tracks(obj, MODE_WRITE_0xFF, numtracks, writecmd, off, trackbuf, tracksize, ior);
			
			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (full write).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;
			}

			methodstack_push(obj, 2, MM_Formatwin_SetText, "Verifying (full write)...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);

			retval = verify_tracks(obj, MODE_WRITE_0xFF, numtracks, writecmd, off, trackbuf, tracksize, ior);

			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (full write verify).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;

				case FRV_VERIFY_ERROR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Verification error (full write).");
					goto out;
					break;
			}

			/* pattern write 1 (0xaa) */
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Pattern write 1...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);
			retval = write_tracks(obj, MODE_WRITE_0xAA, numtracks, writecmd, off, trackbuf, tracksize, ior);
			
			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (full write).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;
			}
			
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Verifying pattern write 1...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);

			retval = verify_tracks(obj, MODE_WRITE_0xAA, numtracks, writecmd, off, trackbuf, tracksize, ior);

			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (pattern write 1 verify).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;

				case FRV_VERIFY_ERROR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Verification error (pattern write 1).");
					goto out;
					break;
			}

			/* pattern write 2 (0x55) */
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Pattern write 2...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);
			retval = write_tracks(obj, MODE_WRITE_0x55, numtracks, writecmd, off, trackbuf, tracksize, ior);

			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (pattern write 2).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;
			}
			
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Verifying pattern write 2...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);

			retval = verify_tracks(obj, MODE_WRITE_0x55, numtracks, writecmd, off, trackbuf, tracksize, ior);

			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (pattern write 2 verify).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;

				case FRV_VERIFY_ERROR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Verification error (pattern write 2).");
					goto out;
					break;
			}

			/* stream write */
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Stream write...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);
			retval = write_tracks(obj, MODE_WRITE_STREAM, numtracks, writecmd, off, trackbuf, tracksize, ior);

			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (stream write).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;
			}
			
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Verifying stream write...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);

			retval = verify_tracks(obj, MODE_WRITE_STREAM, numtracks, writecmd, off, trackbuf, tracksize, ior);

			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (stream write verify).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;

				case FRV_VERIFY_ERROR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Verification error (stream write).");
					goto out;
					break;
			}

			/* actual final format */
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Final formatting...");
			methodstack_push_sync(obj, 2, MM_Formatwin_SetGauge, 0);
			
			retval = write_tracks(obj, MODE_WRITE_0x00, numtracks, writecmd, off, trackbuf, tracksize, ior);

			switch (retval)
			{
				case FRV_IOERR:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "I/O error (final).");
					goto out;
					break;

				case FRV_ABORTED:
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Aborted.");
					goto out;
					break;

				default:
					break;
			}

		}

out:
		FreeMem(trackbuf, tracksize);
	}
	return (retval);
}


/*
 * XXX: we should wait for the disk to become validated before writing out icons (using ACTION_DISK_INFO)
 * XXX: do I have to use maxtransfer? hm.. bah
 */
ULONG tr_format(APTR obj, CONST_STRPTR label, struct device_info *di, ULONG mode, ULONG fs, ULONG flags)
{
	/* XXX: add more errormessages, check for CTRL_C everywhere */
	struct MsgPort *mp;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(di);

	/*
	 * XXX: perhaps add a Forbid() here to
	 * avoid the device to go away.. or hm, lock the
	 * devicelist actually.
	 */
	if ( (mp = DeviceProc(di->name)) ) /* XXX: hm. can't the device go away ? */
	{
		if (DoPkt1(mp, ACTION_INHIBIT, DOSTRUE))
		{
			struct IOStdReq *ior;

			if ( (ior = iostd_open_device(di->devname, di->unit, di->flags)) )
			{
				/*
				 * Check for write protection.
				 */
				ior->io_Command = TD_PROTSTATUS;
				ior->io_Actual  = 0;
				ior->io_Offset  = 0;
				ior->io_Data    = NULL;
				ior->io_Length  = 0;
				if (!DoIO((struct IORequest *)ior))
				{
					if (!ior->io_Actual)
					{
						ULONG ok = TRUE;

						if (mode != MV_Format_Format_Quick)
						{
							ok = full_format(obj, di, ior, mode == MV_Format_Format_Verify);
						}

						if (ok == FRV_OK)
						{
							methodstack_push(obj, 2, MM_Formatwin_SetText, "Initializing...");

							if (fs == FS_AMIGA_SFS)
							{
								struct TagItem tags[5];
								struct TagItem *tag = tags;

								tag->ti_Tag = ASF_NAME;
								tag->ti_Data = (ULONG)label;
								tag++;

								if (flags & FF_SFS_CASE)
								{
									tag->ti_Tag = ASF_CASESENSITIVE;
									tag->ti_Data = TRUE;
									tag++;
								}

								if (!(flags & FF_SFS_RECYCLED))
								{
									tag->ti_Tag = ASF_NORECYCLED;
									tag->ti_Data = TRUE;
									tag++;
								}

								if ((flags & FF_SFS_RECYCLED) && (flags & FF_SFS_SHOWRECYCLED))
								{
									tag->ti_Tag = ASF_SHOWRECYCLED;
									tag->ti_Data = TRUE;
									tag++;
								}
								tag->ti_Tag = TAG_END;
								tag->ti_Data = (ULONG)NULL;

								if (DoPkt1(mp, ACTION_SFS_FORMAT, (LONG)tags))
								{
									methodstack_push(obj, 2, MM_Formatwin_SetText, "Initialized successfully.");
								}
								else
								{
									methodstack_push(obj, 2, MM_Formatwin_SetText, "Initialization failed.");
								}

							}
							else
							{
								TEXT buf[VOLUME_SIZE + 1];

								stccpy(&buf[1], label, sizeof(buf) - 1);  /* limits to 30 chars */
								buf[0] = strlen(&buf[1]);

								/*
								 * Piru says Format() doesn't work on
								 * non mounted devices. Shrug.
								 */
								if (DoPkt2(mp, ACTION_FORMAT, MKBADDR(buf), fs))
								{
									methodstack_push(obj, 2, MM_Formatwin_SetText, "Initialized successfully.");
								}
								else
								{
									methodstack_push(obj, 2, MM_Formatwin_SetText, "Initialization failed.");
								}
							}
						}
					}
					else
					{
						methodstack_push(obj, 2, MM_Formatwin_SetText, "Device is write protected.");
					}
				}
				else
				{
					methodstack_push(obj, 2, MM_Formatwin_SetText, "Can't check protection status.");
				}
				iostd_close_device(ior);
			}
			else
			{
				methodstack_push(obj, 2, MM_Formatwin_SetText, "Couldn't open device.");
			}
			DoPkt1(mp, ACTION_INHIBIT, DOSFALSE);
		}
		else
		{
			methodstack_push(obj, 2, MM_Formatwin_SetText, "Couldn't inhibit volume.");
		}
	}
	return (TRUE); /* XXX: hm.. */
}
