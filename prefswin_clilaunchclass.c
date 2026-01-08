/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: prefswin_clilaunchclass.c,v 1.9 2016/01/02 19:07:23 itix Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefswin.h"
#include "prefsclone.h"

struct Data {
	APTR str_device;
	APTR str_stack;
	APTR str_newshell;
};


DEFNEW
{
	struct Data *data;

	APTR str_device, str_stack, str_newshell;

	obj = DoSuperNew(cl, obj,
		Child, VGroup, 
			Child, RectangleObject, End,
			Child, ColGroup(2), GroupFrameT(GSI(MSG_PREFSWIN_CLI_COMMANDLINE)),
				Child, NSLabel2(MSG_PREFSWIN_CLI_DEVICE),
				Child, str_device = MUICreateString( MSG_PREFSWIN_CLI_DEVICE, PATH_SIZE, "PREF_CLI_DEVICE"),
				Child, NSLabel2(MSG_PREFSWIN_CLI_STACKSIZE),
				Child, str_stack = MUICreateInteger( MSG_PREFSWIN_CLI_STACKSIZE, 8, "PREF_CLI_STACKSIZE", 32768, 1073741824, 1024),
				Child, NSLabel2(MSG_PREFSWIN_CLI_NEWSHELL),
				Child, str_newshell = MUICreateString( MSG_PREFSWIN_CLI_NEWSHELL, PATH_SIZE, "PREF_CLI_NEWSHELL"),
			End,
			Child, RectangleObject, End,
		End,
	End;

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);
	data->str_device = str_device;
	data->str_stack = str_stack;
	data->str_newshell = str_newshell;

	setupprefs(str_device, MUIA_String_Contents, (ULONG) getprefsstr(DSI_CLI_DEVICE));
	setupprefs(str_stack, MUIA_String_Integer, getprefslong(DSI_CLI_STACK));
	setupprefs(str_newshell, MUIA_String_Contents, (ULONG) getprefsstr(DSI_CLI_NEWSHELL));

	return ((ULONG)obj);
}


DEFTMETHOD(Prefswin_Store)
{
	GETDATA;

	storestring(data->str_device, DSI_CLI_DEVICE);
	storeattr(data->str_stack, MUIA_String_Integer, DSI_CLI_STACK);
	storestring(data->str_newshell, DSI_CLI_NEWSHELL);

	return (0);
}


DEFDISPOSE
{
	DoMethod(obj, MM_Prefswin_Store);

	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECTMETHOD(Prefswin_Store)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_clilaunchclass)
