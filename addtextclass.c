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
 * $Id: addtextclass.c,v 1.7 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"


struct Data {
	ULONG added;
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	return ((ULONG)obj);
}


DEFSET
{
	FORTAG(INITTAGS)
	{
		case MA_AddText_Contents:
			{
				STRPTR s;

				s = (STRPTR)getv(obj, MUIA_Text_Contents);

				if (s && *s)
				{
					GETDATA;

					if (data->added)
					{
						DoMethod(obj, MUIM_SetAsString, MUIA_Text_Contents, "%s, %s", s, (STRPTR)tag->ti_Data);
					}
					else
					{
						DoMethod(obj, MUIM_SetAsString, MUIA_Text_Contents, "%s %s", s, (STRPTR)tag->ti_Data);
						data->added = TRUE;
					}
				}
				else
				{
					set(obj, MUIA_Text_Contents, (STRPTR)tag->ti_Data);
				}
			}
			break;
	}
	NEXTTAG

	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECSET
ENDMTABLE

DECSUBCLASS_NC(MUIC_Text, addtextclass)

