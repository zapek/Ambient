/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2015 Ambient Open Source Team
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
 * $Id: destselectwinclass.c,v 1.6 2015/08/11 14:54:38 itix Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>
#include <libraries/asl.h>


/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "screen.h"
#include "threads.h"
#include "actiondispatcherclass.h"
#include "methodstack.h"


struct Data {
	APTR gr_pages;
	APTR lst_views;
	APTR lv_views;
	APTR bt_select;
	APTR bt_cancel;
	APTR str_path;
	APTR thread;
	STRPTR path;
	ULONG accept;
};

static LONG activepage;

DEFNEW
{
	struct Data *data;
	APTR lst_views, lv_views, bt_select, bt_cancel, str_path, gr_pages;
	APTR refwin = NULL, thread = NULL;

	static const CONST_STRPTR pages_all[] = {"Directory","Windows",NULL};

	FORTAG(INITTAGS)
	{
		case MA_DestSelector_Thread:
			thread = (APTR)tag->ti_Data;
			break;
		case MA_DestSelector_RefWin:
			refwin = (APTR)tag->ti_Data;
			break;
	}
	NEXTTAG

	if (activepage == 1)
	{
		LONG viewcount = 0;

		FORCHILD(app, MUIA_Application_WindowList)
		{
			if (child != refwin)
			{
				STRPTR p = (STRPTR)getv(child, MA_Window_Path);
				if (getv(child, MA_Window_Type) == MV_Window_Type_View && p && *p)
				{
					viewcount++;
				}
			}
		}
		NEXTCHILD

		if (viewcount == 0)
			activepage = 0;
	}

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen, get_screen(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, "Ambient · Select destination",
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Moused,
		MUIA_Window_ID, MAKE_ID('D','S','E','L'),
		MUIA_Window_Width, 400,
		MUIA_Window_Height, 550,//MUIV_Window_Width_MinMax(10),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_NoMenus, TRUE,
		MUIA_Window_CloseGadget, FALSE,
		WindowContents, VGroup,
			Child, gr_pages = RegisterGroup(pages_all),
				MUIA_Register_Frame, TRUE,
				MUIA_Group_ActivePage, activepage,
				Child, str_path = MUI_NewObject("Filepanel.mui",
					ASLFR_TitleText, "Ambient · Select destination",
					ASLFR_DrawersOnly, TRUE,
					ASLFR_InitialDrawer, refwin ? (STRPTR) getv(refwin, MA_Window_Path) : (STRPTR) "RAM:",
				End,

				Child, lv_views = ListviewObject,
					MUIA_Listview_List, lst_views = ListObject,
						MUIA_List_ConstructHook, MUIV_List_ConstructHook_String,
						MUIA_List_DestructHook, MUIV_List_DestructHook_String,
						InputListFrame,
						End,
				End,

			End,

			Child, HGroup,
				Child, bt_select = button(MSG_SELECT, 0),
				Child, bt_cancel = button(MSG_CANCEL, 0),
			End,
		End,
	End;

	if (obj == NULL)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);
	data->gr_pages = gr_pages;
	data->lst_views = lst_views;
	data->lv_views = lv_views;
	data->path = NULL;
	data->str_path = str_path;
	data->thread = thread;
	data->accept = FALSE;

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_DestSelector_Accept, FALSE
	);

	DoMethod(bt_select, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_DestSelector_Accept, TRUE
	);

	DoMethod(lst_views, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		obj, 1, MM_DestSelector_SetPath
	);

	DoMethod(lst_views, MUIM_Notify, MUIA_List_DoubleClick, MUIV_EveryTime,
		obj, 2, MM_DestSelector_Accept, TRUE
	);

	/*
	 * Insert paths from all opened views into the list.
	 */

	FORCHILD(app, MUIA_Application_WindowList)
	{
		if (child != refwin)
		{
			STRPTR p = (STRPTR)getv(child, MA_Window_Path);

			if (getv(child, MA_Window_Type) == MV_Window_Type_View && p && *p)
			{
				DoMethod(lst_views, MUIM_List_InsertSingle, p, MUIV_List_Insert_Bottom);
			}
		}
	}
	NEXTCHILD

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

		case MA_DestSelector_Path:
			{
				STRPTR path;
				path = data->accept ? (STRPTR)getv(data->str_path, ASLFR_InitialDrawer) : NULL;
				*msg->opg_Storage = (ULONG)path;
			}
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = (ULONG)NULL;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_DestSelector;
			return (TRUE);
	}
	return (DOSUPER);
}

DEFTMETHOD(DestSelector_SetPath)
{
	STRPTR path;
	GETDATA;

	DoMethod(data->lv_views, MUIM_List_GetEntry, MUIV_List_GetEntry_Active, &path);

	if (path != NULL)
	{
		set(data->str_path, MUIA_String_Contents, path);
	}

	return TRUE;
}

DEFSMETHOD(DestSelector_Accept)
{
	GETDATA;

	activepage = getv(data->gr_pages, MUIA_Group_ActivePage);
	data->accept = msg->accept;
	thread_signal(data->thread, FALSE);

	return TRUE;
}


BEGINMTABLE
DECNEW
DECGET
DECTMETHOD(DestSelector_SetPath)
DECSMETHOD(DestSelector_Accept)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, destselectwinclass)
