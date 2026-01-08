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
 * $Id: loadimage.c,v 1.14 2015/08/15 08:55:12 itix Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "loadimage.h"
#include "datatypes_picture.h"
#include "reggae_picture.h"
#include "common_picture.h"
#include "mui_func.h"
#include "methodstack.h"
#include "gfx_scale.h"
#include "gfx_bitmap.h"
#include "threads.h"
#include "mimeuri.h"

enum {LI_MULTIMEDIA, LI_DATATYPES};

ULONG tr_loadimage(APTR obj, CONST_STRPTR path)
{
	ULONG rc = FALSE;
	
	THREAD;
	CHECKOBJECT(obj);

	if (path && *path)
	{
		APTR dtp = NULL;
		LONG err = 0;
		APTR mctx;
		LONG mode = LI_MULTIMEDIA;
		
		if ((mctx = mimeuri_create()))
		{
			if (mimeuri_gather(mctx, path, TAG_DONE))
			{
				STRPTR s = mimeuri_getattr(mctx, MIMEURIATTR_MIMETYPE);
				bug ("mimeuri_getattr= %s\n", s);
				if (!strcmp(s, MIMETYPE_INTERNAL_MULTIMEDIA))
				{
					mode = LI_MULTIMEDIA;
				}
				else
				{
					mode = LI_DATATYPES;
				}
			
			}
			mimeuri_delete(mctx);
		}
		
		if (mode == LI_DATATYPES)
		{
			dtp = datatypes_picture_create(path,
				PICTAG_ARGB32, TRUE,
				PICTAG_ErrorPtr, &err,
				TAG_DONE);
		}
		else		
		{
			dtp = reggae_picture_create(path,
				PICTAG_ErrorPtr, &err,
				TAG_DONE);
		}

		if (dtp)
		{
			if (((ULONG)picture_getattr(dtp, PICTURE_WIDTH) < 32768) && ((ULONG)picture_getattr(dtp, PICTURE_HEIGHT) < 32768))
			{
				rc = TRUE;
				methodstack_push_sync(obj, 2, MM_Imageview_AddImage, dtp, 0);
			}
			else
			{
				/* XXX: picture too big.. tell so */
				picture_delete(dtp);
			}
		}
		else
		{
			methodstack_push_sync(obj, 2, MM_Imageview_AddError, err);
		}
	}
	return threads_check_abort() ? ABORTED : rc;
}


ULONG tr_scaleimage(APTR obj, APTR dtp, ULONG width, ULONG height)
{
	struct BitMap *bm;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(dtp);
	ASSERT(width);
	ASSERT(height);

	if ( (bm = gfx_bitmap_create(width, height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
	{
		if (gfx_scale(picture_getattr(dtp, PICTURE_BITMAP), bm, width, height,
			SCALETAG_Bilinear, TRUE,
		TAG_DONE))
		{
			methodstack_push_sync(obj, 2, MM_Imageview_AddImageScaled, bm);
			return threads_check_abort() ? ABORTED : TRUE;
		}
		gfx_bitmap_delete(bm);
	}
	return threads_check_abort() ? ABORTED : FALSE;
}

/* XXX: maybe add scaling stuff! */
