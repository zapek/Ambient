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
 * $Id: prefsio.c,v 1.4 2013/10/28 12:04:05 geit Exp $
 */

#include "ambient.h"

/* public */
#include <proto/dos.h>

/* private */
#include "prefsio.h"
#include "file_func.h"
#include "smartreq.h"
#include "threads.h"
#include "prefs.h"


ULONG tr_prefs_save(APTR obj UNUSED, APTR ctx)
{
	ULONG rc = FALSE;

	THREAD;
	ASSERT(ctx);
	
	rc = prefspool_write(ctx, PREFS_PATH MAINPREFS_FILE, MAINPREFSID, TRUE);

	prefspool_delete(ctx);

	return (rc);
}
