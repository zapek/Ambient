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
 * $Id: ttlistviewclass.c,v 1.6 2025/07/23 23:54:26 geit Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"


struct Data {
	int dummy;
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_Listview_DragType, MUIV_Listview_DragType_Immediate,
		MUIA_Dropable, TRUE,
		MUIA_Listview_MultiSelect, MUIV_Listview_MultiSelect_Default,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	return ((ULONG)obj);
}


DEFSET
{
	struct TagItem *ti;

	if ((ti = FindTagItem(MUIA_Listview_DoubleClick, INITTAGS)))
	{
		if (ti->ti_Data)
		{
			STRPTR p;
			ULONG len;
			ULONG pos = getv(obj, MUIA_List_Active);

			/*
			 * Switch tooltype from (BLA) or BLA to
			 * what's needed.
			 */
			DoMethod(obj, MUIM_List_GetEntry, pos, &p);
			if (p)
			{
				len = strlen(p);

				if (*p == '(' && *(p + len - 1) == ')')
				{
					memmove(p, p + 1, 256 - 1);
					*(p + len - 2) = '\0';
				}
				else
				{
					memmove(p + 1, p, 256 - 1);
					*p = '(';
					*(p + len + 1) = ')';
					*(p + len + 2) = '\0';
				}
				DoMethod(obj, MUIM_List_Redraw, MUIV_List_Redraw_Active);
				/* force that notify */
				set(obj, MUIA_List_Quiet, TRUE);
				set(obj, MUIA_List_Active, MUIV_List_Active_Off);
				set(obj, MUIA_List_Active, pos);
				set(obj, MUIA_List_Quiet, FALSE);

				set(obj, MA_TTList_Changed, TRUE);
			}
		}
	}
	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECSET
ENDMTABLE

DECSUBCLASS_NC(MUIC_Listview, ttlistviewclass)
