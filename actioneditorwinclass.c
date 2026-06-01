/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005 Ambient Open Source Team
 * actioneditwinclass.c, Copyright 2005 by Adam Waldenberg
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
 * $Id: actioneditorwinclass.c,v 1.3 2025/09/09 12:46:45 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <exec/types.h>
#include <libraries/mui.h>

/* private */
#include "ambient_cat.h"
#include "classes.h"
#include "screen.h"
#include "mui_func.h"
#include "methodstack.h"


struct Data {
	APTR actioneditgrp;
	APTR action_node;
};

DEFNEW
{
	APTR actioneditgrp, action_node = NULL;
	struct Data *data;
	ULONG mimeaction = FALSE;

	FORTAG(INITTAGS)
	{
		case MA_ActioneditorWin_ActionNode:
			action_node = (APTR) tag->ti_Data;
			break;
		case MA_Actioneditor_MimeAction:
			mimeaction = tag->ti_Data;
			break;
	}
	NEXTTAG

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI(MSG_ACTIONEDITORWINCLASS_TITLE),
		MUIA_Window_ID, MAKE_ID('A','C','E','D'),
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Moused,
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_ShowSnapshot, TRUE,
		MUIA_Window_NoMenus, TRUE,
		WindowContents, actioneditgrp = NewObject(getactioneditorclass(), NULL,
			MA_Actioneditor_ActionNode, action_node,
			MA_Actioneditor_MimeAction, mimeaction,
		End,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		data = INST_DATA(cl, obj);
		data->actioneditgrp = actioneditgrp;
		data->action_node = action_node;

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
			obj, 3, MUIM_Set, MUIA_Window_Open, FALSE
		);
	}

	return (ULONG) obj;
}

DEFDISP
{
	return (DOSUPER);
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
			*msg->opg_Storage = MV_Window_Path_None; /* XXX: how to check for dups ? implement that */
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = MV_Window_Type_MIMEAction;
			return (TRUE);

		case MA_ActioneditorWin_ActionNode:
			*msg->opg_Storage = (ULONG) getv(data->actioneditgrp, MA_Actioneditor_ActionNode);
			return(TRUE);
	}
	return (DOSUPER);
}

DEFTMETHOD(ActioneditorWin_Open)
{
	DoMethod(app, OM_ADDMEMBER, obj);
	set(obj, MUIA_Window_Open, TRUE);

	return 0;
}

DEFTMETHOD(ActioneditorWin_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECTMETHOD(ActioneditorWin_Open)
DECTMETHOD(ActioneditorWin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, actioneditorwinclass)
