/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: diskobject.c,v 1.9 2025/09/03 15:18:42 piru Exp $
 */

#include "globals.h"

/* public */
#include <string.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/wb.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>

/* private */
#include <macros/vapor.h>
#include "icon_internal.h"
#include "clib/icon_protos.h"
#include "freelist.h"
#include "default.h"
#include "pngimage.h"
#if USE_ICONLIB_PNG
#include "pngio.h"
#endif
#include "sxmlc.h"
#include "sxmlsearch.h"
#include "sxmlhelp.h"


#define DB_DELDO 0
#define DB_GETDO 0
#define DB_GETDEFDO 0
#define DB_GETDONEW 0
#define DB_FREEDO 0
#define DB_PUTDO 0
#define DB_PUTDEFDO 0


#if USE_ICONLIB_JUMPTABLE
#ifndef __PPCINLINE_MACROS_H
#include <ppcinline/macros.h>
#endif

extern struct Library *IconBaseInt;

#define GetIcon(__p0, __p1, __p2) \
	LP3(42, BOOL , GetIcon, \
		STRPTR , __p0, a0, \
		struct DiskObject *, __p1, a1, \
		struct FreeList *, __p2, a2, \
		, IconBaseInt, 0, 0, 0, 0, 0, 0)

#define PutIcon(__p0, __p1) \
	LP2(48, BOOL , PutIcon, \
		STRPTR , __p0, a0, \
		struct DiskObject *, __p1, a1, \
		, IconBaseInt, 0, 0, 0, 0, 0, 0)
#endif

/* hackish :) */
void UpdateWorkbench(STRPTR name, BPTR lock, LONG flag); /* shrug */
#define UpdateWorkbench(name, lock, flag) \
	LP3NR(0x1e, UpdateWorkbench, STRPTR, name, a0, BPTR, lock, a1, LONG, flag, d0, \
	, WorkbenchBase, IF_CACHEFLUSHALL, NULL, 0, IF_CACHEFLUSHALL, NULL, 0)


static STRPTR getdeficon(STRPTR name, int namelen, CONST_STRPTR defname)
{
	int len;
	int deflen;

	/*
	 * I'm not sure if it is smart to have this GetVar() call per
	 * every GetDefDiskObject/PutDefDiskObject call. Notify grabbing
	 * the name to a buffer would be more sensible. - piru
	 */

	if (GetVar("DefIcon_Path", name, namelen, GVF_GLOBAL_ONLY) == -1 ||
	    name[0] == '\0')
	{
		if (namelen < 12)
		{
			D(PUTDEFDO,bug("overflow, return error\n"));
			return NULL;
		}
		strcpy(name, "ENVARC:sys/");
		len = 11;
	}
	else
	{
		len = strlen(name);

		if (name[len - 1] != '/' && name[len - 1] != ':')
		{
			if (len + 1 >= namelen)
			{
				return NULL;
			}

			name[len++] = '/';
			name[len] = '\0';
		}
	}

	deflen = strlen(defname);
	if (len + 4 + deflen + 1 <= namelen)
	{
		memcpy(name + len, "def_", 4); len += 4;
		memcpy(name + len, defname, deflen + 1);

		D(PUTDEFDO,bug("iconname <%s>\n", name));
		return name;
	}

	D(PUTDEFDO,bug("overflow, return error\n"));
	return NULL;
}

BOOL DeleteDiskObject(UBYTE *name)
{
	STRPTR iconname;
	LONG success = FALSE;

	D(DELDO,bug("called for name %s\n", name));

	if ((iconname = AllocVecTaskPooled(strlen(name) + 6)))
	{
		BPTR fl, dl = NULL;
		ULONG ioerr = 0;
		ULONG success;

		strcpy(iconname, name);
		strcat(iconname, ".info");

		if ((fl = Lock(iconname, ACCESS_READ)))
		{
			dl = ParentDir(fl);
			UnLock(fl);
		}

		if (!(success = DeleteFile(iconname)))
		{
			ioerr = IoErr();
		}
		D(DELDO,bug("Deleted: %s, %ld, %ld\n", iconname, success, ioerr));

		if (success || (ioerr == ERROR_OBJECT_NOT_FOUND) || dl)
		{
			/*
			 * Notify workbench when the file got deleted.
			 * If the file wasn't there, someone else deleted
			 * it so notify anyway.
			 * XXX: don't do that under Ambient.. useless
			 */
			UpdateWorkbench(iconname, dl, FALSE);
		}
		FreeVecTaskPooled(iconname);
		if (dl)
		{
			UnLock(dl);
		}
	}
	return (success ? FALSE : TRUE);
}


struct DiskObject * GetDiskObject(CONST UBYTE *name)
{
	struct OwnDiskObject *diskobj;

	D(GETDO,bug("called for name: %s\n", name ? name : (STRPTR)"NULL"));

