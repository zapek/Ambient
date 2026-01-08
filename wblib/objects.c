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
 * $Id: objects.c,v 1.5 2016/06/19 16:33:06 itix Exp $
 */

#include "globals.h"

/* public */
#include <string.h>
#include <dos/dosextens.h>
#include <proto/exec.h>
#include <proto/utility.h>

/* private */
#include "clib/wb_protos.h"
#include "qport.h"
#include "sendrexx.h"


#define DB_OPENWBO 0
#define DB_CLOSEWBO 0
#define DB_MAKEWBOVISIBLE 0


/****** workbench.library/OpenWorkbenchObjectA() **********************
*
* NAME
*   OpenWorkbenchObjectA -- Open a desktop object
*
* SYNOPSIS
*   Success = OpenWorkbenchObjectA(target, taglist)
*
*   BOOL OpenWorkbenchObjectA(CONST_STRPTR, struct TagItem *);
*
* FUNCTION
*   Open a desktop object. This can be file or directory.
*
* INPUTS
*   target   - target file or directory
*   taglist  - parameters
*
* RESULT
*   Success  - TRUE if request was successfully passed to Ambient.
*
* SEE ALSO
*   wbstart.library/WBStartTagList()
*
*****************************************************************************
*
*/
BOOL OpenWorkbenchObjectA(CONST_STRPTR name, struct TagItem *tags)
{
	//"LoadURI URI/U,NEW=NEWWIN/S,RELOAD/S,FORCE/S,BROWSER/N,VIEWID/N"
	STRPTR cmd;
	ULONG cmdlen;
	BOOL rc = FALSE;

	D(OPENWBO,bug("called\n"));

	cmdlen = strlen(name) + 64;

	cmd = AllocTaskPooled(cmdlen);

	if (cmd)
	{
		CONST_STRPTR mode = "";
		CONST_STRPTR view = "";

		if (tags)
		{
			FORTAG(tags)
			{
				case WBOPENA_ArgLock:
					break;
				case WBOPENA_ArgName:
					break;

				case WBOPENA_Show:
					switch(tag->ti_Data)
					{
						case DDFLAGS_SHOWDEFAULT:
							mode = "";
							break;
						case DDFLAGS_SHOWICONS:
							mode = "&mode=icons";
							break;
						case DDFLAGS_SHOWALL:
							mode = "&mode=all";
							break;
							}
					break;

				case WBOPENA_ViewBy:
					switch(tag->ti_Data)
					{
						case DDVM_BYDEFAULT:
							view= "";
							break;
						case DDVM_BYICON:
							view = "&view=icon";
							break;
						case DDVM_BYNAME:
							view = "&view=list";
							break;

						/* loaduri needs an extension for these, and should it be displayed in iconview or listview ? */
						/*
						case DDVM_BYDATE:
						case DDVM_BYSIZE:
						case DDVM_BYTYPE:
						*/
					}
					break;
			}
			NEXTTAG
		}

		NewRawDoFmt("LoadURI \"file://%s?%s%s\"", NULL, cmd, name, view, mode);

		rc = sendrexx(cmd);

		FreeTaskPooled(cmd, cmdlen);
	}

	return (rc);
}


BOOL CloseWorkbenchObjectA(CONST_STRPTR name, struct TagItem *tags)
{
	D(CLOSEWBO,bug("called\n"));
	return (0);
}


BOOL MakeWorkbenchObjectVisibleA(CONST_STRPTR name, struct TagItem *tags)
{
	D(MAKEWBOVISIBLE,bug("called\n"));
	return (0);
}
