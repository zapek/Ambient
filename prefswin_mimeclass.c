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
 * $Id: prefswin_mimeclass.c,v 1.4 2025/07/24 01:26:38 geit Exp $
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
		Child, NewObject(getmimegroupclass(), NULL, TAG_DONE),
	End;

	if (!obj)
	{
		return ((ULONG)NULL);
	}

	return ((ULONG)obj);
}


/* XXX: both unecessary I think */
DEFTMETHOD(Prefswin_Store)
{
	return (0);
}


DEFDISPOSE
{
	return (DOSUPER);
}


BEGINMTABLE
DECNEW
DECTMETHOD(Prefswin_Store)
DECDISPOSE
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, prefswin_mimeclass)
