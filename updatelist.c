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
 * $Id: updatelist.c,v 1.22 2018/08/20 21:55:17 itix Exp $
 */

#include "ambient.h"

/* public */
#include <clib/alib_protos.h>
#include <exec/memory.h>
#include <proto/exec.h>

/* private */
#include "updatelist.h"
#include "threads.h"
#include "mui_func.h"
#include "prefs.h"
#include "prefs_startup.h"
#include "keyshortcuts.h"
#include "mimetype.h"
#include "contextmenu.h"


static struct MinList updatelist;
static APTR updatepool;

struct updatenode {
	struct MinNode n;
	ULONG id;
	ULONG newval;
};


ULONG updatelist_begin(void)
{
	NEWLIST(&updatelist);

	if ( (updatepool = CreatePool(MEMF_ANY, 1024, 512)) )
	{
		return (TRUE);
	}
	return (FALSE);
}


void updatelist_add(ULONG id)
{
	struct updatenode *un;

	ITERATELIST(un, &updatelist)
	{
		if (un->id == id)
		{
			un->newval = TRUE;
			return; /* already in there */
		}
	}

	if ( (un = AllocPooled(updatepool, sizeof(*un))) ) /* no big deal if that one fails */
	{
		un->id = id;
		un->newval = TRUE;
		ADDTAIL(&updatelist, un);
	}
}

//#define LOADPREFSID DoMethod(app, MM_Application_LoadPrefs, un->id)
#define LOADPREFSID load_it = TRUE

