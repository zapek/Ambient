/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2007-2015 Ambient Open Source Team
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
 * $Id: patternrenamewinclass.c,v 1.5 2025/09/09 12:46:46 jacadcaps Exp $
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

#define RENAME_CANCEL  0
#define RENAME_PROCEED 1
#define RENAME_SKIP    2

struct Data {
	APTR str_src;
	APTR str_dst;
	TEXT path[PATH_SIZE];
	ULONG filetype;
	ULONG noicon;
	ULONG icontype;
	APTR thread;
	ULONG accept;
	ULONG multiple_rename;
};


DEFNEW
{
	struct Data *data;
	APTR str_src, str_dst, bt_rename, bt_cancel, bt_skip, lab_src;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI( MSG_PATTERNRENAMEWINCLASS_WINDOWTITLE ),
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

			Child, lab_src = MUICreateLabel( MSG_PATTERNRENAMEWINCLASS_OLDNAME, MUIO_Label_Centered ),

			Child, str_src = StringObject,
				StringFrame,
				MUIA_String_Reject, "/:", /* XXX: hm.. */
				MUIA_String_MaxLen, NAME_SIZE,
				MUIA_CycleChain, 1,
				MUIA_ControlChar, MUIGetUnderScore( MSG_PATTERNRENAMEWINCLASS_OLDNAME ),
				MUIA_ShortHelp, GSI( MSG_PATTERNRENAMEWINCLASS_OLDNAME_HELP ),
				MUIA_ShowMe, FALSE,
			End,

			Child, MUICreateLabel( MSG_PATTERNRENAMEWINCLASS_NEWNAME, MUIO_Label_Centered ),
			Child, str_dst = StringObject,
				StringFrame,
				MUIA_String_Reject, "/:", /* XXX: hm.. */
				MUIA_String_MaxLen, NAME_SIZE,
				MUIA_CycleChain, 1,
				MUIA_ControlChar, MUIGetUnderScore( MSG_PATTERNRENAMEWINCLASS_NEWNAME ),
				MUIA_ShortHelp, GSI( MSG_PATTERNRENAMEWINCLASS_NEWNAME_HELP ),
			End,

			Child, HGroup,
				MUIA_Group_SameWidth, TRUE,
				Child, bt_rename = MUICreateButton( MSG_PATTERNRENAMEWINCLASS_RENAME, "PATTERNRENAME_RENAME"),
				Child, bt_skip   = MUICreateButton( MSG_PATTERNRENAMEWINCLASS_SKIP  , "PATTERNRENAME_SKIP"),
				Child, bt_cancel = MUICreateButton( MSG_PATTERNRENAMEWINCLASS_CANCEL, "PATTERNRENAME_CANCEL"),
			End,
		End,
	End;

	if (!obj)
	{
		return ((ULONG) NULL);
	}

	data = INST_DATA(cl, obj);
	data->str_src = str_src;
	data->str_dst = str_dst;
	data->thread = NULL;
	data->accept = FALSE;
	data->multiple_rename = FALSE;

	FORTAG(INITTAGS)
	{
		case MA_PatternRenamewin_PathList:
			data->multiple_rename = TRUE;
			break;

		case MA_PatternRenamewin_Path:
			{
				STRPTR p = (STRPTR)tag->ti_Data;
		
				if (*p)
				{
					strcpy(data->path, p);

					if ((tag = FindTagItem(MA_PatternRenamewin_OldName, INITTAGS)))
					{
						p = (STRPTR)tag->ti_Data;

						if (*p)
						{
							SetAttrs(str_src,
								MUIA_String_Contents, p,
								MUIA_String_BufferPos, strlen(p),
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
			
					if ((tag = FindTagItem(MA_PatternRenamewin_NewName, INITTAGS)))
					{
						p = (STRPTR)tag->ti_Data;

						if (*p)
						{
							SetAttrs(str_dst,
								MUIA_String_Contents, p,
								MUIA_String_BufferPos, strlen(p),
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

		case MA_PatternRenamewin_FileType:
			data->filetype = tag->ti_Data;
			break;

		case MA_PatternRenamewin_NoIcon:
			data->noicon = tag->ti_Data;
			break;

		case MA_PatternRenamewin_IconType:
			data->icontype = tag->ti_Data;
			break;

		case MA_PatternRenamewin_Thread:
			data->thread = (APTR)tag->ti_Data;
			break;
	}
	NEXTTAG

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_PatternRenamewin_Accept, RENAME_CANCEL
	);

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_PatternRenamewin_Accept, RENAME_CANCEL
	);

	DoMethod(bt_skip, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_PatternRenamewin_Accept, RENAME_SKIP
	);

	DoMethod(bt_rename, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_PatternRenamewin_Accept, RENAME_PROCEED
	);

	DoMethod(data->str_dst, MUIM_Notify, MUIA_String_Acknowledge, MUIV_EveryTime,
		obj, 2, MM_PatternRenamewin_Accept, RENAME_PROCEED
	);

	set(lab_src, MUIA_ShowMe, FALSE);

	if(data->multiple_rename)
	{
		set(lab_src, MUIA_ShowMe, TRUE);
		set(str_src, MUIA_ShowMe, TRUE);
	}

	set(obj, MUIA_Window_ActiveObject, data->str_dst);

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

		case MA_Window_Path: /* hm hm */
			*msg->opg_Storage = (ULONG)data->path;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_PatternRename;
			return (TRUE);

		case MA_PatternRenamewin_OldName:
			{
				if(data->multiple_rename)
				{
					STRPTR path;
					path = data->accept == RENAME_PROCEED ? (STRPTR)getv( data->str_src, MUIA_String_Contents ) : NULL;
					*msg->opg_Storage = (ULONG) path;
				}
				else
				{
					*msg->opg_Storage = (ULONG) NULL;
				}
			}
			return (TRUE);

		case MA_PatternRenamewin_NewName:
			{
				STRPTR path;
				path = data->accept == RENAME_PROCEED ? (STRPTR)getv( data->str_dst, MUIA_String_Contents ) : NULL;
				*msg->opg_Storage = (ULONG) path;
			}
			return (TRUE);

		case MA_PatternRenamewin_Result:
			*msg->opg_Storage = data->accept;
			return (TRUE);
	}
	return (DOSUPER);
}

DEFSMETHOD(PatternRenamewin_Accept)
{
	GETDATA;

	data->accept = msg->accept;

	thread_signal( data->thread, FALSE );

	return TRUE;
}

BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(PatternRenamewin_Accept)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, patternrenamewinclass)
