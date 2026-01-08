/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005 Ambient Open Source Team
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
 * $Id: str.c,v 1.7 2008/01/12 14:45:06 fab Exp $
 */

#include "ambient.h"

/* public */
#include <ctype.h>

/* private */
#include "str.h"


const UBYTE _hctodarray[256] =
{
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  0,  0,  0,  0,  0,  0,
	0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0, 10, 11, 12, 13, 14, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};


STRPTR stristr(CONST_STRPTR str1, CONST_STRPTR str2)
{
	ULONG len = strlen(str2);

	while (*str1)
	{
		if (! strnicmp(str1, str2, len))
			return (char *) str1;

		str1++;
	}

	return NULL;
}


/*   valid text data includes linefeeds ('\r', '\n') and whitespaces ('\t', ' ')
 */
const UBYTE _validtextdata[256] =
{
	0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  1,  0,  0, /* tab, \n, \r */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* space - /   */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* 0     - ?   */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* @     - O   */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* P     - _   */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* `     - o   */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0, /* p     - ~   */
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* nbsp  - ... */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* ...         */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* ...         */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* ...         */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1, /* ...         */
	1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1  /* ...   - ÿ   */
};


/*  text parsing support functions
 */

/*  moves to first non whitespace character in s
 */
STRPTR strpassws(STRPTR s)
{
	for (; s[0] == ' ' || s[0] == '\t'; s++);

	return s;
}

/*  removes trailing junk (sets new NUL after last non junk character).
 */
void strterminate(STRPTR s)
{
	ULONG l;
	STRPTR e;
	UBYTE c;

	l = strlen(s);
	e = s + l;

	while ((e > s) &&
		   (((c=e[-1]) == '\n') || (c == '\r') || (c == ' ') || (c == '\t')))
	{
		e--;
	}

	*e = '\0';
}

/* this function is meant to escape strings to be passed to printf & co.
*/
ULONG strescape(CONST_STRPTR s, STRPTR out)
{
	UBYTE c;

	if (!s)
		return 0;

	if (out)
	{
		STRPTR o = out;

		while ((c = *s++))
		{
			if (c == '%')
			{
				*o++ = '%';
				*o++ = '%';
			}
			else
			{
				*o++ = c;
			}
		}

		*o = '\0';

		return o - out; /* return length of encoded string */
	}
	else
	{
		/*  If NULL was passed as second parameter then we just
		 *  calculate the length of the encoded URI. Useful to get
		 *  right size for dynamic allocations first.
		 */
		ULONG newlen = 0;

		while ((c = *s++))
		{
			if (c == '%')
			{
				newlen += 2;
			}
			else
			{
				newlen++;
			}
		}

		return newlen;
	}
}


/*  functions to encode and decode a string, so it can safely used
 *  as parameter in URIs w/o further escaping of special characters
 */


CONST TEXT __hex[] = "0123456789ABCDEF";

static CONST UBYTE  __uriencoded[] =
{
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, /* - .   */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, /* 0-9   */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A-O   */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, /* P-Z _ */
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* a-o   */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, /* p-z   */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};


ULONG uri_encode(CONST_STRPTR s, STRPTR out)
{
	UBYTE c;

	if (!s)
		return 0;

	if (out)
	{
		STRPTR o = out;

		while ((c = *s++))
		{
			if (c == ' ')
			{
				*o++ = '+';
			}
			else if (__uriencoded[c])
			{
				*o++ = '%';
				*o++ = __hex[c >> 0x4];
				*o++ = __hex[c &  0xF];
			}
			else
			{
				*o++ = c;
			}
		}

		*o = '\0';

		return o - out; /* return length of encoded string */
	}
	else
	{
		/*  If NULL was passed as second parameter then we just
		 *  calculate the length of the encoded URI. Useful to get
		 *  right size for dynamic allocations first.
		 */
		ULONG newlen = 0;

		while ((c = *s++))
		{
			if (__uriencoded[c])
			{
				newlen += 3;
			}
			else
			{
				newlen++;
			}
		}

		return newlen;
	}
}


ULONG uri_decode(CONST_STRPTR s, STRPTR out)
{
	UBYTE  c, h1, h2;
	STRPTR o = out;

	if (!(s && out))
		return 0;

	while ((c = *s))
	{
		if (c == '+')
		{
			*o++ = ' ';
		}
		else if ((c == '%')    &&
				 (h1 = *(s+1)) &&
				 (h2 = *(s+2)) &&
				 isxdigit(h1)  &&
				 isxdigit(h2))
		{
			#if 1
			*o++ = (_hctodarray[h1] * 16) + _hctodarray[h2];
			#else
			*o++ = ((((h1 >= '0') && (h1 <= '9')) ? (h1 - '0') : (tolower(h1) - 'a' + 10) * 16)) +
				   ((((h2 >= '0') && (h2 <= '9')) ? (h2 - '0') : (tolower(h2) - 'a' + 10)));
			#endif

			s += 2;
		}
		else
		{
			*o++ = c;
		}

		s++;
	}

	*o = '\0';

	return o - out; /* return length of decoded string */
}

