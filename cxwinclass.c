/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2007 Ambient Open Source Team
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
 * $Id: cxwinclass.c,v 1.15 2025/09/09 12:46:46 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <proto/commodities.h>

/* private */
#include "methodstack.h"
#include "mui_func.h"
#include "ambient_cat.h"
#include "screen.h"
#include "cx.h"


APTR cxwin;

struct Data {
	APTR lv_cx;
	APTR ft_info;
	APTR bt_show;
	APTR bt_hide;
	APTR bt_remove;
	APTR cyc_mode;
};


ULONG BrokerCommand(STRPTR target, ULONG cmd);

#define BrokerCommand(__p0, __p1) \
	LP2(198, ULONG , BrokerCommand, \
		STRPTR , __p0, a0, \
		ULONG , __p1, d0, \
		, CxBase, 0, 0, 0, 0, 0, 0)


DEFNEW
{
	struct Data *data;
	APTR mi_quit;
	APTR lv_cx, ft_info;
	APTR bt_show, bt_hide, bt_remove;
	APTR cyc_mode;

	static STRPTR cyc_state[ 2 + MSG_CX_INACTIVE - MSG_CX_ACTIVE];

	switch (exchange_create())
	{
		case CE_DUP:
			BrokerCommand("Exchange", CXCMD_APPEAR);
			// fallthrough
		case CE_FAILED:
			return (0);
	}

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, "Ambient · Exchange",
		MUIA_Window_ID, MAKE_ID('A','M','C','X'),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_Width, MUIV_Window_Width_Screen(30),

		MUIA_Window_Menustrip, MenustripObject,
			Child, MenuObject,
				MUIA_Menu_Title, GSI( MSG_CX_MENU_PROJECT ),
				Child, mi_quit = MenuitemObject,
					MUIA_Menuitem_Title,    GSI( MSG_CX_MENU_QUIT ),
					MUIA_Menuitem_Shortcut, "Q",
				End,
			End,
		End,

		WindowContents, HGroup,
			Child, lv_cx = NewObject(getcxlistclass(), NULL, MUIA_Weight, 40, MUIA_CycleChain, TRUE, TAG_DONE),
			Child, VGroup,
				Child, ft_info = FloattextObject,
					TextFrame,
				End,

				Child, ColGroup(2),
					Child, bt_show = MUICreateButton( MSG_CX_SHOW_INTERFACE, "EXCHANGE_SHOWINTERFACE"),
					Child, bt_hide = MUICreateButton( MSG_CX_HIDE_INTERFACE, "EXCHANGE_HIDEINTERFACE"),
					Child, cyc_mode = MUICreateCycle( MSG_CX_STATE, cyc_state, MSG_CX_ACTIVE, MSG_CX_INACTIVE, "EXECUTE_STATE"),
					Child, bt_remove = MUICreateButton( MSG_CX_REMOVE, "EXCHANGE_REMOVE"),
				End,
			End,
		End,
	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->lv_cx = lv_cx;
	data->ft_info = ft_info;
	data->bt_show = bt_show;
	data->bt_hide = bt_hide;
	data->bt_remove = bt_remove;
	data->cyc_mode = cyc_mode;

	DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
		obj, 1, MM_Cxwin_Close
	);

	set(data->bt_show, MUIA_Disabled, TRUE);
	set(data->bt_hide, MUIA_Disabled, TRUE);
	set(data->bt_remove, MUIA_Disabled, TRUE);

	set(obj, MUIA_Window_ActiveObject, data->lv_cx);

	DoMethod(data->bt_show, MUIM_Notify, MUIA_Pressed, FALSE,
		data->lv_cx, 2, MM_Cxlist_CxNotify, CXCMD_APPEAR
	);

	DoMethod(data->lv_cx, MUIM_Notify, MUIA_Listview_DoubleClick, TRUE,
		data->lv_cx, 2, MM_Cxlist_CxNotify, CXCMD_APPEAR
	);

	DoMethod(data->bt_hide, MUIM_Notify, MUIA_Pressed, FALSE,
		data->lv_cx, 2, MM_Cxlist_CxNotify, CXCMD_DISAPPEAR
	);

	DoMethod(data->cyc_mode, MUIM_Notify, MUIA_Cycle_Active, 0,
		data->lv_cx, 2, MM_Cxlist_CxNotify, CXCMD_ENABLE
	);

	DoMethod(data->cyc_mode, MUIM_Notify, MUIA_Cycle_Active, 1,
		data->lv_cx, 2, MM_Cxlist_CxNotify, CXCMD_DISABLE
	);

	DoMethod(data->bt_remove, MUIM_Notify, MUIA_Pressed, FALSE,
		data->lv_cx, 2, MM_Cxlist_CxNotify, CXCMD_KILL
	);

/* menu item notifies */

	DoMethod(mi_quit, MUIM_Notify, MUIA_Menuitem_Trigger, MUIV_EveryTime, obj, 1, MM_Cxwin_Close );

	return ((ULONG)obj);
}


DEFDISP
{
	exchange_delete();
	return (DOSUPER);
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
			*msg->opg_Storage = MV_Window_Type_Cx;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFTMETHOD(Cxwin_Rescan)
{
	CxObj *broker;
	struct Node	*n;
	struct MinList *l;

	if ((broker = (CxObj *)getv(app, MUIA_Application_Broker)))
	{
		GETDATA;
		struct PrivateCxObj *mco;

		n = (struct Node *)broker;

		DoMethod(data->lv_cx, MM_Cxlist_InitChange);

		Forbid(); /* XXX: not sure if that is really smart.. oh well, AmigaOS' API sucks donkey balls anyway */

		while (PREVNODE(n))
		{
			n = PREVNODE(n);
		}

		l = (struct MinList *)n;

		ITERATELIST(mco, l)
		{
			if (mco != (struct PrivateCxObj *)broker && (!exsig || mco != (struct PrivateCxObj *)exbroker))
			{
				DoMethod(data->lv_cx, MM_Cxlist_TryAdd, mco);
			}
		}

		Permit();

		DoMethod(data->lv_cx, MM_Cxlist_ExitChange);

		if ((LONG)getv(data->lv_cx, MUIA_List_Active) == MUIV_List_Active_Off)
			set(data->lv_cx, MUIA_List_Active, MUIV_List_Active_Top);
	}
	return (0);
}


DEFSMETHOD(Cxwin_SetStatus)
{
	GETDATA;

	set(data->bt_show, MUIA_Disabled, msg->gui ? FALSE : TRUE);
	set(data->bt_hide, MUIA_Disabled, msg->gui ? FALSE : TRUE);
	set(data->bt_remove, MUIA_Disabled, msg->remove ? FALSE : TRUE);
	//set(data->cyc_mode, MUIA_Disabled, msg->control ? FALSE : TRUE); /* XXX: I think.. verify again */

	set(data->cyc_mode, MUIA_Cycle_Active, msg->control ? 0 : 1);

	DoMethod(data->ft_info, MUIM_SetAsString, MUIA_Floattext_Text, "%s\n%s", (STRPTR)msg->title ? (STRPTR)msg->title : (STRPTR)"-", (STRPTR)msg->descr ? (STRPTR)msg->descr : (STRPTR)"-");

	return (0);
}


DEFTMETHOD(Cxwin_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	cxwin = NULL;
	return (0);
}


BEGINMTABLE
DECNEW
DECDISP
DECGET
DECTMETHOD(Cxwin_Rescan)
DECSMETHOD(Cxwin_SetStatus)
DECTMETHOD(Cxwin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, cxwinclass)
