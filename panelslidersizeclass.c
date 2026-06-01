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
 * $Id: panelslidersizeclass.c,v 1.6 2025/09/16 15:52:30 kronos Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_PANELS

/* public */

/* private */
#include "mui_func.h"
#include "ambient_cat.h"

struct Data {
	int dummy;
};


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_Numeric_Min, 0,
		MUIA_Numeric_Max, 4,
		TAG_MORE, INITTAGS,
	End;

	if (!obj)
	{
		return ((ULONG)NULL);
	}
	return ((ULONG)obj);
}


DEFMMETHOD(Numeric_Stringify)
{
	if(  msg->value >= 0 && msg->value < 5 ) {
		return(  (ULONG) GSI( MSG_PANELSLIDERSIZECLASS_16PIXEL + msg->value ) );
	} else {
		PDB(("wrong value\n"));
		return( 0 );
	}
	return (0);
}


BEGINMTABLE
DECNEW
DECMMETHOD(Numeric_Stringify)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Slider, panelslidersizeclass)
#endif
