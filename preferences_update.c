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
 * $Id: preferences_update.c,v 1.12 2018/08/20 21:55:17 itix Exp $
 */

#include "ambient.h"

/* public */
#include <clib/alib_protos.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <clib/debug_protos.h>
/* private */
#include "ambient_cat.h"
#include "updatelist.h"
#include "threads.h"
#include "mui_func.h"
#include "prefs.h"
#include "prefs_startup.h"
#include "keyshortcuts.h"
#include "mimetype.h"
#include "contextmenu.h"
#include "background.h"
#include "sizes.h"
#include "preferences_update.h"


#if USE_SILLY_ICONPREFS
#define ICONEFFECT_UPPERBOUND		2 + MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_GRADIENT - MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_NORMAL
#else
#define ICONEFFECT_UPPERBOUND		2 + MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_OUTLINE - MSG_PREFSWIN_ICONDISPLAY_TEXT_MODE_NORMAL
#endif

#define ICONSIZE_UPPERBOUND			2 + MSG_PREFSWIN_ICONDISPLAY_RESIZE_HUGE - MSG_PREFSWIN_ICONDISPLAY_RESIZE_MICRO
#define VIEWMODE_UPPERBOUND			2 + MSG_VIEW_THUMB - MSG_VIEW_ICONS
#define VIEWEDIT_UPPERBOUND			2 + MSG_PREFSWIN_LISTER_INLINE_EDIT_LONG_LEFTBUTTON - MSG_PREFSWIN_LISTER_INLINE_EDIT_DISABLED
#define VIEWSEL_UPPERBOUND			2 + MSG_PREFSWIN_LISTER_SELECTION_ALWAYS - MSG_PREFSWIN_LISTER_SELECTION_DEFAULT
#define PANELEFFECT_UPPERBOUND		2 + MSG_PREFSWIN_PANEL_EFFECT_TINT_FADE - MSG_PREFSWIN_PANEL_EFFECT_NONE
#define PANELSPEED_UPPERBOUND		3
#define PANELSIZE_UPPERBOUND		4
#define DRIVE_UPPERBOUND			2 + MSG_PREFSWIN_ICONDISPLAY_DRIVE_TEXTGAUGE - MSG_PREFSWIN_ICONDISPLAY_DRIVE_NONE
#define TOOLBARVIEWS_UPPERBOUND		2 + MSG_PREFSWIN_TOOLBAR_VIEW_LIST - MSG_PREFSWIN_TOOLBAR_VIEW_ICON
#define TOOLBARVMODES_UPPERBOUND	2 + MSG_VIEW_THUMB - MSG_VIEW_ICONS
#define BOOKMARKS_UPPERBOUND		2 + MSG_PREFSWIN_BOOKMARKS_DISPLAYAS_LOCATIONS - MSG_PREFSWIN_BOOKMARKS_DISPLAYAS_NAMESANDLOCATIONS

