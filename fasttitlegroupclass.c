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
 * $Id: fasttitlegroupclass.c,v 1.4 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/graphics.h>

/* private */
#include "mui_func.h"

struct Data {
	APTR txt_title;
	APTR txt_target;
};


DEFNEW
{
	struct Data *data;
	APTR txt_title, txt_target;

	obj = DoSuperNew(cl, obj,
		//ButtonFrame,
		MUIA_Group_Horiz, TRUE,
		MUIA_Group_Spacing, 0,
		//MUIA_FillArea, FALSE,
		//MUIA_CustomBackfill, TRUE,

		MUIA_InnerLeft, 0,
		MUIA_InnerRight, 0,
		MUIA_InnerTop, 0,
		MUIA_InnerBottom, 0,
		
		Child, txt_title = NewObject(getfasttitletextclass(), NULL, TAG_DONE),
		Child, txt_target = NewObject(getfasttitletextclass(), NULL,
			MUIA_Text_PreParse, "\033c",
			MUIA_Text_SetMax, TRUE,
			MUIA_Text_SetMin, TRUE,
		End,

	End;

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	data = INST_DATA(cl, obj);
	data->txt_title = txt_title;
	data->txt_target = txt_target;

	/* XXX: just for fun */
	set(data->txt_title, MUIA_Text_Contents, "Device list");
	set(data->txt_target, MUIA_Text_Contents, " OFF ");

	return ((ULONG)obj);
}


#if 0
DEFMMETHOD(Backfill)
{
	SetAPen(_rp(obj), _pens(obj)[MPEN_BACKGROUND]);

	RectFill(_rp(obj), msg->left, msg->top, msg->right, msg->bottom);

	return (TRUE); /* XXX: does it matter ? */
}
#endif


BEGINMTABLE
DECNEW
#if 0
DECMMETHOD(Backfill)
#endif
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, fasttitlegroupclass)
