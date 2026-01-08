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
 * $Id: datatypes.c,v 1.5 2006/04/12 14:01:53 fab Exp $
 */

#include "ambient.h"

/* public */
#include <datatypes/datatypes.h>
#include <proto/dos.h>
#include <proto/datatypes.h>

/* private */
#include "datatypes.h"


ULONG datatypes_findtype(STRPTR path)
{
	struct DataType *dth;
	BPTR l;
	ULONG type = DTTYPE_NONE;

	THREAD;

	if ((l = Lock(path, ACCESS_READ)))
	{
		if ((dth = ObtainDataType(DTST_FILE, (APTR)l, TAG_DONE)))
		{
			switch (dth->dtn_Header->dth_GroupID)
			{
				case GID_SYSTEM:
					type = DTTYPE_HEX;
					break;

				case GID_TEXT:
					type = DTTYPE_TEXT;
					break;

				case GID_DOCUMENT:
					type = DTTYPE_BOOPSI;
					break;

				case GID_SOUND:
					type = DTTYPE_SOUND;
					break;

				case GID_INSTRUMENT:
					type = DTTYPE_BOOPSI;
					break;

				case GID_MUSIC:
					type = DTTYPE_BOOPSI;
					break;

				case GID_PICTURE:
					type = DTTYPE_IMAGE;
					break;

				case GID_ANIMATION:
					type = DTTYPE_BOOPSI;
					break;

				case GID_MOVIE:
					type = DTTYPE_BOOPSI;
					break;
			}
			ReleaseDataType(dth);
		}
		UnLock(l);
	}
	return (type);
}


STRPTR datatypes_nametype(ULONG type)
{
	switch (type)
	{
		case DTTYPE_HEX:
			return ("binary");

		case DTTYPE_TEXT:
			return ("text");

		case DTTYPE_BOOPSI:
			return ("boopsi");

		case DTTYPE_SOUND:
			return ("sound");

		case DTTYPE_IMAGE:
			return ("picture");

		case DTTYPE_NONE:
			return ("unknown");

		#ifndef DEBUG
		default:
			PDB(("unknown type\n"));
			break;
		#endif
	}
	return (NULL);
}
