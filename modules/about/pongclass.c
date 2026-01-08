/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: pongclass.c,v 1.2 2005/07/04 23:06:13 laire Exp $
 */

#include "globals.h"

/* public */

/* private */
#include "clib/about_protos.h"
#include "mui_func.h"
#include "aboutclasses.h"


struct Data {
	int dummy;
};


DEFNEW
{
	struct Data *data;

	obj = DoSuperNew(cl, obj,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (NULL);
	}

	data = INST_DATA(cl, obj);

	return ((ULONG)obj);
}


BEGINMTABLE
DECNEW
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, pongclass)

APTR About_GetPongClass(void)
{
	return (getpongclass());
}
