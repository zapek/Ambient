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
 * $Id: libfunctable.c,v 1.6 2016/08/11 17:02:38 itix Exp $
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
	(ULONG)&LIB_UpdateWorkbench,
	(ULONG)&LIB_QuoteWorkbench,
	(ULONG)&LIB_StartWorkbench,
	(ULONG)&LIB_AddAppWindowA,
	(ULONG)&LIB_RemoveAppWindow,
	(ULONG)&LIB_AddAppIconA,
	(ULONG)&LIB_RemoveAppIcon,
	(ULONG)&LIB_AddAppMenuItemA,
	(ULONG)&LIB_RemoveAppMenuItem,
	(ULONG)&LIB_WBConfig,
	(ULONG)&LIB_WBInfo,
	(ULONG)&LIB_OpenWorkbenchObjectA,
	(ULONG)&LIB_CloseWorkbenchObjectA,
	(ULONG)&LIB_WorkbenchControlA,
	(ULONG)&LIB_AddAppWindowDropZoneA,
	(ULONG)&LIB_RemoveAppWindowDropZone,
	(ULONG)&LIB_ChangeWorkbenchSelectionA,
	(ULONG)&LIB_MakeWorkbenchObjectVisibleA,
	0xffffffff,

	FUNCARRAY_32BIT_SYSTEMV,
	(ULONG)&LIB_AppWindowObtain,
	(ULONG)&LIB_AppWindowRelease,
	(ULONG)&LIB_ManageDesktopObjectA,
	(ULONG)&LIB_CreateDrawerA,
	(ULONG)&LIB_CreateIconA,
	0xffffffff,

	FUNCARRAY_END
};
