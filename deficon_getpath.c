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
 * $Id: deficon_getpath.c,v 1.4 2007/05/08 19:27:07 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "deficon_getpath.h"

#define DEFICONS_DEFAULTPATH "ENVARC:sys/"

static TEXT deficon_path[ PATH_SIZE ];
static ULONG deficon_path_valid = FALSE;
static ULONG deficon_path_len = 0;

void deficon_updatepath(void)
{
	if (GetVar("DefIcon_Path", deficon_path, sizeof(deficon_path), GVF_GLOBAL_ONLY) == -1)
	{
		strcpy( deficon_path, DEFICONS_DEFAULTPATH );
	}
	else
	{
		if ( strlen( deficon_path ) == 0 )
		{
			strcpy( deficon_path, DEFICONS_DEFAULTPATH );
		}
	}

	deficon_path_len = strlen( deficon_path );

}

STRPTR deficon_getpath(STRPTR buffer, int bufferlen, STRPTR extension)
{
	int len;

	/* initial call */

	#ifdef BUILD_ICONLIB

	/* XXX: I'm not sure notification on envvar will work for iconlib, so just to be on a safe side.. */

	deficon_updatepath();

	#else

	if ( deficon_path_valid == FALSE )
	{
		deficon_updatepath();
		deficon_path_valid = TRUE;
	}

	#endif

	len = deficon_path_len;
	stccpy( buffer, deficon_path, bufferlen);

	if (buffer[len - 1] != '/' && buffer[len - 1] != ':')
	{
		if (len + 1 >= bufferlen)
		{
			buffer[0] = '\0';
			return NULL;
		}
		buffer[len++] = '/';
		buffer[len] = '\0';
	}

	if (extension)
	{
		int extlen = strlen(extension);

		if (len + extlen + 1 > bufferlen)
		{
			buffer[0] = '\0';
			return NULL;
		}
		memcpy(buffer + len, extension, extlen + 1);
	}

	return buffer;
}
