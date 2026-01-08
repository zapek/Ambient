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
 * $Id: viewsizegroupclass.c,v 1.7 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <intuition/extensions.h>
#include <proto/intuition.h>

/* private */
#include "mui_func.h"


struct Data {
	ULONG isroot;
};


DEFNEW
{
	struct Data *data;
	struct TagItem *ti;

	obj = DoSuperNew(cl, obj,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);

	if ((ti = FindTagItem(MA_Viewsizegroup_IsRoot, INITTAGS)))
	{
		data->isroot = ti->ti_Data;
	}

	return ((ULONG)obj);
}


DEFMMETHOD(AskMinMax)
{
	GETDATA;
	DOSUPER;

	if (data->isroot)
	{
		struct Screen *scr = _screen(obj);

		msg->MinMaxInfo->MinWidth = scr->Width;
		msg->MinMaxInfo->MinHeight = scr->Height - (GetSkinInfoAttr(GetScreenDrawInfo(scr), SI_ScreenTitlebarHeight, TAG_DONE));

	}

	return (0);
}


DEFMMETHOD(Setup)
{
	ULONG rc;

	if ((rc = DOSUPER))
	{
		GETDATA;

		if (muiRenderInfo(obj) && _win(obj) && data->isroot)
		{
			DoMethod(_view(obj), MM_View_SetWindowPosition, 0, GetSkinInfoAttr(GetScreenDrawInfo(_screen(obj)), SI_ScreenTitlebarHeight, TAG_DONE));
		}
	}
	return (rc);
}


BEGINMTABLE
DECNEW
DECMMETHOD(AskMinMax)
DECMMETHOD(Setup)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, viewsizegroupclass)
