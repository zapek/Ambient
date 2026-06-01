/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006-2018 Ambient Open Source Team
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
 * $Id: screen.c,v 1.22 2025/09/12 16:06:01 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <hardware/atomic.h>
#include <prefs/screenmode.h>
#include <intuition/screens.h>
#include <proto/intuition.h>
#include <intuition/intuitionbase.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/layers.h>
#include <libraries/mui.h>

/* private */
#include "screen.h"
#include "classes.h"

void dprintf(char *, ...) __attribute__ ((format (printf, 1, 2)));


/*
 * Attempts to lock the screen to eg. do drag & drop
 * operations on it.
 */
#define SCREENLOCK_ATTEMPTS 3

/* looks like this implementation can lock up ambient while dragging,
 * when a new window opens for instance.
 */

/*
APTR lock_screen(struct Screen *scr)
{
	ULONG i = SCREENLOCK_ATTEMPTS;

	ASSERT(scr);

	while (i--)
	{
		if (AttemptSemaphore(&scr->LayerInfo.Lock))
		{
			LockLayers(&scr->LayerInfo);
			ReleaseSemaphore(&scr->LayerInfo.Lock);
			return (&scr->LayerInfo);
		}
		Delay(5);
	}
	return (NULL);
}
*/

APTR lock_screen(struct Screen *screen)
{
	int i = SCREENLOCK_ATTEMPTS;

	while (i--)
	{
		Forbid();
		if (!screen->LayerInfo.Lock.ss_Owner) // AttemptSemaphore() should be used
		{
			LockLayers(&screen->LayerInfo);
			Permit();
			return(&screen->LayerInfo);
		}
		Permit();
		Delay(5);
	}
	return(NULL);
}


void unlock_screen(APTR handle)
{
	ASSERT(handle);

	UnlockLayers(handle);
}


/*
 * This is b0rken by design. There should be lock/unlock
 * pair of routines. Unfortunately get_screen() is used
 * all over, so it's not so trivial to fix. Just adding
 * semaphore to current routines resolves nothing. - piru
 */

static struct Screen *cached_screen;
static LONG holdcount = 0; /* this one is only used by get_screen_hold()/_release() from main loop. no need for sema */

struct Screen *get_screen(void) /* XXX: is that called from threads? check.. yeah it is.. add some semaphore or so.. */
{
	if (!cached_screen)
	{
		DB(("LockPubScreen(Workbench)...\n"));
		if ( (cached_screen = LockPubScreen("Workbench")) )
		{
			DB(("LockPubScreen(Workbench) was successful.\n"));
			UnlockPubScreen(NULL, cached_screen);
		}
	}

	return (cached_screen);
}

struct Screen *get_screen_hold(void)
{
	MAINTASK;
	holdcount++;
	cached_screen = LockPubScreen("Workbench");
	return cached_screen;
}

void get_screen_release()
{
	MAINTASK;
	if (cached_screen != NULL && holdcount > 0)
	{
		holdcount--;
		UnlockPubScreen(NULL, cached_screen);
	}
}

/* For threads only */

struct Screen *screen_lock(void)
{
	THREAD;
	return LockPubScreen("Workbench");
}

struct Screen *screen_lock_by_id(ULONG sid)
{
	THREAD;
	char name[128];

	if (sid == 0)
		return screen_lock();

	snprintf(name, sizeof(name), "Workbench.%ld", sid);
	return LockPubScreen(name);
}

/* Call only if unlock succeeded.
 *
 */
void screen_unlock(struct Screen *locked_screen)
{
	THREAD;
	if (locked_screen)
		UnlockPubScreen(NULL, locked_screen);
}

// Window.mui does not copy strings so it's important we use the hardcoded values here
static const char *screenNames[] = {
	"Workbench",
	"Workbench.1",
	"Workbench.2",
	"Workbench.3",
};

const char *active_screen_name(void)
{
	ULONG sid = 0;
	ULONG il = LockIBase(0);
	if (IntuitionBase->FirstScreen)
	{
		sid = get_screen_id((CONST_STRPTR)xget((Object *)IntuitionBase->FirstScreen, SA_PubName));
	}
	UnlockIBase(il);
	
	if (sid <= AMBIENT_MAX_EXTRA_SCREENS)
		return screenNames[sid];
	return screenNames[0];
}

void flush_screen(void)
{
	cached_screen = NULL;
}

BOOL is_screen_visible(void)
{
	ULONG displayed = FALSE;
	GetAttr(SA_Displayed, get_screen(), &displayed);
	return displayed;
}

ULONG get_screen_id(CONST_STRPTR name)
{
	if (name && !strncmp(name, "Workbench.", 10))
	{
		ULONG id = atoi(name + 10);
		if (id > AMBIENT_MAX_EXTRA_SCREENS)
			return 0;
		return id;
	}
	
	return 0; // Ambient default screen
}

CONST_STRPTR get_screen_pubname(Object *muiArea)
{
	if (muiRenderInfo(muiArea) && _window(muiArea))
		return (CONST_STRPTR)xget((Object *)_window(muiArea)->WScreen, SA_PubName);
	return NULL; // default = Ambient main
}
