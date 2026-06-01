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
 * $Id: makelinkwinclass.c,v 1.7 2025/09/09 12:46:46 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "methodstack.h"
#include "mui_func.h"
#include "screen.h"
#include "threads.h"


struct Data {
	APTR str_name;
	TEXT path[NAME_SIZE];
	TEXT from[NAME_SIZE];
};


DEFNEW
{
	struct Data *data;
	APTR bt_ok, bt_cancel, str_name;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI( MSG_MAKELINKWINCLASS_TITLE ),
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Moused,
		MUIA_Window_Width, MUIV_Window_Width_MinMax(10),
		MUIA_Window_Height, MUIV_Window_Width_MinMax(10),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_NoMenus, TRUE,
		MUIA_Window_CloseGadget, FALSE,
		WindowContents, VGroup,
			Child, TextObject,
				MUIA_Text_Contents, GSI( MSG_MAKELINKWINCLASS_ENTERSHORTCUT ),
			End,

			Child, str_name = StringObject,
				StringFrame,
				MUIA_String_Reject, ":", /* XXX: hm.. */
				MUIA_String_MaxLen, NAME_SIZE,
				MUIA_CycleChain, 1,
			End,

			Child, HGroup,
				MUIA_Group_SameWidth, TRUE,
				Child, bt_ok = MUICreateButton(MSG_MAKELINKWINCLASS_OK, "MAKELINK_OK"), /* XXX: change the order depending if the window was called on view by icon or lister */
				Child, HSpace(0),
				Child, bt_cancel = MUICreateButton(MSG_MAKELINKWINCLASS_CANCEL, "MAKELINK_CANCEL"),
			End,
		End,
	End;

	if (!obj)
	{
		return ((ULONG) NULL);
	}

	data = INST_DATA(cl, obj);
	data->str_name = str_name;

	FORTAG(INITTAGS)
	{
		case MA_Makelinkwin_From:
		{
			STRPTR p = (STRPTR)tag->ti_Data;
			
			if (*p)
			{
				strcpy(data->from, p);
				stccpy(data->path, p, (ULONG)FilePart(p) - (ULONG)p);
			}
		}
		break;
	}
	NEXTTAG

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Makelinkwin_Close
	);

	DoMethod(bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Makelinkwin_Makelink
	);

	DoMethod(data->str_name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 1, MM_Makelinkwin_Makelink
	);

	set(obj, MUIA_Window_ActiveObject, data->str_name);

	return ((ULONG)obj);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = (ULONG)data->path; /* XXX: not applicable I think */
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Makelink;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFTMETHOD(Makelinkwin_Makelink)
{
	GETDATA;

	TEXT t[PATH_SIZE];
	STRPTR n;

	n = (STRPTR)getv(data->str_name, MUIA_String_Contents);

	if (*n)
	{
		stccpy(t, data->path, sizeof(t));
		if (AddPart(t, n, sizeof(t)))
		{
			do_action(NULL, TA_File_Makelink,
				TT_File_Makelink_Object, data->from,
				TT_File_Makelink_Link, t,
			TAG_DONE);
		}
	}

	DoMethod(obj, MM_Makelinkwin_Close);

	return (0);
}


DEFTMETHOD(Makelinkwin_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	return (0);
}


BEGINMTABLE
DECNEW
DECGET
DECTMETHOD(Makelinkwin_Makelink)
DECTMETHOD(Makelinkwin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, makelinkwinclass)
