/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2006 Ambient Open Source Team
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
 * $Id: backdrop.c,v 1.8 2020/08/16 03:15:18 jacadcaps Exp $
 */

#include "ambient.h"

#if USE_DOTBACKDROP

/* public */
#include <proto/asyncio.h>
#include <proto/dos.h>

/* private */
#include "backdrop.h"
#include "methodstack.h"
#include "iconio.h"
#include "mui_func.h"
#include "name.h"


#define IOBUFFERSIZE 8192 /* a bit big but who cares.. */

/*
 * devicename has no ':' at the end.
 */
ULONG backdrop_load(APTR obj, STRPTR devicename)
{
	struct AsyncFile *af;
	STRPTR fullname;
	ULONG devnamelen;
	ULONG rc = FALSE; /* this indicates if an icon was added */

	THREAD;
	CHECKOBJECT(obj);
	ASSERT(devicename);

	devnamelen = strlen(devicename);

	if (fullname = malloc(devnamelen + 11)) /* ":.backdrop" + '\0' */
	{
		strcpy(fullname, devicename);
		strcat(fullname, ":.backdrop");

		if (af = OpenAsync(fullname, MODE_READ, IOBUFFERSIZE))
		{
			TEXT buf[PATH_SIZE];
			APTR o;
			BPTR l;

			while (ReadLineAsync(af, &buf, PATH_SIZE - devnamelen - 5) > 0) /* ".info" */
			{
				if (buf[0] == ':')
				{
					STRPTR nn;
					ULONG len = strlen(buf) - 1;

					if (buf[len] == '\n')
					{
						buf[len] = '\0'; /* nuke the '\n' */
					}

					strins(buf, devicename);
					
					if (nn = name_build_info(buf))
					{
						if (l = Lock(nn, ACCESS_READ))
						{
							strcat(buf, ".info");

							/*
							 * Do not add the icon if it already exists.
							 */
							if (!methodstack_push_sync(obj, 2, MM_Iconview_FindShortcut, buf))
							{
								ULONG viewid;

								rc = TRUE;

								methodstack_push_sync(obj, 3, OM_GET, MA_Viewgroup_ID, &viewid);

								if (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, viewid))
								{
									if (read_icon(buf, o, NULL, NULL, RIF_APPLY_DEFAULT)) /* they can be snapshoted so.. no RIF_NOSNAPSHOT */
									{
										methodstack_push(o, 3, MUIM_Set, MA_Icon_IsShortcut, TRUE);
										methodstack_push_sync(obj, 3, MM_Iconview_AddIcon, o, FALSE);
									}
									else
									{
										methodstack_push(o, 1, OM_RELEASE);
									}
								}
								else
								{
									/* XXX */
								}
							}
							UnLock(l);
						}
						name_delete(nn);
					}
				}
			}
			CloseAsync(af);
		}
	}
	/* XXX */
	return (rc);
}

#endif /* USE_DOTBACKDROP */
