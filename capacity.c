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
 * $Id: capacity.c,v 1.10 2012/06/03 12:32:01 piru Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/locale.h>

/* private */
#include "ambient_cat.h"
#include "capacity.h"
#include "locale.h"


void capacity_format_size(STRPTR s, ULONG size, UQUAD n)
{
	ASSERT(s);

	if (n < 1024 * 10)
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_BYTES), (unsigned int)n);
	}
	else if (n < 1024 * 1024)
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_KBYTES), (unsigned int) ( n / 1024 ), (unsigned int)( n % 1024 * 10 / 1024 ));
	}
	else if (n < 1024 * 1024 * 1024)
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_MBYTES), (unsigned int) n / (1024 * 1024), (unsigned int)( n % (1024 * 1024) * 10 / (1024 * 1024) ));
	}
	else if (n < (UQUAD)1024 * 1024 * 1024 * 1024)
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_GBYTES), (unsigned int) ( n / ( 1024 * 1024 * 1024 ) ), (unsigned int) ( n % (1024 * 1024 * 1024) * 10 / (1024 * 1024 * 1024 ) ));
	}
	else
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_TBYTES), (unsigned int) ( n / ( (UQUAD)1024 * 1024 * 1024 * 1024 ) ), (unsigned int)( n % ((UQUAD)1024 * 1024 * 1024 * 1024) * 10 / ((UQUAD)1024 * 1024 * 1024 * 1024) ) );
	}
}

void capacity_format_size_compact(STRPTR s, ULONG size, UQUAD n)
{
	ASSERT(s);

	if (n < 10000)
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_COMPACT_BYTES), (unsigned int)n);
	}
	else if (n < 10000 * 1024)
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_COMPACT_KBYTES), (unsigned int) ( n / 1024 ));
	}
	else if (n < (UQUAD)10000 * 1024 * 1024)
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_COMPACT_MBYTES), (unsigned int) ( n / ( 1024 * 1024) ) );
	}
	else if (n < (UQUAD)10000 * 1024 * 1024 * 1024)
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_COMPACT_GBYTES), (unsigned int) ( n / ( 1024 * 1024 * 1024 ) ) );
	}
	else
	{
		snprintf(s, size, GSI(MSG_CAPACITY_FORMAT_COMPACT_TBYTES), (unsigned int) ( n / ( (UQUAD)1024 * 1024 * 1024 * 1024 ) ) );
	}
}

void capacity_format_size_separated(STRPTR s, ULONG size, UQUAD n)
{
	ASSERT(locale);

	/*  TODO: make use of loc_Grouping too ?
	 */

	*s = 0;

	if (n)
	{
		UQUAD  n2;
		long   rank = 0;
		long   rank2= 0;
		char   buffer[12];
		CONST_STRPTR grpsep = locale->loc_GroupSeparator;

		n2 = n;

		while (n2 / 1000)
		{
			n2 /= 1000;
			rank++;
		}

		rank2 = rank;

		n2 = n;

		while(rank>=0)
		{
			LONG r = rank;
			UQUAD d = 1;

			while(r-->0)
			{
				d *= 1000;
			}

			if(rank == rank2)
			{
				snprintf(buffer, 12, "%llu", (UQUAD) n2 / d);
			}
			else
			{
				snprintf(buffer, 12, "%s%.3llu", grpsep, (UQUAD) n2 / d);
			}
			strncat(s, buffer, size);

			n2 -= n2 / d * d;
			rank--;
		}
	}
}
