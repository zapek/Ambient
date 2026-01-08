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
 * $Id: columnslistclass.c,v 1.4 2013/10/28 11:00:31 geit Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"
#include "listviewclass.h"
#include "ambient_cat.h"
#include "columnslistclass.h"

struct Data {
	int dummy;
};

DEFMMETHOD(List_Construct)
{
	struct column_entry * ce = (struct column_entry *) msg->entry;
	struct column_entry * ce_copy;
	
	ce_copy = (struct column_entry *) malloc(sizeof(*ce_copy));

	if(ce_copy)
	{
		memcpy(ce_copy, ce, sizeof(*ce));
		ce_copy->name = (ce_copy->msgid)?(STRPTR) GSI(ce_copy->msgid):(STRPTR) GSI( MSG_COLUMNSLIST_ICON);  /* icon column a 0 msgid */
	}

	return ((ULONG)ce_copy);
}


DEFMMETHOD(List_Destruct)
{
	free((struct column_entry *)msg->entry);
	return (0);
}


DEFMMETHOD(List_Display)
{
	if(msg->entry)
	{
		struct column_entry * e = (struct column_entry *)msg->entry;
		if(e->sort_column)
		{
			msg->array[0] = (e->sort_direction == 1)?"\033c\033I[6:38]":"\033c\033I[6:39]";
		}
		else
		{
			msg->array[0] = " ";
		}
		msg->array[1] = ((struct column_entry *)msg->entry)->name;
		msg->array[2] = ((struct column_entry *)msg->entry)->hidden?"\033cx":" ";
	}
	else
	{
		msg->array[0] = GSI( MSG_COLUMNSLIST_SORTBY );
		msg->array[1] = GSI( MSG_COLUMNSLIST_NAME );
		msg->array[2] = GSI( MSG_COLUMNSLIST_HIDDEN );
	}
	return (0);
}

#if 0 /* was not used, so I disabled it (geit) */
DEFMMETHOD(List_Compare)
{
	struct column_entry *ce1 = msg->entry1;
	struct column_entry *ce2 = msg->entry2;

	return (stricmp(ce1->name, ce2->name));
}
#endif

DEFSMETHOD(Listviewlist_DoubleClick)
{
	struct column_entry * entry;
	struct MUI_List_TestPos_Result res;

	DoMethod(obj, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &entry);

	if(entry)
	{
		DoMethod(obj, MUIM_List_TestPos, _window(obj)->MouseX, _window(obj)->MouseY, &res);

		if(res.entry != -1)
		{
			/* hidden column */
			if(res.column == 2)
			{
				if(entry->hidden)
				{
					entry->hidden = FALSE;
				}
				else
				{
					entry->hidden = TRUE;
				}

				DoMethod(obj, MUIM_List_Redraw, res.entry);
			}

			/* sort column */
			if(res.column == 0)
			{
				int i;
				struct column_entry * e;

				for(i=0;; i++)
				{
					DoMethod(obj, MUIM_List_GetEntry, i, &e);

					if(e)
					{
						/* clear sort_column for all the other entries */
						if(i != res.entry)
						{
							e->sort_column = FALSE;
						}
						else
						{
							/* if the current entry is already used to sort, invert direction */
							if(e->sort_column)
							{
								e->sort_direction *= -1;
							}
							e->sort_column = TRUE;							  
						}
					}
					else
					{
						break;
					}
				}

				DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_All);
			}
		}
	}

	return (0);
}


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		InputListFrame,
		MUIA_List_AdjustWidth, FALSE,
		MUIA_List_AutoVisible, TRUE,
		MUIA_List_Title, TRUE,
		MUIA_Dropable, TRUE,
		MUIA_List_DragType, 1,
		MUIA_List_ShowDropMarks, TRUE,
		MUIA_List_DragSortable, TRUE,
		MUIA_List_Format, "W=0 BAR,W=0 BAR,MIW=-1",
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (NULL);
	}

	DoMethod(obj, MUIM_Notify, MUIA_Listview_DoubleClick, MUIV_EveryTime,
			 obj, 2, MM_Listviewlist_DoubleClick, MUIV_TriggerValue);

	return ((ULONG)obj);
}

BEGINMTABLE
DECNEW
DECMMETHOD(List_Construct)
DECMMETHOD(List_Destruct)
DECMMETHOD(List_Display)
DECSMETHOD(Listviewlist_DoubleClick)
ENDMTABLE

DECSUBCLASS_NC(MUIC_List, columnslistclass)
