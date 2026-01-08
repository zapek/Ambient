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
 * $Id: z.c,v 1.2 2005/07/04 23:06:12 laire Exp $
 */

#include "globals.h"

/* public */
#include <exec/memory.h>
#include <proto/exec.h>

/* private */
#include "z.h"
#include "zmem.h"
#include "lib.h"


struct ExecBase *SysBase;
APTR z_mempool;

int lib_init(struct ExecBase *SBase)
{
	SysBase = SBase;

	return (TRUE);
}

int lib_open(void)
{
	return ((int)z_mempool = CreatePool(MEMF_ANY | MEMF_SEM_PROTECTED | MEMF_CLEAR, 16384, 8192));
}


void lib_cleanup(void)
{
	if (z_mempool)
	{
		DeletePool(z_mempool);
	}
}

