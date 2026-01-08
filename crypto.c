/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: crypto.c,v 1.5 2006/04/12 14:01:53 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "crypto.h"


STRPTR crypto_decrypt_txt(STRPTR s)
{
	ULONG cnt = 0;
	STRPTR p = s + 1;
	TEXT ch;

	if (*s != 1) /* marker to only process the strings once */
	{
		*s = 1;

		while ( (ch = *p ^ cnt) )
		{
			*p++ = ch;
			cnt = (cnt + 1) & 0xff;
		}
		*p = '\0';
	}
	return (s + 1);
}

