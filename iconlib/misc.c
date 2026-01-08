/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * © 2001-2004 by David Gerber <zapek@morphos.net>
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
 * $Id: misc.c,v 1.4 2017/07/25 20:24:30 piru Exp $
 */

#include "globals.h"

/* public */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* private */
#include "clib/icon_protos.h"

#define STRINGSIZE 31 /* including NULL terminator */

#define DB_BUMPREV 0

static __inline__ ULONG	str2ul(const STRPTR nptr)
{
	const char *p = nptr, *q;
	ULONG r = 0;
  
	while (*p == ' ') p++;

	q = p;

	for (;;)
	{
		int a;
		if (*q < '0' || *q > '9')
			break;
		a = *q - '0';

		if (r > (0xffffffff - a) / 10 || r * 10 > 0xffffff - a)
		{
			return (-1); /* overflow */
		}
		else
		{
			r = r * 10 + a;
			q++;
		}
  
		if (q == p) /* Not a single number read */
		{
			return -1;
		}
	}
	return (r);
}



STRPTR BumpRevision(UBYTE *newname, UBYTE *oldname)
{
	D(BUMPREV,bug("called\n"));
	if (!strnicmp("copy_", oldname, 5))
	{
		STRPTR p;

		if ((p = strchr(oldname + 5, '_')) && (p - (STRPTR)oldname <= 13))
		{
			if (!strnicmp("of_", oldname + 6, 3) && strlen(oldname) >= 9)
			{
				/* XXX: tofix! */
				snprintf(newname, STRINGSIZE, "copy_2_of_%s", p + 1);
				return (newname);
			}
			else
			{
				char a[10]; /* limit is 8 chars */
				ULONG num;

				stccpy(a, oldname + 5, p - ((STRPTR)oldname + 5) + 1);
				num = str2ul(a); /* XXX: check for -1.. */
				snprintf(newname, STRINGSIZE, "copy_%ld_of_%s", ++num, p + 1);
				return (newname);
			}
		}
	}
	/*
	 * Simply append "copy_of_"
	 */
	snprintf(newname, STRINGSIZE, "copy_of_%s", oldname);
	
	return (newname);
}
