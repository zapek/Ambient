/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
 * Copyright 2006-2007 Ambient Open Source Team
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
 * $Id: shortcuts.c,v 1.13 2021/11/02 01:15:37 jacadcaps Exp $
 */

#include "ambient.h"

#if USE_SHORTCUTS

/* public */
#include <proto/dos.h>
#include <workbench/workbench.h> /* XXX: for NO_ICON_POSITION.. sucks */

/* private */
#include "deficonpool.h"
#include "doslistcache.h"
#include "shortcuts.h"
#include "prefs.h"
#include "prefs_desktop.h"
#include "prefs_advanced.h"
#include "name.h"
#include "methodstack.h"
#include "iconio.h"
#include "file_func.h"
#include "prefs_advanced.h"


static struct SignalSemaphore scsem;
static APTR scppool;

ULONG shortcuts_init(void)
{
	InitSemaphore(&scsem);

	return (TRUE);
}


void shortcuts_cleanup(void)
{
	/* nothing yet */
}


static void add_mycomputer_icon(APTR obj)
{
	APTR o;
	ULONG viewid;

	methodstack_push_sync(obj, 3, OM_GET, MA_Viewgroup_ID, &viewid);

	if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, viewid)) )
	{
		if (deficonpool_apply_default_icon(o, MV_Icon_Type_MyComputer, NULL, NULL))
		{
			ULONG x, y;

			dprefs_mymorphos_iconpos_get(&x, &y);

			if (x != NO_ICON_POSITION && y != NO_ICON_POSITION)
			{
				methodstack_push(o, 3, MUIM_Set, MA_Icon_X, x);
				methodstack_push(o, 3, MUIM_Set, MA_Icon_Y, y);
				methodstack_push(o, 3, MUIM_Set, MA_Icon_HasPos, TRUE);
			}

			methodstack_push(o, 3, MUIM_Set, MA_Icon_Fade, TRUE);
			methodstack_push(o, 3, MUIM_Set, MA_Icon_Type, MV_Icon_Type_MyComputer);
			methodstack_push(obj, 3, MM_Iconview_AddIcon, o, FALSE);
		}
		else
		{
			methodstack_push(o, 1, OM_RELEASE);
		}
	}
}


/*
 * To be called from a thread.
 */
