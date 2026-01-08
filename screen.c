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
 * $Id: screen.c,v 1.16 2018/07/26 15:19:46 itix Exp $
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

/* private */
#include "screen.h"


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
#if USE_MULTIPLE_DESKTOP
	char *activescreenname;
	if (!cached_screen)
	{
		activescreenname=active_screen_name();
		DB(("LockPubScreen(%s)...\n", activescreenname));
		if ( (cached_screen = LockPubScreen(activescreenname)) || (cached_screen = LockPubScreen("Workbench")) )
		{
			DB(("LockPubScreen(%s) was successful.\n", activescreenname));
			UnlockPubScreen(NULL, cached_screen);
		}
	}
#else
	if (!cached_screen)
	{
		DB(("LockPubScreen(Workbench)...\n"));
		if ( (cached_screen = LockPubScreen("Workbench")) )
		{
			DB(("LockPubScreen(Workbench) was successful.\n"));
			UnlockPubScreen(NULL, cached_screen);
		}
	}
#endif
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

/* Call only if unlock succeeded.
 *
 */
void screen_unlock(struct Screen *locked_screen)
{
	THREAD;
	UnlockPubScreen(NULL, locked_screen);
}


char *active_screen_name(void)
{
#if USE_MULTIPLE_DESKTOP
	struct Screen *ScreenName;

	ScreenName=IntuitionBase->ActiveScreen;
	return (ScreenName->DefaultTitle);
#else
	return "Workbench";
#endif
}


#if USE_MULTIPLE_DESKTOP
void create_screen(char *name)
{
	struct Screen *new_screen = NULL;

	if (IntuitionBase != NULL)
	{
		new_screen = OpenScreenTags(NULL,
				SA_LikeWorkbench, TRUE,
				SA_Title, name,
                                SA_PubName, name,
				TAG_DONE);
		PubScreenStatus(new_screen,0);
	}
}
#endif

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
