/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: device_func.c,v 1.7 2023/01/11 15:14:30 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <dos/dostags.h>
#include <dos/filehandler.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "device_func.h"

/*
 * Given a path, build a device_info (path can be anything
 * on the device we want).
 * XXX: if the path is some random string, we'll get a lock on SYS: :(
 */
struct device_info * deviceinfo_build(STRPTR path, BOOL fssmLookup)
{
	struct MsgPort *mp;
	ULONG found = FALSE;
	char *tmpPath = NULL;

	THREAD;
	ASSERT(path);
	
	ULONG pathLen = strlen(path);
	if (pathLen == 0)
		return NULL;
	
	if (path[pathLen - 1] != ':' && !strchr(path, ':'))
	{
		tmpPath = malloc(pathLen + 2);
		if (tmpPath)
		{
			stccpy(tmpPath, path, pathLen + 1);
			tmpPath[pathLen] = ':';
			tmpPath[pathLen + 1] = 0;
		}
		else
			return NULL;
	}

	// TODO: this should be using GetDeviceProc
	if ((mp = DeviceProc(tmpPath ? tmpPath : path)))
	{
		struct DosList *dl;
		struct device_info *di;

		if ((di = malloc(sizeof(*di))))
		{
			memset(di, 0, sizeof(*di));

			dl = LockDosList(LDF_DEVICES | LDF_READ);

			while ((dl = NextDosEntry(dl, LDF_DEVICES)))
			{
				if (dl->dol_Task == mp)
				{
					if ((di = malloc(sizeof(*di))))
					{
						STRPTR p;
						
						memset(di, 0, sizeof(*di));

						p = BADDR(dl->dol_Name);
						if (p && p[0] && (di->name = malloc(p[0] + 2))) /* + ":" */
						{
							struct FileSysStartupMsg *fssm;

							stccpy(di->name, &p[1], p[0] + 1);
							strcat(di->name, ":");

							fssm = BADDR(dl->dol_misc.dol_handler.dol_Startup);
							if (fssmLookup && isfssm(fssm))
							{
								struct DosEnvec *denv;

								denv = (struct DosEnvec *)BADDR(fssm->fssm_Environ);
								p = BADDR(fssm->fssm_Device);

								if (p && p[0] && p[1] && (di->devname = malloc(p[0] + 1)))
								{
									memcpy(di->devname, &p[1], p[0]);
									di->devname[p[0]] = '\0';
									di->unit = fssm->fssm_Unit;
									di->flags = fssm->fssm_Flags;
									di->lowcyl = denv->de_LowCyl;
									di->highcyl = denv->de_HighCyl;
									di->surfaces = denv->de_Surfaces;
									di->blocksize = denv->de_SizeBlock * 4;
									di->blockspertrack = denv->de_BlocksPerTrack;
									di->bufmemtype = denv->de_BufMemType;
									di->dostype = denv->de_DosType; /* XXX: is that ok? I hope so.. well, Piru would want to use DiskType if isndos() is false.. */
									found = TRUE;
								}
							}
							else if (!fssmLookup)
							{
								found = TRUE;
							}
						}
					}
					break;
				}
			}
			UnLockDosList(LDF_DEVICES | LDF_READ);

			if (found)
			{
				free(tmpPath);
				return (di);
			}
			deviceinfo_delete(di);
		}
	}
	free(tmpPath);
	return (NULL);
}


void deviceinfo_delete(struct device_info *di)
{
	if (di)
	{
		if (di->name)
		{
			free(di->name);
		}
		if (di->devname)
		{
			free(di->devname);
		}
		free(di);
	}
}


/*
 * Checks if a device has a NDOS disk.
 */
ULONG isndos(STRPTR devname)
{
	BPTR l;
	TEXT t[33];
	int len;

	THREAD;
	ASSERT(devname);

	if (*devname == '\0')
	{
		/* Caller with "" will get a NDOS */
		return TRUE;
	}

	stccpy(t, devname, sizeof(t) - 1);
	len = strlen(t);
	if (t[len - 1] != ':')
	{
		t[len++] = ':';
		t[len] = '\0';
	}
	if ((l = Lock(t, ACCESS_READ)))
	{
		UnLock(l);
	}
	else
	{
		if (IoErr() == ERROR_NOT_A_DOS_DISK)
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


/*
 * Checks if a device has a valid fssm.
 */
ULONG isfssm(APTR fssm)
{
	struct FileSysStartupMsg *myfssm = fssm;

	if (TypeOfMem(myfssm))
	{
		STRPTR devname;

		devname = BADDR(myfssm->fssm_Device);

		if (TypeOfMem(devname))
		{
			struct DosEnvec *denv;

			denv = BADDR(myfssm->fssm_Environ);

			if (TypeOfMem(denv))
			{
				if (!(denv->de_TableSize & 0xffffff00))
				{
					if (!((UBYTE *)myfssm)[0] || !((UBYTE *)devname)[0])
					{
						return (TRUE);
					}
				}
			}
		}
	}
	return (FALSE);
}
