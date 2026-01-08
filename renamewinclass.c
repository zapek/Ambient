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
 * $Id: renamewinclass.c,v 1.12 2017/08/11 23:32:05 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefs_desktop.h"
#include "screen.h"
#include "threads.h"


struct Data {
	APTR str_name;
	TEXT path[PATH_SIZE];
	ULONG filetype;
	ULONG noicon;
	ULONG icontype;
};


DEFNEW
{
	struct Data *data;
	APTR str_name, bt_rename, bt_cancel;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen, get_screen(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI(MSG_RENAME_WINDOW_TITLE),
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
			Child, MUICreateLabel( MSG_RENAME_NEW_NAME, MUIO_Label_Centered ),
			Child, str_name = StringObject,
				StringFrame,
				MUIA_String_Reject, "/:", /* XXX: hm.. */
				MUIA_String_MaxLen, NAME_SIZE,
				MUIA_CycleChain, 1,
				MUIA_ShortHelp, GSI(MSG_RENAME_NEW_NAME_HELP),
			End,

			Child, HGroup,
				MUIA_Group_SameWidth, TRUE,
				Child, bt_rename = MUICreateButton( MSG_RENAME_RENAME, "RENAME_RENAME"),
				Child, RectangleObject, End,
				Child, bt_cancel = MUICreateButton( MSG_RENAME_CANCEL, "RENAME_CANCEL"),
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
		case MA_Renamewin_Path:
			{
				STRPTR p = (STRPTR)tag->ti_Data;
		
				if (*p)
				{
					strcpy(data->path, p);
			
					if ((tag = FindTagItem(MA_Renamewin_Name, INITTAGS)))
					{
						p = (STRPTR)tag->ti_Data;

						if (*p)
						{
							STRPTR str;
							ULONG n;
							/* we search for the last '.' in file name and calculate number of chars to mark */

							if( (str = strrchr( p, '.' ) ))
							{
								n = ((STRPTR) str) - ((STRPTR) p) - 1;
							} else {
								n = strlen(p);
							}

							SetAttrs(str_name,
								MUIA_String_Contents, p,
								MUIA_Textinput_MarkStart, 0,
								MUIA_Textinput_MarkEnd, n,
								MUIA_Textinput_CursorPos, 0,
								MUIA_Textinput_ResetMarkOnCursor, TRUE,
								TAG_DONE
							);                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         

						}
						#ifdef DEBUG
						else
						{
							PDB(("ahe.. empty name tag?\n"));
						}
						#endif
					}
					#ifdef DEBUG
					else
					{
						PDB(("huh.. no name?\n"));
					}
					#endif
				}
			}
			break;

		case MA_Renamewin_FileType:
			data->filetype = tag->ti_Data;
			break;

		case MA_Renamewin_NoIcon:
			data->noicon = tag->ti_Data;
			break;

		case MA_Renamewin_IconType:
			data->icontype = tag->ti_Data;
			break;
	}
	NEXTTAG

	DoMethod( obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		obj, 1, MM_Renamewin_Close
	);

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Renamewin_Close
	);

	DoMethod(bt_rename, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 1, MM_Renamewin_Rename
	);

	DoMethod(data->str_name, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 1, MM_Renamewin_Rename
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
			*msg->opg_Storage = (ULONG)data->path;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Rename;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFTMETHOD(Renamewin_Rename)
{
	STRPTR n;
	GETDATA;

	if ((n = (STRPTR)getv(data->str_name, MUIA_String_Contents)))
	{
		if (data->icontype == MV_Icon_Type_MyComputer)
		{
			dprefs_mymorphos_name_set(n);
			do_action(NULL, TA_DesktopPrefs_Save, TAG_DONE);
		}
		else
		{
			if (*n)
			{
				do_action(NULL, TA_File_Rename,
					TT_File_Rename_Path, data->path,
					TT_File_Rename_Name, n,
					TT_File_Rename_NoIcon, data->noicon,
				TAG_DONE); /* XXX */
			}
		}
	}
	
	DoMethod(obj, MM_Renamewin_Close);

	return (0);
}


DEFTMETHOD(Renamewin_Close)
{
	DoMethod(_app(obj), MUIM_Application_PushMethod, app, 2, MM_Application_DisposeWindow, obj);
	return (0);
}


BEGINMTABLE
DECNEW
DECGET
DECTMETHOD(Renamewin_Rename)
DECTMETHOD(Renamewin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, renamewinclass)
