/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: prefswin_mainclass.c,v 1.25 2026/03/01 15:06:20 kronos Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "legacy.h"
#include "methodstack.h"
#include "mui_func.h"
#include "name.h"
#include "prefsclone.h" /* XXX: needed ? */
#include "prefswin.h"
#include "screen.h"
#include "updatelist.h"
#include "prefslist_general_logo.h"
#include "prefslist_icondisplay_logo.h"
#include "actioneditor_menu_logo.h"
#include "prefslist_cli_logo.h"
#include "prefslist_dnd_logo.h"
#include "prefslist_lister_logo.h"
#include "prefslist_miscellaneous_logo.h"
#include "prefswin_miscellaneousclass.h"
#include "prefslist_window_logo.h"
#if USE_INTERNAL_PANELS
#include "prefslist_panels_logo.h"
#endif
#include "prefslist_mimetype_logo.h"
#include "prefslist_keyboard_logo.h"
#include "prefs_advanced.h"

APTR prefswin;

extern ULONG panel_modus;

const struct prefsgroup prefsgrp[PREFSWIN_NUMPAGES] = { /* !!! DON'T FORGET TO CHANGE NUMPAGES IN prefswin.h !!! */
	{NULL,"Backgrounds",    MSG_PREFSWIN_TAB_BACKGROUNDS,   getprefswin_backgroundclass,    prefslist_general      }, /* XXX: hm, the name is wrong.. oh well :) */
	{NULL,"Icon display",   MSG_PREFSWIN_TAB_ICONDISPLAY,   getprefswin_icondisplayclass,   prefslist_icondisplay  },
	{NULL,"Miscellaneous",  MSG_PREFSWIN_TAB_MISCELLANEOUS, getprefswin_miscellaneousclass, prefslist_miscellaneous},
	{NULL,"CLI launching",  MSG_PREFSWIN_TAB_CLILAUNCHING,  getprefswin_clilaunchclass,     prefslist_cli          },
#if USE_DROP_EFFECT_PREFS
	{NULL,"Drag & Drop",    MSG_PREFSWIN_TAB_DRAGANDDROP,   getprefswin_dragdropclass,      prefslist_dnd          },
#endif
#if USE_INTERNAL_PANELS
	{NULL,"Panels",         MSG_PREFSWIN_TAB_PANELS,        getprefswin_panelclass,         prefslist_panels       },
#endif
	{NULL,"Lister",         MSG_PREFSWIN_TAB_LISTER,        getprefswin_listerclass,        prefslist_lister       },
	{NULL,"Mime",           MSG_PREFSWIN_TAB_MIME,          getprefswin_mimeclass,          prefslist_mimetype     }, /* XXX */
	{NULL,"Window",         MSG_PREFSWIN_TAB_WINDOW,        getprefswin_windowclass,        prefslist_window       },
	{NULL,"Keyboard",       MSG_PREFSWIN_TAB_KEYBOARD,      getprefswin_keyboardclass,      prefslist_keyboard     }, /* XXX */
#if 0 /* disabled, so bigfoot can stop whining */
	{NULL,"Advanced",       MSG_PREFSWIN_TAB_ADVANCED,      getprefswin_advancedclass,      prefslist_dnd          }, /* XXX ? (image is fine IMHO -- tokai ) */
#endif
	{NULL,"Bookmarks",      MSG_PREFSWIN_TAB_BOOKMARKS,     getprefswin_bookmarksclass,     actioneditor_menu      },  /* XXX image missing... */
	{"mossys:Ambient/PanelPrefs.pobj","Panels2",	0,		0,								prefslist_panels       }
};

struct Data {
	LONG oldactive;
	APTR lv_sel;
	APTR prefscontainer;
	APTR prefscontents;
	APTR miscclass;
	ULONG has_backup;
};

static int lastactivepage;

/*
 * Helper routines.
 */
APTR pstring(ULONG pid, ULONG maxlen, CONST_STRPTR label)
{
	APTR o;

	if ((o = MUI_MakeObject(MUIO_String, label, maxlen)))
	{
		SetAttrs(o, MUIA_CycleChain, 1, MUIA_String_Contents, getprefsstr(pid), TAG_DONE);
	}
	return (o);
}


