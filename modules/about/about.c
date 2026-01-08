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
 * $Id: about.c,v 1.4 2006/04/12 14:02:00 fab Exp $
 */

#include "globals.h"

/* public */
#include <exec/memory.h>
#include <proto/exec.h>

/* private */
#include "about.h"
#include "lib.h"
#include "aboutclasses.h"


struct ExecBase *SysBase;
struct Library *MUIMasterBase;

int lib_init(struct ExecBase *SBase)
{
	SysBase = SBase;

	return (TRUE);
}


int lib_open(void)
{
	if (!MUIMasterBase)
	{
		if ((MUIMasterBase = OpenLibrary("muimaster.library", 20)))
		{
			if (!create_pongclass()) return (FALSE);

			return (TRUE);
		}
	}

	if (MUIMasterBase)
	{
		return (TRUE);
	}
	else
	{
		return (FALSE);
	}
}


void lib_cleanup(void)
{
	if (MUIMasterBase)
	{
		delete_pongclass();
		CloseLibrary(MUIMasterBase);
	}
}

