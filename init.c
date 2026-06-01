/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2015 Ambient Open Source Team
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
 * $Id: init.c,v 1.18 2025/09/16 15:52:29 kronos Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>

/* private */
#include "init.h"
#include "libs.h"
#include "contextmenu.h"
#include "classes.h"
#include "iconmem.h"
#include "methodstack.h"
#include "prefs_desktop.h"
#include "colormap.h"
#include "time_func.h"
#include "threads.h"
#include "locale.h"
#include "menus.h"
#include "prefs.h"
#include "wbstart.h"
#include "memhandler_threads.h"
#include "rexx.h"
#include "dosnotify.h"
#include "ipc.h"
#include "devices.h"
#include "doslistcache.h"
#include "pngicon.h"
#include "wbstartlib.h"
#include "dosreq.h"
#include "pngio.h"
#include "deficonpool.h"
#include "memtrack.h"
#include "cx.h"
#include "input.h"
#include "ambient_altivec.h"
#include "smartreq.h"
#include "panelprefs.h"
#include "shortcuts.h"
#include "pointer.h"
#include "random.h"
#include "vars.h"
#include "viewapi.h"
#include "mimetype.h"
#include "mimeprefs.h"
#include "modules.h"
#include "sound.h"
#include "playsound.h"
#include "notify.h"
#include "soundlib.h"
#include "cpu.h"
#include "multimedia.h"
#include "imagecache.h"
#include "clipboard.h"
#include "vfs.h"
#include "cache.h"
#include "storage.h"
#include "prefs_advanced.h"
#include "soundwin.h"
#include "thumbs.h"
#include "trashcan.h"
#include "ambient_lib.h"


#if USE_AVCODEC
#include "avcodec.h"
#endif

#include "keyshortcuts.h"

struct initdesc {
	#ifdef DEBUG
	STRPTR initname;
	#endif
	ULONG (*initfunc)(void);
	void (*exitfunc)(void);
};

#ifdef DEBUG
#define INITENTRY(name) { #name	, name##_init, name##_cleanup}
#define ENDENTRY {NULL, NULL, NULL}
#else
#define INITENTRY(name) { name##_init, name##_cleanup}
#define ENDENTRY {NULL, NULL}
#endif

static struct initdesc id[] = {
	#if USE_MEMTRACK
	INITENTRY(memtrack),
	#endif
	INITENTRY(malloc),
	INITENTRY(input),
	#ifdef DEBUG
	INITENTRY(debug),
	#endif
	INITENTRY(dosreq),
	INITENTRY(libs),
	INITENTRY(threads), /* the thread cleanup is done in application too */
	INITENTRY(locale),
	INITENTRY(vars),
	INITENTRY(cpu),
	INITENTRY(modules),
	INITENTRY(classes),
	INITENTRY(altivec),
	INITENTRY(notify),
	#if USE_DOSNOTIFY
	INITENTRY(dosnotify),
	#endif
	INITENTRY(prefs_advanced),
	INITENTRY(smartreq),
	INITENTRY(iconmem),
	INITENTRY(prefs),
#if USE_INTERNAL_PANELS
	INITENTRY(panelprefs),
#endif
	INITENTRY(methodstack),
	INITENTRY(timer),
	INITENTRY(random),
	INITENTRY(magicwb_cm),
	INITENTRY(menus),
	INITENTRY(pointer),
	INITENTRY(doslistcache),
	INITENTRY(devices),
	#if USE_MULTIMEDIA
	INITENTRY(multimedia),
	#endif
	#if USE_SOUND
	INITENTRY(sound),
	#endif
	INITENTRY(playsound),
	INITENTRY(mimeprefs),
	INITENTRY(mimetype),
	INITENTRY(wbstart),
	INITENTRY(memhandler_threads),
	INITENTRY(ipc),
	INITENTRY(cx),
	#if USE_REXX
	INITENTRY(rexx),
	#endif
	#if USE_SOUNDLIB
	INITENTRY(soundlib),
	#endif
	#if USE_WBSTARTLIB
	INITENTRY(wbstartlib),
	#endif
	INITENTRY(deficonpool),
	#if USE_SHORTCUTS
	INITENTRY(shortcuts),
	#endif
	INITENTRY(viewapi),
	INITENTRY(imagecache),
	INITENTRY(clipboard),
	INITENTRY(vfs),
	INITENTRY(cache),
	INITENTRY(storage),
	INITENTRY(hiddendrives),
	INITENTRY(contextmenu),
	INITENTRY(keyshortcuts),
	INITENTRY(soundwin),
	#if USE_AVCODEC
	INITENTRY(libavcodec),
	#endif
	INITENTRY(thumb),
	INITENTRY(trashcan),
	#if USE_AMBIENT_LIB 
	INITENTRY(ambientlib),
	#endif
	ENDENTRY
};

ULONG init_open(void)
{
	ULONG i;

	for (i = 0; id[i].initfunc || id[i].exitfunc; i++)
	{
		if (id[i].initfunc)
		{
			ULONG rc;

			D(INIT,bug("initializing %s..\n", id[i].initname));

			rc = id[i].initfunc();

			id[i].initfunc = NULL;	// mark this entry called

			if (!rc)
			{
				D(INIT,bug("initialization of %s failed, bailing out..\n", id[i].initname));
				return (FALSE);
			}
		}
	}

	return (TRUE);
}


void init_close(void)
{
	LONG i;

	for (i = sizeof(id) / sizeof(struct initdesc) - 2; i >= 0; i--)
	{
		// dont call exitfunc if initfunc was not called

		if (id[i].exitfunc && !id[i].initfunc)
		{
			D(INIT,bug("cleaning up %s..\n", id[i].initname));
			id[i].exitfunc();
		}
	}
}


#define PRECLOSEENTRY(name) { preclose_##name }
#define ENDPRECLOSEENTRY {NULL}

struct preclosedesc {
	ULONG (*exitfunc)(void);
};


static const struct preclosedesc idc[] = {
	PRECLOSEENTRY(wbstart),
	#if USE_WBSTARTLIB
	PRECLOSEENTRY(wbstartlib),
	#endif
	#if USE_SOUNDLIB
	PRECLOSEENTRY(soundlib),
	#endif

	ENDPRECLOSEENTRY
};


ULONG init_preclose(void)
{
	LONG i;

	for (i = 0; idc[i].exitfunc; i++)
	{
		if (!idc[i].exitfunc())
		{
			return (FALSE);
		}
	}
	return (TRUE);
}
