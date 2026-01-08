/*
 * Ambient - the ultimate desktop
 * ------------------------------
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
 * $Id: createicon.c,v 1.2 2016/08/28 15:07:59 itix Exp $
 */

#include "globals.h"
#include <string.h>

/* public */
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <proto/utility.h>

/* private */
#include "clib/wb_protos.h"
#include "icondata.h"
#include "lib.h"
#include "sendrexx.h"

#define GET_MIMEPATH "GetDefIconPath MIMETYPE=%s"
#define GET_FILEPATH "GetMimeType PATH=\"%s\""

STATIC CONST_STRPTR GetPath(CONST_STRPTR template, int template_length, CONST_STRPTR param)
{
	int cmdlen = template_length + strlen(param);
	STRPTR cmd = AllocMem(cmdlen, MEMF_ANY);
	STRPTR path = NULL;

	if (cmd)
	{
		NewRawDoFmt(template, NULL, cmd, param);
		sendrexxarg(cmd, &path);
		FreeMem(cmd, cmdlen);
	}

	return path;
}


STATIC CONST_STRPTR GetDefIconFromMimeType(CONST_STRPTR mimetype)
{
	return GetPath(GET_MIMEPATH, sizeof(GET_MIMEPATH), mimetype);
}


STATIC CONST_STRPTR GetDefIconFromName(CONST_STRPTR filename)
{
	CONST_STRPTR s = GetPath(GET_FILEPATH, sizeof(GET_FILEPATH), filename);

	if (s)
	{
		s = GetDefIconFromMimeType(s);
	}

	return s;
}


struct icondata
{
	ULONG size;
	UBYTE data[0];
};

static APTR read_data(CONST_STRPTR name)
{
	BPTR fh = Open(name, MODE_OLDFILE);
	struct icondata *ic = NULL;

	if (fh)
	{
		struct FileInfoBlock fib;

		if (ExamineFH(fh, &fib))
		{
			ic = AllocMem(fib.fib_Size + sizeof(*ic), MEMF_ANY);

			if (ic)
			{
				ic->size = fib.fib_Size;

				Read(fh, ic->data, fib.fib_Size);
			}	
		}

		Close(fh);
	}

	return ic;
}


static void free_data(struct icondata *ic)
{
	FreeMem(ic, ic->size + sizeof(*ic));
}


static BOOL write_data(CONST_STRPTR target, CONST_APTR data, LONG datalen)
{
	BPTR fh = Open(target, MODE_NEWFILE);

	if (fh)
	{
		Write(fh, (APTR)data, datalen);
		Close(fh);
	}

	return fh ? TRUE : FALSE;
}

static STRPTR get_filename(CONST_STRPTR s)
{
	char *p = strrchr(s, '.');
	STRPTR buf = NULL;

	if (p && stricmp(p, ".info") == 0)
	{
		int len = strlen(s) - 4;
		buf = AllocVec(len, MEMF_ANY);

		if (buf)
		{
			stccpy(buf, s, len);
		}
	}

	return buf;
}


/****** workbench.library/CreateIconA() *************************************
*
* NAME
*   CreateIconA -- Create new icon
*
* SYNOPSIS
*   Success = CreateIconA(name, taglist)
*
*   BOOL CreateIconA(CONST_STRPTR, struct TagItem *);
*
* FUNCTION
*   Creates all the drawers in a specified path.
*
* INPUTS
*   name     - name of the icon file (with .info)
*   taglist  - parameters
*
* TAGS
*   WBCREATEICON_MimeType   - source mimetype
*   WBCREATEICON_File       - file to get mimetype from
*   WBCREATEICON_DefIcon    - create deficon, default is TRUE
*   WBCREATEICON_AppendInfo - append name with .info, default is FALSE
*
* RESULT
*   Success  - TRUE icon was created successfully, FALSE otherwise.
*
*****************************************************************************
*
*/
BOOL LIB_CreateIconA(CONST_STRPTR name, struct TagItem *tags)
{
	struct TagItem *tstate = tags, *tag;
	BOOL rc = FALSE, deficon = TRUE;
	CONST_STRPTR mimetype = NULL, filename = NULL;
	STRPTR tmpbuf = NULL;

	while ((tag = NextTagItem(&tstate)))
	{
		size_t data = tag->ti_Data;

		switch (tag->ti_Tag)
		{
			case WBCREATEICON_MimeType  : mimetype = (APTR)data; break;
			case WBCREATEICON_File      : filename = (APTR)data; break;
			case WBCREATEICON_DefIcon   : deficon = data; break;

			case WBCREATEICON_AppendInfo:
				tmpbuf = AllocVec(strlen(name) + sizeof(".info"), MEMF_ANY);

				if (!tmpbuf)
					return rc;

				strcpy(tmpbuf, name);
				strcat(tmpbuf, ".info");

				name = tmpbuf;
				break;
		}
	}

	if (!deficon)
	{
		CONST_STRPTR s = NULL;
		STRPTR tmp = NULL;

		if (!filename && !mimetype)
		{
			filename = tmp = get_filename(name);
		}

		if (filename)
			s = GetDefIconFromName(filename);
		else if (mimetype)
			s = GetDefIconFromMimeType(mimetype);

		if (s)
		{
			struct icondata *ic = read_data(s);

			if (ic)
			{
				rc = write_data(name, ic->data, ic->size);
				free_data(ic);
			}

			DeleteArgstring((STRPTR)s);
		}

		FreeVec(tmp);
	}
	else
	{
		rc = write_data(name, default_icon, sizeof(default_icon));
	}

	FreeVec(tmpbuf);

	return rc;
}
