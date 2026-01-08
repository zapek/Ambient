/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2018 Ambient Open Source Team
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
 * $Id: common_picture.c,v 1.6 2018/08/12 20:53:15 itix Exp $
 */


#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <proto/datatypes.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/multimedia.h>

/* private */
#include "common_picture.h"
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_alpha.h"
#include "gfx_cmap.h"
#include "dos_internal.h"
#include "gfx.h"



APTR picture_getattr(APTR ctx, ULONG attr)
{
	struct common_picture *ct = ctx;

	ASSERT(ct);
	ASSERT(ct->bm);

	switch (attr)
	{
		case PICTURE_BITMAP:
			return (ct->bm);

		case PICTURE_WIDTH:
			return ((APTR)gfx_bitmap_width(ct->bm));

		case PICTURE_HEIGHT:
			return ((APTR)gfx_bitmap_height(ct->bm));

		#ifdef DEBUG
		default:
			PDB(("argh\n"));
			break;
		#endif
	}
	return (NULL);
}


void picture_set_bitmap(APTR ctx, APTR bm)
{
	struct common_picture *ct = ctx;

	ASSERT(ct);

	if (ct->picture_o)
	{
		ASSERT(ct->bm);
	
		if (ct->picture_delete)
			ct->picture_set_bitmap(ctx, bm);
		ct->picture_o = NULL;
	}

	gfx_bitmap_delete(ct->bm);
	ct->bm = bm;
}


void picture_delete(APTR ctx)
{
	struct common_picture *ct = ctx;

	if (!ct)
		return;

	ASSERT(ct);
	ASSERT(ct->bm);

	if (ct->picture_o)
	{
		if (ct->picture_delete)
			ct->picture_delete(ctx);
	}

	gfx_bitmap_delete(ct->bm);
	free(ct);
}


CONST_STRPTR picture_errorstring(LONG err)
{
	CONST_STRPTR s;

	if (err >= DTERROR_UNKNOWN_DATATYPE )
	{
		s = GetDTString(err);
	}
	else if (err >= MMERR_BASE )
	{
		s = MediaFault(err);
	}
	else
	{
		s = DosGetString(err);
	}

	if (s && *s)
	{
		return (s);
	}
	else
	{
		PDB(("unspecified error: %ld\n", err));
		return ("not specified");
	}
}


APTR picture_create(LONG width, LONG height, LONG depth, struct Screen *screen_friend, LONG vmem)
{
	struct common_picture *ct = malloc(sizeof(*ct));

	if (ct != NULL)
	{
		ct->picture_o = NULL;
		ct->type = 0;
		ct->picture_set_bitmap = NULL;
		ct->picture_delete = NULL;
		ct->picture_errorstring = NULL;

		ct->bm = gfx_bitmap_create(width, height, depth,
			BITMAPTAG_ScreenFriend, screen_friend,
			BITMAPTAG_VMem, vmem,
			TAG_DONE);

		if (!ct->bm)
		{
			free(ct);
			ct = NULL;
		}
	}

	return ct;
}

