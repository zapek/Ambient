/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006-2007 by Ilkka Lehtoranta
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
 * $Id: prefs_desktop.c,v 1.10 2017/11/06 20:18:11 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <exec/lists.h>
#include <workbench/workbench.h>

/* private */
#include "ambient_cat.h"
#include "classes.h"
#include "doslistcache.h"
#include "prefs.h"
#include "prefs_desktop.h"

struct hidden_device_node
{
	struct MinNode n;
	LONG x, y;		/* change to floats in the future... */
	LONG shown;
	TEXT name[0];
};

struct desktop_prefs
{
	LONG mymorphos_iconx;
	LONG mymorphos_icony;

	LONG mymorphos_window_left;
	LONG mymorphos_window_top;
	LONG mymorphos_window_width;
	LONG mymorphos_window_height;
	ULONG mymorphos_window_flags;

	BOOL mymorphos_show;
	BOOL mymorphos_doubleclick;

	TEXT mymorphos_name[32];
};

static struct SignalSemaphore semaphore;
static struct MinList hdlist;
static struct desktop_prefs prefs;
static APTR dprefspool;
static APTR mymorphos_object;

static void hdd_add_node(CONST_STRPTR name, LONG shown, LONG x, LONG y)
{
	struct hidden_device_node *node = malloc(sizeof(*node) + strlen(name) + 1);

	if (node)
	{
		node->x = x;
		node->y = y;
		node->shown = shown;
		strcpy(node->name, name);
		Forbid();
		ADDTAIL(&hdlist, node);
		Permit();
	}
}


ULONG hiddendrives_init(void)
{
	if ((dprefspool = prefspool_create(0)))
	{
		InitSemaphore(&semaphore);
		NEWLIST(&hdlist);

		/* initialize some defaults */

		prefs.mymorphos_iconx = NO_ICON_POSITION;
		prefs.mymorphos_icony = NO_ICON_POSITION;

		/* we cant read _conf() here */
		prefs.mymorphos_window_left = 400;
		prefs.mymorphos_window_top = 200;
		prefs.mymorphos_window_width = 400;
		prefs.mymorphos_window_height = 200;

		prefs.mymorphos_show = TRUE;

		/* we assume locale system is functioning */
		stccpy(prefs.mymorphos_name, GSI(MSG_ICON_MYMORPHOS), sizeof(prefs.mymorphos_name));

		return (TRUE);
	}
	return (FALSE);
}


void hiddendrives_cleanup(void)
{
	if (dprefspool)
	{
		prefspool_delete(dprefspool);
	}
}


/* Called from devices.c */
ULONG tr_hiddendrives_load(APTR obj)
{
	ULONG rc = FALSE;
	THREAD;
	CHECKOBJECT(obj);

	ObtainSemaphore(&semaphore);

	DB(("attempting to load banlist..\n"));

	if (prefspool_read(dprefspool, PREFS_PATH DESKTOP_FILE, DESKTOPPREFSID, TRUE) == PREFSPOOL_IO_OK)
	{
		ULONG *dummy, i = 0;
		APTR pl, pi; /* list, item */
		STRPTR name;

		DB(("Desktop.prefs exists, loading..\n"));

		if ((pl = prefspool_item_get(dprefspool, NULL, DDSI_LISTPOOL_HIDDENDRIVE, NULL, NULL)))
		{
			while ((pi = prefspool_item_get(dprefspool, pl, i | DSF_LISTPOOL, NULL, NULL)) && prefspool_item_get(dprefspool, pi, DDSI_LISTPOOL_HIDDENDRIVE_NAME, (APTR)&name, NULL))
			{
				LONG x = NO_ICON_POSITION, y = NO_ICON_POSITION, shown = TRUE;

				if (prefspool_item_get(dprefspool, pi, DDSI_LISTPOOL_HIDDENDRIVE_ICONX, (APTR)&dummy, NULL))
					x = *dummy;

				if (prefspool_item_get(dprefspool, pi, DDSI_LISTPOOL_HIDDENDRIVE_ICONY, (APTR)&dummy, NULL))
					y = *dummy;

				if (prefspool_item_get(dprefspool, pi, DDSI_LISTPOOL_HIDDENDRIVE_SHOWN, (APTR)&dummy, NULL))
					shown = *dummy;

				if (name)
				{
					hdd_add_node(name, shown, x, y);
				}
				/* XXX */
				i++;
			}
		}

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_ICONX, (APTR)&dummy, NULL))
			prefs.mymorphos_iconx = *dummy;

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_ICONY, (APTR)&dummy, NULL))
			prefs.mymorphos_icony = *dummy;

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_LEFT, (APTR)&dummy, NULL))
			prefs.mymorphos_window_left = *dummy;

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_TOP, (APTR)&dummy, NULL))
			prefs.mymorphos_window_top = *dummy;

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_WIDTH, (APTR)&dummy, NULL))
			prefs.mymorphos_window_width = *dummy;

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_HEIGHT, (APTR)&dummy, NULL))
			prefs.mymorphos_window_height = *dummy;

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_FLAGS, (APTR)&dummy, NULL))
			prefs.mymorphos_window_flags = *dummy;

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_NAME, (APTR)&name, NULL))
			stccpy(prefs.mymorphos_name, name, sizeof(prefs.mymorphos_name));

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_SHOW, (APTR)&dummy, NULL))
			prefs.mymorphos_show = *dummy;

		if (prefspool_item_get(dprefspool, NULL, DDSI_MYMORPHOS_DOUBLECLICK, (APTR)&dummy, NULL))
			prefs.mymorphos_doubleclick = *dummy;

		rc = TRUE;
	}
	/* XXX */

	ReleaseSemaphore(&semaphore);

	return (rc);
}


