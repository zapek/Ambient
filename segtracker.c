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
 * $Id: segtracker.c,v 1.4 2013/10/28 12:04:05 geit Exp $
 */

#include "ambient.h"

#ifdef DEBUG

/* public */
#include <exec/execbase.h>
#include <dos/segtracker.h>
#include <proto/exec.h>

/* private */
#include "segtracker.h"


/*
 * Unfortunately there's no proper way to know valid memory in
 * MorphOS for such case. Everyone just copies the routine from
 * exec sources and embeds it, which cannot be done for an
 * opensource release. I'm afraid you'll have to figure out that
 * one by yourself.
 */
void showppcstackhistory(ULONG *stack UNUSED, ULONG *stackend UNUSED)
{
	kprintf("\nAmbient PPCStackFrame History:\n");

	kprintf("N/A\n");
}



#endif /* DEBUG */
