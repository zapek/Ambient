/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: datatypeicon.c,v 1.8 2014/01/13 18:10:55 rzookol Exp $
 */

#include "ambient.h"

#if USE_DTICONS

/* public */
#include <intuition/screens.h>
#include <datatypes/pictureclass.h>

/* private */
#include "datatypeicon.h"
#include "datatypes_picture.h"
#include "common_picture.h"
#include "mui_func.h"
#include "methodstack.h"
#include "iconio.h"


ULONG datatypeicon_read(STRPTR filename, APTR obj, ULONG end, struct Screen *scr UNUSED)
{
	struct datatype_picture *dtp;
	ULONG retval = FALSE;

	THREAD;

	if ( (dtp = datatypes_picture_create(filename,
		TAG_DONE)) )
	{
		APTR bm = datatypes_picture_clone_bm(dtp);

		if (bm)
		{
			methodstack_push(obj, 4,
				MM_Icon_AddBitMap,
				bm,
				MV_Icon_BitMap_DTicon,
				MV_Icon_BitMap_Normal
			);

			if (end)
			{
				methodstack_push_sync(obj, 1,
					MM_Icon_End
				);
			}

			retval = TRUE;
		}

		picture_delete(dtp);
	}
	return (retval);
}

#endif /* USE_DTICONS */
