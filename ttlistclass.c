/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: ttlistclass.c,v 1.8 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"


struct Data {
	ULONG changed;
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_List_DragSortable, TRUE,
		MUIA_List_ShowDropMarks, TRUE,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (0);
	}

	return ((ULONG)obj);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_TTList_IsList:
			*msg->opg_Storage = TRUE;
			return (TRUE);

		case MA_TTList_Changed:
			{
				GETDATA;

				*msg->opg_Storage = data->changed;
			}
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSET
{
	FORTAG(INITTAGS)
	{
		case MA_TTList_Changed:
			{
				GETDATA;

				data->changed = tag->ti_Data;
			}
			break;
	}
	NEXTTAG

	return (DOSUPER);
}


DEFMMETHOD(DragQuery)
{
	LONG islist;

	if (msg->obj == obj)
	{
		/* move entries inside ourself */
		return (DOSUPER);
	}
	else if (get(msg->obj, MA_TTList_IsList, &islist))
	{
		if (islist)
			return (MUIV_DragQuery_Accept);
	}
	return (MUIV_DragQuery_Refuse);
}


DEFMMETHOD(DragDrop)
{
	if (msg->obj == obj)
	{
		/* move entries inside ourself */
		set(obj, MA_TTList_Changed, TRUE);

		return (DOSUPER);
	}
	else
	{
		LONG id = MUIV_List_NextSelected_Start;
		APTR entry;
		STRPTR p;
		LONG dropmark;
		ULONG i;

		set(obj, MUIA_List_Quiet, TRUE);

		for (i = 0; ; i++)
		{
			LONG len;

			DoMethod(msg->obj, MUIM_List_NextSelected, &id);

			if (id == MUIV_List_NextSelected_End) break;

			DoMethod(msg->obj, MUIM_List_GetEntry, id, &entry);

			len = strlen((STRPTR) entry) + 1;

			if ((p = malloc(len)))  /* p is released by tt_dest_hook */
			{
				memcpy(p, (STRPTR)entry, len);
				dropmark = getv(obj, MUIA_List_DropMark);
				DoMethod(obj, MUIM_List_InsertSingle, p, dropmark + i);
			}
		}

		dropmark = getv(obj, MUIA_List_InsertPosition);
		set(obj, MUIA_List_Active, dropmark);
	   
		set(obj, MUIA_List_Quiet, FALSE);

		set(msg->obj, MUIA_List_Active, MUIV_List_Active_Off);

		set(obj, MA_TTList_Changed, TRUE);
	}
	return (0);
}


DEFMMETHOD(List_InsertSingle)
{
	ULONG rc;

	rc = DOSUPER;

	set(obj, MA_TTList_Changed, TRUE);

	return (rc);
}


DEFMMETHOD(List_Remove)
{
	ULONG rc;

	rc = DOSUPER;

	set(obj, MA_TTList_Changed, TRUE);

	return (rc);
}


BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECMMETHOD(List_InsertSingle)
DECMMETHOD(List_Remove)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, ttlistclass)