ULONG tr_shortcuts_load(APTR obj)
{
	#if USE_DOTBACKDROP
	struct dlcnode *n;
	ULONG added = FALSE;
	#endif
	ULONG rc = FALSE;
	THREAD;
	CHECKOBJECT(obj);

	add_mycomputer_icon(obj);

	ObtainSemaphore(&scsem);

	D(SHORTCUT,bug("attempting to load shortcuts..\n"));

	if ( (scppool = prefspool_create(0)) )
	{
		D(SHORTCUT,bug("created scppool at %p\n", scppool));

		if (prefspool_read(scppool, PREFS_PATH SHORTCUTS_FILE, SHORTCUTPREFSID, TRUE) == PREFSPOOL_IO_OK)
		{
			ULONG i = 0;
			APTR pl, pi; /* list, item */
			APTR o;
			STRPTR path, infopath;
			LONG *posx, *posy;

			D(SHORTCUT,bug("shortcuts.prefs exists, loading..\n"));

			if ( (pl = prefspool_item_get(scppool, NULL, DSI_LISTPOOL_SHORTCUT, NULL, NULL)) )
			{
				while ((pi = prefspool_item_get(scppool, pl, i | DSF_LISTPOOL, NULL, NULL)) && prefspool_item_get(scppool, pi, DSI_LISTPOOL_SHORTCUT_PATH, (APTR)&path, NULL))
				{
					if (path)
					{
						TEXT t[VOLUME_SIZE+1];
						LONG len, filetype, is_assign;

						filetype = MV_Icon_FileType_File;
						is_assign = FALSE;

						D(SHORTCUT,bug("Shortcut initial name:<%s>\n", path));

						len = strlen( path );
						if ( len && path[ len - 1 ] == '/')
						{
							/* XXX: That is actualy a workaround for some broken shortcuts containing / at the end of dir path */

							path[ len - 1 ] = '\0';
						}

						if ( len && path[ len - 1 ] == ':')
						{

							struct dlcnode *dlc;

							doslistcache_lock();

							dlc = doslistcache_find_dlcdevice_by_volumename(path);

							if (!dlc)
							{
								if ((dlc = doslistcache_find_dlcvolume_by_devicename(path)))
								{
									snprintf(t, sizeof(t), "%s:", dlc->name);
									path = t;
								}
							}

							if (dlc)
							{
								is_assign = FALSE;
								filetype = MV_Icon_FileType_Device;
							}
							else
							{
								is_assign = TRUE;
								filetype = MV_Icon_FileType_Directory;
							}

							doslistcache_unlock();
						}
						else if( isdir( path ) )
						{
							filetype = MV_Icon_FileType_Directory;
						}

						if ( (infopath = name_build_info(path)) )
						{
							ULONG viewid;

							methodstack_push_sync(obj, 3, OM_GET, MA_Viewgroup_ID, &viewid);

							if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, viewid)) )
							{
								D(SHORTCUT,bug("reading icon for %s (assign:%s)\n", path, is_assign?"Yes":"No"));

								if (icon_read(infopath, o, ICONTAG_Deficon, TRUE, ICONTAG_IsAssign, is_assign, TAG_DONE))
								{
									ULONG refine;

									D(SHORTCUT,bug("Pathinfo:%s\n", infopath));

									methodstack_push(o, 3, MUIM_Set, MA_Icon_IsShortcut, TRUE);
									methodstack_push_sync(o, 3, MUIM_Set, MA_Icon_Fade, TRUE);
									if (prefspool_item_get(scppool, pi, DSI_LISTPOOL_SHORTCUT_X, (APTR)&posx, NULL))
									{
										methodstack_push(o, 3, MUIM_Set, MA_Icon_X, *posx);
									}
									if (prefspool_item_get(scppool, pi, DSI_LISTPOOL_SHORTCUT_Y, (APTR)&posy, NULL))
									{
										methodstack_push(o, 3, MUIM_Set, MA_Icon_Y, *posy);
									}

									if (*posx != (LONG)NO_ICON_POSITION && *posy != (LONG)NO_ICON_POSITION)
									{
										methodstack_push(o, 3, MUIM_Set, MA_Icon_HasPos, TRUE);
									}

									methodstack_push_sync(o, 3, MUIM_Set, MA_Icon_Path, path );

									methodstack_push(o, 3, MUIM_Set, MA_Icon_FileType, filetype);

                                    /* We want to have proper deficon from the start here */

									methodstack_push_sync(o, 3, OM_GET, MA_Icon_Refine, &refine);
									if ( refine )
									{
										ULONG icontype;
										methodstack_push_sync(o, 3, OM_GET, MA_Icon_Type, &icontype);

										deficonpool_apply_default_icon(o, icontype, path, DEFICON_MIMETYPE_RECOGNIZE);
									}

									D(SHORTCUT,bug("add shortcut %s\n", path));
									methodstack_push(obj, 3, MM_Iconview_AddIcon, o, FALSE);
								}
								else
								{
									D(SHORTCUT,bug("failed to load icon for %s (assign:%s)\n", infopath, is_assign?"Yes":"No"));
									methodstack_push(o, 1, OM_RELEASE);
								}
								/* XXX */
								name_delete(infopath);
							}
						}
					}
					/* XXX */
					i++;
				}
			}
		}
		rc = TRUE;
	}
	/* XXX */

	ReleaseSemaphore(&scsem);

	#if USE_DOTBACKDROP
	/*
	 * Ok, this .backdrop mess really sucks. What to do if a removable is
	 * inserted and it has a .backdrop? The WB probably puts them on the
	 * desktop but I don't like it, it has no practical purpose. So instead
	 * I just load them from all devices mounted on bootup then don't bother.
	 */
	ITERATEDLC(n)
	{
		if (n->type == DLT_VOLUME)
		{
			if (backdrop_load(obj, n->name))
			{
				added = TRUE;
			}
		}
	}

	/*
	 * Transfer them to the new scheme.
	 */
	if (added)
	{
		#if USE_SHORTCUTS
		tr_shortcuts_save(obj, FALSE);
		#endif
	}
	#endif

	return (rc);
}


ULONG tr_shortcuts_save(APTR obj, ULONG reporterror)
{
	ULONG rc = FALSE;

	THREAD;

	ObtainSemaphore(&scsem);

	D(SHORTCUT,bug("attempting to save shortcuts..\n"));

	if (scppool)
	{
		D(SHORTCUT,bug("scppool exists, flushing..\n"));
		prefspool_flush(scppool);

		D(SHORTCUT,bug("collecting shortcuts..\n"));
		methodstack_push_sync(obj, 2, MM_Iconview_SaveShortcuts2, scppool);

		/* save the shortcuts */
		D(SHORTCUT,bug("saving to disk..\n"));
		if (prefspool_write(scppool, PREFS_PATH SHORTCUTS_FILE, SHORTCUTPREFSID, reporterror))
		{
			rc = TRUE;
		}
		D(SHORTCUT,bug("result: %ld\n", rc));
	}
	
	ReleaseSemaphore(&scsem);

	/* it is convenient to save desktop prefs state at the same time */
	tr_dprefs_save(obj);

	return (rc);
}


