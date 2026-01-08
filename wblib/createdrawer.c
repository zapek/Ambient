/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2016 Ambient Open Source Team
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
 * $Id: createdrawer.c,v 1.8 2016/08/30 10:09:39 geit Exp $
 */

#include "globals.h"
#include <string.h>

/* public */
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

/* private */
#include "clib/wb_protos.h"
#include "icondata.h"

#ifndef WBCREATEDRAWER_NoIcons
#define WBCREATEDRAWER_NoIcons          (WBA_Dummy + 81)
#define WBCREATEDRAWER_CreateHook       (WBA_Dummy + 82)
#define WBCREATEDRAWER_ErrorHook        (WBA_Dummy + 83)
#endif


#define DEFICONS_DEFAULTPATH "ENVARC:sys/"

struct Data
{
	struct Hook *errorhook;
	struct Hook *createhook;
};

BOOL LIB_CreateDrawerA(CONST_STRPTR drawer, struct TagItem *tags);

static CONST_STRPTR get_path_component(CONST_STRPTR path, LONG depth)
{
	CONST_STRPTR s = path;
	STRPTR p = NULL;
	int len = 0;

	for (;;)
	{
		s = strchr(s, '/');

		if (!s || depth <= 0)
			break;

		s++;
		depth--;
	}

	if (s)
	{
		len = (size_t)s - (size_t)path + 1;
	}
	else if (depth == 0)
	{
		len = strlen(path) + 1;
	}

	if (len > 1)
	{
		p = AllocVec(len, MEMF_ANY);

		if (p)
		{
			stccpy(p, path, len);
		}
	}

	return p;
}

static BOOL copy_icon(CONST_STRPTR target)
{
	BPTR fh = Open(target, MODE_NEWFILE);

	if (fh)
	{
		Write(fh, (APTR)default_icon, sizeof(default_icon));
		Close(fh);
	}

	return fh ? TRUE : FALSE;
}

static STRPTR name_build_info(CONST_STRPTR path)
{
	STRPTR p = AllocVec(strlen(path) + sizeof(".info"), MEMF_ANY);

	if (p)
	{
		NewRawDoFmt("%s.info", NULL, p, path);
	}

	return p;
}

static VOID create_icon(struct Data *data, CONST_STRPTR path, LONG overwrite)
{
	STRPTR icon;

	if ((icon = name_build_info(path)))
	{
		BPTR lock;

		if (!overwrite && (lock = Lock(icon, ACCESS_READ)))
		{
			UnLock(lock);
		}
		else
		{
			if (copy_icon(icon) && data->createhook)
				CallHookPkt(data->createhook, (APTR)icon, NULL);
		}

		FreeVec(icon);
	}
}

static BOOL createdir(struct Data *data, CONST_STRPTR path, ULONG createicon)
{
	BOOL rc = FALSE;

	for (;;)
	{
		BPTR l;

		if ((l = CreateDir(path)))
		{
			UnLock(l);

			if (data->createhook)
				CallHookPkt(data->createhook, (APTR)path, NULL);

			if (createicon)
				create_icon(data, path, TRUE);

			rc = TRUE;
		}
		else
		{
			LONG ioerr = IoErr();

			if (ioerr == ERROR_OBJECT_EXISTS)
			{
				if (createicon)
					create_icon(data, path, FALSE);

				rc = TRUE;
				break;
			}

			if (data->errorhook && CallHookPkt(data->errorhook, (APTR)path, NULL))
			{
				continue;
			}
		}

		break;
	}

	return rc;
}

/****** workbench.library/CreateDrawerA() ***********************************
*
* NAME
*   CreateDrawerA -- Create new drawer
*
* SYNOPSIS
*   Success = CreateDrawerA(drawer, taglist)
*
*   BOOL CreateDrawerA(CONST_STRPTR, struct TagItem *);
*
* FUNCTION
*   Creates all the drawers in a specified path.
*
* INPUTS
*   drawer   - the drawer to create
*   taglist  - parameters
*
* TAGS
*   WBCREATEDRAWER_CreateIcon - Create drawers with icon, defaults to TRUE
*   WBCREATEDRAWER_IgnoreFile - ignores a file name component in drawer
*                               name, default is FALSE
*
* RESULT
*   Success  - TRUE drawers were created successfully, FALSE otherwise.
*
*****************************************************************************
*
*/
BOOL LIB_CreateDrawerA(CONST_STRPTR drawer, struct TagItem *tags)
{
	struct TagItem *tstate = tags, *tag;
	struct Data _data = { NULL, NULL }, *data = &_data;
	BOOL rc = TRUE, create_icon = TRUE, ignorefile = FALSE;
	int depth = 0;

	while ((tag = NextTagItem(&tstate)))
	{
		switch (tag->ti_Tag)
		{
			case WBCREATEDRAWER_CreateIcon: create_icon = tag->ti_Data; break;
			case WBCREATEDRAWER_IgnoreFile: ignorefile = tag->ti_Data; break;
			case WBCREATEDRAWER_ErrorHook : data->errorhook = (APTR)tag->ti_Data; break;
			case WBCREATEDRAWER_CreateHook: data->createhook = (APTR)tag->ti_Data; break;
		}
	}

	do
	{
		CONST_STRPTR dir = get_path_component(drawer, depth);

		if (!dir)
			break;
		/* if next component fails and we should ignore name,
		   this is the name to ignore, so break */
		if ( ignorefile ) {
			CONST_STRPTR name = get_path_component(drawer, depth + 1);
			if( !name ) {
				break;
			} else {
				FreeVec((APTR)name);
			}
		}

		depth++;

		rc = createdir(data, dir, create_icon);
		FreeVec((APTR)dir);
	}
	while (rc);

	return rc;
}
