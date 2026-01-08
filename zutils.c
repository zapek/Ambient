/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2007 Ambient Open Source Team
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
 * $Id: zutils.c,v 1.2 2013/10/28 11:52:07 geit Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "zutils.h"

#if (USE_GLOWICONS32 || USE_PNGICONS) && !BUILD_ICONLIB
void *z_alloc(void *p UNUSED, int items, int size)
{
	return AllocVecTaskPooled(items * size);
}

void z_free(void *p UNUSED, void *addr)
{
	if (addr)
		FreeVecTaskPooled(addr);
}
#endif
