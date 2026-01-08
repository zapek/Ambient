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
 * $Id: tooltype.c,v 1.9 2017/07/25 20:24:30 piru Exp $
 */

#include "globals.h"

/* public */
#include <string.h>

/* private */
#include "clib/icon_protos.h"


#define DB_FINDTT 0
#define DB_MATCHTV 0

UBYTE * FindToolType(UBYTE **toolTypeArray, CONST UBYTE *typeName)
{
	D(FINDTT,bug("called to find <%s>\n", typeName ? typeName : (STRPTR)"nothing"));
	if (toolTypeArray && typeName && *typeName)
	{
		while (*toolTypeArray)
		{
			D(FINDTT,bug("comparing: <%s> with <%s>\n", *toolTypeArray, typeName));
			if (!strnicmp(*toolTypeArray, typeName, strlen(typeName)))
			{
				STRPTR p;
				char c;

				D(FINDTT,bug("found!\n"));

				p = *toolTypeArray + strlen(typeName);

				D(FINDTT,bug("toolend <%s>\n",p));

				while (*p == ' ') p++;
			
				D(FINDTT,bug("skipped spaces <%s>\n",p));

				c = *p++;
				if (c == '=')
				{
					D(FINDTT,bug("argstart <%s>\n",p));
					while (*p == ' ') p++;
					D(FINDTT,bug("found (confirmed)! returning <%s>\n", p));
					return (p);
				}
				else if (c == '\0')
				{
					/*
					 * I'm not sure if tooltypes without a '=' are allowed..but let's
					 * show some mercy.
					 */
					p--;
					D(FINDTT,bug("found (confirmed)! returning empty value\n"));
					return (p);
				}
			}
			toolTypeArray++;
		}
	}
	return (NULL);
}


BOOL MatchToolValue(UBYTE *typeString, UBYTE *value)
{
	STRPTR p;

	D(MATCHTV,bug("called\n"));
	if (typeString && *typeString && value && *value)
	{
		if ((p = strchr(typeString, '|')))
		{
			STRPTR q;
			if (!strnicmp(typeString, value, p - (STRPTR)typeString) && value[p - (STRPTR)typeString] == '\0')
			{
				return (TRUE);
			}

			q = ++p;

			while ((p = strchr(q, '|')))
			{
				if (!strnicmp(q, value, p - q) && value[p - q] == '\0')
				{
					return (TRUE);
				}
				q = ++p;
			}
		}
		else
		{
			if (!stricmp(typeString, value))
			{
				return (TRUE);
			}
		}
	}
	return (FALSE);
}
