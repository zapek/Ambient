/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
 * Copyright 2005-2007 Ambient Open Source Team
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
 * $Id: devices.c,v 1.20 2020/08/16 03:15:18 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <workbench/workbench.h> /* XXX: for NO_ICON_POSITION.. sucks */
#include <proto/dos.h>
#include <dos/dosextens.h>
#include <proto/exec.h>

/* private */
#include "ambient_cat.h"
#include "deficonpool.h"
#include "doslistcache.h"
#include "iconio.h"
#include "prefs.h"
#include "methodstack.h"
#include "prefs_desktop.h"
#include "devices.h"
#include "name.h"
#include "notify.h"
#include "capacity.h"
#include "shortcuts.h"
#include "threads.h"
#include "info64.h"
#include "time_func.h"
#include "screen.h"

static struct SignalSemaphore devsem;
static ULONG hiddendrives_loaded;

ULONG devices_init(void)
{
	InitSemaphore(&devsem);
	return (TRUE);
}


void devices_cleanup(void)
{

}

/*
 * Creates an icon for a devicelist entry and adds
 * it into an iconview object.
 */
static ULONG create_assigns_icon(APTR obj, BOOL fade)
{
	ULONG rc = TRUE;
	ULONG viewid;

	struct dlcnode *n;
	APTR o;

	CHECKOBJECT(obj);

	methodstack_push_sync(obj, 3, OM_GET, MA_Viewgroup_ID, &viewid);

	ITERATELIST(n, &dlclist)
	{
		if (threads_check_abort())
		{
			rc = ABORTED;
			break;
		}

		if (n->type == DLT_DIRECTORY && (n->state == DLC_NEW || n->state == DLC_ADDED))
		{
			STRPTR name = name_build_colon(n->name);

			if (name)
			{
				STRPTR s = name_build_info(name);

				if (s)
				{
					D(DEVICEIO, bug("opening <%s>\n", name)); /* XXX: beware.. we should put a default icon for files that don't exist */

					if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, FALSE, viewid)) )
					{
						if (icon_read(s, o,
							ICONTAG_IsAssign, TRUE,
							ICONTAG_Deficon, TRUE,
							TAG_DONE))
						{
							methodstack_push(o, 3, MUIM_Set, MA_Icon_DeviceType, MV_Icon_DeviceType_Fixed + 1);
							methodstack_push(o, 3, MUIM_Set, MA_Icon_Fade, fade);
							methodstack_push(o, 3, MUIM_Set, MA_Icon_Path, name);
							methodstack_push_sync(obj, 3, MM_Iconview_AddIcon, o, FALSE);
						}
						else
						{
							methodstack_push(o, 1, OM_RELEASE);
						}
					}
					name_delete(s);
				}
				name_delete(name);
			}
		}
	}

	return rc;

}

/*
 * Creates an icon for a devicelist entry and adds
 * it into an iconview object.
 */

static ULONG create_devices_icon(APTR obj, BOOL fade)
{
	ULONG rc = TRUE;

	struct dlcnode *n;
	STRPTR s;
	APTR o;
	ULONG viewid;

	CHECKOBJECT(obj);

	methodstack_push_sync(obj, 3, OM_GET, MA_Viewgroup_ID, &viewid);

	doslistcache_lock();

	ITERATELIST(n, &dlclist)
	{
		if (threads_check_abort())
		{
			rc = ABORTED;
			break;
		}

		if ( n->type == DLT_VOLUME && (n->state == DLC_NEW || n->state == DLC_ADDED))
		{
			TEXT t[VOLUME_SIZE+1];

			snprintf(t, sizeof(t), "%s:", n->name);

			if ( (s = name_build_info(t)) )
			{
				D(DEVICEIO, bug("opening <%s>\n", s)); /* XXX: beware.. we should put a default icon for files that don't exist */

				if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, FALSE, viewid)) )
				{
					if (icon_read(s, o,
						ICONTAG_FileType, MV_Icon_FileType_Device,
						ICONTAG_Deficon, TRUE,
					TAG_DONE))
					{
						methodstack_push(o, 3, MUIM_Set, MA_Icon_DiskType, n->disktype);
						methodstack_push(o, 3, MUIM_Set, MA_Icon_DeviceType, MV_Icon_DeviceType_Fixed);
						methodstack_push(o, 3, MUIM_Set, MA_Icon_Fade, fade);
						methodstack_push(o, 3, MUIM_Set, MA_Icon_Path, t);
						methodstack_push_sync(obj, 3, MM_Iconview_AddIcon, o, FALSE);

						D(DEVICEIO,bug("added entry for:<%s>\n", n->name));
					}
					else
					{
						methodstack_push(o, 1, OM_RELEASE);
					}
				}
				/* XXX */
				name_delete(s);
			}
			else
			{
				D(DEVICEIO,bug("failed to build info for:<%s><%s>\n", n->name, t));
			}
		}
		else
		{
			D(DEVICEIO,bug("node failed criteria:<%s> : %d,%d\n", n->name ? (STRPTR)n->name : (STRPTR)"Unnamed", n->type, n->state));
		}
	}

	doslistcache_unlock();

	return rc;

}


