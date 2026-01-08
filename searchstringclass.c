/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2015 Ambient Open Source Team
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
 * $Id: searchstringclass.c,v 1.5 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <devices/rawkeycodes.h>

/* private */
#include "mui_func.h"

struct Data {
	struct MUI_EventHandlerNode ehnode;
};


DEFNEW
{
	struct Data *data;

	obj = DoSuperNew(cl, obj,
		StringFrame, MUIA_Font, MUIV_Font_Normal,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);

	data->ehnode.ehn_Object = obj;
	data->ehnode.ehn_Class = cl;
	data->ehnode.ehn_Events = IDCMP_RAWKEY;
	data->ehnode.ehn_Priority = -127;
	data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;

	return ((ULONG)obj);
}

DEFMMETHOD(Setup)
{
	ULONG rc;
	
	if ((rc = DOSUPER))
	{
		GETDATA;

		DoMethod(_win(obj), MUIM_Window_AddEventHandler, &data->ehnode);
	}

	return rc;
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	DoMethod(_win(obj), MUIM_Window_RemEventHandler, &data->ehnode);

	return (DOSUPER);
}


DEFMMETHOD(HandleEvent)
{
	struct IntuiMessage *imsg;

	if ((imsg = msg->imsg))
	{
		switch (imsg->Class)
		{
			case IDCMP_RAWKEY:
			{
				switch (imsg->Code)
				{
					case RAWKEY_F2:
						DoMethod(_parent(obj), MM_Searchbar_Search,
							MV_Search_SearchPrev
						);
						return (MUI_EventHandlerRC_Eat);

					case RAWKEY_F3:
						DoMethod(_parent(obj), MM_Searchbar_Search,
							MV_Search_SearchNext
						);
						return (MUI_EventHandlerRC_Eat);
				}
			}
		}
	}
	return (DOSUPER);
}

BEGINMTABLE
DECNEW
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
ENDMTABLE

DECSUBCLASS_NC(MUIC_String, searchstringclass)

