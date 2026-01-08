/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: prefswin_bookmarksclass.c,v 1.6 2016/12/25 22:00:57 geit Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/asl.h>

/* private */
#include "mui_func.h"
#include "prefswin.h"
#include "ambient_cat.h"
#include "prefsclone.h"
#include "bookmarks.h"

struct Data {
	APTR lst_bookmarks;
	APTR str_name;
	APTR str_uri;
	APTR cyc_showonly;
};


DEFNEW
{
	struct Data *data;
	APTR lst_bookmarks, bt_add, bt_remove, str_name, str_uri, cyc_showonly;

	static STRPTR cycleshowonly[ 2 + MSG_PREFSWIN_BOOKMARKS_DISPLAYAS_LOCATIONS - MSG_PREFSWIN_BOOKMARKS_DISPLAYAS_NAMESANDLOCATIONS ];

	obj = DoSuperNew(cl, obj,
		Child, VGroup, GroupFrameT(GSI(MSG_PREFSWIN_BOOKMARKS_GROUP)),
			Child, lst_bookmarks = NewObject(getbookmarklistclass(), NULL,
									MUIA_List_DragSortable, TRUE,
									MUIA_List_DragType, 1,
									MUIA_List_ShowDropMarks, TRUE,
									MUIA_ShortHelp, GSI(MSG_PREFSWIN_BOOKMARKS_HELP),
									TAG_DONE),
			Child, HGroup,
				Child, bt_add    = MUICreateButton( MSG_PREFSWIN_BOOKMARKS_ADD    , NULL ),
				Child, bt_remove = MUICreateButton( MSG_PREFSWIN_BOOKMARKS_REMOVE , NULL ),
				End,
			Child, ColGroup(2),
				Child, NSLabel2( MSG_PREFSWIN_BOOKMARKS_NAME ),
				Child, str_name  = MUICreateString( MSG_PREFSWIN_BOOKMARKS_NAME, 0, ""),
				Child, NSLabel2( MSG_PREFSWIN_BOOKMARKS_LOCATION ),
				Child, PopaslObject,
					MUIA_Popasl_Type, ASL_FileRequest,
					MUIA_Popstring_String, str_uri = MUICreateString( MSG_PREFSWIN_BOOKMARKS_LOCATION, 255, ""),
					MUIA_Popstring_Button, MUICreatePopButton( MSG_PREFSWIN_BOOKMARKS_LOCATION, MUII_PopFile, NULL ),
					ASLFR_TitleText, GSI(MSG_BOOKMARKS_ADD),
					End,
				End,
			End,
			Child, HGroup, GroupFrameT(GSI(MSG_PREFSWIN_BOOKMARKS_DISPLAY_GROUP)),
				Child, MUICreateLabel(MSG_PREFSWIN_BOOKMARKS_DISPLAYAS, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned),
				Child, cyc_showonly = MUICreateCycle( MSG_PREFSWIN_BOOKMARKS_DISPLAYAS, cycleshowonly, MSG_PREFSWIN_BOOKMARKS_DISPLAYAS_NAMESANDLOCATIONS, MSG_PREFSWIN_BOOKMARKS_DISPLAYAS_LOCATIONS, ""),
				Child, HSpace(0),
			End,
		End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->lst_bookmarks = lst_bookmarks;
	data->str_uri = str_uri;
	data->str_name = str_name;
	data->cyc_showonly = cyc_showonly;

	DoMethod(lst_bookmarks, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		obj, 1, MM_Prefswin_Bookmarks_Selected);

	DoMethod(str_uri, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 1, MM_Prefswin_Bookmarks_Modified);

	DoMethod(str_name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 1, MM_Prefswin_Bookmarks_Modified);

	DoMethod(bt_remove, MUIM_Notify, MUIA_Pressed, FALSE,
		lst_bookmarks, 2, MUIM_List_Remove, MUIV_List_Remove_Active);

	DoMethod(bt_add, MUIM_Notify, MUIA_Pressed, FALSE,
		lst_bookmarks, 3, MUIM_List_InsertSingle, NULL, MUIV_List_Insert_Bottom);

	DoMethod(bt_add, MUIM_Notify, MUIA_Pressed, FALSE,
		lst_bookmarks, 3, MUIM_Set, MUIA_List_Active, MUIV_List_Active_Bottom);


	setupprefs(cyc_showonly, MUIA_Cycle_Active, getprefslong(DSI_BOOKMARKS_SHOWONLY));

	return ((ULONG)obj);
}


DEFTMETHOD(Prefswin_Store)
{
	GETDATA;

	setprefslong(DSI_BOOKMARKS_SHOWONLY, getv(data->cyc_showonly, MUIA_Cycle_Active));
	return (0);
}


DEFDISPOSE
{
	GETDATA;
	
	bookmarks_updatefromlist(data->lst_bookmarks, TRUE);

	return (DOSUPER);
}

DEFTMETHOD(Prefswin_Bookmarks_Selected)
{
	GETDATA;
	
    LONG active = getv(data->lst_bookmarks, MUIA_List_Active);

	if (active != MUIV_List_Active_Off)
	{
		struct bookmarkitem *item = NULL;
		DoMethod(data->lst_bookmarks, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &item);

		set(data->str_uri, MUIA_String_Contents, item->location);
		set(data->str_name, MUIA_String_Contents, item->description);
	}
	else
	{
	}

	return 0;
}

DEFTMETHOD(Prefswin_Bookmarks_Modified)
{
	GETDATA;

	LONG active = getv(data->lst_bookmarks, MUIA_List_Active);

	if (active != MUIV_List_Active_Off)
	{
		struct bookmarkitem item;

		set(data->lst_bookmarks, MUIA_List_Quiet, TRUE);

		item.location = (STRPTR)getv(data->str_uri, MUIA_String_Contents);
		item.description = (STRPTR)getv(data->str_name, MUIA_String_Contents);

		DoMethod(data->lst_bookmarks, MUIM_List_InsertSingle, &item, active);
		DoMethod(data->lst_bookmarks, MUIM_List_Remove, active + 1);

		set(data->lst_bookmarks, MUIA_List_Active, active);
		set(data->lst_bookmarks, MUIA_List_Quiet, FALSE);

	}

	return 0;
}


BEGINMTABLE
DECNEW
DECTMETHOD(Prefswin_Store)
DECTMETHOD(Prefswin_Bookmarks_Selected)
DECTMETHOD(Prefswin_Bookmarks_Modified)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_bookmarksclass)

