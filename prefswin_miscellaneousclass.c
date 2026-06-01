/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
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
 * $Id: prefswin_miscellaneousclass.c,v 1.15 2026/04/25 23:05:00 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/asl.h>
#include <exec/resident.h>
#include <exec/types.h>
#include <proto/dos.h>

/* private */
#include "deficon_getpath.h"
#include "deficonpool.h"
#include "ambient_cat.h"
#include "mui_func.h"
#include "name.h"
#include "prefswin.h"
#include "prefsclone.h"
#include "prefswin_miscellaneousclass.h"

struct Data {
	APTR str_wbpath;
	APTR str_defpath;
	APTR bt_remwins;
	APTR bt_remdocs;

	APTR bt_mymorphos;
	APTR bt_desktopdoubleclick;
	APTR bt_smartfileoperations;
	APTR bt_trapmore;
	APTR bt_trapmultiview;
	APTR bt_contextmenuimages;
	APTR bt_createiconfornewdrawer;
	APTR bt_hidedotfilenames;
};

TEXT buf[PATH_SIZE];
TEXT oldbuf[PATH_SIZE];
static BOOL newdefdir = FALSE;

void prefswin_misc_setdeficonpath(int action)
{
	LONG retcode = 1;

	/* XXX: Currently we ignore the return value of SetVar(), question is if
	 * its better to continue quietly or notify the user... (how much can
	 * the user do about it?)
	 */
	switch (action)
	{
		static BOOL testing = FALSE;

		case MV_Prefswin_Main_Close_Save:
			retcode = SetVar("DefIcon_Path", buf, -1, GVF_GLOBAL_ONLY | GVF_SAVE_VAR);
			break;

		case MV_Prefswin_Main_Close_Test:
			if (newdefdir)
			{
				testing = TRUE;
				retcode = SetVar("DefIcon_Path", buf, -1, GVF_GLOBAL_ONLY);
			}
			break;

		case MV_Prefswin_Main_Close_Use:
			strcpy(oldbuf, buf);
			retcode = SetVar("DefIcon_Path", buf, -1, GVF_GLOBAL_ONLY);
			break;

		case MV_Prefswin_Main_Close_Cancel:
			if (testing)
			{
				newdefdir = TRUE;
				retcode = SetVar("DefIcon_Path", oldbuf, -1, GVF_GLOBAL_ONLY);
			}
			else
			{
				newdefdir = FALSE;
			}
			break;
	}

	#ifdef DEBUG
	if (!retcode)
	{
		PDB(("Failed to set DefIcon_Path environment variable\n"));
	}
	#endif

	if (newdefdir && retcode)
	{
		newdefdir = FALSE;
		DoMethod(app, MM_Application_ReloadIcons, FALSE);

		/* flush cache as old deficons shouldn't be referenced anymore */

		deficonpool_flush();
	}
}

