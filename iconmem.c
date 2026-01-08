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
 * $Id: iconmem.c,v 1.5 2006/04/12 14:01:54 fab Exp $
 */

#include "ambient.h"

/* public */
#include <clib/alib_protos.h>
#include <exec/memory.h>
#include <proto/exec.h>

/* private */
#include "memtrack.h"
#include "iconmem.h"


/*
 * Icon memory handling subsystem.
 */

#if !USE_MEMTRACK
APTR iconpool;
#endif


/*
 * Called to initialize the memory subsystem.
 */
ULONG iconmem_init(void)
{
	#if USE_MEMTRACK
	return (TRUE);
	#else
	if ((iconpool = CreatePool(MEMF_SEM_PROTECTED | MEMF_ANY, 16384, 8192)))
	{
		return (TRUE);
	}
	return (FALSE);
	#endif
}


/*
 * Called to cleanup the memory subsystem.
 */
void iconmem_cleanup(void)
{
	#if !USE_MEMTRACK
	if (iconpool)
	{
		DeletePool(iconpool);
	}
	#endif
}

