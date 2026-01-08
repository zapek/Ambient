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
 * $Id: info64.c,v 1.5 2007/11/11 17:59:38 piru Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <proto/dos.h>

/* private */
#include "info64.h"
#include "doslistcache.h"


ULONG info64(CONST_STRPTR name, struct devinfo64 *di)
{
	BPTR l;

	if (!name || !di)
	{
		return (FALSE);
	}

	if ( (l = Lock(name, ACCESS_READ)) )
	{
		LONG res;
		D_S(struct InfoData, id);

		res = Info(l, id);
		UnLock(l);

		if (res)
		{
			#define CP(x) di->di_ ## x = id->id_ ## x

			CP(NumSoftErrors);
			CP(UnitNumber);
			CP(DiskState);
			di->di_NumBlocks     = (ULONG)id->id_NumBlocks;
			di->di_NumBlocksUsed = (ULONG)id->id_NumBlocksUsed;
			CP(BytesPerBlock);
			CP(DiskType);
			CP(VolumeNode);
			CP(InUse);
			di->di_DeviceType    = -1; /* info N/A */
			di->di_Flags         = 0;

			#if !USE_LEGACY

			if (DOSBase->dl_lib.lib_Version >= 51)
			{
				ULONG lval;
				UQUAD qval;

				if (GetFileSysAttr(name, FQA_NumBlocks, &qval, sizeof(qval)))
				{
					di->di_NumBlocks = qval;
				}
				if (GetFileSysAttr(name, FQA_NumBlocksUsed, &qval, sizeof(qval)))
				{
					di->di_NumBlocksUsed = qval;
				}

				if (GetFileSysAttr(name, FQA_IsCaseSensitive, &lval, sizeof(lval)))
				{
					if (lval)
					{
						di->di_Flags |= DIF_CASE;
					}
				}

				if (GetFileSysAttr(name, FQA_MaxFileSize, &qval, sizeof(qval)))
				{
					if (qval > 0xffffffffULL)
					{
						di->di_Flags |= DIF_64BIT;
					}
				}

				if (GetFileSysAttr(name, FQA_DeviceType, &lval, sizeof(lval)))
				{
					di->di_DeviceType = lval;
				}
			}

			#endif

			return (TRUE);
		}
	}

	return (FALSE);
}

