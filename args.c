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
 * $Id: args.c,v 1.8 2013/10/28 11:06:31 geit Exp $
 */

#include "ambient.h"

/* public */
#include <dos/rdargs.h>
#include <exec/memory.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <strings.h>

/* private */
#include "args.h"

static struct RDArgs *rda, *rdax;

static CONST_STRPTR help = "All your base are belong to us";  /* I guess this is the easter egg. ho-ho */
static CONST_STRPTR templ = "WBSTARTUP/S";

struct arguments args;


/*
 * Starts the argument parsing. Used to handle tooltypes in the
 * past but then I changed my mind :)
 * The complete code is in zvnc.
 */
ULONG args_start(CONST_STRPTR str UNUSED, ULONG len UNUSED)
{
	if ( (rdax = (struct RDArgs *)AllocDosObject(DOS_RDARGS, NULL)) )
	{
		rdax->RDA_ExtHelp = (STRPTR)help;
		if ( (rda = ReadArgs(templ, (LONG *)&args, rdax)) )
		{
			D(ARGS,bug("all fine\n"));
			return (TRUE);
		}

		D(ARGS,bug("returning..\n"));
	}
	return (FALSE);
}


void args_end(void)
{
	if (rda)
	{
		FreeArgs(rda);
	}

	if (rdax)
	{
		FreeDosObject(DOS_RDARGS, rdax);
	}
}