/*
 * Creates an icon for a devicelist entry and adds
 * it into an iconview object.
 */
static void create_rootdevices_icon(APTR obj, BOOL fade UNUSED)
{
	struct dlcnode *n;
	ULONG i = 0;

	CHECKOBJECT(obj);

	ForeachNode(&dlclist, n)
	{
		if (n->type == DLT_VOLUME && n->state == DLC_NEW)
		{
			struct dlcnode *dev;
			TEXT t[VOLUME_SIZE];

			n->state = DLC_ADDED;

			strcpy(t, n->name);
			strcat(t, ":");

			notify_action(t, NOTIFYTAG_Monitor_Device, NOTIFYTAG_Monitor_Device_Mount);

			dev = doslistcache_find_dlcdevice(n->mp);

			if (dev)
			{
				TEXT devname[VOLUME_SIZE];
				LONG x, y;

				strcpy(devname, dev->name);
				strcat(devname, ":");

				if (!tr_hiddendrives_hidden(devname, &x, &y))
				{
					/* This is special case. */
					tr_shortcuts_add(obj, t, x, y, FALSE, MV_Icon_FileType_Device);
					i++;
				}
			}
		}
	}

	if (i)
	{
		//tr_shortcuts_save(obj, FALSE);
	}
}


/* Only root object shall call this */
static ULONG devices_remove(APTR obj, BOOL needstate)
{
	struct dlcnode *n, *n2;

	THREAD;
	CHECKOBJECT(obj);

	ObtainSemaphore(&devsem);

	methodstack_push(obj, 1, MUIM_Group_InitChange);
	ITERATEDLCSAFE(n, n2)
	{
		if (needstate ? (n->state == DLC_DEAD) : TRUE)
		{
			methodstack_push(obj, 2, MM_Iconview_RemoveByName, n->name);
			n->state = needstate ? DLC_REMOVED : DLC_NEW;

			if (needstate)
			{
				notify_action(n->name, NOTIFYTAG_Monitor_Device, NOTIFYTAG_Monitor_Device_UnMount); /* XXX: let's hope the name is right.. (no .info) */
			}
		}
	}
	methodstack_push_sync(obj, 1, MUIM_Group_ExitChange);

	ReleaseSemaphore(&devsem);

	if ( threads_check_abort() )
		return ABORTED;
	
	return TRUE; /* XXX */
}

/*
 * Wanders down the doslist to find
 * volumes which matches devices and
 * adds them to the iconview object.
 */
ULONG tr_devices_show(APTR obj, BOOL assigns, BOOL isroot, BOOL fade)
{
	ULONG rc = TRUE;

	THREAD;
	CHECKOBJECT(obj);

	ObtainSemaphore(&devsem);

	doslistcache_update();

	if (isroot)
	{
		if (!hiddendrives_loaded)
		{
			hiddendrives_loaded = 1;
			tr_hiddendrives_load(obj);

			#if USE_SHORTCUTS
			tr_shortcuts_load(obj);
			#endif

			methodstack_push(obj, 1, MM_Iconview_DoLayout);
		}

		create_rootdevices_icon(obj, fade);
	}
	else
	{
		rc = create_devices_icon(obj, fade);

		if ( rc == TRUE && assigns)
			rc = create_assigns_icon(obj, fade);
	}

	ReleaseSemaphore(&devsem);

	if ( rc == TRUE )
		methodstack_push_sync(obj, 1, MM_Iconview_DoLayout); /* sync */

	if ( threads_check_abort() )
		rc = ABORTED;

	return rc;
}


ULONG tr_devices_add(APTR obj, ULONG fade UNUSED)
{
	return tr_devices_show(obj, FALSE, TRUE, TRUE);
}


ULONG tr_devices_remove(APTR obj)
{
	return devices_remove(obj, TRUE);
}


ULONG tr_devices_removeall(APTR obj)
{
	return devices_remove(obj, FALSE);
}

struct tempnode{
	struct MinNode n;
	QUAD free;
	QUAD size;
	LONG namelen;
	TEXT name[0];
};

