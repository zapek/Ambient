/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2018 Ambient Open Source Team
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
 * $Id: makedirwinclass.c,v 1.15 2018/07/26 15:19:46 itix Exp $
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
#include "prefs.h"

struct Data {
	APTR str_name;
	STRPTR path;
	ULONG pathlen;
};


DEFNEW
{
	struct Data *data;
	APTR str_name, bt_without, bt_with, bt_cancel;
	BOOL with_icon = _conf(misc_createiconfornewdrawer);
	int i;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI(MSG_MAKEDIR_WINDOWTITLE),
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
				MUIA_Text_Contents, GSI(MSG_MAKEDIR_ENTERDIRECTORYNAME),
			End,

			Child, str_name = StringObject,
				StringFrame,
				MUIA_String_Reject, ":",
				MUIA_String_MaxLen, 2048, /* Probably enough */
				MUIA_String_Contents, GSI(MSG_MAKEDIR_UNNAMED),
				MUIA_CycleChain, 1,
			End,

			Child, HGroup,
				MUIA_Group_SameWidth, TRUE,
				Child, bt_with    = MUICreateButton( MSG_MAKEDIR_WITHICON,    "MAKEDIR_WITHICON"    ), 
				Child, bt_without = MUICreateButton( MSG_MAKEDIR_WITHOUTICON, "MAKEDIR_WITHOUTICON" ),
				Child, bt_cancel  = MUICreateButton( MSG_MAKEDIR_CANCEL,      "MAKEDIR_CANCEL"      ),
			End,
		End,
	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->str_name = str_name;

	FORTAG(INITTAGS)
	{
		case MA_Makedirwin_Path:
		{
			STRPTR p = (STRPTR)tag->ti_Data;
			
			if (*p)
			{
				int len = strlen(p) + 1;
				STRPTR s = AllocMem(len, MEMF_ANY);

				if (s)
				{
					data->path = s;
					data->pathlen = len;
					stccpy(s, p, len);
				}
			}
		}
		break;

		case MA_Makedirwin_Icon:
			/*  only change status if explicitly set to FALSE with "NOICON" switch 
			 */ 
			if (tag->ti_Data == FALSE)
			{
				with_icon = FALSE;
			}
			break;
	}
	NEXTTAG

	DoMethod(obj, MUIM_Notify, MUIA_Window_InputEvent, "f1", obj, 2, MM_Makedirwin_Makedir, MV_Makedirwin_Makedir_With);
	DoMethod(obj, MUIM_Notify, MUIA_Window_InputEvent, "f2", obj, 2, MM_Makedirwin_Makedir, MV_Makedirwin_Makedir_Without);
	DoMethod(obj, MUIM_Notify, MUIA_Window_InputEvent, "esc", obj, 1, MM_Makedirwin_Close);

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Makedirwin_Close
	);

	DoMethod(bt_with, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Makedirwin_Makedir, MV_Makedirwin_Makedir_With
	);

	DoMethod(bt_without, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Makedirwin_Makedir, MV_Makedirwin_Makedir_Without
	);

	DoMethod(data->str_name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 2, MM_Makedirwin_Makedir, (with_icon ? MV_Makedirwin_Makedir_With : MV_Makedirwin_Makedir_Without) 
	);

	set(obj, MUIA_Window_ActiveObject, data->str_name);
	
	DoMethod(data->str_name, MUIM_Textinput_DoMarkAll);

	return ((ULONG)obj);
}


DEFDISPOSE
{
	GETDATA;

	if (data->path)
	{
		FreeMem(data->path, data->pathlen);
	}

	return DOSUPER;
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
			*msg->opg_Storage = MV_Window_Type_Makedir;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSMETHOD(Makedirwin_Makedir)
{
	GETDATA;

	STRPTR n = (STRPTR)getv(data->str_name, MUIA_String_Contents);

	if (*n)
	{
		do_action(NULL, TA_File_Makedir,
			TT_File_BaseDir, data->path,
			TT_File_Makedir_Path, n,
			TT_File_Makedir_Icon, (msg->mode == MV_Makedirwin_Makedir_With) ? TRUE : FALSE,
		TAG_DONE);
	}

	return DoMethod(obj, MM_Makedirwin_Close);
}


DEFTMETHOD(Makedirwin_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	return (0);
}


BEGINMTABLE
DECNEW
DECDISPOSE
DECGET
DECTMETHOD(Makedirwin_Makedir)
DECTMETHOD(Makedirwin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, makedirwinclass)
