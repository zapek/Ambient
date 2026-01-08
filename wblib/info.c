/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2016 Ambient Open Source Team
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
 * $Id: info.c,v 1.4 2016/06/19 16:33:06 itix Exp $
 */

#include "globals.h"

/* public */
#include <string.h>
#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

/* private */
#include "clib/wb_protos.h"
#include "sendrexx.h"

#define DB_WBINFO 0


/****** workbench.library/WBInfo() ************************************
*
* NAME
*   WBInfo -- Display an icon information dialog
*
* SYNOPSIS
*   Success = WBInfo(lock, target, screen)
*
*   BOOL WBInfo(BPTR, CONST_STRPTR, struct Screen *);
*
* FUNCTION
*   Display an icon information dialog for the specified target.
*
* INPUTS
*   lock     - a lock to a parent directory
*   target   - name of file or directory
*   screen   - not used
*
* RESULT
*   Success  - TRUE if request was successfully passed to Ambient.
*
*****************************************************************************
*
*/
/* Bugs: screen pointer is currently discarded */
void WBInfo(BPTR lock, CONST_STRPTR name, struct Screen *screen)
{
	/* "IconInfo PATH,WAIT/S" */
	STRPTR path;
	ULONG pathlen;

	D(WBINFO,bug("called\n"));

	pathlen = 256;

	while ((path = AllocVecTaskPooled(pathlen)))
	{
		if (NameFromLock(lock, path, pathlen))
			break;

		FreeVecTaskPooled(path);
		pathlen += 256;
		path = NULL;

		if (IoErr() != ERROR_LINE_TOO_LONG)
			break;
	}

	if (path)
	{
		STRPTR s;

		pathlen += strlen(name) + 1;

		s = AllocVecTaskPooled(pathlen + 16);

		if (s)
		{
			strcpy(s, "IconInfo PATH=");
			strcpy(&s[14], path);
			AddPart(&s[14], name, pathlen);
			sendrexx(s);
			FreeVecTaskPooled(s);
		}

		FreeVecTaskPooled(path);
	}
}