ULONG tr_devices_updateinfo(APTR obj)
{
	ULONG rc = TRUE;
	struct timerequest *timer;

	THREAD;
	CHECKOBJECT(obj);

	timer = timer_create(UNIT_VBLANK, NULL);

	if (timer)
	{
		struct timeval tv = {10, 0};
		ULONG abort = FALSE, changed, initial = TRUE;

		for (;;)
		{
			struct dlcnode *dln;
			struct tempnode *tn, *tnn;
			struct MinList l;

			changed = FALSE;

			if (is_screen_visible())
			{
				/* traverse all volumes on doslist cache list, copy to temp list */

				doslistcache_lock();

				NEWLIST(&l);
				ITERATELIST(dln, &dlclist)
				{
					if (dln->type == DLT_DEVICE)
					{
						LONG len = strlen(dln->name);
						tn = malloc(sizeof(*tn) + len + 2);
						if (tn != NULL)
						{
							snprintf(tn->name, len + 2, "%s:", dln->name);
							tn->namelen = len;
							tn->free = dln->free;
							tn->size = dln->size;
							ADDTAIL(&l, tn);
						}
					}
				}

				doslistcache_unlock();

				/* do the possibly-blocking stuff with dostlist cache unlocked */

				ITERATELIST(tn, &l)
				{
					QUAD oldsize = tn->size;
					QUAD oldfree = tn->free;
					struct devinfo64 di;

					if (threads_check_abort())
					{
						rc = ABORTED;
						abort = TRUE;
						break;
					}

					if (info64(tn->name, &di))
					{
						tn->size = di.di_NumBlocks * di.di_BytesPerBlock;
						tn->free = (di.di_NumBlocks - di.di_NumBlocksUsed) * di.di_BytesPerBlock;
						if (tn->size != oldsize || (tn->free & ~1023) != (oldfree & ~1023) || initial)
							changed = TRUE;
						else
							tn->namelen = -1; /* some way to signal unchanged entry */
					}
				}

				/* update entries on doslist cache. use simple optimization which assumes order didn't change */

				doslistcache_lock();

				dln = FIRSTNODE(&dlclist);

				ITERATELISTSAFE(tn, tnn, &l)
				{
					if (tn->namelen != -1)
					{
						struct dlcnode *ndln;
						tn->name[tn->namelen] = 0;

						for(ndln = dln; NEXTNODE(ndln); ndln = NEXTNODE(ndln))
						{
							if (ndln->type == DLT_DEVICE)
							{
								if (0 == strcmp(tn->name, ndln->name))
								{
									ndln->size = tn->size;
									ndln->free = tn->free;
									dln = ndln;
									break;
								}
							}
						}
					}
					free(tn);
				}

				doslistcache_unlock();

				/* if information was updated refresh all view windows and root */

				if (changed)
				{
					DoMethod(app, MUIM_Application_PushMethod, app, 6 | MUIF_PUSHMETHOD_SINGLE, MM_Application_RootDoMethodByAttr, MA_Window_Type, MV_Window_Type_Rootview, MM_Window_DoView, NULL, MM_Iconview_UpdateDriveInfo);
					DoMethod(app, MUIM_Application_PushMethod, app, 6 | MUIF_PUSHMETHOD_SINGLE, MM_Application_WindowDoMethodByAttr, MA_Window_Type, MV_Window_Type_View, MM_Window_DoView, NULL, MM_Iconview_UpdateDriveInfo);
				}

				initial = FALSE;
			}

			/* done */

			tv.tv_secs = changed ? 1 : tv.tv_secs * 2;

			if (tv.tv_secs > 10)
				tv.tv_secs = 10;

			timer_addreq_async(timer, &tv);
			threads_waitsig(1 << timer->tr_node.io_Message.mn_ReplyPort->mp_SigBit, &abort);

			if (abort)
			{
				timer_abort(timer);
				rc = ABORTED;
				break;
			}
		}

		timer_delete(timer);
	}

	return (rc);
}

ULONG device_get_information(CONST_STRPTR path, struct device_information * inf)
{
	STRPTR p;
	ULONG rc = FALSE;

	THREAD;

	if (path && path[0] != ':' && (p = strchr(path, ':')))
	{
		if (p - path <= VOLUME_SIZE)
		{
			TEXT devname[VOLUME_SIZE + 1];
			struct devinfo64 di;

			stccpy(devname, path, p + 1 - path + 1);

			if (info64(devname, &di))
			{
				inf->used  = di.di_NumBlocksUsed * di.di_BytesPerBlock;
				inf->total = di.di_NumBlocks * di.di_BytesPerBlock;
				inf->state = di.di_DiskState;
				rc = TRUE;
			}
		}
	}
	else
	{
		PDB(("path is not absolute, <%s>\n", path));
	}
	return (rc);
}

