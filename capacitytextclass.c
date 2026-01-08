/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: capacitytextclass.c,v 1.6 2006/04/12 14:01:53 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"
#include "capacity.h"


struct Data {
	TEXT s[20]; /* should be enough for everyone (tm) */
	QUAD totalsize;
};


DEFNEW
{
	struct Data *data;
	struct TagItem *ti;

	obj = DoSuperNew(cl, obj,
		MUIA_DoubleBuffer, TRUE,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);

	if (FindTagItem(MUIA_Text_Contents, INITTAGS))
	{
		ti = FindTagItem(MA_CapacityText_Total, INITTAGS);

		DoMethod(obj, MM_CapacityText_Build, ti ? ti->ti_Data : NULL);
	}
	return ((ULONG)obj);
}


DEFSET
{
	FORTAG(INITTAGS)
	{
		case MUIA_Text_Contents:
			SetSuperAttrs(cl, obj, MUIA_Text_Contents, tag->ti_Data, TAG_DONE);
			DoMethod(obj, MM_CapacityText_Build, NULL);
			return (0); /* XXX: wrong.. well, doesn't matter much in our case but.. */

		case MA_CapacityText_Total:
		{
			GETDATA;
			memcpy(&data->totalsize, (QUAD *)tag->ti_Data, sizeof(QUAD));
			DoMethod(obj, MM_CapacityText_Build, NULL);
			return (0); /* XXX: ditto */
		}
	}
	NEXTTAG

	return (DOSUPER);
}


/*
 * Gets a size in bytes and sets 
 * the correct capacity.
 * XXX: fix locale support
 */
DEFSMETHOD(CapacityText_Build)
{
	GETDATA;

	QUAD n;

	n = atoll((STRPTR)getv(obj, MUIA_Text_Contents));

	capacity_format_size(data->s, sizeof(data->s), n);

	if (msg->totalsize)
	{
		snprintf(data->s, sizeof(data->s), "%s (%d.%d%%)", data->s, (int)( n * 100 / *msg->totalsize ), (int)( n * 100 % *msg->totalsize * 10 / *msg->totalsize ));
	}
	else if (data->totalsize)
	{
		snprintf(data->s, sizeof(data->s), "%s (%d.%d%%)", data->s, (int)( n * 100 / data->totalsize ), (int)( n * 100 % data->totalsize * 10 / data->totalsize ));
	}
	SetSuperAttrs(cl, obj, MUIA_Text_Contents, data->s, TAG_DONE);

	return (0);
}



BEGINMTABLE
DECNEW
DECSET
DECSMETHOD(CapacityText_Build)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Text, capacitytextclass)
