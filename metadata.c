/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2012 Ambient Open Source Team
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
 * $Id: metadata.c,v 1.9 2012/05/20 06:29:05 itix Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "file_func.h"
#include "time_func.h"
#include "metadata.h"
#include "mui_func.h"
#include "methodstack.h"
#include "mimeuri.h"
#include "capacity.h"
#include "ambient_cat.h"
#include "mimetype.h"


ULONG tr_metadata_gather(APTR obj, CONST_STRPTR path, CONST_STRPTR descr)
{
	APTR ctx;
	ULONG rc = FALSE;

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(path);

	if ((ctx = mimeuri_create()))
	{
		if (mimeuri_gather(ctx, path,
			MIMEURIGATHERTAG_Recurse, TRUE,
		TAG_DONE))
		{
			/*
			 * That's quite a bit of stack used there. Lets hope this thing
			 * is never called with low stack or we're screwed. - piru
			 */
			TEXT t[4096]; /* should be enough (tm) */
			LONG v;

			if (descr == NULL)
			{
				if (!(descr = mimeuri_getattr(ctx, MIMEURIATTR_MIME_DESCRIPTION)))
				{
					descr = mimeuri_getattr(ctx, MIMEURIATTR_MIMETYPE);
				}
			}

			/* XXX: add a mimetype description field.. really.. */
			v = snprintf(t, sizeof(t), GSI( MSG_METADATA_TYPE ),
				descr ? descr : GSI( MSG_METADATA_UNKNOWN )
			);

			if (v >= 0 && v < sizeof(t) &&
			    mimeuri_getattr(ctx, MIMEURIATTR_MIME_FILEINFO))
			{
				TEXT date[64];
				TEXT time[16];
				struct DateStamp ds;

				seconds_to_datestamp((ULONG)mimeuri_getattr(ctx, MIMEURIATTR_MIME_SECONDS), &ds);

				if (datestamp_to_str(&ds, date, time))
				{
					TEXT size[16];

					capacity_format_size(size, sizeof(size), *(UQUAD*)mimeuri_getattr(ctx, MIMEURIATTR_MIME_FILESIZEPTR));

					snprintf(t + v, sizeof(t) - v, GSI( MSG_METADATA_DATESIZE ),
						date,
						time,
						size
					);
				}
			}

			methodstack_push_sync(obj, 3,
				MM_Metadata_Info,
				t
			);
		}
		mimeuri_delete(ctx);
	}
	/* XXX */
	return (rc);
}