ULONG tr_shortcuts_add(APTR obj, STRPTR path, LONG x, LONG y, LONG save, LONG filetype)
{
	ULONG rc = FALSE;
	
	THREAD;
	CHECKOBJECT(obj);
	ASSERT(path);
		
	ObtainSemaphore(&scsem);

	if (scppool)
	{
		TEXT t[VOLUME_SIZE+1];
		STRPTR opath = path;
		LONG is_assign = FALSE;

		if (filetype != MV_Icon_FileType_Device && filetype != MV_Icon_FileType_File)
		{
			LONG oldtype = filetype;

			if (filetype == MV_Icon_FileType_Directory || isdir(path))
			{
				ULONG len;

				filetype = MV_Icon_FileType_Directory;
				len = strlen(path);

				if (len && path[len - 1] == ':')
				{
					is_assign = TRUE;

					/* Shortcuts added via REXX interface can not have filetype set.
					 * We must verify was it an assign or device/volume.
					 */

					if (oldtype == MV_Icon_FileType_None)
					{
						struct dlcnode *dlc;

						doslistcache_lock();

						dlc = doslistcache_find_dlcdevice_by_volumename(path);

						if (!dlc)
						{
							if ((dlc = doslistcache_find_dlcvolume_by_devicename(path)))
							{
								snprintf(t, sizeof(t), "%s:", dlc->name);
								path = t;
							}
						}

						if (dlc)
						{
							is_assign = FALSE;
							filetype = MV_Icon_FileType_Device;
						}

						doslistcache_unlock();
					}
				}
			}
		}

		D(SHORTCUT,bug("adding shortcut for path <%s>\n", path));
			
		if ( (path = name_build_info(path)) )
		{
			D(SHORTCUT,bug("lookup of %s\n", path));
			if (!methodstack_push_sync(obj, 2, MM_Iconview_FindShortcut, path))
			{
				APTR o;
				ULONG viewid;

				methodstack_push_sync(obj, 3, OM_GET, MA_Viewgroup_ID, &viewid);

				if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, viewid)) )
				{
					D(SHORTCUT,bug("reading icon for %s (assign:%s)\n", path, is_assign?"Yes":"No"));

					if (icon_read(path, o, ICONTAG_Deficon, TRUE, ICONTAG_IsAssign, is_assign, TAG_DONE))
					{
						ULONG refine;

						D(SHORTCUT,bug("read icon properly, setting position at x: %ld, y: %ld\n", x, y));

						//DB(("ICON:%s,%s,%s\n", getv(o,MA_Icon_Name),getv(o,MA_Icon_Path), getv(o,MA_Icon_PathInfo)));

						if (filetype == MV_Icon_FileType_Device)
						{
							hiddendrives_updatedevice(obj, opath, TRUE, x, y);
							methodstack_push(o, 3, MUIM_Set, MA_Icon_Type, MV_Icon_Type_Disk);
						}

						methodstack_push(o, 3, MUIM_Set, MA_Icon_IsShortcut, TRUE);
						methodstack_push(o, 3, MUIM_Set, MA_Icon_X, x);
						methodstack_push(o, 3, MUIM_Set, MA_Icon_Y, y);
						methodstack_push(o, 3, MUIM_Set, MA_Icon_HasPos, TRUE);
						methodstack_push(o, 3, MUIM_Set, MA_Icon_Fade, save ? FALSE : TRUE);

						methodstack_push(o, 3,
							MUIM_Set,
							MA_Icon_Path, opath
						);

						methodstack_push(o, 3, MUIM_Set, MA_Icon_FileType, filetype == MV_Icon_FileType_Device ? MV_Icon_FileType_Device : (filetype == MV_Icon_FileType_Directory ? MV_Icon_FileType_Directory : MV_Icon_FileType_File));

						/* We want to have proper deficon from the start here */

						methodstack_push_sync(o, 3, OM_GET, MA_Icon_Refine, &refine);
						if ( refine )
						{
							ULONG icontype;
							methodstack_push_sync(o, 3, OM_GET, MA_Icon_Type, &icontype);

							deficonpool_apply_default_icon(o, icontype, opath, DEFICON_MIMETYPE_RECOGNIZE);
						}

						methodstack_push_sync(obj, 3, MM_Iconview_AddIcon, o, FALSE); /* sync */

						if (save)
						{
							methodstack_push(obj, 1, MM_Iconview_DoLayout);

							/* XXX: when adding device shortcut we should not save shortcuts but desktop prefs file */
							if (tr_shortcuts_save(obj, TRUE))
							{
								D(SHORTCUT,bug("saved properly\n"));
								rc = TRUE;
							}
						}
					}
					else
					{
						methodstack_push(o, 1, OM_RELEASE);
					}
				}
			}

			name_delete(path);
		}
	}

	ReleaseSemaphore(&scsem);

	D(SHORTCUT,bug("rc: %ld\n", rc));

	return (rc);
}


ULONG tr_shortcuts_list_add(APTR obj, struct MinList *l)
{
	struct shortcutnode *sn, *nextsn;
	ULONG rc = TRUE;
	
	THREAD;
	CHECKOBJECT(obj);
	ASSERT(l);

	ObtainSemaphore(&scsem);

	ITERATELISTSAFE(sn, nextsn, l)
	{
		/* Calling tr_shortcuts_add() with scppool == NULL is safe */
		if (!tr_shortcuts_add(obj, sn->path, sn->x, sn->y, TRUE, sn->filetype))
		{
			rc = FALSE;
		}
		free(sn);
	}
	free(l);

	ReleaseSemaphore(&scsem);

	return (rc);
}


#endif /* USE_SHORTCUTS */
