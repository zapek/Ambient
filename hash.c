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
 * $Id: hash.c,v 1.7 2008/04/02 20:24:20 kiero Exp $
 */

#include "ambient.h"

/* public */
#ifdef PRINT_HASH
#include <stdio.h>
#endif

/* private */
#include "hash.h"


static __inline__ ULONG hashadd(ULONG h, TEXT c)
{
	h += (h << 5); /* multiply by 33 */
	return (h ^ c);
}


/*
 * String hashing function (taken from Bernstein's cdb).
 */
ULONG hash(CONST_STRPTR s)
{
	ULONG h = 5381;
	TEXT c;

	ASSERT(s);

	while ( (c = *s++) )
	{
		h = hashadd(h, c);
	}
	return (h);
}


ULONG hash_nocase(CONST_STRPTR s)
{
	ULONG h = 5381;
	TEXT c;

	ASSERT(s);

	while ( (c = *s++) )
	{
		h = hashadd(h, (TEXT)toupper(c));
	}
	return (h);
}   

ULONG hash_nocase_seconds(CONST_STRPTR s, ULONG seconds)
{
	ULONG h = hash_nocase( s );

	h = hashadd(h, (TEXT)seconds>>8);
	h = hashadd(h, (TEXT)seconds);
	return (h);
}


#ifdef PRINT_HASH
int main(int argc, char **argv)
{
	if (argc == 2)
	{
		printf("hash for <%s> is %p\n", argv[1], hash(argv[1]));
	}
	return (0);
}
#endif

/*
 * String hashing function (taken from sdbm)
 */
#if 0
ULONG sdbm_hash(CONST_STRPTR s)
{
	ULONG h = 0;
	int c;

	ASSERT(s);

	while (c = *s++)
	{
		h = c + (h << 6) + (h << 16) - h;
	}
	return (h);
}
#endif