void storestring(APTR strobj, ULONG pid)
{
	setprefsstr_ctx(cloneprefspool, pid, (STRPTR)getv(strobj, MUIA_String_Contents));
}

void storeattr(APTR obj, ULONG attr, ULONG pid)
{
	setprefslong_ctx(cloneprefspool, pid, getv(obj, attr));
}

void storepath(APTR strobj, ULONG pid)
{
	STRPTR t;

	if( ( t =  (STRPTR) getv( strobj, MUIA_String_Contents ) ) )
	{
		if( ( t = name_build_sysify( t ) ) )
		{
			setprefsstr_ctx( cloneprefspool, pid, t );
		}
		free( t );
	}
}


DEFNEW
{
	struct Data *data;
	APTR lv_sel;
	APTR prefscontainer, prefscontents;
	APTR lg;
	#if !USE_INSTANTAPPLY_PREFS
	APTR bt_test;
	#endif
	APTR bt_save, bt_use, bt_cancel;
	APTR pfrimg;
	/* we only know 1 possible external prefs object atm */
#if USE_EXTERNAL_PANELS
	APTR ext_panel_obj;
#endif
	ULONG c;
	const struct prefsgroup *pg = prefsgrp;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ID, MAKE_ID('P','R','E','F'),
		MUIA_Window_Title, GSI(MSG_PREFSWIN_MAIN_TITLE),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_NoMenus, TRUE,
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_Width, MUIV_Window_Width_Screen(50),
		MUIA_Window_Height, MUIV_Window_Height_Screen(60),
		MUIA_Window_RootObject, VGroup,

			Child, HGroup,
				Child, lg = VGroup,
					Child, lv_sel = ListviewObject,
						MUIA_CycleChain, 1,
						MUIA_Listview_List, NewObject(getprefswin_listclass(), NULL, TAG_DONE),
					End,
				End,

				Child, prefscontainer = VGroup,
					MUIA_Frame,      MUIV_Frame_Page,         
					MUIA_Background, MUII_PageBack,
					Child, prefscontents = VCenter((text2("\033cLoading subsystem, please wait.."))),
				End,
			End,

			Child, HGroup,
/*
				Child, bt_save = Button(MSG_SAVE, 0),
				Child, HSpace(0),
				Child, bt_use = button(MSG_USE, 0),
				Child, HSpace(0),
				#if !USE_INSTANTAPPLY_PREFS
				Child, bt_test = button(MSG_TEST, 0),
				Child, HSpace(0),
				#endif
				Child, bt_cancel = button(MSG_CANCEL, 0),
*/
				Child, bt_save = MUICreateButton(MSG_PREFSWIN_SAVE, "PREF_SAVE"),
				Child, HSpace(0),
				Child, bt_use = MUICreateButton(MSG_PREFSWIN_USE, "PREF_USE"),
				Child, HSpace(0),
				#if !USE_INSTANTAPPLY_PREFS
				Child, bt_test = MUICreateButton(MSG_PREFSWIN_TEST, "PREF_TEST"),
				Child, HSpace(0),
				#endif
				Child, bt_cancel = MUICreateButton(MSG_PREFSWIN_CANCEL, "PREF_CANCEL"),
			End,
		End,
	End;

	if (!obj)
	{
		return (0);
	}

	pfrimg = MUI_NewObject(MUIC_Popfrimage,
		MUIA_Window_Title,      GSI(MSG_PREFSWIN_MAIN_CLIPBOARD_FRAMEIMAGE),
		MUIA_Framedisplay_Spec, NULL,
		MUIA_Imagedisplay_Spec, NULL,
		MUIA_FixHeight,         16,
		MUIA_ShortHelp,         GSI(MSG_PREFSWIN_MAIN_CLIPBOARD_FRAMEIMAGE_HELP),
		MUIA_CycleChain,        TRUE,
	End;

	if (pfrimg &&
		DoMethod(lg, MUIM_Group_InitChange))
	{
		DoMethod(lg, OM_ADDMEMBER, pfrimg);
		DoMethod(lg, MUIM_Group_ExitChange);
	}

	if (!updatelist_begin())
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->oldactive = MUIV_List_Active_Off;
	data->lv_sel = lv_sel;
	data->prefscontainer = prefscontainer;
	data->prefscontents = prefscontents;
	data->miscclass = NULL;

	prefsclone_init();

	/*
	 * Build the label list.
	 */
	
	for (c = 0; c < PREFSWIN_NUMPAGES; c++, pg++)
	{
		if(pg->class_name)
		{
			/* only add new panels here if both are new and old are used */
			if(panel_modus == 3) DoMethod(lv_sel, MUIM_List_InsertSingle, pg->english_name, MUIV_List_Insert_Bottom);
		}
		else
		{
			#if 0
			if((pg->labelid == MSG_PREFSWIN_TAB_PANELS) && (panel_modus == 2))
			{
				/* if old panels are not used put new ones here */
				DoMethod(lv_sel, MUIM_List_InsertSingle, "Panels", MUIV_List_Insert_Bottom);
			}
			else
			#endif
			{
				DoMethod(lv_sel, MUIM_List_InsertSingle, GSI(pg->labelid), MUIV_List_Insert_Bottom);
			}
		}
	}
	/* the path isn't optimal but will do till a better structure is decided */
#if USE_EXTERNAL_PANELS
/*	if((ext_panel_obj = MUI_NewObject("mossys:Ambient/PanelPrefs.pobj",TAG_DONE)))
	{
		DoMethod(lv_sel, MUIM_List_InsertSingle, "BonusPanel", MUIV_List_Insert_Bottom);
		MUI_DisposeObject(ext_panel_obj);
	}*/
#endif

	DoMethod(lv_sel, MUIM_Notify, MUIA_List_Active, MUIV_EveryTime,
		app, 5, MUIM_Application_PushMethod, obj, 2, MM_Prefswin_Main_SelectChange, MUIV_TriggerValue
	);

	set(lv_sel, MUIA_List_Active, lastactivepage);
	SetAttrs(obj, MUIA_Window_ActiveObject, lv_sel, MUIA_Window_DefaultObject, lv_sel, TAG_DONE);

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		obj, 2, MM_Prefswin_Main_Close, MV_Prefswin_Main_Close_Cancel
	);

	DoMethod(bt_save, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Prefswin_Main_Close, MV_Prefswin_Main_Close_Save
	);
	DoMethod(bt_use, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Prefswin_Main_Close, MV_Prefswin_Main_Close_Use
	);

	#if !USE_INSTANTAPPLY_PREFS
	DoMethod(bt_test, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Prefswin_Main_Close, MV_Prefswin_Main_Close_Test
	);
	#endif

	DoMethod(bt_cancel, MUIM_Notify, MUIA_Pressed, FALSE,
		obj, 2, MM_Prefswin_Main_Close, MV_Prefswin_Main_Close_Cancel
	);

	return ((ULONG)obj);
}


