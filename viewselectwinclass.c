/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006 Vladimir Alaev
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
 * $Id: viewselectwinclass.c,v 1.3 2013/10/28 10:36:08 geit Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "screen.h"
#include "threads.h"
#include "actiondispatcherclass.h"
#include "methodstack.h"

struct ViewEntry
{
	STRPTR path;
	ULONG id;
};

struct Data {
	ULONG sel_left;
	ULONG sel_top;
	ULONG sel_width;
	ULONG sel_height;
};

static void doset(APTR obj, struct Data *data, struct TagItem *tags);

DEFNEW
{
	struct Data *data;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen, get_screen(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI(MSG_PREFSWIN_TOOLBAR_GETFROMWINDOW_TITLE),
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Moused,
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_NoMenus, TRUE,
		MUIA_Window_CloseGadget, TRUE,
		WindowContents, VGroup,
							Child,TextObject, MUIA_Text_Contents, GSI(MSG_PREFSWIN_TOOLBAR_GETFROMWINDOW_TEXT),
								MUIA_Text_SetMin, FALSE, End,
							Child, VSpace(0),
						End,
	End;

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);
	doset(obj, data, INITTAGS);

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		obj, 2, MM_ViewSelector_Accept, TRUE
	);

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

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_ViewSelector;
			return (TRUE);

		case MA_ViewSelector_Left:
			*msg->opg_Storage = data->sel_left;
			return (TRUE);

		case MA_ViewSelector_Top:
			*msg->opg_Storage = data->sel_top;
			return (TRUE);

		case MA_ViewSelector_Width:
			*msg->opg_Storage = data->sel_width;
			return (TRUE);

		case MA_ViewSelector_Height:
			*msg->opg_Storage = data->sel_height;
			return (TRUE);

	}
	return (DOSUPER);
}

DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS);

	return (DOSUPER);
}


static void doset(APTR obj, struct Data *data, struct TagItem *tags)
{
	FORTAG(tags)
	{
		case MA_ViewSelector_Left:
			data->sel_left = tag->ti_Data;
			break;

		case MA_ViewSelector_Top:
			data->sel_top = tag->ti_Data;
			break;

		case MA_ViewSelector_Width:
			data->sel_width = tag->ti_Data;
			break;

		case MA_ViewSelector_Height:
			data->sel_height = tag->ti_Data;
			break;

		case MUIA_Window_Open:
			SetAttrs(obj,
				MUIA_Window_LeftEdge, data->sel_left,
				MUIA_Window_TopEdge, data->sel_top,
				MUIA_Window_Width, data->sel_width,
				MUIA_Window_Height, data->sel_height,
			TAG_DONE);
			break;

	}
	NEXTTAG
}


DEFSMETHOD(ViewSelector_Accept)
{
	GETDATA;

	data->sel_width  = getv(obj, MUIA_Window_Width);
	data->sel_height = getv(obj, MUIA_Window_Height);
	data->sel_left   = getv(obj, MUIA_Window_LeftEdge);
	data->sel_top    = getv(obj, MUIA_Window_TopEdge);

	set(obj, MUIA_Window_Open, FALSE);

	return TRUE;
}


BEGINMTABLE
DECNEW
DECGET
DECSET
DECSMETHOD(ViewSelector_Accept)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, viewselectwinclass)
