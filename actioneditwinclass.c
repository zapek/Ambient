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
 * $Id: actioneditwinclass.c,v 1.3 2025/09/09 12:46:45 jacadcaps Exp $
 */

#include "ambient.h"
#include "classes.h"
#include "screen.h"
#include "methodstack.h"
#include "mui_func.h"

#include <exec/types.h>
#include <libraries/mui.h>

struct Data {
	APTR actioneditgrp, mime_node;
	ULONG commandset;
};

DEFNEW
{
	ULONG commandset = MV_Actionedit_Commandset_Normal;
	APTR actioneditgrp, mime_node = NULL;
	struct Data *data;

	FORTAG(INITTAGS)
	{
		case MA_ActioneditWin_MimeNode:
			mime_node = (APTR) tag->ti_Data;
			break;

		case MA_ActioneditWin_Commandset:
			commandset = tag->ti_Data;
	}
	NEXTTAG

	if (!mime_node)
		return (ULONG) obj;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_PublicScreen, active_screen_name(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, "Ambient · Action editor",
		MUIA_Window_LeftEdge, MUIV_Window_LeftEdge_Moused,
		MUIA_Window_TopEdge, MUIV_Window_TopEdge_Moused,
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_NoMenus, TRUE,
		WindowContents, actioneditgrp = NewObject(getactioneditclass(), NULL,
			MA_Actionedit_MimeNode, mime_node,
			MA_Actionedit_Commandset, commandset,
		End,
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		data = INST_DATA(cl, obj);
		data->actioneditgrp = actioneditgrp;
		data->commandset = commandset;
		data->mime_node = mime_node;

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
			obj, 1, MM_ActioneditWin_Close
		);
	}

	return (ULONG) obj;
}

DEFTMETHOD(ActioneditWin_Open)
{
	DoMethod(app, OM_ADDMEMBER, obj);
	set(obj, MUIA_Window_Open, TRUE);

	return 0;
}

DEFTMETHOD(ActioneditWin_Close)
{
	GETDATA;

	DoMethod(data->actioneditgrp, MM_Actionedit_CloseCommands); /* We need to close this one ... */
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	return 0;
}

BEGINMTABLE
DECNEW
DECTMETHOD(ActioneditWin_Open)
DECTMETHOD(ActioneditWin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, actioneditwinclass)
