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
 * $Id: input.c,v 1.5 2006/04/12 14:01:54 fab Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <proto/input.h>

/* private */
#include "input.h"
#include "iostdreq.h"


struct Library *InputBase;
static struct IOStdReq *io;


ULONG input_init(void)
{
	if ( (io = iostd_open_device("input.device", 0, 0)) )
	{
		InputBase = (struct Library *)io->io_Device;

		return (TRUE);
	}
	return (FALSE);
}


void input_cleanup(void)
{
	if (io)
	{
		iostd_close_device(io);
	}
}


ULONG check_qualifier(ULONG qual)
{
	if (PeekQualifier() & qual)
	{
		return (TRUE);
	}
	return (FALSE);
}
