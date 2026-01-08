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
 * $Id: selectwinclass.c,v 1.7 2015/12/21 13:15:04 geit Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "screen.h"
#include "threads.h"
#include "methodstack.h"
#include "storage.h"


struct Data {
	APTR str_pattern;
	TEXT pattern[PATH_SIZE];
	ULONG viewid;
};


DEFNEW
{
	struct Data *data;
	APTR str_pattern, bt_ok, bt_cancel;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen, get_screen(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI( MSG_SELECTWINCLASS_WINDOWTITLE ),
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Moused,
		MUIA_Window_Width, MUIV_Window_Width_MinMax(10),
		MUIA_Window_Height, MUIV_Window_Width_MinMax(10),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_NoMenus, TRUE,
		MUIA_Window_CloseGadget, TRUE,
		WindowContents, VGroup,
			Child, TextObject,
				TextFrame,
				//MUIA_Background, MUII_TextBack,
				MUIA_Text_Contents, GSI( MSG_SELECTWINCLASS_ENTERSELECTIONPATTERN ),
			End,

			Child, str_pattern = StringObject,
				StringFrame,
				MUIA_String_Reject, "/:",
				MUIA_String_MaxLen, PATH_SIZE,
				MUIA_CycleChain, 1,
			End,

			Child, HGroup,
				Child, bt_ok     = MUICreateButton( MSG_SELECTWINCLASS_OK, NULL ),
				Child, bt_cancel = MUICreateButton( MSG_SELECTWINCLASS_CANCEL, NULL ),
			End,
		End,
	End;

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);
	data->str_pattern = str_pattern;
	data->pattern[0] = '\0';

	FORTAG(INITTAGS)
	{
		case MA_Selectwin_Pattern:
		{
			STRPTR p = (STRPTR)tag->ti_Data;
			
			if (*p)
			{
				stccpy(data->pattern, p, sizeof(data->pattern));
			}
		}
		break;

		case MA_Viewgroup_ID:
		{
			data->viewid = tag->ti_Data;
		}
		break;
	}
	NEXTTAG

	DoMethod( obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		obj, 1, MM_Selectwin_Close
	);

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Selectwin_Close
	);

	DoMethod(bt_ok, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Selectwin_Select
	);

	DoMethod(data->str_pattern, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 1, MM_Selectwin_Select
	);

	set(obj, MUIA_Window_ActiveObject, data->str_pattern);

	if(data->pattern[0] == '\0')
	{
		char * ptr;
		storage_get(STORAGE_SELECTION_PATTERN, STORAGE_MSTRING, (APTR *)&ptr);

		if(ptr)
		{
			stccpy(data->pattern, ptr, sizeof(data->pattern));
		}
		else
		{
			stccpy(data->pattern, "*", sizeof(data->pattern));
		}
	}

	set(data->str_pattern, MUIA_String_Contents, data->pattern);

	return ((ULONG)obj);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Select;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFTMETHOD(Selectwin_Select)
{
	GETDATA;

	STRPTR n;

	n = (STRPTR)getv(data->str_pattern, MUIA_String_Contents);

	if (*n)
	{
		APTR wo = (APTR)methodstack_push_sync( app, 2, MM_Application_FindWindowByID, data->viewid);

		if ( wo )
		{
			APTR viewobj = NULL;
			methodstack_push_sync( wo, 3, OM_GET, MA_Window_Viewobj, &viewobj );

			if(viewobj)
			{
				DoMethod(viewobj, MM_View_Select, MV_View_Select_Pattern, n);
			}
		}

		storage_set(STORAGE_SELECTION_PATTERN, STORAGE_MSTRING, n);
	}

	DoMethod(obj, MM_Selectwin_Close);

	return (0);
}


DEFTMETHOD(Selectwin_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	return (0);
}


BEGINMTABLE
DECNEW
DECGET
DECTMETHOD(Selectwin_Select)
DECTMETHOD(Selectwin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, selectwinclass)
