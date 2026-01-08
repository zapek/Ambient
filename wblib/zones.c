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
 * $Id: zones.c,v 1.2 2005/07/04 23:06:15 laire Exp $
 */

#include "globals.h"

/* public */
#include <dos/dosextens.h>

/* private */
#include "clib/wb_protos.h"


#define DB_APPWINDOWDROPZONE 0
#define DB_REMOVEAPPWINDOWDROPZONE 0


struct AppWindowDropZone * AddAppWindowDropZoneA(struct AppWindow *aw, ULONG id, ULONG userdata, struct TagItem *tags)
{
	D(APPWINDOWDROPZONE,bug("called\n"));
	return (0);
}


BOOL RemoveAppWindowDropZone(struct AppWindow *aw, struct AppWindowDropZone *dropZone)
{
	D(REMOVEAPPWINDOWDROPZONE,bug("called\n"));
	return (0);
}
