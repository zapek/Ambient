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
 * $Id: menuitemclass.c,v 1.6 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"
#include "command.h"
#include "contextmenu.h"

struct Data {
	ULONG menutype;
	struct command_menu *cm;
	ULONG freecm;
	ULONG subtype;
};


DEFNEW
{
	struct Data *data;

	obj = DoSuperNew(cl, obj,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);

	FORTAG(INITTAGS)
	{
		case MA_Menuitem_MenuType:
			data->menutype = tag->ti_Data;
			break;
	
		case MA_Menuitem_Command:
			data->cm = (APTR)tag->ti_Data;
			break;

		case MA_Menuitem_FreeCommand:
			data->freecm = tag->ti_Data;
			break;

		case MA_Menuitem_SubType:
			data->subtype = tag->ti_Data;
			break;
	}
	NEXTTAG

	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;

	if (data->freecm)
	{
		ASSERT(data->cm);
		contextmenu_delete_cm(data->cm);
	}
	return (DOSUPER);
}


DEFMMETHOD(FindUData)
{
	return ((ULONG)findudata(obj, msg));
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Menuitem_MenuType:
			{
				GETDATA;
				*msg->opg_Storage = data->menutype;
				return (TRUE);
			}

		case MA_Menuitem_Command:
			{
				GETDATA;
				*msg->opg_Storage = (ULONG)data->cm;
				return (TRUE);
			}

		case MA_Menuitem_SubType:
			{
				GETDATA;
				*msg->opg_Storage = data->subtype;
				return (TRUE);
			}
	}
	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECDISP
DECGET
DECMMETHOD(FindUData)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Menuitem, menuitemclass)


APTR findudata(APTR obj, struct MUIP_FindUData *msg)
{
	struct command_menu *cm;
	ULONG rc;

	cm = (struct command_menu *)getv(obj, MA_Menuitem_Command);

	if (cm && cm->args && !stricmp((STRPTR)msg->udata, cm->args))
	{
		return (obj);
	}


	FORCHILD(obj, MUIA_Family_List)
	{
		if ((rc = DoMethodA(child, (Msg)msg)))
		{
			return ((APTR)rc);
		}
	}
	NEXTCHILD

	return (NULL);
}
