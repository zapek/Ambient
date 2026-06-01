/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: name.c,v 1.1 2026/02/08 13:43:39 kronos Exp $
 */


#warning
#define PATH_SIZE 256          /* maximum path size (including filename + NULL) */
#define NAME_SIZE 108          /* maximum filename size (+ NULL) */
#define VOLUME_SIZE 31         /* maximum volume size (+ NULL) */
#warning

//#include "ambient.h"
#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
/* public */
#include <sys/param.h>
#include <dos/dos.h>
#include <proto/dos.h>

/* private */
#include "name.h"
//#include "deficon_getpath.h"
//#include "doslistcache.h"

//#include "debug.h"

ULONG name_isinfo(CONST_STRPTR name)
{

	ULONG len;

	//ASSERT(name);

	/*  filters out filenames like ".info", it is not an icon file! 
	 */

	len = strlen(name);

	if (len >= 6)
	{
		UBYTE c;

		name += len - 6;
		c    =  name[0];

		if (((c = name[0]) != ':') && (c != '/') && 
		    (name[1] == '.') && 
		    (((c = name[2]) == 'i') || (c == 'I')) && 
		    (((c = name[3]) == 'n') || (c == 'N')) && 
		    (((c = name[4]) == 'f') || (c == 'F')) && 
		    (((c = name[5]) == 'o') || (c == 'O'))
		)
		{
			return (TRUE);
		}
	}
	return (FALSE);
}

#warning
#warning
#warning
#warning
#warning
#warning
#warning
#warning
#warning
#warning
#warning
#warning


APTR name_truncateinfo(STRPTR name)
{
	ULONG len;

	len = strlen(name);

	if (len > 5)
	{
		if (name[len - 5] == '.'
			&& (name[len - 4] == 'i' || name[len - 4] == 'I')
			&& (name[len - 3] == 'n' || name[len - 3] == 'N')
			&& (name[len - 2] == 'f' || name[len - 2] == 'F')
			&& (name[len - 1] == 'o' || name[len - 1] == 'O')
		)
		{
			name[ len - 5 ] = '\0';
			return name + len - 5;
		}

	}
	return NULL;
}

void name_restoreinfo( STRPTR name, APTR truncation )
{
	if ( name && truncation )
	{
		name[ (STRPTR)truncation - name ] = '.';
	}
}



APTR name_truncateprefs(STRPTR name)
{
	ULONG len;

	len = strlen(name);

	if (len > 6)
	{
		if (name[len - 6] == '.'
			&& (name[len - 5] == 'p' || name[len - 5] == 'P')
			&& (name[len - 4] == 'r' || name[len - 4] == 'R')
			&& (name[len - 3] == 'e' || name[len - 3] == 'E')
			&& (name[len - 2] == 'f' || name[len - 2] == 'F')
			&& (name[len - 1] == 's' || name[len - 1] == 'S')
		)
		{
			name[ len - 6 ] = '\0';
			return name + len - 6;
		}

	}

	return NULL;
}


void name_restoreprefs( STRPTR name, APTR truncation )
{
	if ( name && truncation )
	{
		name[ (STRPTR)truncation - name ] = '.';
	}
}

