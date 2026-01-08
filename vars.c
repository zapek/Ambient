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
 * $Id: vars.c,v 1.7 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

#include <stddef.h> /* for offsetof() */

/* private */
#include "vars.h"
#include "prefs.h"


struct global_vars *gvars;


ULONG vars_init(void)
{
	if ( (gvars = malloc(sizeof(*gvars))) )
	{
		refresh_vars();

		return (TRUE);
	}
	return (FALSE);
}


void vars_cleanup(void)
{
	if (gvars)
	{
		free(gvars);
	}
}


/*
 * Only supports bool types for now. Maybe I can toy around
 * with typeof() later on if I need more advanced stuff than
 * the var being here or not.
 */
#define DEFVAR(n) { #n , offsetof(struct global_vars, n), sizeof(((struct global_vars *)0)->n) } /* don't touch those spaces! */

static const struct {
	CONST_STRPTR name;
	UWORD        offset;
	UWORD        size;
} vararray[] = {

	DEFVAR(prefsio_partial),
	DEFVAR(prefsio_ignore_crc),
	DEFVAR(nowildstar),
	DEFVAR(audiodev),
	DEFVAR(nocachepretouch),
	DEFVAR(noaltivec),

};
const int number_of_vars = sizeof(vararray) / sizeof(vararray[0]);

void refresh_vars(void)
{
	BPTR dirlock;

	if ( (dirlock = Lock(VARS_PATH, ACCESS_READ)) )
	{
		BPTR olddir;
		int i;

		olddir = CurrentDir(dirlock);

		for (i = 0; i < number_of_vars; i++)
		{
			BPTR lock;
			APTR ptr;
			ULONG val;

			if ( (lock = Lock(vararray[i].name, ACCESS_READ)) )
			{
				UnLock(lock);
			}

			ptr = (APTR) (((IPTR) gvars) + vararray[i].offset);
			val = lock ? TRUE : FALSE;

			switch (vararray[i].size)
			{
				case 1:
					*(UBYTE *)ptr = val;
					break;

				case 2:
					*(UWORD *)ptr = val;
					break;

				case 4:
					*(ULONG *)ptr = val;
					break;
			}
		}

		(void)CurrentDir(olddir);

		UnLock(dirlock);
	}
	else
	{
		/* no VARS_PATH, all vars are 0 */
		memset(gvars, 0, sizeof(*gvars));
	}
}
