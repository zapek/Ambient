/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: addbookmarkwinclass.c,v 1.4 2025/09/09 12:46:45 jacadcaps Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "methodstack.h"
#include "mui_func.h"
#include "screen.h"
#include "threads.h"
#include "bookmarks.h"
#include "mimeuri.h"
#include "mimetype.h"
#include "name.h"

struct Data {
	STRPTR uri;
	STRPTR name;
	LONG permanent;
	LONG viewid;

	APTR str_name;
	APTR txt_uri;
};


DEFNEW
{
	struct Data *data;

	APTR str_name, txt_uri, bt_ok, bt_cancel;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI(MSG_ADDBOOKMARKWINCLASS_WINDOWTITLE),
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
				MUIA_Text_Contents, GSI(MSG_ADDBOOKMARKWINCLASS_ENTERBOOKMARKNAME),
			End,

			Child, txt_uri = TextObject,
				MUIA_Text_Contents, "",
				MUIA_Text_Copy, TRUE,
				MUIA_Text_PreParse, "\033c",
			End,

			Child, str_name = StringObject,
				StringFrame,
				MUIA_String_Contents, GSI(MSG_BOOKMARK_UNNAMED),
				MUIA_String_Reject, "/:", /* XXX: hm.. */
				MUIA_String_MaxLen, NAME_SIZE,
				MUIA_CycleChain, 1,
			End,

			Child, HGroup,
				MUIA_Group_SameWidth, TRUE,
				Child, bt_ok     = MUICreateButton( MSG_ADDBOOKMARKWINCLASS_OK, NULL ),
				Child, bt_cancel = MUICreateButton( MSG_ADDBOOKMARKWINCLASS_CANCEL, NULL ),
			End,
		End,
	End;

	if (obj == NULL)
	{
		return( (ULONG) NULL);
	}

	data = INST_DATA(cl, obj);
	data->str_name = str_name;
	data->txt_uri = txt_uri;
	data->permanent = FALSE;

	FORTAG(INITTAGS)
	{
		case MA_AddBookmarkwin_Uri:
		{
			STRPTR uri = (STRPTR)tag->ti_Data;
			if (uri != NULL && *uri)
			{
				STRPTR path = uri;
				APTR mimectx;

				if ((mimectx = mimeuri_create()))
				{
					if (mimeuri_gather(mimectx, uri,	
						MIMEURIGATHERTAG_FileIO, FALSE,
						MIMEURIGATHERTAG_Extension, FALSE,
						MIMEURIGATHERTAG_Protocol, FALSE,
						TAG_DONE))
					{
						path = mimeuri_getattr(mimectx, MIMEURIATTR_PATH);
					}
				}

				set(txt_uri, MUIA_Text_Contents, path);
				data->uri = name_build(uri);

				if (mimectx != NULL)
					mimeuri_delete(mimectx);
			}
		}
		break;

		case MA_AddBookmarkwin_Name:
		{
			STRPTR p = (STRPTR)tag->ti_Data;
			if (p != NULL && *p)
			{
				set(str_name, MUIA_String_Contents, p);
			}
		}
		break;

		case MA_AddBookmarkwin_Permanent:
			data->permanent = tag->ti_Data;
			break;

		case MA_Viewgroup_ID:
			data->viewid = tag->ti_Data;
			break;

	}
	NEXTTAG

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_AddBookmarkwin_Close
	);

	DoMethod(bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_AddBookmarkwin_Add
	);

	DoMethod(data->str_name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 1, MM_AddBookmarkwin_Add
	);

	set(obj, MUIA_Window_ActiveObject, data->str_name);
	
	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	if (data->uri != NULL)
		name_delete(data->uri);

	return (DOSUPER);
}

DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_AddBookmark;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFTMETHOD(AddBookmarkwin_Add)
{
	GETDATA;

	STRPTR n = (STRPTR)getv(data->str_name, MUIA_String_Contents);

	if (*n)
	{
		bookmarks_adduri(data->uri, (STRPTR)getv(data->str_name, MUIA_String_Contents), data->permanent);
		DoMethod(obj, MM_AddBookmarkwin_Close);
	}

	return (0);
}


DEFTMETHOD(AddBookmarkwin_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	return (0);
}


BEGINMTABLE
DECNEW
DECDISP
DECGET
DECTMETHOD(AddBookmarkwin_Add)
DECTMETHOD(AddBookmarkwin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, addbookmarkwinclass)
