/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: iffio.c,v 1.3 2006/02/22 14:48:20 fab Exp $
 */

#include "ambient.h"

#if USE_IFF_IO

/* public */
#include <exec/memory.h>
#include <dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/asyncio.h>
#include <proto/iffparse.h>
#include <prefs/prefhdr.h>
#include <prefs/screenmode.h>

/* private */
#include "iffio.h"

#define ASYNCBUFFERSIZE 8192 /* size of the asynchronous I/O buffer */

/*
 * Asynchronous IFF routines
 */
//ULONG ASM SAVEDS iffasyncfunc(__reg(a0, struct Hook *streamHook), __reg(a2, struct IFFHandle *iffh), __reg(a1, struct IFFStreamCmd *iffcmd))
MUI_HOOK(iffasyncfunc, struct IFFHandle *iffh, struct IFFStreamCmd *iffcmd)
{
	switch(iffcmd->sc_Command)
	{
		case IFFCMD_READ:
			if (ReadAsync((struct AsyncFile *)iffh->iff_Stream, iffcmd->sc_Buf, iffcmd->sc_NBytes) == -1)
			{
				return(IFFERR_READ);
			}
			return(0);
			break;

		case IFFCMD_WRITE:
			if (WriteAsync((struct AsyncFile *)iffh->iff_Stream, iffcmd->sc_Buf, iffcmd->sc_NBytes) == -1)
			{
				return(IFFERR_WRITE);
			}
			return(0);
			break;

		case IFFCMD_SEEK:
			if (SeekAsync((struct AsyncFile *)iffh->iff_Stream, iffcmd->sc_NBytes, MODE_CURRENT) == -1) /* XXX: yeah, not 64-bits.. who cares */
			{
				return(IFFERR_SEEK);
			}
			return(0);
			break;

		default:
			return(0);
			break;
	}
}


/*
 * Writes an IFF chunk
 */
int writechunk(struct IFFHandle *iffh, LONG id, APTR buf, LONG len)
{
	if (!PushChunk(iffh, ID_PREF, id, IFFSIZE_UNKNOWN))
	{
		if (!WriteChunkBytes(iffh, buf, len) != len)
		{
			if (!PopChunk(iffh))
			{
				return(TRUE);
			}
		}
	}
	return(FALSE);
}


/*
 * Reads an IFF chunk
 */
int readchunk(struct IFFHandle *iffh, LONG id, APTR buf, LONG len)
{
	struct ContextNode *cn;	
	
	if (!ParseIFF(iffh, IFFPARSE_SCAN))
	{
		if (cn = CurrentChunk(iffh))
		{
			if (!((cn->cn_Type != ID_PREF) || (cn->cn_ID != id) || (cn->cn_Size != (len))))
			{
				if (!ReadChunkBytes(iffh, buf, cn->cn_Size) != cn->cn_Size)
				{
					return(TRUE);
				}
			}
		}
	}
	return(FALSE);
}
 

/*
 * Loads the screenmode prefs.
 */
struct ScreenModePrefs * load_screenmode_prefs(STRPTR filename)
{
	struct ScreenModePrefs *smp;
	ULONG retval = FALSE;
	struct IFFHandle *iffh;
	struct PrefHeader prhd;

	if (smp = malloc(sizeof(*smp)))
	{
		/* init iff handle */
		if (!(iffh = AllocIFF()))
			goto done;

		if (!(iffh->iff_Stream = (ULONG)OpenAsync(filename, MODE_READ, ASYNCBUFFERSIZE)))
			goto done;
		InitIFF(iffh, IFFF_FSEEK | IFFF_RSEEK, &iffasyncfunc_hook);

		if (OpenIFF(iffh, IFFF_READ))
			goto done;

		/* stop at these chunks */
		if (StopChunk(iffh, ID_PREF, ID_PRHD))
			goto done;

		if (StopChunk(iffh, ID_PREF, ID_SCRM))
			goto done;

		/* right version ? */
		if (!readchunk(iffh, ID_PRHD, &prhd, sizeof(prhd)))
			goto done;

		DB(("found PRHD chunk, phrd.ph_Version == %lu\n", prhd.ph_Version));

		//if (prhd.ph_Version < PREFS_VERSION)
		//	  goto done;

		//DB(("version is ok\n"));

		if (!readchunk(iffh, ID_SCRM, smp, sizeof(*smp)))
			goto done;

		retval = TRUE;

done:
		if (iffh)
		{
			CloseIFF(iffh);
			CloseAsync((struct AsyncFile *)iffh->iff_Stream);
			FreeIFF(iffh);
		}

		if (!retval)
		{
			free(smp);
			smp = NULL;
		}
	}
	return(smp);
}

#endif /* USE_IFF_IO */
