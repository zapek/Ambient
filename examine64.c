/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: examine64.c,v 1.8 2025/08/17 17:18:57 piru Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

/* private */
#include "examine64.h"
#include "doslistcache.h"


ULONG examine64(CONST_STRPTR name, struct fileinfo64 *fi)
{
	if (!name || !fi)
	{
		return (FALSE);
	}

	#if !USE_LEGACY

	if (1) //if (LIB_MINVER(&DOSBase->dl_lib, 51, 28))
	{
		BPTR l;

		if ( (l = Lock(name, ACCESS_READ)) )
		{
			LONG res;
			D_S(struct FileInfoBlock, fib);

			res = Examine64(l, fib, NULL);
			UnLock(l);

			if (res)
			{
				fi->fi_Type           = fib->fib_DirEntryType;
				strcpy(fi->fi_FileName, fib->fib_FileName);
				fi->fi_Protection     = fib->fib_Protection;
				fi->fi_Size           = fib->fib_Size64;
				fi->fi_NumBlocks      = fib->fib_NumBlocks64;
				fi->fi_Date.ds_Days   = fib->fib_Date.ds_Days;
				fi->fi_Date.ds_Minute = fib->fib_Date.ds_Minute;
				fi->fi_Date.ds_Tick   = fib->fib_Date.ds_Tick;
				strcpy(fi->fi_Comment, fib->fib_Comment);
				fi->fi_OwnerUID       = fib->fib_OwnerUID;
				fi->fi_OwnerGID       = fib->fib_OwnerGID;

				return (TRUE);
			}
		}
		return (FALSE);
	}

	#endif

	{
		BPTR l;

		if ( (l = Lock(name, ACCESS_READ)) )
		{
			LONG res;
			D_S(struct FileInfoBlock, fib);

			res = Examine(l, fib);
			UnLock(l);

			if (res)
			{
				fi->fi_Type           = fib->fib_DirEntryType;
				strcpy(fi->fi_FileName, fib->fib_FileName);
				fi->fi_Protection     = fib->fib_Protection;
				fi->fi_Size           = (ULONG)fib->fib_Size;
				fi->fi_NumBlocks      = (ULONG)fib->fib_NumBlocks;
				fi->fi_Date.ds_Days   = fib->fib_Date.ds_Days;
				fi->fi_Date.ds_Minute = fib->fib_Date.ds_Minute;
				fi->fi_Date.ds_Tick   = fib->fib_Date.ds_Tick;
				strcpy(fi->fi_Comment, fib->fib_Comment);
				fi->fi_OwnerUID       = fib->fib_OwnerUID;
				fi->fi_OwnerGID       = fib->fib_OwnerGID;

				return (TRUE);
			}
		}
	}

	return (FALSE);
}

ULONG examinefh64(BPTR fh, struct fileinfo64 *fi)
{
	if (!fh || !fi)
	{
		return (FALSE);
	}

	#if !USE_LEGACY

	if (1) //LIB_MINVER(&DOSBase->dl_lib, 51, 28))
	{
		D_S(struct FileInfoBlock, fib);

		if (ExamineFH64(fh, fib, NULL))
		{
			fi->fi_Type           = fib->fib_DirEntryType;
			strcpy(fi->fi_FileName, fib->fib_FileName);
			fi->fi_Protection     = fib->fib_Protection;
			fi->fi_Size           = fib->fib_Size64;
			fi->fi_NumBlocks      = fib->fib_NumBlocks64;
			fi->fi_Date.ds_Days   = fib->fib_Date.ds_Days;
			fi->fi_Date.ds_Minute = fib->fib_Date.ds_Minute;
			fi->fi_Date.ds_Tick   = fib->fib_Date.ds_Tick;
			strcpy(fi->fi_Comment, fib->fib_Comment);
			fi->fi_OwnerUID       = fib->fib_OwnerUID;
			fi->fi_OwnerGID       = fib->fib_OwnerGID;

			return (TRUE);
		}
		return (FALSE);
	}

	#endif

	{
		D_S(struct FileInfoBlock, fib);

		if (ExamineFH(fh, fib))
		{
			fi->fi_Type           = fib->fib_DirEntryType;
			strcpy(fi->fi_FileName, fib->fib_FileName);
			fi->fi_Protection     = fib->fib_Protection;
			fi->fi_Size           = (ULONG)fib->fib_Size;
			fi->fi_NumBlocks      = (ULONG)fib->fib_NumBlocks;
			fi->fi_Date.ds_Days   = fib->fib_Date.ds_Days;
			fi->fi_Date.ds_Minute = fib->fib_Date.ds_Minute;
			fi->fi_Date.ds_Tick   = fib->fib_Date.ds_Tick;
			strcpy(fi->fi_Comment, fib->fib_Comment);
			fi->fi_OwnerUID       = fib->fib_OwnerUID;
			fi->fi_OwnerGID       = fib->fib_OwnerGID;

			return (TRUE);
		}
	}

	return (FALSE);
}