DEFGET
{
	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = MV_Window_Path_None;
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_Prefs;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFSMETHOD(Prefswin_Main_SelectChange)
{
	GETDATA;
		
	DoMethod(data->prefscontainer, MUIM_Group_InitChange);

	DoMethod(data->prefscontainer, OM_REMMEMBER, data->prefscontents);

	MUI_DisposeObject(data->prefscontents);

	data->prefscontents = NULL;

	/*
	 * Creating new object
	 */
	data->oldactive = msg->listentry;

	if (msg->listentry >= 0)
	{
		{
			if(prefsgrp[data->oldactive].class_name)
			{
				data->prefscontents = MUI_NewObject(prefsgrp[data->oldactive].class_name,TAG_DONE);
			}
			else
			{
				if((data->oldactive == 4)&&(panel_modus == 2))
				{
					data->prefscontents = MUI_NewObject(prefsgrp[PREFSWIN_NUMPAGES - 1].class_name,TAG_DONE);
				}
				else
				{
					data->prefscontents = NewObject(prefsgrp[data->oldactive].class(), NULL, TAG_DONE);
				}
			}
		}
		if (data->oldactive == 2) /* Means we are over the Misc class */
			data->miscclass = data->prefscontents;
	}

	if (data->prefscontents)
	{
		DoMethod(data->prefscontainer, OM_ADDMEMBER, data->prefscontents);
	}
	else
	{
		data->prefscontents = VCenter((TextObject, TextFrame, MUIA_Text_Contents, "SEVERE INTERNAL ERROR WHILE\nSETTING UP PREFS WINDOW!", End));
	}

	DoMethod(data->prefscontainer, MUIM_Group_ExitChange);

	return (0);
}


DEFSMETHOD(Prefswin_Main_SetPage)
{
	ULONG i;
	GETDATA;
	for (i = 0; i < PREFSWIN_NUMPAGES; i++)
	{
		if (!stricmp(prefsgrp[i].english_name, msg->name))
		{
			set(data->lv_sel, MUIA_List_Active, i);

			break;
		}
	}
	return (0);
}

/*
 * CANCEL-MEANS-REVERT if realtime prefs is enabled..
 */
DEFSMETHOD(Prefswin_Main_Close)
{
	GETDATA;
	/* stopgap solution to save/reset panel_prefs which aren't really connected (yet) to the rest of the prefs*/
	if(data->prefscontents)DoMethod(data->prefscontents,MM_Prefswin_Main_Close,msg->mode);
	/* XXX: Special case for deficonpath env variable... */
	if (data->miscclass)
		prefswin_misc_setdeficonpath(msg->mode);

	if (msg->mode == MV_Prefswin_Main_Close_Test)
	{
		DoMethod(data->prefscontainer, MM_Prefswin_Store);

		if (!data->has_backup)
		{
			data->has_backup = backup_prefs();
		}

		if (data->has_backup)
		{
			/* XXX: er.. this is broken as the current panel won't change the methods.. we have to add a new method to all the panels.. */
			prefsclone_cleanup(TRUE, FALSE);
			updatelist_end(TRUE, FALSE);

			/*
			 * We don't close the window and keep the
			 * updatelist around.
			 */
		}
	}
	else
	{
		if (data->has_backup && msg->mode == MV_Prefswin_Main_Close_Cancel)
		{
			restore_prefs();
		}

		lastactivepage = data->oldactive;

		DoMethod(data->prefscontainer, MUIM_Group_InitChange);
		DoMethod(data->prefscontainer, OM_REMMEMBER, data->prefscontents);
		DoMethod(data->prefscontainer, MUIM_Group_ExitChange);
		MUI_DisposeObject(data->prefscontents);

		/*
		 * Save the changes.
		 */
		prefsclone_cleanup(msg->mode, TRUE);
		if (msg->mode == MV_Prefswin_Main_Close_Save)
		{
			DoMethod(app, MM_Application_SavePrefs, FALSE);
		}

		/*
		 * Execute the changes.
		 */
		if (msg->mode == MV_Prefswin_Main_Close_Cancel)
		{
			DoMethod(app, MM_Application_LoadPrefs, MV_Application_LoadPrefs_All, TRUE);
		}

		updatelist_end(data->has_backup ? TRUE : msg->mode, TRUE);

		methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
		prefswin = NULL;

		/* XXX: set_prefs_globals() */
	}



	/*   Handle Advanced.conf saving/ reverting
	 *
	 *   XXX: make threaded ?
	 *
	 *   XXX: only refresh or save if there was actually a change to a variable
	 */
	if (msg->mode == MV_Prefswin_Main_Close_Cancel)
	{
		/*  reload old values from prefsfile (possible could load new
		 *  values if the user changed file manually in the meantime,
		 */
		prefs_advanced_refresh(FALSE);
	}
	else if (msg->mode == MV_Prefswin_Main_Close_Save)
	{
		if (!prefs_advanced_save())
		{
			/* XXX: bring up a requester if failed? */
		}
	}

	return (0);
}


BEGINMTABLE
DECNEW
DECGET
DECSMETHOD(Prefswin_Main_SelectChange)
DECSMETHOD(Prefswin_Main_SetPage)
DECSMETHOD(Prefswin_Main_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, prefswin_mainclass)