	if ((diskobj = AllocVec(sizeof(*diskobj), MEMF_ANY | MEMF_CLEAR)))
	{
		D(GETDO,bug("diskobj 0x%lx\n", (ULONG)diskobj));
		#if USE_ICONLIB_PNG
		diskobj->ownmagic = OWN_MAGIC;
		diskobj->ownptr = diskobj;
		/* MEMF_CLEAR */
		#endif

		if ((diskobj->fl = create_freelist(NULL)))
		{
			D(GETDO,bug("diskobj's fl 0x%lx\n", (ULONG)diskobj->fl));

			NEWLIST(&diskobj->tooltypelist);
			diskobj->origflags = ~0;
			diskobj->origviewmodes = ~0;

			if (name)
			{
				if (GetIcon((STRPTR) name, (struct DiskObject *) diskobj, diskobj->fl))
				{
					D(GETDO,bug("success\n"));
					return ((struct DiskObject *)diskobj);
				}
				else
				{
					D(GETDO,bug("failed\n"));
				}
			}
			else
			{
				D(GETDO,bug("no name..return empty diskobj\n"));
				return ((struct DiskObject *)diskobj);
			}
			FreeDiskObject((struct DiskObject *)diskobj);
		}
		else
		{
			FreeVec(diskobj);
		}
	}
	return (NULL);
}


struct deftype {
	CONST_STRPTR name;
	struct KnownDiskObject *defobj;
};

const struct deftype defnames[] = {
	{"Disk",     &defdisk},     /* WBDISK */
	{"Drawer",   &defdrawer},   /* WBDRAWER */
	{"Tool",     &deftool},     /* WBTOOL */
	{"Project",  &defproject},  /* WBPROJECT */
	{"Trashcan", &deftrashcan}, /* WBGARBAGE */
	{"Device",   &defdevice},   /* WBDEVICE */
	{"Kick",     &defkick},     /* WBKICK */
};


struct DiskObject * GetDefDiskObject(LONG type)
{
	D(GETDEFDO,bug("called for type: %ld\n", type));

	if (type >= WBDISK && type < WBAPPICON)
	{
		TEXT name[256];

		if (getdeficon(name, sizeof(name), defnames[type - 1].name))
		{
			struct DiskObject *diskobj;

			if ((diskobj = GetDiskObject(name)))
			{
				return (diskobj);
			}
		}

		/*
		 * Return built-in default.
		 */
		D(GETDEFDO,bug("returning built-in default..\n"));
		return ((struct DiskObject *)defnames[type - 1].defobj);
	}
	else
	{
		SetIoErr(ERROR_OBJECT_WRONG_TYPE);
	}
	return (NULL);
}


struct DiskObject * GetDiskObjectNew(UBYTE *name)
{
	struct DiskObject *diskobj;

	D(GETDONEW,bug("called with name: %s\n", name ? name : (STRPTR)"NULL"));

	if ((diskobj = GetDiskObject(name)))
	{
		D(GETDONEW,bug("diskobj 0x%lx\n", (ULONG)diskobj));
		return (diskobj);
	}
	return (GetDefDiskObject(WBTOOL));
}


void FreeDiskObject(struct DiskObject *diskobj)
{
	D(FREEDO,bug("called diskobj 0x%lx\n", (ULONG)diskobj));
	if (diskobj)
	{
		#if USE_ICONLIB_PNG || USE_ICONLIB_SVG
		struct OwnDiskObject *odo = (struct OwnDiskObject *)diskobj;
		if (ISOWN(odo))
		{
			#if USE_ICONLIB_PNG
			/* better make sure */
			odo->ownmagic = 0;
			odo->ownptr = 0;

			if (odo->png_context)
			{
				pngio_delete(odo->png_context);
			}
			#if USE_ICONLIB_PNGLIB
			pngimage_delete(odo);
			#endif
			#endif

			#if USE_ICONLIB_SVG
			if(odo->svgdoc)
			{
				XMLDoc_free(odo->svgdoc);
				FreeMem(odo->svgdoc, sizeof(XMLDoc));
			}
			#endif
		}
		#endif
		if (((struct OwnDiskObject *)diskobj)->fl) /* we must not free default icons */
		{
			FreeFreeList(((struct OwnDiskObject *)diskobj)->fl);
			FreeMem(((struct OwnDiskObject *)diskobj)->fl, sizeof(struct FreeList));
			FreeVec(diskobj);
		}
	}
}


BOOL PutDiskObject(CONST UBYTE *name, struct DiskObject *diskobj)
{
	D(PUTDO,bug("called for %s\n", name));

	return (PutIcon((STRPTR) name, diskobj));
}


BOOL PutDefDiskObject(struct DiskObject *diskObject)
{
	D(PUTDEFDO,bug("called\n"));

	if (diskObject && diskObject->do_Type >= WBDISK && diskObject->do_Type <= WBAPPICON)
	{
		TEXT name[256];

		if (getdeficon(name, sizeof(name), defnames[diskObject->do_Type-1].name))
		{
			return (PutDiskObject(name, diskObject));
		}
	}
	return (FALSE);
}

