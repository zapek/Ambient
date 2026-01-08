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
 * $Id: strap.c,v 1.7 2008/07/20 15:24:39 fab Exp $
 */

#include "ambient.h"

/* public */

/* private */
#include "strap.h"
#include "mui_func.h"
#include "command.h" /* XXX: sigh.. this is annoying.. */
#include "rexx.h"
#include "prefs.h"
#include "threads.h"
#include "viewapi.h"
#include "prefs.h"
#include "mimeprefs.h"
#include "smartreq.h"
#include "vars.h"
#include "crypto.h"
#include "cryptstr.h"
#include "file_func.h"
#include "timeout.h"


/*
 * System is monotasking at that point.
 */
ULONG strap_start_single(void)
{
	#ifdef SHOW_RELEASE
	if (!timeout_check())
	{
		MUI_Request(app, NULL, 0, crypto_decrypt_txt(timeout_failed_title), crypto_decrypt_txt(timeout_failed_buttons), crypto_decrypt_txt(timeout_failed_boyd));
		return (FALSE);
	}
	#endif
	
	if (!_var(nowildstar))
	{
		set_wildstar();
	}

	if (prefspool_read(mainprefspool, PREFS_PATH MAINPREFS_FILE, MAINPREFSID, TRUE) == PREFSPOOL_IO_MISSING)
	{
		prefspool_read(mainprefspool, PREFS_OLDPATH MAINPREFS_FILE, MAINPREFSID, TRUE);
	}
	set_default_prefs();

	#ifdef SHOW_RELEASE
	smartreq_info("Ambient", MV_Notification_Help, "This is a special version only for computer shows.\nDo not redistribute.", NULL);
	#else
	#endif

	
	/*
	 * Assigns the prefs to global vars.
	 */

	DoMethod(app, MM_Application_LoadPrefs, MV_Application_LoadPrefs_All, FALSE);

    /*
	 * Load the mimes (kiero: This is the place where mimetypes database is initialy built).
	 */

	mimeprefs_load(); /* XXX: same here.. we must detect failures from non existent files */

	/*
	 * Scan for available view modules.
	 */

	viewapi_scan(); /* XXX: check retcode */

	return (TRUE);
}


/*
 * System is multitasking. Threads are
 * handled.
 */
void strap_start_multi(void)
{
	/* XXX: the following should be replaced by a 'LoadBackground' command or so later on */
	DoMethod(app, MM_Application_LoadBackground, MF_Application_LoadBackground_Root | MF_Application_LoadBackground_Window);

	/*
	 * The font loading will go further.
	 */
}
