/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: libfunctable.c,v 1.3 2006/02/22 14:48:24 fab Exp $
 */

#include "globals.h"

/* public */
#include <dos/dosextens.h> /* makes gcc shutup */

/* private */
#include "lib.h"

ULONG LibFuncTable[] =
{
	FUNCARRAY_BEGIN,
	FUNCARRAY_32BIT_NATIVE,
	(ULONG)&LIB_Open,
	(ULONG)&LIB_Close,
	(ULONG)&LIB_Expunge,
	(ULONG)&LIB_GetQueryAttr,
	(ULONG)&LIB_GetWBObject,
	(ULONG)&LIB_PutWBObject,
	(ULONG)&LIB_GetIcon,
	(ULONG)&LIB_PutIcon,
	(ULONG)&LIB_FreeFreeList,
	(ULONG)&LIB_FreeWBObject,
	(ULONG)&LIB_AllocWBObject,
	(ULONG)&LIB_AddFreeList,
	(ULONG)&LIB_GetDiskObject,
	(ULONG)&LIB_PutDiskObject,
	(ULONG)&LIB_FreeDiskObject,
	(ULONG)&LIB_FindToolType,
	(ULONG)&LIB_MatchToolValue,
	(ULONG)&LIB_BumpRevision,
	(ULONG)&LIB_FreeAlloc,
	(ULONG)&LIB_GetDefDiskObject,
	(ULONG)&LIB_PutDefDiskObject,
	(ULONG)&LIB_GetDiskObjectNew,
	(ULONG)&LIB_DeleteDiskObject,
	/* start of the "OS" 3.5 mess */
	(ULONG)&LIB_FreeFree,
	(ULONG)&LIB_DupDiskObjectA,
	(ULONG)&LIB_IconControlA,
	(ULONG)&LIB_DrawIconStateA,
	(ULONG)&LIB_GetIconRectangleA,
	(ULONG)&LIB_NewDiskObject,
	(ULONG)&LIB_GetIconTagList,
	(ULONG)&LIB_PutIconTagList,
	(ULONG)&LIB_LayoutIconA,
	(ULONG)&LIB_ChangeToSelectedIconColor,
	0xffffffff,
	FUNCARRAY_END
};
