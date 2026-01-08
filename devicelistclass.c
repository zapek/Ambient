/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005 Ambient Open Source Team
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
 * $Id: devicelistclass.c,v 1.5 2006/09/18 23:17:28 fab Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dosextens.h>

/* private */
#include "mui_func.h"
#include "doslistcache.h"
#include "device_func.h"
#include "devicelistclass.h"
#include "ambient_cat.h"

struct Data {
	int dummy;
};

struct volume_entry * volumeentry_build(STRPTR name)
{
	struct volume_entry * ve;
	ve = (struct volume_entry *) malloc(sizeof(*ve));

	if(ve)
	{
		memset(ve, 0, sizeof(*ve));

		ve->hidden = FALSE;
		ve->name = (STRPTR) malloc(strlen(name)+1);

		if(ve->name)
		{
			strcpy(ve->name, name);
			return ve;
		}
	
	}

	volumeentry_delete(ve);

	return NULL;
}

void volumeentry_delete(struct volume_entry * ve)
{
	if (ve)
	{
		if (ve->name)
		{
			free(ve->name);
		}
		free(ve);
	}
}


DEFMMETHOD(List_Construct)
{
	TEXT t[VOLUME_SIZE + 1];

	stccpy(t, (STRPTR)msg->entry, sizeof(t) - 1);
	strcat(t, ":");

	return ((ULONG)volumeentry_build(t));
}


DEFMMETHOD(List_Destruct)
{
	volumeentry_delete((struct volume_entry *)msg->entry);
	return (0);
}


DEFMMETHOD(List_Display)
{
	if(msg->entry)
	{
		msg->array[0] = ((struct volume_entry *)msg->entry)->hidden?"x":" ";
		msg->array[1] = ((struct volume_entry *)msg->entry)->name;
	}
	else
	{
		msg->array[0] = " ";
		msg->array[1] = GSI(MSG_DEVICELISTCLASS_HIDDENDRIVESCOLUMN);
	}
	return (0);
}


DEFMMETHOD(List_Compare)
{
	struct volume_entry *ve1 = msg->entry1;
	struct volume_entry *ve2 = msg->entry2;

	return (stricmp(ve1->name, ve2->name));
}


DEFNEW
{
	struct dlcnode *dlcn;

	obj = DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_AdjustWidth, FALSE,
		MUIA_List_AutoVisible, TRUE,
		MUIA_List_Title, TRUE,
		MUIA_List_Format, "WEIGHT=0 BAR, MINWIDTH=-1",
	End;

	if (!obj)
	{
		return (NULL);
	}

	ITERATEDLC(dlcn)
	{
		if (dlcn->type == DLT_VOLUME)
		{
			DoMethod(obj, MUIM_List_InsertSingle, dlcn->name, MUIV_List_Insert_Sorted);
		}
	}
	return ((ULONG)obj);
}

BEGINMTABLE
DECNEW
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECMMETHOD(List_Compare)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, devicelistclass)