static ULONG tr_desktop_prefs_save(ULONG reporterror)
{
	ULONG rc = FALSE;
	APTR pl, pi;

	THREAD;

	D(SHORTCUT,bug("attempting to save hidden drives prefs..\n"));

	if (dprefspool)
	{
		DB(("dprefspool exists, flushing..\n"));
		prefspool_flush(dprefspool);

		DB(("collecting drives..\n"));

		if (!(pl = prefspool_item_get(dprefspool, NULL, DDSI_LISTPOOL_HIDDENDRIVE, NULL, NULL)))
		{
			pl = prefspool_item_add(dprefspool, NULL, DDSI_LISTPOOL_HIDDENDRIVE, NULL, 0);
		}

		if (pl)
		{
			struct hidden_device_node *node;
			ULONG i = 0;

			ForeachNode(&hdlist, node)
			{
				if (!(pi = prefspool_item_get(dprefspool, pl, i | DSF_LISTPOOL, NULL, NULL)))
				{
					pi = prefspool_item_add(dprefspool, pl, i | DSF_LISTPOOL, NULL, 0);
				}
				
				if (pi)
				{
					setprefsstr_lp(dprefspool, pi, DDSI_LISTPOOL_HIDDENDRIVE_NAME, node->name);
					setprefslong_lp(dprefspool, pi, DDSI_LISTPOOL_HIDDENDRIVE_ICONX, node->x);
					setprefslong_lp(dprefspool, pi, DDSI_LISTPOOL_HIDDENDRIVE_ICONY, node->y);
					setprefslong_lp(dprefspool, pi, DDSI_LISTPOOL_HIDDENDRIVE_SHOWN, node->shown);
				}
				i++;
			}
		}

		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_ICONX, prefs.mymorphos_iconx);
		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_ICONY, prefs.mymorphos_icony);
		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_LEFT, prefs.mymorphos_window_left);
		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_TOP, prefs.mymorphos_window_top);
		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_WIDTH, prefs.mymorphos_window_width);
		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_HEIGHT, prefs.mymorphos_window_height);
		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_WINDOW_FLAGS, prefs.mymorphos_window_flags);

		setprefsstr_lp(dprefspool, NULL, DDSI_MYMORPHOS_NAME, prefs.mymorphos_name);
		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_SHOW, prefs.mymorphos_show);
		setprefslong_lp(dprefspool, NULL, DDSI_MYMORPHOS_DOUBLECLICK, prefs.mymorphos_doubleclick);

		/* save the shortcuts */
		DB(("saving to disk..\n"));
		if (prefspool_write(dprefspool, PREFS_PATH DESKTOP_FILE, DESKTOPPREFSID, reporterror))
		{
			rc = TRUE;
		}
		DB(("result: %ld\n", rc));
	}
	
	return (rc);
}


