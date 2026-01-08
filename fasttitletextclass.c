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
 * $Id: fasttitletextclass.c,v 1.4 2017/08/21 06:17:44 cyfm Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mui_func.h"

struct Data {
	int dummy;
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_Frame, "200000",
		MUIA_Background, MUII_BACKGROUND,
		MUIA_InnerLeft, 2, /* XXX: mandatory ? can't that format the text better or so ? */
		MUIA_InnerRight, 2,
		MUIA_InnerTop, 0,
		MUIA_InnerBottom, 0,
		//MUIA_FillArea, FALSE,
		//MUIA_CustomBackfill, TRUE,
		TAG_MORE, INITTAGS,
	End;

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	return ((ULONG)obj);
}


BEGINMTABLE
DECNEW
ENDMTABLE

DECSUBCLASS_NC(MUIC_Text, fasttitletextclass)

