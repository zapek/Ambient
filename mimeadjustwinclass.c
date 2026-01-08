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
 * $Id: mimeadjustwinclass.c,v 1.4 2006/09/18 23:17:29 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "ambient_cat.h"
#include "methodstack.h"
#include "mui_func.h"
#include "screen.h"

struct Data {
	APTR mimeadjustgrp;
};


DEFNEW
{
	struct Data *data;
	APTR mimeadjustgrp;

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Screen, get_screen(),
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_Title, GSI(MSG_MIMEADJUSTWINCLASS_TITLE),
		MUIA_Window_ID, MAKE_ID('M','I','E','D'),
		MUIA_Window_ShowIconify, FALSE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_ShowSnapshot, TRUE,
		MUIA_Window_NoMenus, TRUE,
		WindowContents, mimeadjustgrp = NewObject(getmimeadjustgroupclass(), NULL, TAG_MORE, INITTAGS),
	End;

	if (obj)
	{
		data = INST_DATA(cl, obj);
		data->mimeadjustgrp = mimeadjustgrp;

		DoMethod(obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
			mimeadjustgrp, 2, MM_Mimeadjustgroup_Accept, FALSE
		);
	}


	return ((ULONG)obj);
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
			*msg->opg_Storage = MV_Window_Type_MIMEAdjust;
			return (TRUE);

		case MA_Mimeadjustwin_MimeNode:
			*msg->opg_Storage = (ULONG) getv(data->mimeadjustgrp, MA_Mimeadjustwin_MimeNode);
			return (TRUE);

		case MA_Mimeadjustwin_Generic:
			*msg->opg_Storage = (ULONG) getv(data->mimeadjustgrp, MA_Mimeadjustwin_Generic);
			return (TRUE);
            
	}
	return (DOSUPER);
}


DEFTMETHOD(Mimeadjustwin_Close)
{
	methodstack_push(_app(obj), 2, MM_Application_DisposeWindow, obj);
	return (0);
}


BEGINMTABLE
DECNEW
DECGET
DECDISP
DECTMETHOD(Mimeadjustwin_Close)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, mimeadjustwinclass)

