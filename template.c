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
 * $Id: template.c,v 1.5 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "template.h"


/*
 * Transforms the 'from' string ito a 'to' string of 'maxlen'
 * expanding the template given as pair argument in the vararg. First argument
 * is the one letter to replace, second argument is the replacement, eg:
 * 'f', fullpath.
 */
#define INCRLEN(x) do { len++; if (len >= maxlen) { x; return (len); } } while (0)

ULONG template_expand(CONST_STRPTR s, STRPTR d, ULONG maxlen, ...)
{
	ULONG cnt = 0;
	LONG ch;
	STRPTR repl;
	ULONG len = 0;

	ASSERT(s);
	ASSERT(d);
	ASSERT(maxlen);

	while (*s)
	{
		if (*s == '%')
		{
			va_list va;

			s++;

			va_start(va, maxlen);
			ch = va_arg(va, LONG);

			while (ch)
			{
				repl = va_arg(va, STRPTR);

				if (ch == *s)
				{
					/* found format char */
					s++;

					if (repl)
					{
						while (*repl)
						{
							*d++ = *repl++;
							INCRLEN(va_end(va););
						}
						break;
					}
				}
				ch = va_arg(va, LONG);
			}

			va_end(va);

			if (!ch)
			{
				/* not found, copy verbatim */
				*d++ = '%';
				*d++ = *s++;
				INCRLEN(;);
			}
		}
		else
		{
			*d++ = *s++;
			cnt++;
			INCRLEN(;);
		}
	}
	*d = '\0';

	return (len);
}

