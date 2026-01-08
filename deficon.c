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
 * $Id: deficon.c,v 1.6 2014/07/21 07:26:52 leif Exp $
 */

#include "ambient.h"

#if USE_DEFICONS

/* public */
#include <dos/filehandler.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "deficon_getpath.h"
#include "deficon.h"
#include "file_func.h"
#include "device_func.h"
#include "name.h"
#include "doslistcache.h"

static void kill_digits(STRPTR s)
{
	ASSERT(s);

	while (*s)
	{
		if (*s >= '0' && *s <= '9')
		{
			*s = 'x';
		}
		s++;
	}
}

STRPTR deficon_build_devicename(STRPTR filename)
{
	TEXT path[PATH_SIZE + 64];
	BPTR l = NULL;
	UBYTE devname[64];
	UBYTE *buf;
	struct dlcnode *dn;
	
	THREAD;
	ASSERT(filename);
	
	D(DEFICON, bug("filename %s..\n", filename));
	

	/* note: that 64 is left for the actual filename */
	if (!deficon_getpath(path, PATH_SIZE, "def_"))
	{
		/* would overflow anyway, so don't bother */
		return NULL;
	}
	buf = path + strlen(path);
	
	
	dn = doslistcache_find_dlcdevice_by_volumename(filename);
	if (!dn) {
		D(DEFICON, bug("no legal device available\n"));
		return NULL; // no legal device available
	}	
	strlcpy(devname, dn->name, 64);

	sprintf(buf, "%sdisk.info", devname);
	D(DEFICON, bug("dostype: 0x%x\n", dn->disktype));
	D(DEFICON, bug("computed name: %s\n", path));

	if (!(l = Lock(path, ACCESS_READ)))
	{
		/*
		 * Not found. Try with a more generic name:
		 * (ie. PC2disk -> PCxdisk).
		 */

		kill_digits(buf);
		D(DEFICON, bug("trying %s..\n", path));

		if (!(l = Lock(path, ACCESS_READ)))
		{
			/*
			 * Still not found. Try something like
			 * def_MSD0disk.
			 * XXX: a dostype can be NULL! eg. Barthel's smbfs, ram. handle that
			 */
			UBYTE dt[5];

			dostype_to_str(dn->disktype, dt);
			sprintf(buf, "%sdisk.info", dt);

			D(DEFICON, bug("trying %s..\n", path));

			if (!(l = Lock(path, ACCESS_READ)))
			{
				/*
				 * Gnah, try a more generic like def_MSDxdisk.
				 */
				kill_digits(buf);
				D(DEFICON, bug("trying %s..\n", path));
				 
				if (!(l = Lock(path, ACCESS_READ)))
				{
					BOOL def_disk_tried = FALSE;

					/*
					 * Sigh. Try to detect if it's a floppy.
					 * In that case, get disk.info.
					 */
					if (devname[0] == 'D' && devname[1] == 'F' &&
						 devname[2] >= '0' && devname[2] <= '9' && /* NOTE: catweasel might be mounted as DF4: for example */
						 devname[3] == '\0')
					{
						strcpy(buf, "disk.info");

						D(DEFICON, bug("oh, it's a floppy disk, trying %s..\n", path));
						if ( (l = Lock(path , ACCESS_READ)) )
						{
							goto skip;
						}
						def_disk_tried = TRUE;
					}

					/*
					 * Sigh. Try def_device.info..
					 */
					strcpy(buf, "device.info");

					D(DEFICON, bug("trying %s..\n", path));

					if (!(l = Lock(path, ACCESS_READ)))
					{
						/*
						 * Damn. What a crappy system. Try def_disk.info
						 * then give up.
						 */
						if (!def_disk_tried)
						{
							strcpy(buf, "disk.info");

							D(DEFICON, bug("holy fuck, trying %s..\n", path));
							l = Lock(path, ACCESS_READ);
						}
					}
				}
			}
		}
	}

	skip:

	if (l)
	{
		/* nice buffer re-use here */
		if (NameFromLock(l, path, sizeof(path)))
		{
			UnLock( l );

			return name_build(path);
		}
		else
		{
			UnLock( l );
		}

		/* XXX */
	}

	return (NULL);
}


void deficon_delete_devicename(STRPTR s)
{
	name_delete(s);
}

#endif /* USE_DEFICONS */