static const struct Key2DSI k2d[] = {
	{ "BACKGROUND_ROOT_BGRENDER",			PREFS_TYPE_LONG,	0,	6,							DSI_BACKGROUND_ROOT_BGRENDER },
	{ "BACKGROUND_ROOT_BGCOLOR",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_BACKGROUND_ROOT_BGCOLOR},
	{ "BACKGROUND_ROOT",					PREFS_TYPE_STRPTR,	0,	0,							DSI_BACKGROUND_ROOT},
	{ "BACKGROUND_ROOT_REFRESH_DELAY",		PREFS_TYPE_LONG,	0,	40,							DSI_BACKGROUND_ROOT_REFRESH_DELAY},
	{ "BACKGROUND_WINDOW_BGRENDER",			PREFS_TYPE_LONG,	0,	1,							DSI_BACKGROUND_WINDOW_BGRENDER},
	{ "BACKGROUND_WINDOW_BGCOLOR",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_BACKGROUND_WINDOW_BGCOLOR},
	{ "BACKGROUND_WINDOW",					PREFS_TYPE_STRPTR,	0,	0,							DSI_BACKGROUND_WINDOW},
	{ "BACKGROUND_TRANSITION",				PREFS_TYPE_LONG,     0, 0,							DSI_BACKGROUND_TRANSITION},
	{ "COLOR_ROOT",							PREFS_TYPE_PENSPEC,	0,	0,							DSI_COLOR_ROOT},
	{ "COLOR_ROOT2",						PREFS_TYPE_PENSPEC,	0,	0,							DSI_COLOR_ROOT2},
	{ "FONT_ROOT",							PREFS_TYPE_STRPTR,	0,	0,							DSI_FONT_ROOT},
	{ "FONT_ROOT_SMALL",					PREFS_TYPE_STRPTR,	0,	0,							DSI_FONT_ROOT_SMALL},
	{ "FONT_ROOT_EFFECT",					PREFS_TYPE_LONG,	0,	ICONEFFECT_UPPERBOUND,		DSI_FONT_ROOT_EFFECT},
#if USE_SILLY_ICONPREFS
	{ "FONT_ROOTSPACE",						PREFS_TYPE_LONG,	0,	0,							DSI_FONT_ROOTSPACE},
#endif
	{ "COLOR_WINDOW",						PREFS_TYPE_PENSPEC,	0,	0,							DSI_COLOR_WINDOW},
	{ "COLOR_WINDOW2",						PREFS_TYPE_PENSPEC,	0,	0,							DSI_COLOR_WINDOW2},
	{ "FONT_WINDOW",						PREFS_TYPE_STRPTR,	0,	0,							DSI_FONT_WINDOW},
	{ "FONT_WINDOW_EFFECT",					PREFS_TYPE_LONG,	0,	ICONEFFECT_UPPERBOUND,		DSI_FONT_WINDOW_EFFECT},
	{ "ICON_MINSIZE",						PREFS_TYPE_LONG,	0,	ICONSIZE_UPPERBOUND,		DSI_ICON_MINSIZE},
	{ "ICON_MAXSIZE",						PREFS_TYPE_LONG,	0,	ICONSIZE_UPPERBOUND,		DSI_ICON_MAXSIZE},
	{ "ICON_DEFGHOSTED",					PREFS_TYPE_LONG,	0,	1,							DSI_ICON_DEFGHOSTED},
	{ "ICON_SHORTCUTSIDENTIFIER",			PREFS_TYPE_LONG,	0,	1,							DSI_ICON_SHORTCUTSIDENTIFIER},
	{ "ICON_APPICONSIDENTIFIER",			PREFS_TYPE_LONG,	0,	1,							DSI_ICON_APPICONSIDENTIFIER},
	{ "ICON_HOVER",							PREFS_TYPE_LONG,	0,	1,							DSI_ICON_HOVER},
	{ "ICON_SEPARATEFILES",					PREFS_TYPE_LONG,	0,	1,							DSI_ICON_SEPARATEFILES},
	{ "COLOR_LASSO",						PREFS_TYPE_PENSPEC,	0,	0,							DSI_COLOR_LASSO},
#if USE_SILLY_ICONPREFS
	{ "FONT_WINDOWSPACE",					PREFS_TYPE_LONG,	0,	0xFFFF,						DSI_FONT_WINDOWSPACE},
#endif
	{ "DRAGDROP_TINTVAL",					PREFS_TYPE_PENSPEC,	0,	0,							DSI_DRAGDROP_TINTVAL},

#if USE_DROP_EFFECT_PREFS
#if USE_SOLIDDRAG
	{ "DRAGDROP_DISPLAY",					PREFS_TYPE_LONG,	0,	0,							DSI_DRAGDROP_DISPLAY},
#endif
	{ "DRAGDROP_DROPEFFECT",				PREFS_TYPE_LONG,	0,	0,							DSI_DRAGDROP_DROPEFFECT},
	{ "DRAGDROP_TINTFADEVAL",				PREFS_TYPE_PENSPEC,	0,	0,							DSI_DRAGDROP_TINTFADEVAL},
	{ "DRAGDROP_BRIGHTENVAL",				PREFS_TYPE_LONG,	0,	0,							DSI_DRAGDROP_BRIGHTENVAL},
	{ "DRAGDROP_DARKENVAL",					PREFS_TYPE_LONG,	0,	0,							DSI_DRAGDROP_DARKENVAL},
#endif
	{ "FASTLIST_COLOR_FILE_FG",				PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_FILE_FG},
	{ "FASTLIST_COLOR_FILE_BG",				PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_FILE_BG},
	{ "FASTLIST_COLOR_FILE_SEL_FG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_FILE_SEL_FG},
	{ "FASTLIST_COLOR_FILE_SEL_BG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_FILE_SEL_BG},
	{ "FASTLIST_COLOR_DIRECTORY_FG",		PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_DIRECTORY_FG},
	{ "FASTLIST_COLOR_DIRECTORY_BG",		PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_DIRECTORY_BG},
	{ "FASTLIST_COLOR_DIRECTORY_SEL_FG",	PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_DIRECTORY_SEL_FG},
	{ "FASTLIST_COLOR_DIRECTORY_SEL_BG",	PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_DIRECTORY_SEL_BG},
	{ "FASTLIST_COLOR_SOFTLINK_FG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_SOFTLINK_FG},
	{ "FASTLIST_COLOR_SOFTLINK_BG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_SOFTLINK_BG},
	{ "FASTLIST_COLOR_HARDLINK_FG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_HARDLINK_FG},
	{ "FASTLIST_COLOR_HARDLINK_BG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_HARDLINK_BG},
	{ "FASTLIST_COLOR_VOLUME_FG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_VOLUME_FG},
	{ "FASTLIST_COLOR_VOLUME_BG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_VOLUME_BG},
	{ "FASTLIST_COLOR_ASSIGN_FG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_ASSIGN_FG},
	{ "FASTLIST_COLOR_ASSIGN_BG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_ASSIGN_BG},
	{ "FASTLIST_COLOR_SOURCE_FG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_SOURCE_FG},
	{ "FASTLIST_COLOR_SOURCE_BG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_SOURCE_BG},
	{ "FASTLIST_COLOR_DESTINATION_FG",		PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_DESTINATION_FG},
	{ "FASTLIST_COLOR_DESTINATION_BG",		PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_DESTINATION_BG},
	{ "FASTLIST_COLOR_COLUMN_FG",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_FASTLIST_COLOR_COLUMN_FG},
	{ "FASTLIST_FONT",						PREFS_TYPE_STRPTR,	0,	0,							DSI_FASTLIST_FONT},
	{ "FASTLIST_HILIGHTED_SORTING_COLUMN",	PREFS_TYPE_LONG,	0,	1,							DSI_FASTLIST_HILIGHTED_SORTING_COLUMN},
	{ "FASTLIST_ALTERNATED_ROWS",			PREFS_TYPE_LONG,	0,	1,							DSI_FASTLIST_ALTERNATED_ROWS},
	{ "FASTLIST_BOLD_DIRECTORIES",			PREFS_TYPE_LONG,	0,	1,							DSI_FASTLIST_BOLD_DIRECTORIES},
	{ "FASTLIST_WINDOW_BG",					PREFS_TYPE_LONG,	0,	1,							DSI_FASTLIST_WINDOW_BG},
	{ "FASTLIST_DEFAULT_FORMAT_FILES",		PREFS_TYPE_STRPTR,	0,	0,							DSI_FASTLIST_DEFAULT_FORMAT_FILES},
	{ "FASTLIST_DEFAULT_FORMAT_DEVICES",	PREFS_TYPE_STRPTR,	0,	0,							DSI_FASTLIST_DEFAULT_FORMAT_DEVICES},
	{ "FASTLIST_DEFAULT_MODE_FILES",		PREFS_TYPE_STRPTR,	0,	0,							DSI_FASTLIST_DEFAULT_MODE_FILES},
	{ "FASTLIST_DEFAULT_MODE_DEVICES",		PREFS_TYPE_STRPTR,	0,	0,							DSI_FASTLIST_DEFAULT_MODE_DEVICES},
	{ "FASTLIST_COMPACT_SIZE_DISPLAY",		PREFS_TYPE_LONG,	0,	1,							DSI_FASTLIST_COMPACT_SIZE_DISPLAY},
	{ "FASTLIST_ASSIGN_DISPLAY",			PREFS_TYPE_LONG,	0,	1,							DSI_FASTLIST_ASSIGN_DISPLAY},
	{ "FASTLIST_INLINE_EDIT_MODE",			PREFS_TYPE_LONG,	0,	VIEWEDIT_UPPERBOUND,		DSI_FASTLIST_INLINE_EDIT_MODE},
	{ "FASTLIST_SELECTION_MODE",			PREFS_TYPE_LONG,	0,	VIEWSEL_UPPERBOUND,			DSI_FASTLIST_SELECTION_MODE},
#if !USE_DROP_EFFECT_PREFS
	{ "ICON_SELECTEFFECT",					PREFS_TYPE_LONG,	0,	0,							DSI_ICON_SELECTEFFECT},
	{ "ICON_TINTVAL",						PREFS_TYPE_PENSPEC, 0,	0,							DSI_ICON_TINTVAL},
	{ "ICON_TINTFADEVAL",					PREFS_TYPE_PENSPEC,	0,	0,							DSI_ICON_TINTFADEVAL},
	{ "ICON_BRIGHTENVAL",					PREFS_TYPE_LONG,	0,	0,							DSI_ICON_BRIGHTENVAL},
	{ "ICON_DARKENVAL",						PREFS_TYPE_LONG,	0,	0,							DSI_ICON_DARKENVAL},
#endif
	{ "PANEL_ZIPSPEED",						PREFS_TYPE_LONG,	0,	PANELSPEED_UPPERBOUND,		DSI_PANEL_ZIPSPEED},
	{ "PANEL_AUTOSAVE_DROP",				PREFS_TYPE_LONG,	0,	1,							DSI_PANEL_AUTOSAVE_DROP},
	{ "PANEL_AUTOSAVE_DELETE",				PREFS_TYPE_LONG,	0,	1,							DSI_PANEL_AUTOSAVE_DELETE},
	{ "PANEL_AUTOSAVE_MOVE",				PREFS_TYPE_LONG,	0,	1,							DSI_PANEL_AUTOSAVE_MOVE},
	{ "PANEL_AUTOSAVE_WINDOWPOS",			PREFS_TYPE_LONG,	0,	1,							DSI_PANEL_AUTOSAVE_WINDOWPOS},
	{ "PANEL_HIGHLIGHT_EFFECT",				PREFS_TYPE_LONG,	0,	PANELEFFECT_UPPERBOUND,		DSI_PANEL_HIGHLIGHT_EFFECT},
	{ "PANEL_DRAGDROP_EFFECT",				PREFS_TYPE_LONG,	0,	PANELEFFECT_UPPERBOUND,		DSI_PANEL_DRAGDROP_EFFECT},
	{ "PANEL_SELECTED_EFFECT",				PREFS_TYPE_LONG,	0,	PANELEFFECT_UPPERBOUND,		DSI_PANEL_SELECTED_EFFECT},
	{ "PANEL_HIGHLIGHT_BRIGHTEN",			PREFS_TYPE_LONG,	0,	255,						DSI_PANEL_HIGHLIGHT_BRIGHTEN},
	{ "PANEL_HIGHLIGHT_DARKEN",				PREFS_TYPE_LONG,	0,	255,						DSI_PANEL_HIGHLIGHT_DARKEN},
	{ "PANEL_DRAGDROP_BRIGHTEN",			PREFS_TYPE_LONG,	0,	255,						DSI_PANEL_DRAGDROP_BRIGHTEN},
	{ "PANEL_DRAGDROP_DARKEN",				PREFS_TYPE_LONG,	0,	255,						DSI_PANEL_DRAGDROP_DARKEN},
	{ "PANEL_SELECTED_BRIGHTEN",			PREFS_TYPE_LONG,	0,	255,						DSI_PANEL_SELECTED_BRIGHTEN},
	{ "PANEL_SELECTED_DARKEN",				PREFS_TYPE_LONG,	0,	255,						DSI_PANEL_SELECTED_DARKEN},
	{ "PANEL_HIGHLIGHT_TINT",				PREFS_TYPE_PENSPEC,	0,	0,							DSI_PANEL_HIGHLIGHT_TINT},
	{ "PANEL_DRAGDROP_TINT",				PREFS_TYPE_PENSPEC,	0,	0,							DSI_PANEL_DRAGDROP_TINT},
	{ "PANEL_SELECTED_TINT",				PREFS_TYPE_PENSPEC,	0,	0,							DSI_PANEL_SELECTED_TINT},
	{ "PANEL_HIGHLIGHT_TINTFADE",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_PANEL_HIGHLIGHT_TINTFADE},
	{ "PANEL_DRAGDROP_TINTFADE",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_PANEL_DRAGDROP_TINTFADE},
	{ "PANEL_SELECTED_TINTFADE",			PREFS_TYPE_PENSPEC,	0,	0,							DSI_PANEL_SELECTED_TINTFADE},
	{ "PANEL_LAYOUT_GRID",					PREFS_TYPE_LONG,	0,	1,							DSI_PANEL_LAYOUT_GRID},
	{ "MISC_REMEMBER_WINDOWS",				PREFS_TYPE_LONG,	0,	1,							DSI_MISC_REMEMBER_WINDOWS},
	{ "MISC_REMEMBER_DOCUMENTS",			PREFS_TYPE_LONG,	0,	1,							DSI_MISC_REMEMBER_DOCUMENTS},
	{ "MISC_DRIVE_INFO",					PREFS_TYPE_LONG,	0,	DRIVE_UPPERBOUND	,		DSI_MISC_DRIVE_INFO},
	{ "TOOLBAR_DISPLAYMODE",				PREFS_TYPE_LONG,	0,	3,							DSI_TOOLBAR_DISPLAYMODE},
	{ "WINDOW_STATUSBARFORMAT",				PREFS_TYPE_STRPTR,	0,	0,							DSI_WINDOW_STATUSBARFORMAT},
	{ "TOOLBAR_DEFINITION",					PREFS_TYPE_STRPTR,	0,	0,							DSI_TOOLBAR_DEFINITION},
	{ "TOOLBAR_BUTTONFRAMESPEC",			PREFS_TYPE_STRPTR,	0,	0,							DSI_TOOLBAR_BUTTONFRAMESPEC},
	{ "TOOLBAR_BUTTONIMAGESPEC",			PREFS_TYPE_STRPTR,	0,	0,							DSI_TOOLBAR_BUTTONIMAGESPEC},
	{ "TOOLBAR_BROWSERMODE",				PREFS_TYPE_LONG,	0,	1,							DSI_TOOLBAR_BROWSERMODE},
	{ "WINDOW_DEFAULT_LEFT",				PREFS_TYPE_LONG,	0,	0xFFFF,						DSI_WINDOW_DEFAULT_LEFT},
	{ "WINDOW_DEFAULT_TOP",					PREFS_TYPE_LONG,	0,	0xFFFF,						DSI_WINDOW_DEFAULT_TOP},
	{ "WINDOW_DEFAULT_WIDTH",				PREFS_TYPE_LONG,	0,	0xFFFF,						DSI_WINDOW_DEFAULT_WIDTH},
	{ "WINDOW_DEFAULT_HEIGHT",				PREFS_TYPE_LONG,	0,	0xFFFF,						DSI_WINDOW_DEFAULT_HEIGHT},
	{ "WINDOW_DEFAULT_VIEW",				PREFS_TYPE_LONG,	0,	TOOLBARVIEWS_UPPERBOUND,	DSI_WINDOW_DEFAULT_VIEW},
	{ "WINDOW_DEFAULT_VIEWMODE",			PREFS_TYPE_LONG,	0,	TOOLBARVMODES_UPPERBOUND,	DSI_WINDOW_DEFAULT_VIEWMODE},
	{ "WINDOW_INHERIT_VIEWMODE",			PREFS_TYPE_LONG,	0,	1,							DSI_WINDOW_INHERIT_VIEWMODE},
	{ "LISTPOOL_KEYSHORTCUT_CHANGED",		PREFS_TYPE_LONG,	0,	0x7FFFFFFF,					DSI_LISTPOOL_KEYSHORTCUT_CHANGED},
	{ "MISC_MYMORPHOSICON",					PREFS_TYPE_LONG,	0,	1,							DSI_MISC_MYMORPHOSICON},
	{ "MISC_SMARTFILEOPERATIONS",			PREFS_TYPE_LONG,	0,	1,							DSI_MISC_SMARTFILEOPERATIONS},
	{ "MISC_TRAPMORE",						PREFS_TYPE_LONG,	0,	1,							DSI_MISC_TRAPMORE},
	{ "MISC_TRAPMULTIVIEW",					PREFS_TYPE_LONG,	0,	1,							DSI_MISC_TRAPMULTIVIEW},
	{ "MISC_CREATEICONFORNEWDRAWER",		PREFS_TYPE_LONG,	0,	1,							DSI_MISC_CREATEICONFORNEWDRAWER},
	{ "MISC_DESKTOPDOUBLECLICK",			PREFS_TYPE_LONG,	0,	1,							DSI_MISC_DESKTOPDOUBLECLICK},
	{ "MISC_CONTEXTMENUIMAGES",				PREFS_TYPE_LONG,	0,	1,							DSI_MISC_CONTEXTMENUIMAGES},
	{ "BOOKMARKS_SHOWONLY",					PREFS_TYPE_LONG,	0,	BOOKMARKS_UPPERBOUND,		DSI_BOOKMARKS_SHOWONLY},				
	{ "WINDOW_STATUSBARCOLOR",				PREFS_TYPE_PENSPEC, 0,  0,							DSI_WINDOW_STATUSBARCOLOR}, // bitRocky
};

#define ReloadPrefs() update_reloadprefs = TRUE

/*
	Each setting is accessible from ARexx with a key name 
	based on the DSI_XXX define.

	The Arexx argument is passed to the matching arexxfindkey function
	which returns if found the array index. See prefs.h

	If the key isn't found, it returns -1 meaning not found.
*/

static ULONG arexxfindkey(STRPTR key)
{
	ULONG i;
	
	for (i=0; i < sizeof(k2d) / sizeof(struct Key2DSI); i++)
	{
		if (Stricmp(key, k2d[i].key) == 0)
		{
			return i;
		}
	}
	return -1;
}


/*
	This function applies the preferences change.
	Inspired from updatelist.c
*/

ULONG preferences_apply_update(Object *app, ULONG id)
{
	ULONG update_root = 0;
	ULONG update_window = 0;
	ULONG update_rootbg = FALSE;
	ULONG update_windowbg = FALSE;
	ULONG update_startup = FALSE;
	ULONG update_mimetypes = FALSE;
	ULONG update_mymorphos = FALSE;
	ULONG update_reloadprefs = FALSE;

	switch (id)
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
			ReloadPrefs();
		break;

		case DSI_BACKGROUND_WINDOW_BGRENDER:
		case DSI_BACKGROUND_WINDOW_BGCOLOR:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Background;
			// fallthrough
		case DSI_BACKGROUND_WINDOW:
			update_windowbg = TRUE;
		break;

		case DSI_COLOR_ROOT:
		case DSI_COLOR_ROOT2:
			ReloadPrefs();
			update_root |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_FONT_ROOT:
		case DSI_FONT_ROOT_SMALL:
		case DSI_FONT_ROOT_EFFECT:
			ReloadPrefs();
			update_root |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_FONT_ROOTSPACE:
			ReloadPrefs();
			update_root |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_COLOR_WINDOW:
		case DSI_COLOR_WINDOW2:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_FONT_WINDOW:
		case DSI_FONT_WINDOW_EFFECT:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_ICON_MINSIZE:
		case DSI_ICON_MAXSIZE:
		case DSI_ICON_DEFGHOSTED:
		case DSI_ICON_SHORTCUTSIDENTIFIER:
		case DSI_ICON_APPICONSIDENTIFIER:
			ReloadPrefs();
			update_root |= MF_Application_DisplayUpdate_Size;
			update_window |= MF_Application_DisplayUpdate_Size;
		break;

		/* hover update. no need to refresh anything */
		case DSI_ICON_HOVER:
			ReloadPrefs();
		break;

		case DSI_ICON_SEPARATEFILES:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Size;
		break;

		case DSI_COLOR_LASSO:
			ReloadPrefs();
		break;

		case DSI_FONT_WINDOWSPACE:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
		break;

#if USE_SOLIDDRAG
		case DSI_DRAGDROP_DISPLAY:
			ReloadPrefs();
#endif

		case DSI_DRAGDROP_DROPEFFECT:
			ReloadPrefs();
		break;

		case DSI_DRAGDROP_TINTVAL:
			ReloadPrefs();
		break;

		case DSI_DRAGDROP_TINTFADEVAL:
			ReloadPrefs();
		break;

		case DSI_DRAGDROP_BRIGHTENVAL:
			ReloadPrefs();
		break;

		case DSI_DRAGDROP_DARKENVAL:
			ReloadPrefs();
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
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_FASTLIST_WINDOW_BG:
			ReloadPrefs();
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
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
		break;


		/* XXX: force a reselection or so ? */
		case DSI_ICON_SELECTEFFECT:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
			update_root |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_ICON_TINTVAL:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
			update_root |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_ICON_TINTFADEVAL:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
			update_root |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_ICON_BRIGHTENVAL:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_Fonts;
			update_root |= MF_Application_DisplayUpdate_Fonts;
		break;

		case DSI_ICON_DARKENVAL:
			ReloadPrefs();
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
			ReloadPrefs();
		break;

		case DSI_MISC_REMEMBER_WINDOWS:
		case DSI_MISC_REMEMBER_DOCUMENTS:
			ReloadPrefs();
			update_startup = TRUE;
		break;

		/* disable drive info XXX: should rebuild drive icons */
		case DSI_MISC_DRIVE_INFO:
			ReloadPrefs();
			update_root |= MF_Application_DisplayUpdate_Size;	/* XXX: Sucks, but doesn't cause nasty flicker */
			update_window |= MF_Application_DisplayUpdate_Size;
		break;

		/* we should update toolbars... */
		case DSI_TOOLBAR_DISPLAYMODE:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_UI;
		break;

		case DSI_WINDOW_STATUSBARCOLOR: // bitRocky
		case DSI_WINDOW_STATUSBARFORMAT:
		case DSI_TOOLBAR_DEFINITION:
		case DSI_TOOLBAR_BUTTONFRAMESPEC:
		case DSI_TOOLBAR_BUTTONIMAGESPEC:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_UI;
		break;

		case DSI_TOOLBAR_BROWSERMODE:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_UI;
		break;

		case DSI_WINDOW_DEFAULT_LEFT:
		case DSI_WINDOW_DEFAULT_TOP:
		case DSI_WINDOW_DEFAULT_WIDTH:
		case DSI_WINDOW_DEFAULT_HEIGHT:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_UI;
		break;

		case DSI_WINDOW_DEFAULT_VIEW:
		case DSI_WINDOW_DEFAULT_VIEWMODE:
		case DSI_WINDOW_INHERIT_VIEWMODE:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_UI;
		break;


		case DSI_LISTPOOL_KEYSHORTCUT_CHANGED:
			ReloadPrefs();
			update_window |= MF_Application_DisplayUpdate_UI;
		break;

		case DSI_MISC_MYMORPHOSICON:
			ReloadPrefs();
			update_mymorphos = TRUE;
		break;

		case DSI_MISC_SMARTFILEOPERATIONS:
			ReloadPrefs();
			update_mimetypes = TRUE;
		break;

		case DSI_MISC_TRAPMORE:
		case DSI_MISC_TRAPMULTIVIEW:
		case DSI_MISC_CREATEICONFORNEWDRAWER:
		case DSI_MISC_DESKTOPDOUBLECLICK:
			ReloadPrefs();
		break;

		case DSI_MISC_CONTEXTMENUIMAGES:
			ReloadPrefs();
		break;

		case DSI_BOOKMARKS_SHOWONLY:
			ReloadPrefs();
		break;
		
		default:
			return FALSE;
		break;
	}

	if (update_reloadprefs)
	{
		DoMethod(app, MM_Application_LoadPrefs, id, FALSE);
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
		{
			set(vo, MA_Iconview_ShowMyMorphos, _conf(misc_mymorphosicon));
		}
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
		mimetype_invalidate(NULL, NULL);
		mimetype_load_database(NULL);
	}

	return TRUE;
}

/*
 * Little helper function to convert a string
 * formated as 0xAARRGGBB to MUI PenSpec.
 * Return a default black PenSpec if the convertion fails.
 */
static struct MUI_PenSpec tmp_penspec;	/* penspec size is 32 bytes long. strcpy can safely copy the string to. */

static APTR convertARGBtoPenSpec(STRPTR argb)
{
	ULONG failed = TRUE;
	BYTE  *penspec = (BYTE*)&tmp_penspec;
	
	/* does the length of the provided string is correct? */
	if (strlen(argb) == strlen("0xAARRGGBB"))
	{
		/* checks for correctness starting format - should be 0x or 0X. */
		if (argb[0] == '0' && (argb[1] == 'x' || argb[1] == 'X'))
		{
			ULONG i;

			/* default penspec formating */
			penspec[0]  = 'r';
			penspec[9]  = ',';
			penspec[18] = ',';
			
			/* checking correctness of each char for hex values */
			for ( i=2; i < 10; i++)
			{
				/* killing the chars outside the valid range... at least will try to produce a color... */
				if ( ! ((argb[i] >= '0' && argb[i] <= '9') || (argb[i] >= 'a' && argb[i] <= 'f') || (argb[i] >= 'A' && argb[i] <= 'F')) )
				{
					argb[i] = '0';
				}

				switch(i)
				{
					case 2:		/* this is AA part. Not used */
					case 3:
					break;
					case 4:		/* this is upper R part */
						penspec[1] = argb[i];
						penspec[3] = argb[i];
						penspec[5] = argb[i];
						penspec[7] = argb[i];
					break;
					case 5:		/* this is lower R part */
						penspec[2] = argb[i];
						penspec[4] = argb[i];
						penspec[6] = argb[i];
						penspec[8] = argb[i];
					break;
					case 6:		/* this is upper G part */
						penspec[10] = argb[i];
						penspec[12] = argb[i];
						penspec[14] = argb[i];
						penspec[16] = argb[i];
					break;
					case 7:		/* this is lower G part */
						penspec[11] = argb[i];
						penspec[13] = argb[i];
						penspec[15] = argb[i];
						penspec[17] = argb[i];
					break;
					case 8:		/* this is upper B part */
						penspec[19] = argb[i];
						penspec[21] = argb[i];
						penspec[23] = argb[i];
						penspec[25] = argb[i];
					break;
					case 9:		/* this is lower B part */
						penspec[20] = argb[i];
						penspec[22] = argb[i];
						penspec[24] = argb[i];
						penspec[26] = argb[i];
					break;
				}
			}
			failed = FALSE;
		}
	}

	/* We didn't succeeded with conversion. Let's use a black penspec as a fallback
	 * Some improvement could be using the default Ambient color settings instead.
	 */
	if (failed)
	{
		strcpy((char*)&tmp_penspec, "r00000000,00000000,00000000");
	}
	return &tmp_penspec;
}

/*
	This function handles ARexx RXCMD_SetPrefs command
*/

ULONG preferences_process_SetPrefs(Object *app, STRPTR key, STRPTR value)
{
	ULONG rc = 20;	/* default error level of 20 is returned */
	ULONG id = arexxfindkey(key);
	
	/* not found - return error level 20. */
	if (id == -1) return rc;

	/* now, if something goes bad, error level will be 10 */
	rc = 10;
	
	switch(k2d[id].dsi)
	{
		case DSI_BACKGROUND_ROOT_BGRENDER:			/* LONG */
		{
			LONG bgrendermode = -1;

			/*
			 * Command format arguments is as follow:
			 *
			 * SetPrefs BACKGROUND_ROOT_BGRENDER [COLOR|TILED|CENTERED|SCALED|STRETCHED|ZOOMED|0|1|2|3|4|5]
			 */
			
			/* XXX: This suxx. There is no central point for validating the correctness of a value for a setting.
			 * If setting evolves, this part has to be modified to take account of the new settings bounds.
			 * Same applies to mostly every following settings.
			 */
	
			if (Stricmp("COLOR", value) == 0)
			{
				bgrendermode = BGRENDER_Color;
			}

			if (Stricmp("TILED", value) == 0)
			{
				bgrendermode = BGRENDER_Tiled;
			}

			if (Stricmp("CENTERED", value) == 0)
			{
				bgrendermode = BGRENDER_Centered;
			}

			if (Stricmp("SCALED", value) == 0)
			{
				bgrendermode = BGRENDER_Scaled;
			}

			if (Stricmp("STRETCHED", value) == 0)
			{
				bgrendermode = BGRENDER_Stretched;
			}

			if (Stricmp("ZOOMED", value) == 0)
			{
				bgrendermode = BGRENDER_Zoomed;
			}

			/* Let's accept numerical value as well if previous step failed.
			 * The setting being numeric. Make sense.
			 */

			if(bgrendermode == -1)
			{
				if (StrToLong(value, &bgrendermode) != -1)
				{
					/* Checks for range validity, BGRENDER_enum goes from 0 to 5 */
					if (bgrendermode >= 0 && bgrendermode <= 5)
					{
						setprefslong(DSI_BACKGROUND_ROOT_BGRENDER, bgrendermode);
						rc = 0;
					}
				}
			}
			else
			{
				setprefslong(DSI_BACKGROUND_ROOT_BGRENDER, bgrendermode);
				rc = 0;
			}
		}
		break;

		case DSI_BACKGROUND_WINDOW_BGRENDER:		/* LONG */
		{
			LONG bgrendermode = -1;

			/*
			 * Command format arguments is as follow:
			 *
			 * SetPrefs BACKGROUND_WINDOW_BGRENDER [COLOR|TILED|0|1]
			 */

			/* XXX: This suxx. There is no central point for validating the correctness of a value for a setting.
			 * If setting evolves, this part has to be modified to take account of the new settings bounds.
			 * Same applies to mostly every following settings.
			 */

			if (Stricmp("COLOR", value) == 0)
			{
				bgrendermode = BGRENDER_Color;
			}

			if (Stricmp("TILED", value) == 0)
			{
				bgrendermode = BGRENDER_Tiled;
			}

			/* Let's accept numerical value as well if previous step failed.
			 * The setting being numeric. Make sense.
			 */

			if(bgrendermode == -1)
			{
				if (StrToLong(value, &bgrendermode) != -1)
				{
					/* Checks for range validity, BGRENDER_enum goes from 0 to 1 for window. Beware. Root window has more BGRENDER mode. */
					if (bgrendermode >= 0 && bgrendermode <= 1)
					{
						setprefslong(DSI_BACKGROUND_WINDOW_BGRENDER, bgrendermode);
						rc = 0;
					}
				}
			}
			else
			{
				setprefslong(DSI_BACKGROUND_WINDOW_BGRENDER, bgrendermode);
				rc = 0;
			}
		}
		break;

		default:
			switch(k2d[id].prefs_type)
			{
				case PREFS_TYPE_LONG:
				{
					LONG val;

					if (StrToLong(value, &val) != -1)
					{
						if(val >= k2d[id].lowerbound && val <= k2d[id].upperbound)
						{
							setprefslong(k2d[id].dsi, val);
							rc = 0;
						}
					}
				}
				break;
				
				case PREFS_TYPE_STRPTR:
					setprefsstr(k2d[id].dsi, value);
					rc = 0;
				break;
				
				case PREFS_TYPE_PENSPEC:
					setprefs(k2d[id].dsi, sizeof(struct MUI_PenSpec), convertARGBtoPenSpec(value));
					rc = 0;
				break;
			}
		break;
	}

	preferences_apply_update(app, k2d[id].dsi);

	return rc;
}

/*
	This function handles ARexx RXCMD_GetPrefs command
*/

#if DEBUG
static VOID preferences_arexx_unit_test(VOID);
#endif

STRPTR preferences_process_GetPrefs(STRPTR key)
{
	STRPTR res = NULL;
	ULONG id = arexxfindkey(key);

#if DEBUG
	if(id == -1) preferences_arexx_unit_test();
#endif

	/* not found */
	if (id == -1) return res;

	switch(k2d[id].prefs_type)
	{
		case PREFS_TYPE_LONG:
		{
			LONG val;
			val = getprefslong(k2d[id].dsi);
			res = malloc(8);
			if (res)
			{
				snprintf(res, 8, "%ld", val);
			}
		}
		break;

		case PREFS_TYPE_STRPTR:
		case PREFS_TYPE_PENSPEC:
		{
			STRPTR val;
			val = getprefsstr(k2d[id].dsi);

			if (val)
			{
				res = malloc(strlen(val) + 1);
				if (res)
				{
					sprintf(res, "%s", val);
				}
			}
		}
		break;
	}
	return res;
}

/*
	This function dumps an ARexx script in debug output for unit testing.
*/

#if DEBUG
static VOID preferences_arexx_unit_test(VOID)
{
	ULONG i;

	DB(("/* Ambient ARexx Unit Test */\n"));
	DB(("Options Results\n"));
	DB(("ADDRESS 'AMBIENT'\n"));

	for (i=0; i < sizeof(k2d) / sizeof(struct Key2DSI); i++)
	{
		switch(k2d[i].prefs_type)
		{
			case PREFS_TYPE_LONG:
				DB(("SAY '%s LONG: %ld'\n", k2d[i].key, getprefslong(k2d[i].dsi)));
			break;

			case PREFS_TYPE_STRPTR:
				DB(("SAY '%s STRPTR: %s'\n", k2d[i].key, getprefs(k2d[i].dsi)));
			break;

			case PREFS_TYPE_PENSPEC:
				DB(("SAY '%s PENSPEC: %s'\n", k2d[i].key, getprefs(k2d[i].dsi)));
			break;
		}
		DB(("'GetPrefs %s'\n", k2d[i].key));
		DB(("IF(RC>0) THEN SAY '*Error*'\n"));
		DB(("ELSE SAY Result\n"));
	}
}
#endif