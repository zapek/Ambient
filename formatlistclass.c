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
 * $Id: formatlistclass.c,v 1.5 2023/01/11 15:14:30 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dosextens.h>

/* private */
#include "mui_func.h"
#include "doslistcache.h"
#include "device_func.h"

struct Data {
	int dummy;
};


DEFMMETHOD(List_Construct)
{
	TEXT t[VOLUME_SIZE + 1];

	stccpy(t, (STRPTR)msg->entry, sizeof(t) - 1);
	strcat(t, ":");
	
	return ((ULONG)deviceinfo_build(t, TRUE));
}


DEFMMETHOD(List_Destruct)
{
	deviceinfo_delete((struct device_info *)msg->entry);
	return (0);
}


DEFMMETHOD(List_Display)
{
	msg->array[0] = ((struct device_info *)msg->entry)->name;
	return (0);
}


DEFMMETHOD(List_Compare)
{
	struct device_info *di1 = msg->entry1;
	struct device_info *di2 = msg->entry2;

	return (stricmp(di1->name, di2->name));
}


DEFNEW
{
	struct dlcnode *dlcn;

	obj = DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_AdjustWidth, TRUE,
		MUIA_List_AutoVisible, TRUE,
	End;

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	/*
	 * Fill ourself in.
	 */
	ITERATEDLC(dlcn)
	{
		if (dlcn->type == DLT_DEVICE && dlcn->is_fs == DLC_FS_FILESYSTEM)
		{
			DoMethod(obj, MUIM_List_InsertSingle, dlcn->name, MUIV_List_Insert_Sorted);
		}
	}
	return ((ULONG)obj);
}

/* XXX: add a GET method to get the entry's "di" structure.. or.. actually just the entry is that so.. */

BEGINMTABLE
DECNEW
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, formatlistclass)