void updatelist_end(ULONG doit, ULONG clearit)
{
	MAINTASK;

	if (doit)
	{
		struct updatenode *un;
		ULONG update_root = 0;
		ULONG update_window = 0;
		ULONG update_rootbg = FALSE;
		ULONG update_windowbg = FALSE;
		ULONG update_startup = FALSE;
		ULONG update_mimetypes = FALSE;
		ULONG update_mymorphos = FALSE;

		ASSERT(updatepool);

		ITERATELIST(un, &updatelist)
		{
			if (un->newval || clearit)
			{
				ULONG load_it = FALSE;

				switch (un->id)
				{
					/* XXX: hm.. what happens if the 3 cases change at once ? would suck I think.. perhaps find a way to clear the list if one of them is set */
					case DSI_BACKGROUND_ROOT_BGRENDER:
					case DSI_BACKGROUND_ROOT_BGCOLOR:
						update_root |= MF_Application_DisplayUpdate_Background;
						// fallthrough
					case DSI_BACKGROUND_ROOT:
						update_rootbg = TRUE;
						break;

					case DSI_BACKGROUND_TRANSITION:
					case DSI_BACKGROUND_ROOT_REFRESH_DELAY:
						LOADPREFSID;
						break;

					case DSI_BACKGROUND_WINDOW_BGRENDER:
					case DSI_BACKGROUND_WINDOW_BGCOLOR:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Background;
						// fallthrough
					case DSI_BACKGROUND_WINDOW:
						update_windowbg = TRUE;
						break;

					case DSI_COLOR_ROOT:
					case DSI_COLOR_ROOT2:
						LOADPREFSID;
						update_root |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_FONT_ROOT:
					case DSI_FONT_ROOT_SMALL:
					case DSI_FONT_ROOT_EFFECT:
						LOADPREFSID;
						update_root |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_FONT_ROOTSPACE:
						LOADPREFSID;
						update_root |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_COLOR_WINDOW:
					case DSI_COLOR_WINDOW2:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_FONT_WINDOW:
					case DSI_FONT_WINDOW_EFFECT:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_ICON_MINSIZE:
					case DSI_ICON_MAXSIZE:
					case DSI_ICON_DEFGHOSTED:
					case DSI_ICON_SHORTCUTSIDENTIFIER:
					case DSI_ICON_APPICONSIDENTIFIER:
						LOADPREFSID;
						update_root |= MF_Application_DisplayUpdate_Size;
						update_window |= MF_Application_DisplayUpdate_Size;
						break;

					/* hover update. no need to refresh anything */
					case DSI_ICON_HOVER:
						LOADPREFSID;
						break;

					case DSI_ICON_SEPARATEFILES:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Size;
						break;

					case DSI_COLOR_LASSO:
						LOADPREFSID;
						break;

					case DSI_FONT_WINDOWSPACE:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						break;

					#if USE_SOLIDDRAG
					case DSI_DRAGDROP_DISPLAY:
						LOADPREFSID;
					#endif

					case DSI_DRAGDROP_DROPEFFECT:
						LOADPREFSID;
						break;

					case DSI_DRAGDROP_TINTVAL:
						LOADPREFSID;
						break;

					case DSI_DRAGDROP_TINTFADEVAL:
						LOADPREFSID;
						break;

					case DSI_DRAGDROP_BRIGHTENVAL:
						LOADPREFSID;
						break;

					case DSI_DRAGDROP_DARKENVAL:
						LOADPREFSID;
						break;

					case DSI_FASTLIST_COLOR_FILE_FG:
					case DSI_FASTLIST_COLOR_FILE_BG:
					case DSI_FASTLIST_COLOR_FILE_SEL_FG:
					case DSI_FASTLIST_COLOR_FILE_SEL_BG:
					case DSI_FASTLIST_COLOR_DIRECTORY_FG:
					case DSI_FASTLIST_COLOR_DIRECTORY_BG:
					case DSI_FASTLIST_COLOR_DIRECTORY_SEL_FG:
					case DSI_FASTLIST_COLOR_DIRECTORY_SEL_BG:
					case DSI_FASTLIST_COLOR_SOFTLINK_FG:
					case DSI_FASTLIST_COLOR_SOFTLINK_BG:
					case DSI_FASTLIST_COLOR_HARDLINK_FG:
					case DSI_FASTLIST_COLOR_HARDLINK_BG:
					case DSI_FASTLIST_COLOR_VOLUME_FG:
					case DSI_FASTLIST_COLOR_VOLUME_BG:
					case DSI_FASTLIST_COLOR_ASSIGN_FG:
					case DSI_FASTLIST_COLOR_ASSIGN_BG:
					case DSI_FASTLIST_COLOR_SOURCE_FG:
					case DSI_FASTLIST_COLOR_SOURCE_BG:
					case DSI_FASTLIST_COLOR_DESTINATION_FG:
					case DSI_FASTLIST_COLOR_DESTINATION_BG:
					case DSI_FASTLIST_COLOR_COLUMN_FG:
					case DSI_FASTLIST_FONT:
					case DSI_FASTLIST_HILIGHTED_SORTING_COLUMN:
					case DSI_FASTLIST_ALTERNATED_ROWS:
					case DSI_FASTLIST_BOLD_DIRECTORIES:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_FASTLIST_WINDOW_BG:

						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Background;
						update_windowbg = TRUE;
						break;

					case DSI_FASTLIST_DEFAULT_FORMAT_FILES:
					case DSI_FASTLIST_DEFAULT_FORMAT_DEVICES:
					case DSI_FASTLIST_DEFAULT_MODE_FILES:
					case DSI_FASTLIST_DEFAULT_MODE_DEVICES:
					case DSI_FASTLIST_COMPACT_SIZE_DISPLAY:
					case DSI_FASTLIST_ASSIGN_DISPLAY:
					case DSI_FASTLIST_INLINE_EDIT_MODE:
					case DSI_FASTLIST_SELECTION_MODE:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						break;


					/* XXX: force a reselection or so ? */
					case DSI_ICON_SELECTEFFECT:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						update_root |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_ICON_TINTVAL:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						update_root |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_ICON_TINTFADEVAL:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						update_root |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_ICON_BRIGHTENVAL:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						update_root |= MF_Application_DisplayUpdate_Fonts;
						break;

					case DSI_ICON_DARKENVAL:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_Fonts;
						update_root |= MF_Application_DisplayUpdate_Fonts;
						break;
					/* XXX: til there.. */

					case DSI_PANEL_ZIPSPEED:
					case DSI_PANEL_AUTOSAVE_DROP:
					case DSI_PANEL_AUTOSAVE_DELETE:
					case DSI_PANEL_AUTOSAVE_MOVE:
					case DSI_PANEL_AUTOSAVE_WINDOWPOS:
					case DSI_PANEL_HIGHLIGHT_EFFECT:
					case DSI_PANEL_DRAGDROP_EFFECT:
					case DSI_PANEL_SELECTED_EFFECT:
					case DSI_PANEL_HIGHLIGHT_BRIGHTEN:
					case DSI_PANEL_HIGHLIGHT_DARKEN:
					case DSI_PANEL_DRAGDROP_BRIGHTEN:
					case DSI_PANEL_DRAGDROP_DARKEN:
					case DSI_PANEL_SELECTED_BRIGHTEN:
					case DSI_PANEL_SELECTED_DARKEN:
					case DSI_PANEL_HIGHLIGHT_TINT:
					case DSI_PANEL_DRAGDROP_TINT:
					case DSI_PANEL_SELECTED_TINT:
					case DSI_PANEL_HIGHLIGHT_TINTFADE:
					case DSI_PANEL_DRAGDROP_TINTFADE:
					case DSI_PANEL_SELECTED_TINTFADE:
					case DSI_PANEL_LAYOUT_GRID:
						LOADPREFSID;
						break;

					case DSI_MISC_REMEMBER_WINDOWS:
					case DSI_MISC_REMEMBER_DOCUMENTS:
						LOADPREFSID;
						update_startup = TRUE;
						break;

					/* disable drive info XXX: should rebuild drive icons */
					case DSI_MISC_DRIVE_INFO:
						LOADPREFSID;
						update_root |= MF_Application_DisplayUpdate_Size;	/* XXX: Sucks, but doesn't cause nasty flicker */
						update_window |= MF_Application_DisplayUpdate_Size;
						break;

					/* we should update toolbars... */
					case DSI_TOOLBAR_DISPLAYMODE:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_UI;
						break;

					case DSI_WINDOW_STATUSBARCOLOR: // bitRocky
						update_windowbg = TRUE; // XXX: to update the status bar text, is there a other way?
						// fall through
					case DSI_WINDOW_STATUSBARFORMAT:
					case DSI_TOOLBAR_DEFINITION:
					case DSI_TOOLBAR_BUTTONFRAMESPEC:
					case DSI_TOOLBAR_BUTTONIMAGESPEC:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_UI;
						break;

					case DSI_TOOLBAR_BROWSERMODE:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_UI;
						break;

					case DSI_WINDOW_DEFAULT_LEFT:
					case DSI_WINDOW_DEFAULT_TOP:
					case DSI_WINDOW_DEFAULT_WIDTH:
					case DSI_WINDOW_DEFAULT_HEIGHT:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_UI;
						break;
					case DSI_WINDOW_DEFAULT_VIEW:
					case DSI_WINDOW_DEFAULT_VIEWMODE:
					case DSI_WINDOW_INHERIT_VIEWMODE:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_UI;
						break;


					case DSI_LISTPOOL_KEYSHORTCUT_CHANGED:
						LOADPREFSID;
						update_window |= MF_Application_DisplayUpdate_UI;
						break;

					case DSI_MISC_MYMORPHOSICON:
						LOADPREFSID;
						update_mymorphos = TRUE;
						break;

					case DSI_MISC_SMARTFILEOPERATIONS:
						LOADPREFSID;
						update_mimetypes = TRUE;
						break;

					case DSI_MISC_TRAPMORE:
					case DSI_MISC_TRAPMULTIVIEW:
					case DSI_MISC_CREATEICONFORNEWDRAWER:
					case DSI_MISC_DESKTOPDOUBLECLICK:
						LOADPREFSID;
						break;

					case DSI_MISC_CONTEXTMENUIMAGES:
						LOADPREFSID;
						break;

					case DSI_BOOKMARKS_SHOWONLY:
						LOADPREFSID;
						break;
					#ifdef DEBUG
					default:
						SDB(("unknown id: 0x%lx\n", un->id));
					#endif
				}

				un->newval = FALSE;

				if (load_it)
				{
					DoMethod(app, MM_Application_LoadPrefs, un->id, FALSE);
				}
			}
		}

		/*
		 * Global stuff.
		 */
		DoMethod(app, MM_Application_LoadBackground, /* XXX: hm.. really ? */
			(update_rootbg ? (MF_Application_LoadBackground_ClearRoot | MF_Application_LoadBackground_Root) : 0) |
			(update_windowbg ? (MF_Application_LoadBackground_ClearWindow | MF_Application_LoadBackground_Window) : 0));

		if (update_root)
		{
			DoMethod(app, MM_Application_DisplayUpdate, update_root | MF_Application_DisplayUpdate_Root);
		}

		if (update_window)
		{
			DoMethod(app, MM_Application_DisplayUpdate, update_window | MF_Application_DisplayUpdate_Windows);
		}

		if (update_mymorphos)
		{
			APTR wo = (APTR)DoMethod(app, MM_Application_FindWindowByType, MV_Window_Type_Rootview);
			APTR vo = wo ? (APTR)getv(wo, MA_Window_Viewobj) : NULL;

			if (vo != NULL)
				set(vo, MA_Iconview_ShowMyMorphos, _conf(misc_mymorphosicon));
		}

		if (update_startup)
		{
			if (!_conf(misc_remember_windows) && !_conf(misc_remember_documents))
			{
				sprefs_cleanup();
			}
			else
			{
				sprefs_setup();
			}
		}

		if (update_mimetypes)
		{
			DB(("Reload recognition database\n"));
			mimetype_invalidate(NULL, NULL);
			mimetype_load_database(NULL);
		}

	}

	if (clearit)
	{
		DeletePool(updatepool);

		#ifdef DEBUG
		NEWLIST(&updatelist);
		updatepool = NULL;
		#endif
	}
}
