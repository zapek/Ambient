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
 * $Id: random.c,v 1.4 2006/08/08 13:31:36 fab Exp $
 */

#include "ambient.h"

/* public */
#include <exec/execbase.h>
#include <exec/system.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/timer.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#if USE_RANDOM_LIB
#include <proto/random.h>
#endif

/* private */
#include "random.h"


#if USE_STRONG_RANDOM

#if USE_RANDOM_LIB

ULONG random_init(void)
{
	return (TRUE);
}


void random_cleanup(void)
{
	/* nothing yet */
}


ULONG random_ulong(void)
{
	return (Random());
}


UBYTE random_ubyte(void)
{
	return (RandomByte());
}

#else

#error not available

#endif

#else

static struct SignalSemaphore randsem;

/*
 * Must be called *AFTER* init_timer().
 */
ULONG random_init(void)
{
	#if !USE_LEGACY
	ASSERT(cpu_clock);
	#endif

	InitSemaphore(&randsem);

	srand(time(NULL));

	return (TRUE);
}


void random_cleanup(void)
{
	/* nothing yet */
}


ULONG random_ulong(void)
{
	ULONG rc;

	ObtainSemaphore(&randsem);

	rc = (ULONG)rand();

	ReleaseSemaphore(&randsem);

	return (rc);
}


UBYTE random_ubyte(void)
{
	UBYTE rc;

	ObtainSemaphore(&randsem);

	rc = (UBYTE)rand();

	ReleaseSemaphore(&randsem);

	return (rc);
}

#endif
