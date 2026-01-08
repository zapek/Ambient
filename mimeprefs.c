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
 * $Id: mimeprefs.c,v 1.6 2007/11/28 21:08:03 kiero Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "mimeprefs.h"
#include "mimetype.h"
#include "prefs.h"
#include "file_func.h"


APTR mimeprefsctx;

static struct SignalSemaphore mpsem;


ULONG mimeprefs_init(void)
{
	InitSemaphore(&mpsem);

	return (TRUE);
}


void mimeprefs_cleanup(void)
{
	/* nothing */
}


void mimeprefs_lock_shared(void) /* XXX: don't forget to call it while we're scanning the mimetypes.. */
{
	ObtainSemaphoreShared(&mpsem);
}


void mimeprefs_unlock(void)
{
	ReleaseSemaphore(&mpsem);
}


void mimeprefs_lock(void) /* XXX: don't forget to call it during the prefs transfers.. */
{
	ObtainSemaphore(&mpsem);
}


ULONG mimeprefs_load(void)
{
	#if 0
	if ((mimeprefsctx = prefspool_create(0)))
	{
		if (prefspool_read(mimeprefsctx, PREFS_PATH MIME_FILE, MIMEPREFSID, TRUE) > 0)
		{
			return (TRUE);
		}
	}
	#else
	mimetype_load_database(NULL);
	#endif

	return (FALSE);
}


ULONG tr_savemime(APTR pctx)
{
	THREAD;
	ASSERT(pctx);

	if (prefspool_write(pctx, PREFS_PATH MIME_FILE, MIMEPREFSID, TRUE) > 0)
	{
		return (TRUE);
	}
	return (FALSE);
}