void hiddendrives_updatedevice(APTR obj UNUSED, CONST_STRPTR devname, LONG shown, LONG x, LONG y)
{
	struct dlcnode *dlc;
	TEXT t[VOLUME_SIZE + 1];

	ASSERT(devname);

	doslistcache_lock();

	if( (dlc = doslistcache_find_dlcdevice_by_volumename(devname)) || 
	    (dlc = doslistcache_find_dlcdevice_by_devicename(devname)) )
	{
		stccpy(t, dlc->name, VOLUME_SIZE);

		/* XXX: big sigh, we need to update state ourselves, to keep doslistcache synchronized between
		   device and volume nodes. let's hope it works !
		 */
		 /*
		if(dlc->state == DLC_DEAD)
		{
			dlc->state = DLC_REMOVED;
		} */
	}

	doslistcache_unlock();

	if (dlc)
	{
		struct hidden_device_node *node;
		LONG success = FALSE;

		if (!strchr(t, ':'))
			strcat(t, ":");

		ForeachNode(&hdlist, node)
		{
			if (!stricmp(node->name, t))
			{
				success = TRUE;
				node->shown = shown;
				node->x = x;
				node->y = y;
				break;
			}
		}

		if (!success)
		{
			hdd_add_node(t, shown, x, y);
		}
	}
}


ULONG tr_hiddendrives_hidden(CONST_STRPTR devname, LONG *x, LONG *y)
{
	struct hidden_device_node * hdnode;
	ULONG rc = FALSE;

	THREAD;
	ASSERT(devname);

	*x = NO_ICON_POSITION;
	*y = NO_ICON_POSITION;

	ForeachNode(&hdlist, hdnode)
	{
		if (!stricmp(devname, hdnode->name))
		{
			if (hdnode->shown)
			{
				*x = hdnode->x;
				*y = hdnode->y;
			}
			else
			{
				rc = TRUE;
			}
			break;
		}
	}

	return (rc);
}


void dprefs_mymorphos_iconpos_get(LONG *x, LONG *y)
{
	*x = prefs.mymorphos_iconx;
	*y = prefs.mymorphos_icony;
}


void dprefs_mymorphos_iconpos_set(LONG x, LONG y)
{
	prefs.mymorphos_iconx = x;
	prefs.mymorphos_icony = y;
}


ULONG tr_dprefs_save(APTR obj UNUSED)
{
	THREAD;

	ObtainSemaphore(&semaphore);
	tr_desktop_prefs_save(FALSE);
	ReleaseSemaphore(&semaphore);

	return (TRUE);
}


void dprefs_mymorphos_window_get(LONG *x, LONG *y, LONG *w, LONG *h, ULONG *flags)
{
	*x = prefs.mymorphos_window_left;
	*y = prefs.mymorphos_window_top;
	*w = prefs.mymorphos_window_width;
	*h = prefs.mymorphos_window_height;
	*flags = prefs.mymorphos_window_flags;
}


void dprefs_mymorphos_window_set(LONG x, LONG y, LONG w, LONG h, ULONG flags)
{
	prefs.mymorphos_window_left = x;
	prefs.mymorphos_window_top = y;
	prefs.mymorphos_window_width = w;
	prefs.mymorphos_window_height = h;
	prefs.mymorphos_window_flags = flags;
}


/*
 * Functions below must stay single threaded (no calling from threads!)
 */

CONST_STRPTR dprefs_mymorphos_name_get(void)
{
	MAINTASK;
	return (prefs.mymorphos_name);
}


void dprefs_mymorphos_name_set(CONST_STRPTR name)
{
	MAINTASK;

	/* Name may not change while saving prefs
	 *
	 * XXX: this is fubar. we wait for disk I/O to finish. how stupid is that? suxx...
	 *
	 * XXX: is semaphore protection important when we are going to save prefs anyway?
	 *
	 * XXX: i want my single threaded MS-DOS :)
    */
	ObtainSemaphore(&semaphore);
	stccpy(prefs.mymorphos_name, name && *name ? name : GSI(MSG_ICON_MYMORPHOS), sizeof(prefs.mymorphos_name));
	ReleaseSemaphore(&semaphore);

	if (mymorphos_object)
		set(mymorphos_object, MA_Icon_Type, MV_Icon_Type_MyComputer);
}


void dprefs_mymorphos_notify_setobj(APTR obj)
{
	MAINTASK;

	/* Hmm... good? Easier to notify about mymorphos icon changes... */
	mymorphos_object = obj;
}


ULONG dprefs_mymorphos_show(void)
{
	return (prefs.mymorphos_show);
}


ULONG dprefs_mymorphos_doubleclick(void)
{
	return (prefs.mymorphos_doubleclick);
}
