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
 * $Id: panelsliderspeedclass.c,v 1.7 2012/09/03 03:28:58 geit Exp $
 */

#include "ambient.h"
#include "ambient_cat.h"


/* public */

/* private */
#include "mui_func.h"

/************************************************************************/

struct Data {
	int dummy;
};

/************************************************************************/

DEFNEW
{
	if( ( obj = DoSuperNew( cl, obj,
									MUIA_Numeric_Min, 0,
									MUIA_Numeric_Max, 3,
									TAG_MORE, INITTAGS
									) ) )
	{

	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(Numeric_Stringify)
{
	switch( msg->value )
	{
		case 0:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_SLOW) );

		case 1:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_MEDIUM) );

		case 2:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_FAST) );

		case 3:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_INSTANT) );

		default:
			PDB(("wrong value\n"));
			break;
	}
	return( 0 );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECMMETHOD(Numeric_Stringify)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Slider, panelsliderspeedclass)

