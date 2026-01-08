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
 * $Id: smarttextclass.c,v 1.5 2006/04/12 14:01:56 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/graphics.h>

/* private */
#include "mui_func.h"

#define MAXSIZE 63

struct Data {
	ULONG viewchars; /* in number of chars */
	ULONG viewsize;
};


DEFNEW
{
	struct Data *data;
	struct TagItem *ti;
	ULONG viewchars;

	if ((ti = FindTagItem(MA_SmartText_ViewChars, INITTAGS)))
	{
		viewchars = ti->ti_Data;
	}
	else
	{
		viewchars = 0;
	}

	obj = DoSuperNew(cl, obj,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);
	data->viewchars = (viewchars > MAXSIZE) ? MAXSIZE : viewchars;

	return ((ULONG)obj);
}


DEFMMETHOD(AskMinMax)
{
	struct RastPort rp; /* XXX: is a temp rastport necessary ? I don't think so */
	GETDATA;
	TEXT buf[MAXSIZE + 1];
	ULONG i;

	DOSUPER;

	if (data->viewchars)
	{
		for (i = 0; i < data->viewchars; i++)
		{
			buf[i] = 'M';
		}
		buf[i] = '\0';

		InitRastPort(&rp);
		SetFont(&rp, _font(obj));

		data->viewsize = TextLength(&rp, buf, data->viewchars);

		msg->MinMaxInfo->MinWidth += data->viewsize;
		msg->MinMaxInfo->DefWidth += data->viewsize;
		msg->MinMaxInfo->MaxWidth += data->viewsize; /* XXX: hm.. */
	}
	return (0);
}


BEGINMTABLE
DECNEW
DECMMETHOD(AskMinMax)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Text, smarttextclass)