DEFNEW
{
	struct Data *data;
	APTR bt_remdirs, bt_remdocs;
	APTR str_wbpath;
	APTR str_defpath, wb_str, def_str;
	APTR bt_mymorphos;
	APTR bt_desktopdoubleclick;
	APTR bt_smartfileoperations;
	APTR bt_trapmore;
	APTR bt_trapmultiview;
	APTR bt_contextmenuimages;
	APTR bt_createiconfornewdrawer;
	APTR bt_hidedotfilenames;

	if (!deficon_getpath(buf, PATH_SIZE, NULL))
	{
		/* envvar is fucked, replace with something sane */
		strcpy(buf, "ENVARC:sys/");
	}

	obj = DoSuperNew(cl, obj,
		Child, ScrollgroupObject,
			MUIA_Scrollgroup_FreeVert, TRUE,
			MUIA_Scrollgroup_AutoBars, TRUE,
			MUIA_Scrollgroup_Contents,
			VirtgroupObject,
				Child, HGroup, End,
				Child, HGroup, GroupFrameT(GSI(MSG_PREFSWIN_WBSTARTUP)),
					Child, ColGroup(2),
						Child, MUICreateLabel(MSG_PREFSWIN_WBSTARTUP_PATH, MUIO_Label_SingleFrame),
						Child, str_wbpath = PopaslObject,
							MUIA_Popasl_Type, ASL_FileRequest,
							MUIA_Popstring_String, wb_str = pstring(DSI_MISC_WBSTARTUP_PATH, PATH_SIZE, GSI(MSG_PREFSWIN_WBSTARTUP_PATH)),
							MUIA_Popstring_Button, MUICreatePopButton(MSG_PREFSWIN_WBSTARTUP_PATH, MUII_PopDrawer, NULL),
							ASLFR_TitleText, GSI(MSG_PREFSWIN_WBSTARTUP_PATH_REQ),
							ASLFR_DrawersOnly, TRUE,
							MUIA_ShortHelp, GSI(MSG_PREFSWIN_WBSTARTUP_PATH_HELP),
						End,

						Child, MUICreateLabel(MSG_PREFSWIN_WBSTART_REMEMBER, MUIO_Label_SingleFrame),
						Child, HGroup,
							Child, bt_remdirs = MUICreateCheckbox(MSG_PREFSWIN_WBSTART_REMEMBERDIRECTORIES, FALSE, ""),
							Child, MUICreateLabel(MSG_PREFSWIN_WBSTART_REMEMBERDIRECTORIES, MUIO_Label_SingleFrame),
							Child, RectangleObject, End,
						End,
						Child, HSpace(1),
						Child, HGroup,
							Child, bt_remdocs = MUICreateCheckbox(MSG_PREFSWIN_WBSTART_REMEMBERDOCUMENTS, FALSE, ""),
							Child, MUICreateLabel(MSG_PREFSWIN_WBSTART_REMEMBERDOCUMENTS, MUIO_Label_SingleFrame),
							Child, RectangleObject, End,
						End,
					End,
				End,
				Child, HGroup, End,
				Child, HGroup, GroupFrameT(GSI(MSG_PREFSWIN_DEFICONS)),
					Child, ColGroup(2),
						Child, MUICreateLabel(MSG_PREFSWIN_DEFICONS_PATH, MUIO_Label_SingleFrame),
						Child, str_defpath = PopaslObject,
							MUIA_Popasl_Type, ASL_FileRequest,
							MUIA_Popstring_String, def_str = String(buf, PATH_SIZE),
							MUIA_Popstring_Button, MUICreatePopButton(MSG_PREFSWIN_DEFICONS_PATH, MUII_PopDrawer, NULL),
							ASLFR_TitleText, GSI(MSG_PREFSWIN_DEFICONS_PATH_REQ),
							ASLFR_DrawersOnly, TRUE,
							MUIA_ShortHelp, GSI(MSG_PREFSWIN_DEFICONS_PATH_HELP),
						End,
					End,
				End,
				Child, HGroup, End,
				Child, HGroup, GroupFrameT( GSI(MSG_PREFSWIN_MISCELLANEOUS)),
					Child, ColGroup(2),
						Child, bt_mymorphos = MUICreateCheckbox( MSG_PREFSWIN_MISCELLANEOUS_DISPLAYMYMORPHOS, TRUE, "PREF_MISC_DISPLAYMYMORPHOS"),
						Child, MUICreateLabel( MSG_PREFSWIN_MISCELLANEOUS_DISPLAYMYMORPHOS, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, bt_desktopdoubleclick = MUICreateCheckbox( MSG_PREFSWIN_MISCELLANEOUS_DOUBLECLICKMYMORPHOS, TRUE, "PREF_MISC_DOUBLECLICKMYMORPHOS"),
						Child, MUICreateLabel( MSG_PREFSWIN_MISCELLANEOUS_DOUBLECLICKMYMORPHOS, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, bt_smartfileoperations = MUICreateCheckbox( MSG_PREFSWIN_MISCELLANEOUS_DRAGDROPMOVE, TRUE, "PREF_MISC_DRAGDROPMOVE"),
						Child, MUICreateLabel( MSG_PREFSWIN_MISCELLANEOUS_DRAGDROPMOVE, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, bt_trapmore = MUICreateCheckbox( MSG_PREFSWIN_MISCELLANEOUS_OVERRIDEMOREDEFAULTTOOL, TRUE, "PREF_MISC_OVERRIDEMOREDEFAULTTOOL"),
						Child, MUICreateLabel( MSG_PREFSWIN_MISCELLANEOUS_OVERRIDEMOREDEFAULTTOOL, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, bt_trapmultiview = MUICreateCheckbox( MSG_PREFSWIN_MISCELLANEOUS_OVERRIDEMULTIVIEWDEFAULTTOOL, TRUE, "PREF_MISC_OVERRIDEMULTIVIEWDEFAULTTOOL"),
						Child, MUICreateLabel( MSG_PREFSWIN_MISCELLANEOUS_OVERRIDEMULTIVIEWDEFAULTTOOL, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, bt_contextmenuimages = MUICreateCheckbox( MSG_PREFSWIN_MISCELLANEOUS_SHOWCONTEXTIMAGES, TRUE, "PREF_MISC_SHOWCONTEXTIMAGES"),
						Child, MUICreateLabel( MSG_PREFSWIN_MISCELLANEOUS_SHOWCONTEXTIMAGES, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, bt_createiconfornewdrawer = MUICreateCheckbox( MSG_PREFSWIN_MISCELLANEOUS_CREATEICONFORNEWDRAWER, TRUE, "PREF_MISC_CREATEICONFORNEWDRAWER"),
						Child, MUICreateLabel( MSG_PREFSWIN_MISCELLANEOUS_CREATEICONFORNEWDRAWER, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
						Child, bt_hidedotfilenames = MUICreateCheckbox( MSG_PREFSWIN_MISCELLANEOUS_HIDEDOTFILENAMES, TRUE, "PREF_MISC_HIDEDOTFILENAMES"),
						Child, MUICreateLabel( MSG_PREFSWIN_MISCELLANEOUS_HIDEDOTFILENAMES, MUIO_Label_SingleFrame | MUIO_Label_LeftAligned ),
					End,
					Child, HSpace(0),
				End,
				Child, HGroup, End,
			End,
		End,
	End;

	if (obj == NULL)
		return (0);

	data = INST_DATA(cl, obj);
	data->str_wbpath  = str_wbpath;
	data->str_defpath = str_defpath;
	data->bt_remwins = bt_remdirs;
	data->bt_remdocs = bt_remdocs;

	data->bt_mymorphos = bt_mymorphos;
	data->bt_desktopdoubleclick = bt_desktopdoubleclick;
	data->bt_smartfileoperations = bt_smartfileoperations;
	data->bt_trapmore = bt_trapmore;
	data->bt_trapmultiview = bt_trapmultiview;
	data->bt_contextmenuimages = bt_contextmenuimages;
	data->bt_createiconfornewdrawer = bt_createiconfornewdrawer;
	data->bt_hidedotfilenames = bt_hidedotfilenames;

	SetAttrs( def_str, MUIA_CycleChain, TRUE, MUIA_ControlChar, MUIGetUnderScore( MSG_PREFSWIN_DEFICONS_PATH ), TAG_DONE);

	strcpy(oldbuf, buf);

	setupprefs_noset(wb_str,  MUIA_String_Acknowledge, MUIV_EveryTime);
	setupprefs_noset(def_str, MUIA_String_Acknowledge, MUIV_EveryTime);
	setupprefs(bt_remdirs, MUIA_Selected, getprefslong(DSI_MISC_REMEMBER_WINDOWS));
	setupprefs(bt_remdocs, MUIA_Selected, getprefslong(DSI_MISC_REMEMBER_DOCUMENTS));
	
	setupprefs(bt_mymorphos, MUIA_Selected, getprefslong(DSI_MISC_MYMORPHOSICON));
	setupprefs(bt_desktopdoubleclick, MUIA_Selected, getprefslong(DSI_MISC_DESKTOPDOUBLECLICK));
	setupprefs(bt_smartfileoperations, MUIA_Selected, getprefslong(DSI_MISC_SMARTFILEOPERATIONS));
	setupprefs(bt_trapmore, MUIA_Selected, getprefslong(DSI_MISC_TRAPMORE));
	setupprefs(bt_trapmultiview, MUIA_Selected, getprefslong(DSI_MISC_TRAPMULTIVIEW));
	setupprefs(bt_contextmenuimages, MUIA_Selected, getprefslong(DSI_MISC_CONTEXTMENUIMAGES));
	setupprefs(bt_createiconfornewdrawer, MUIA_Selected, getprefslong(DSI_MISC_CREATEICONFORNEWDRAWER));
	setupprefs(bt_hidedotfilenames, MUIA_Selected, gprefs->hide_dot_filenames);

	DoMethod(str_defpath, MUIM_Notify, MUIA_String_Contents, MUIV_EveryTime,
	         obj, 1, MM_Prefswin_Misc_SetDefIconPath);

	return ((ULONG)obj);
}


DEFTMETHOD(Prefswin_Store)
{
	GETDATA;

	storepath(data->str_wbpath, DSI_MISC_WBSTARTUP_PATH);
	setprefslong(DSI_MISC_REMEMBER_WINDOWS, getv(data->bt_remwins, MUIA_Selected));
	setprefslong(DSI_MISC_REMEMBER_DOCUMENTS, getv(data->bt_remdocs, MUIA_Selected));

	setprefslong(DSI_MISC_MYMORPHOSICON, getv(data->bt_mymorphos, MUIA_Selected));
	setprefslong(DSI_MISC_DESKTOPDOUBLECLICK, getv(data->bt_desktopdoubleclick, MUIA_Selected));
	setprefslong(DSI_MISC_SMARTFILEOPERATIONS, getv(data->bt_smartfileoperations, MUIA_Selected));
	setprefslong(DSI_MISC_TRAPMORE, getv(data->bt_trapmore, MUIA_Selected));
	setprefslong(DSI_MISC_TRAPMULTIVIEW, getv(data->bt_trapmultiview, MUIA_Selected));
	setprefslong(DSI_MISC_CONTEXTMENUIMAGES, getv(data->bt_contextmenuimages, MUIA_Selected));
	setprefslong(DSI_MISC_CREATEICONFORNEWDRAWER, getv(data->bt_createiconfornewdrawer, MUIA_Selected));

	setprefslong(DSI_MISC_HIDEDOTFILENAMES, getv(data->bt_hidedotfilenames, MUIA_Selected));
	gprefs->hide_dot_filenames = getv(data->bt_hidedotfilenames, MUIA_Selected);

	return (0);
}

DEFTMETHOD(Prefswin_Misc_SetDefIconPath)
{
	GETDATA;

	APTR   o = (APTR) getv( data->str_defpath, MUIA_Popstring_String );
	STRPTR c = (STRPTR) getv( o, MUIA_String_Contents );

	if( ( c = name_build_sysify( c ) ) )
	{
		stccpy( buf, c, sizeof( buf ) );
		free( c );
		newdefdir = TRUE;
	}
	return( 0 );
}


DEFDISPOSE
{
	DoMethod(obj, MM_Prefswin_Store);

	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECTMETHOD(Prefswin_Store)
DECTMETHOD(Prefswin_Misc_SetDefIconPath)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_miscellaneousclass)
