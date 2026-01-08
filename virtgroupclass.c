/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005 Ambient Open Source Team
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
 * $Id: virtgroupclass.c,v 1.4 2006/04/12 14:01:56 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"

struct Data {
	ULONG horiz;
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
			NoFrame,
			TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data = INST_DATA(cl, obj);

		data->horiz = GetTagData(MUIA_Group_Horiz, FALSE, INITTAGS);
	}

	return ((ULONG)obj);
}


DEFMMETHOD(AskMinMax)
{
	GETDATA;
	DOSUPER;

	if (data->horiz)
	{
		msg->MinMaxInfo->MinWidth = msg->MinMaxInfo->DefWidth;
//		msg->MinMaxInfo->DefHeight = msg->MinMaxInfo->MinHeight;
//		msg->MinMaxInfo->MaxHeight += MUI_MAXMAX;
	}
	else
	{
		msg->MinMaxInfo->MinHeight = msg->MinMaxInfo->DefHeight;
//		msg->MinMaxInfo->DefWidth = msg->MinMaxInfo->MinWidth;
//		msg->MinMaxInfo->MaxWidth += MUI_MAXMAX;
	}

	return (0);
}


BEGINMTABLE
DECNEW
DECMMETHOD(AskMinMax)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Virtgroup, virtgroupclass)
