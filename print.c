/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2007 Ilkka Lehtoranta
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
 * $Id: print.c,v 1.2 2013/10/28 20:00:16 geit Exp $
 */

#include "ambient.h"

/* public */
#include <devices/turboprint.h>
#include <proto/exec.h>
#include <proto/graphics.h>

/* private */
#include "ambient_cat.h"
#include "classes.h"
#include "gfx_bitmap.h"
#include "print.h"
#include "smartreq.h"
#include "threads.h"


struct PrinterData
{
	struct MsgPort port;
	union
	{
		struct IOStdReq    ios;
		struct IODRPReq    drp;
		struct IOPrtCmdReq pcr;
	} io;
};


static ULONG openprinter(struct PrinterData *pd)
{
	pd->port.mp_SigBit = AllocSignal(-1);
	if ((BYTE)pd->port.mp_SigBit != -1)
	{
		pd->port.mp_Node.ln_Type = NT_MSGPORT;
		pd->port.mp_Flags        = PA_SIGNAL;
		pd->port.mp_SigTask      = FindTask(NULL);
		NEWLIST(&pd->port.mp_MsgList);

		pd->io.ios.io_Message.mn_Node.ln_Type = NT_REPLYMSG;
		pd->io.ios.io_Message.mn_ReplyPort    = &pd->port;
		pd->io.ios.io_Message.mn_Length       = sizeof(pd->io);

		if (!OpenDevice("printer.device", 0, (struct IORequest *)&pd->io.ios, 0))
		{
			return (TRUE);
		}

		FreeSignal(pd->port.mp_SigBit);

		smartreq_request_sync(NULL, GSI(MSG_PRINT_REQUESTERTITLE), GSI(MSG_PRINT_OK), MV_Notification_Error, GSI(MSG_PRINT_COULDNOTFINDPRINTER) );
	}

	return (FALSE);
}


static VOID closeprinter(struct PrinterData *pd)
{
	CloseDevice((struct IORequest *)&pd->io.ios);
	FreeSignal(pd->port.mp_SigBit);
}


static LONG doprint(struct PrinterData *pd)
{
	ULONG abort;
	LONG rc;

	abort = FALSE;
	rc = TRUE;

	SendIO((struct IORequest *)&pd->io);

	threads_waitsig(1 << pd->io.ios.io_Message.mn_ReplyPort->mp_SigBit, &abort);

	if (abort)
	{
		AbortIO((struct IORequest *)&pd->io);

		rc = ABORTED;
	}

	WaitIO((struct IORequest *)&pd->io);

	return (rc);
}


static LONG dump_text(struct PrinterData *pd, CONST_STRPTR text)
{
	pd->io.ios.io_Command = PRD_RAWWRITE;
	pd->io.ios.io_Data    = (APTR)text;
	pd->io.ios.io_Length  = -1;

	return (doprint(pd));
}


static LONG dump_bitmap(struct PrinterData *pd, APTR bm)
{
	struct ColorMap *dummycm;
	LONG rc = FALSE;

	/*
	 * It would seem sensible to only use colormap when
	 * the bitmap isn't truecolor. However, it seems TP uses
	 * io_ColorMap regardless.
	 */
	dummycm = GetColorMap(256);
	if (dummycm)
	{
		struct TPExtIODRP ExtIoDrp;
		struct RastPort rp;
		struct BitMap newbm;

		InitBitMap(&newbm, 1, gfx_bitmap_width(bm), gfx_bitmap_height(bm));
		newbm.BytesPerRow = gfx_bitmap_width(bm) * 3;
		newbm.Planes[0] = gfx_bitmap_array(bm);

		InitRastPort(&rp);
		rp.BitMap = &newbm;

		pd->io.drp.io_Command   = PRD_TPEXTDUMPRPORT;
		pd->io.drp.io_RastPort  = &rp;
		pd->io.drp.io_ColorMap  = dummycm;
		pd->io.drp.io_SrcX      = 0;
		pd->io.drp.io_SrcY      = 0;
		pd->io.drp.io_SrcWidth  = gfx_bitmap_width(bm);
		pd->io.drp.io_SrcHeight = gfx_bitmap_height(bm);
		pd->io.drp.io_Special   = 0;

		pd->io.drp.io_Modes = (IPTR)&ExtIoDrp;

		ExtIoDrp.PixAspX = 1;
		ExtIoDrp.PixAspY = 1;
		ExtIoDrp.Mode    = TPFMT_RGB24;

		rc = doprint(pd);

		FreeColorMap(dummycm);
	}

	return (rc);
}


ULONG tr_print(APTR obj UNUSED, APTR buffer, ULONG type)
{
	struct PrinterData pd;
	ULONG rc = FALSE;

	THREAD;
	ASSERT(buffer);

	if (openprinter(&pd))
	{
		switch (type)
		{
			case PRT_BITMAP:
				rc = dump_bitmap( &pd, buffer );
				break;

			case PRT_TEXT:
				rc = dump_text( &pd, buffer );
				break;
		}

		closeprinter(&pd);
	}

	switch (type)
	{
		case PRT_BITMAP:
			gfx_bitmap_delete(buffer);
			break;

		case PRT_TEXT:
			FreeVec(buffer);
			break;
	}

	return (rc);
}
