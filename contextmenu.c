/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2006 Ambient Open Source Team
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
 * $Id: contextmenu.c,v 1.39 2023/01/12 20:33:12 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/gadtools.h>

/* private */ 
#include "ambient_cat.h"
#include "contextmenu.h"
#include "mui_func.h"
#include "command.h"
#include "viewapi.h"
#include "tags.h"
#include "action.h"
#include "mimeuri.h"
#include "mimetype.h"
#include "name.h"
#include "config.h"
#include "prefs.h"
#include "prefs_advanced.h"
#include "rexx.h"

#include "clipboard.h"

#define MENU_SEPARATOR     { "",   AC_TITLE, AS_ANY, AF_ENABLED,          0, NULL, NULL, NULL, NULL, 0 }
#define MENU_SEPARATOR_SUB { "",   AC_TITLE, AS_ANY, AF_ENABLED | AF_SUB, 0, NULL, NULL, NULL, NULL, 0 }
#define MENU_END           { NULL, 0,        0,      0,                   0, NULL, NULL, NULL, NULL, 0 }

static TEXT cm_command_about[CM_BUFFERSIZE];
static TEXT cm_command_addpanel[CM_BUFFERSIZE];
static TEXT cm_command_cut[CM_BUFFERSIZE];
static TEXT cm_command_copy[CM_BUFFERSIZE];
static TEXT cm_command_delete[CM_BUFFERSIZE];
static TEXT cm_command_deletebutton[CM_BUFFERSIZE];
static TEXT cm_command_deletefrompanel[CM_BUFFERSIZE];
static TEXT cm_command_deletepanel[CM_BUFFERSIZE];
static TEXT cm_command_panelcreate[CM_BUFFERSIZE];
static TEXT cm_command_savepanel[CM_BUFFERSIZE];
static TEXT cm_command_panelprefs[CM_BUFFERSIZE];
static TEXT cm_command_format[CM_BUFFERSIZE];
static TEXT cm_command_eject[CM_BUFFERSIZE];
static TEXT cm_command_unmount[CM_BUFFERSIZE];
static TEXT cm_command_help[CM_BUFFERSIZE];
static TEXT cm_command_information[CM_BUFFERSIZE];
static TEXT cm_command_invert[CM_BUFFERSIZE];
static TEXT cm_command_locked[CM_BUFFERSIZE];
static TEXT cm_command_stayopen[CM_BUFFERSIZE];
static TEXT cm_command_move[CM_BUFFERSIZE];
static TEXT cm_command_newdrawer[CM_BUFFERSIZE];
static TEXT cm_command_paste[CM_BUFFERSIZE];
static TEXT cm_command_pasteinto[CM_BUFFERSIZE];
static TEXT cm_command_pasteas[CM_BUFFERSIZE];
static TEXT cm_command_pasteasinto[CM_BUFFERSIZE];
static TEXT cm_command_properties[CM_BUFFERSIZE];
static TEXT cm_command_putaway[CM_BUFFERSIZE];
static TEXT cm_command_rename[CM_BUFFERSIZE];
static TEXT cm_command_selectall[CM_BUFFERSIZE];
static TEXT cm_command_show[CM_BUFFERSIZE];
static TEXT cm_command_snapshotwindow[CM_BUFFERSIZE];
static TEXT cm_command_unsnapshotwindow[CM_BUFFERSIZE];
static TEXT cm_command_sort[CM_BUFFERSIZE];
static TEXT cm_command_sort_bydate[CM_BUFFERSIZE];
static TEXT cm_command_sort_byname[CM_BUFFERSIZE];
static TEXT cm_command_sort_bysize[CM_BUFFERSIZE];
static TEXT cm_command_sort_bytype[CM_BUFFERSIZE];
static TEXT cm_command_snapshot[CM_BUFFERSIZE];
static TEXT cm_command_unsnapshot[CM_BUFFERSIZE];
static TEXT cm_command_view[CM_BUFFERSIZE];
static TEXT cm_command_panel[CM_BUFFERSIZE];
static TEXT cm_command_listview_defaultformat[CM_BUFFERSIZE];
static TEXT cm_command_listview_getsizes[CM_BUFFERSIZE];
static TEXT cm_command_window[CM_BUFFERSIZE];
static TEXT cm_command_all[CM_BUFFERSIZE];
static TEXT cm_command_icons[CM_BUFFERSIZE];
static TEXT cm_command_open[CM_BUFFERSIZE];
static TEXT cm_command_trash[CM_BUFFERSIZE];
static TEXT cm_command_restore[CM_BUFFERSIZE];
static TEXT cm_command_empty[CM_BUFFERSIZE];
static TEXT cm_command_openParent[CM_BUFFERSIZE]; // bitRocky
static TEXT cm_command_netsettings[CM_BUFFERSIZE];
static TEXT cm_command_netconnect[CM_BUFFERSIZE];

static BOOL allow_unsnapshot(ULONG viewallow)
{
	return (viewallow & AS_ICONVIEW ? !_aprefs(autosort) : FALSE);
}

static BOOL allow_icon_unsnapshot(ULONG viewallow)
{
	return (viewallow & AS_ICONVIEW ? viewallow & AS_DEVICEVIEW ? FALSE : !_aprefs(autosort) : FALSE);
}

static BOOL allow_icon_snapshot(ULONG viewallow)
{
	return ((viewallow & AS_DEVICEVIEW || !(viewallow & AS_ICONVIEW)) ? FALSE : TRUE);
}

static BOOL allow_networks(ULONG viewallow)
{
	return (viewallow & AS_NETWORKSFS) != 0;
}

static struct command_shortcut command_shortcut[] = {
	{ AS_SHORTCUT | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE   , (APTR)MSG_CMENU_INFO_SHORTCUT,   "IconInfo"       },
	{ 0           | AS_TOOL | AS_PROJECT | AS_DRAWER | 0           , (APTR)MSG_CMENU_CUT_SHORTCUT,    "ClipboardCut"   },
	{ 0           | AS_TOOL | AS_PROJECT | AS_DRAWER | 0           , (APTR)MSG_CMENU_COPY_SHORTCUT,   "ClipboardCopy"  },
	{ 0           | AS_TOOL | AS_PROJECT | AS_DRAWER | 0           , (APTR)MSG_CMENU_PASTE_SHORTCUT,  "ClipboardPaste" },
	{ AS_SHORTCUT | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE   , (APTR)MSG_CMENU_RENAME_SHORTCUT, "Rename"         },
	{ 0           | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_REMOVABLE, (APTR)MSG_CMENU_EJECT_SHORTCUT,  "Eject"          },
	{ 0           | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_UNMOUNTABLE, (APTR)MSG_CMENU_EJECT_SHORTCUT,"Unmount"        },
	{ 0                                                            , NULL, NULL }
};

/*
 * Operations possible to do with an icon
 */
#define MENU_ICON_OPERATIONS \
	MENU_SEPARATOR, \
	{ cm_command_rename,      AC_INTERNAL, AS_SHORTCUT | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE | AS_MYCOMPUTER, AF_ENABLED, 0, "Rename",          NULL, NULL, NULL, MSG_CMENU_RENAME_SHORTCUT }, \
	{ cm_command_delete,      AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER,                                           AF_ENABLED, 0, "Delete",          NULL, NULL, NULL, 0 }, \
	{ cm_command_format,      AC_INTERNAL, AS_FORMATABLE,                                                              AF_ENABLED, 0, "Format",          NULL, NULL, NULL, 0 }, \
	{ cm_command_eject,       AC_INTERNAL, AS_REMOVABLE,                                                               AF_ENABLED, 0, "Eject",           NULL, NULL, NULL, MSG_CMENU_EJECT_SHORTCUT }, \
	{ cm_command_unmount,     AC_INTERNAL, AS_UNMOUNTABLE,                                                             AF_ENABLED, 0, "Unmount",         NULL, NULL, NULL, MSG_CMENU_EJECT_SHORTCUT }, \
	{ cm_command_putaway,     AC_INTERNAL, AS_SHORTCUT,                                                                AF_ENABLED, 0, "Shortcut Remove", NULL, NULL, NULL, 0 }, \
	{ cm_command_trash,       AC_INTERNAL, AS_HASTRASHCAN,                                                             AF_ENABLED, 0, "Trash",           NULL, NULL, NULL, 0 }, \
	{ cm_command_restore,     AC_INTERNAL, AS_INTRASHCAN,                                                              AF_ENABLED, 0, "Restore",         NULL, NULL, NULL, 0 }, \
	{ cm_command_empty,       AC_INTERNAL, AS_ISTRASHCAN,                                                              AF_ENABLED, 0, "EmptyTrashcan",   NULL, NULL, NULL, 0 }, \
	{ cm_command_netsettings, AC_INTERNAL, AS_NETWORKSFS | AS_TOOL | AS_PROJECT | AS_DRAWER,                           AF_ENABLED, 0, "NetworksSettings",NULL, NULL, allow_networks, 0 }

/*
 * Clipboard operations
 */
#define MENU_CLIPBOARD \
	MENU_SEPARATOR, \
	{ cm_command_cut,         AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_FILEVIEW,                 AF_ENABLED, 0, "ClipboardCut",      NULL, clipboard_menu_copy_enable, NULL, MSG_CMENU_CUT_SHORTCUT }, \
	{ cm_command_copy,        AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_FILEVIEW,                 AF_ENABLED, 0, "ClipboardCopy",     NULL, clipboard_menu_copy_enable, NULL, MSG_CMENU_COPY_SHORTCUT }, \
	{ cm_command_pasteinto,   AC_INTERNAL, AS_DRAWER | AS_DEVICE | AS_ASSIGN |  AS_SHORTCUT | AS_FILEVIEW, AF_ENABLED, 0, "ClipboardPaste",    NULL, clipboard_menu_paste_enable, NULL, MSG_CMENU_PASTE_SHORTCUT }, \
	{ cm_command_pasteasinto, AC_INTERNAL, AS_DRAWER | AS_DEVICE | AS_ASSIGN |  AS_SHORTCUT | AS_FILEVIEW, AF_ENABLED, 0, "ClipboardPaste AS", NULL, clipboard_menu_paste_enable, NULL, 0 }


#define MENU_CLIPBOARD_VIEW \
	MENU_SEPARATOR, \
	{ cm_command_cut,         AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_FILEVIEW,                 AF_ENABLED, 0, "ClipboardCut",      NULL, clipboard_menu_copy_enable, NULL, MSG_CMENU_CUT_SHORTCUT }, \
	{ cm_command_copy,        AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_FILEVIEW,                 AF_ENABLED, 0, "ClipboardCopy",     NULL, clipboard_menu_copy_enable, NULL, MSG_CMENU_COPY_SHORTCUT }, \
	{ cm_command_paste,       AC_INTERNAL, AS_DRAWER | AS_DEVICE | AS_ASSIGN | AS_FILEVIEW,                AF_ENABLED, 0, "ClipboardPaste",    NULL, clipboard_menu_paste_enable, NULL, MSG_CMENU_PASTE_SHORTCUT }, \
	{ cm_command_pasteas,     AC_INTERNAL, AS_DRAWER | AS_DEVICE | AS_ASSIGN | AS_FILEVIEW,                AF_ENABLED, 0, "ClipboardPaste AS", NULL, clipboard_menu_paste_enable, NULL, 0 }

/*
 * Directory management
 */
#define MENU_DIRECTORY \
	MENU_SEPARATOR, \
	{ cm_command_newdrawer,   AC_INTERNAL, AS_DRAWER | AS_PROJECT | AS_TOOL | AS_FILEVIEW,     AF_ENABLED | AF_NO_GROUPING, 0, "Makedir",         NULL, NULL, NULL, 0 }, \
	{ cm_command_netconnect,  AC_INTERNAL, AS_NETWORKSFS | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_FILEVIEW, AF_ENABLED | AF_NO_GROUPING, 0, "NetworksConnect",NULL, NULL, allow_networks, 0 }

/*
 * Snapshot features
 */
#define MENU_SNAPSHOT \
	/* Only if autosort is off */ \
	{ cm_command_snapshot,       AC_TITLE,    AS_ANY,      AF_ENABLED,          0, NULL,                      NULL, NULL, NULL, 0 }, \
	{ cm_command_icons,          AC_INTERNAL, AS_HASICONS, AF_ENABLED | AF_SUB, 0, "Snapshot Icons",          NULL, NULL, allow_icon_snapshot, 0 }, \
	{ cm_command_window,         AC_INTERNAL, AS_ANY,      AF_ENABLED | AF_SUB, 0, "Snapshot Window",         NULL, NULL, NULL, 0 }, \
	{ cm_command_all,            AC_INTERNAL, AS_HASICONS, AF_ENABLED | AF_SUB, 0, "Snapshot Icons Window",   NULL, NULL, allow_icon_snapshot, 0 }, \
	{ cm_command_unsnapshot,     AC_TITLE,    AS_ANY,      AF_ENABLED,          0, NULL,                      NULL, NULL, allow_unsnapshot, 0 }, \
	{ cm_command_icons,          AC_INTERNAL, AS_HASICONS, AF_ENABLED | AF_SUB, 0, "UnSnapshot Icons",        NULL, NULL, allow_icon_unsnapshot, 0 }, \
	{ cm_command_window,         AC_INTERNAL, AS_ANY,      AF_ENABLED | AF_SUB, 0, "UnSnapshot Window",       NULL, NULL, allow_unsnapshot, 0 }, \
	{ cm_command_all,            AC_INTERNAL, AS_HASICONS, AF_ENABLED | AF_SUB, 0, "UnSnapshot Icons Window", NULL, NULL, allow_icon_unsnapshot, 0 }

#define MENU_SNAPSHOT_GROUP \
	MENU_SEPARATOR, \
	{ cm_command_snapshot,       AC_INTERNAL, AS_ANY, AF_ENABLED, 0, "Snapshot Icons Selection",   NULL, NULL, allow_icon_snapshot, 0 }, \
	{ cm_command_unsnapshot,     AC_INTERNAL, AS_ANY, AF_ENABLED, 0, "UnSnapshot Icons Selection", NULL, NULL, allow_icon_snapshot, 0 }

#define MENU_SELECTION_GROUP \
	MENU_SEPARATOR, \
	{ cm_command_selectall,      AC_INTERNAL, AS_HASICONS, AF_ENABLED,          0, "Select All",              NULL, NULL, NULL, MSG_CMENU_SELECTALL_SHORTCUT }, \
	{ cm_command_invert,         AC_INTERNAL, AS_HASICONS, AF_ENABLED,          0, "Select",                  NULL, NULL, NULL, MSG_CMENU_INVERT_SHORTCUT }

#define MENU_SORT \
	MENU_SEPARATOR, \
	{ cm_command_sort,           AC_TITLE,    AS_ICONVIEW,      AF_ENABLED,                           0, NULL,                      NULL, NULL, NULL, 0 }, \
	{ cm_command_sort_byname,    AC_INTERNAL, AS_ICONVIEW,      AF_ENABLED | AF_SUB | AF_NO_GROUPING, 0, "Sort Alpha",              NULL, NULL, NULL, 0 }, \
	{ cm_command_sort_bytype,    AC_INTERNAL, AS_ICONVIEW,      AF_ENABLED | AF_SUB | AF_NO_GROUPING, 0, "Sort Type",               NULL, NULL, NULL, 0 }, \
	{ cm_command_sort_bysize,    AC_INTERNAL, AS_ICONVIEW,      AF_ENABLED | AF_SUB | AF_NO_GROUPING, 0, "Sort Size",               NULL, NULL, NULL, 0 }, \
	{ cm_command_sort_bydate,    AC_INTERNAL, AS_ICONVIEW,      AF_ENABLED | AF_SUB | AF_NO_GROUPING, 0, "Sort Date",               NULL, NULL, NULL, 0 }

//	{ "Resize to fit",           AC_INTERNAL, AS_ICONVIEW,      AF_ENABLED,          0, "Cleanup Resize",          NULL, NULL, NULL, 0 },
//	{ "Cleanup",                 AC_INTERNAL, AS_ICONVIEW,      AF_ENABLED,          0, "Cleanup All",             NULL, NULL, NULL, 0 },



static const struct command_menu icon_default_menu[] = {
//  { "Open",                 AC_INTERNAL, AS_ANY,                                                     AF_ENABLED, 0, "Open",            NULL, NULL },
	{ cm_command_information, AC_INTERNAL, AS_SHORTCUT | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE | AS_ASSIGN, AF_ENABLED, 0, "IconInfo",        NULL, NULL, NULL, MSG_CMENU_INFO_SHORTCUT },

	MENU_SEPARATOR,
	{ cm_command_snapshot,    AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE | AS_MYCOMPUTER | AS_SHORTCUT, AF_ENABLED, 0, "Snapshot Icons",        NULL, NULL, allow_icon_snapshot, 0 },
	{ cm_command_unsnapshot,  AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE | AS_MYCOMPUTER | AS_SHORTCUT, AF_ENABLED, 0, "Unsnapshot Icons",      NULL, NULL, allow_icon_snapshot, 0 },

	MENU_SORT,
	MENU_DIRECTORY,
	MENU_CLIPBOARD,
	MENU_ICON_OPERATIONS,

	MENU_END
};


static const struct command_menu icongroup_default_menu[] = {
	{ cm_command_information, AC_INTERNAL, AS_SHORTCUT | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE | AS_ASSIGN, AF_ENABLED,          0, "IconInfo",      NULL, NULL, NULL, MSG_CMENU_INFO_SHORTCUT },
	/* We should support this but target must be current window, not an icon */
	MENU_DIRECTORY,

	MENU_SNAPSHOT_GROUP,
	MENU_SORT,

	MENU_SEPARATOR,
	{ cm_command_cut,         AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER,                                       AF_ENABLED, 0, "ClipboardCut",    NULL, NULL, NULL, MSG_CMENU_CUT_SHORTCUT },
	{ cm_command_copy,        AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER,                                       AF_ENABLED, 0, "ClipboardCopy",   NULL, NULL, NULL, MSG_CMENU_COPY_SHORTCUT },

	MENU_SEPARATOR,
	{ cm_command_rename,      AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE,               AF_ENABLED,          0, "Rename",        NULL, NULL, NULL, 0 },
	{ cm_command_delete,      AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE,               AF_ENABLED,          0, "Delete",        NULL, NULL, NULL, 0 },
	{ cm_command_putaway,     AC_INTERNAL, AS_SHORTCUT,                                                AF_ENABLED,          0, "Shortcut Remove", NULL, NULL, NULL, 0 },
	{ cm_command_trash,       AC_INTERNAL, AS_HASTRASHCAN,                                             AF_ENABLED, 0, "Trash",           NULL, NULL, NULL, 0 },
	{ cm_command_restore,     AC_INTERNAL, AS_INTRASHCAN,                                              AF_ENABLED, 0, "Restore",         NULL, NULL, NULL, 0 },
	{ cm_command_empty,       AC_INTERNAL, AS_ISTRASHCAN,                                              AF_ENABLED, 0, "EmptyTrashcan",   NULL, NULL, NULL, 0 },
	{ cm_command_netsettings, AC_INTERNAL, AS_NETWORKSFS | AS_DRAWER | AS_TOOL | AS_PROJECT,           AF_ENABLED, 0, "NetworksSettings",NULL, NULL, allow_networks, 0 }, 

	MENU_END
};


static const struct command_menu listgroup_default_menu[] = {
	{ cm_command_information, AC_INTERNAL, AS_SHORTCUT | AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE, AF_ENABLED,          0, "IconInfo",      NULL, NULL, NULL, MSG_CMENU_INFO_SHORTCUT },
	/* We should support this but target must be current window, not an icon */
	/* MENU_DIRECTORY, */

	MENU_SEPARATOR,
	{ cm_command_cut,         AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER ,                          AF_ENABLED,          0, "ClipboardCut",  NULL, NULL, NULL, MSG_CMENU_CUT_SHORTCUT },
	{ cm_command_copy,        AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER ,                          AF_ENABLED,          0, "ClipboardCopy", NULL, NULL, NULL, MSG_CMENU_COPY_SHORTCUT },
	MENU_SEPARATOR,
	{ cm_command_rename,      AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE,               AF_ENABLED,          0, "Rename",        NULL, NULL, NULL, 0 },
	{ cm_command_delete,      AC_INTERNAL, AS_TOOL | AS_PROJECT | AS_DRAWER | AS_DEVICE,               AF_ENABLED,          0, "Delete", NULL, NULL, NULL, 0 },
	{ cm_command_trash,       AC_INTERNAL, AS_HASTRASHCAN,                                             AF_ENABLED, 0, "Trash",           NULL, NULL, NULL, 0 },
	{ cm_command_restore,     AC_INTERNAL, AS_INTRASHCAN,                                              AF_ENABLED, 0, "Restore",         NULL, NULL, NULL, 0 },
	{ cm_command_empty,       AC_INTERNAL, AS_ISTRASHCAN,                                              AF_ENABLED, 0, "EmptyTrashcan",   NULL, NULL, NULL, 0 },
	{ cm_command_netsettings, AC_INTERNAL, AS_NETWORKSFS | AS_TOOL | AS_PROJECT | AS_DRAWER,           AF_ENABLED, 0, "NetworksSettings",NULL, NULL, allow_networks, 0 }, 
	MENU_END
};


#define PLACEHOLDER_VIEWS "Placeholder Views"
#define PLACEHOLDER_MODES "Placeholder Modes"
static const struct command_menu view_default_menu[] = {
	{ cm_command_view, AC_INTERNAL, AS_ANY, AF_ENABLED, 0, PLACEHOLDER_VIEWS, NULL, NULL, NULL, 0 }, /* just a placeholder */
	{ cm_command_show, AC_INTERNAL, AS_ANY, AF_ENABLED, 0, PLACEHOLDER_MODES, NULL, NULL, NULL, 0 }, /* just a placeholder */
	MENU_SEPARATOR,
	MENU_END
};


static const struct command_menu iconview_default_menu[] = {
	MENU_SNAPSHOT,
	MENU_SORT,
	MENU_SELECTION_GROUP,
	MENU_DIRECTORY,
	MENU_CLIPBOARD_VIEW,
	MENU_END
};


static const struct command_menu appicon_default_menu[] = {
	{ cm_command_open, AC_INTERNAL, AS_APPICON, AF_ENABLED, 0, "IconSelect", NULL, NULL, NULL, 0 },
	MENU_END
};

static const struct command_menu panelgroup_default_menu[] = {
	{ cm_command_deletefrompanel, AC_INTERNAL, AS_ANY,   					AF_ENABLED,                    0, "Panel RemoveItem", NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_move,            AC_INTERNAL, AS_ANY,					   	AF_ENABLED,                    0, "Panel MoveItem",   NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_openParent,	  AC_INTERNAL, AS_ANY,                      AF_ENABLED,                    0, "Panel OpenParent", NULL, NULL, NULL, 0 }, /* XXX */
	MENU_SEPARATOR,
	{ cm_command_locked,      AC_INTERNAL, AS_ANY,                          AF_ENABLED | AF_TOGGLE | AF_CHECKIT, 0, "Panel ToggleLock", NULL, NULL, NULL, 0 },

	MENU_SEPARATOR,
	{ cm_command_panelprefs,  AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Settings \"INTERNAL Panels\"",     NULL, NULL, NULL, 0 },
	{ cm_command_panelcreate, AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Panel Create",     NULL, NULL, NULL, 0 },
	{ cm_command_deletepanel, AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Panel Delete",     NULL, NULL, NULL, 0 },
	{ cm_command_savepanel,   AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Panel Save",     NULL, NULL, NULL, 0 },
	MENU_END
};

static const struct command_menu panelgroup_drag_default_menu[] = {
	{ cm_command_locked,      AC_INTERNAL, AS_ANY,                          AF_ENABLED | AF_TOGGLE | AF_CHECKIT, 0, "Panel ToggleLock", NULL, NULL, NULL, 0 },

	MENU_SEPARATOR,
	{ cm_command_panelprefs,  AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Settings \"INTERNAL Panels\"",     NULL, NULL, NULL, 0 },
	{ cm_command_panelcreate, AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Panel Create",     NULL, NULL, NULL, 0 },
	{ cm_command_deletepanel, AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Panel Delete",     NULL, NULL, NULL, 0 },
	{ cm_command_savepanel,   AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Panel Save",     NULL, NULL, NULL, 0 },
	MENU_END
};



static const struct command_menu panelgroup_sub_default_menu[] = {
    { cm_command_panel,           AC_TITLE,    AS_ANY,                            AF_ENABLED,                                   0, NULL,               NULL, NULL, NULL, 0 },
	{ cm_command_addpanel,        AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED | AF_SUB,                          0, "Panel Additem",    NULL, NULL, NULL, 0 },
	{ cm_command_properties,      AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED | AF_SUB,                          0, "Panel Config",     NULL, NULL, NULL, 0 },
	{ cm_command_locked,          AC_INTERNAL, AS_ANY,                            AF_ENABLED | AF_TOGGLE | AF_CHECKIT | AF_SUB, 0, "Panel ToggleLock", NULL, NULL, NULL, 0 },
	MENU_SEPARATOR_SUB,
	{ cm_command_panelprefs,  AC_INTERNAL, AS_ANY,                          AF_ENABLED,                          0, "Settings \"INTERNAL Panels\"",     NULL, NULL, NULL, 0 },
	{ cm_command_panelcreate,     AC_INTERNAL, AS_ANY,                            AF_ENABLED | AF_SUB,                          0, "Panel Create",     NULL, NULL, NULL, 0 },
	{ cm_command_deletepanel,     AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED | AF_SUB,                          0, "Panel Delete",     NULL, NULL, NULL, 0 },
	MENU_SEPARATOR,
	{ cm_command_properties,      AC_INTERNAL, AS_PANEL_PROPERTIES | AS_DISABLED, AF_ENABLED,                                   0, "Foobar",           NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_help,            AC_INTERNAL, AS_PANEL_HELP | AS_DISABLED,       AF_ENABLED,                                   0, "Foobar",           NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_about,           AC_INTERNAL, AS_PANEL_ABOUT | AS_DISABLED,      AF_ENABLED,                                   0, "Foobar",           NULL, NULL, NULL, 0 }, /* XXX */
	MENU_SEPARATOR,
	{ cm_command_deletefrompanel, AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel RemoveItem", NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_move,            AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel MoveItem",   NULL, NULL, NULL, 0 }, /* XXX */
	MENU_END
};


static const struct command_menu panelgroup_subpanel_default_menu[] = {
	{ cm_command_deletefrompanel, AC_INTERNAL, AS_ANY,   					AF_ENABLED,                    0, "Panel RemoveItem", NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_move,            AC_INTERNAL, AS_ANY,					   	AF_ENABLED,                    0, "Panel MoveItem",   NULL, NULL, NULL, 0 }, /* XXX */
	MENU_SEPARATOR,
	{ cm_command_locked,      AC_INTERNAL, AS_ANY,                          AF_ENABLED | AF_TOGGLE | AF_CHECKIT, 0, "Panel ToggleLock", NULL, NULL, NULL, 0 },
	{ cm_command_stayopen,    AC_INTERNAL, AS_ANY,                          AF_ENABLED | AF_TOGGLE | AF_CHECKIT, 0, "Panel StayOpen", NULL, NULL, NULL, 0 },
	MENU_END
};

static const struct command_menu panelgroup_dirpanel_default_menu[] = {
	{ cm_command_stayopen,    AC_INTERNAL, AS_ANY,                          AF_ENABLED | AF_TOGGLE | AF_CHECKIT, 0, "Panel StayOpen", NULL, NULL, NULL, 0 },
	MENU_END
};


static struct command_menu panelbutton_default_menu[] = {
	{ cm_command_deletefrompanel, AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel RemoveItem", NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_move,            AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel MoveItem",   NULL, NULL, NULL, 0 }, /* XXX */
	MENU_END
};

static struct command_menu panelspacer_default_menu[] = {
	{ cm_command_deletefrompanel, AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel RemoveItem", NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_move,            AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel MoveItem",   NULL, NULL, NULL, 0 }, /* XXX */
	MENU_END
};

static struct command_menu panelseparator_default_menu[] = {
	{ cm_command_deletefrompanel, AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel RemoveItem", NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_move,            AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel MoveItem",   NULL, NULL, NULL, 0 }, /* XXX */
	MENU_END
};

static struct command_menu panelpopup_default_menu[] = {
	{ cm_command_deletefrompanel, AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel RemoveItem", NULL, NULL, NULL, 0 }, /* XXX */
	{ cm_command_move,            AC_INTERNAL, AS_PANEL_UNLOCKED | AS_DISABLED,   AF_ENABLED,                                   0, "Panel MoveItem",   NULL, NULL, NULL, 0 }, /* XXX */
	MENU_END
};

static const struct command_menu listview_properties_menu[] = {
	{ cm_command_listview_getsizes,      AC_INTERNAL, AS_ANY, AF_ENABLED | AF_TOGGLE | AF_CHECKIT, 0, "Listview GetSizes",      NULL, NULL, NULL, 0 },
	MENU_SEPARATOR,
	{ cm_command_listview_defaultformat, AC_INTERNAL, AS_ANY, AF_ENABLED,                          0, "Listview DefaultFormat", NULL, NULL, NULL, 0 },
	MENU_END
};


struct tempobj {
	struct MinNode n;
	APTR obj;
};

static void context_menu_cleanup(struct MinList *l)
{
	struct tempobj *to, *nextto;

	ITERATELISTSAFE(to, nextto, l)
	{
		free(to);
	}
}



struct cmenustrings_init {
	STRPTR buf;
	STRPTR image;
	ULONG  lsid;
};

#define DUMMY (STRPTR)1 /* layout needs to be fixed with an empty dummy image (what a mess!) */

static const struct cmenustrings_init cmenustrings_initarr[] = {
	{ cm_command_about,                  DUMMY,           MSG_CMENU_ABOUT         },
	{ cm_command_addpanel,               DUMMY,           MSG_CMENU_ADDTOPANEL    },
	{ cm_command_cut,                    "cut",           MSG_CMENU_CUT           },
	{ cm_command_copy,                   "copy",          MSG_CMENU_COPY          },
	{ cm_command_delete,                 "delete",        MSG_CMENU_DELETE        },
	{ cm_command_deletebutton,           DUMMY,           MSG_CMENU_DELBUTTON     },
	{ cm_command_deletefrompanel,        DUMMY,           MSG_CMENU_PANELDELFROM  },
	{ cm_command_deletepanel,            DUMMY,           MSG_CMENU_PANELDEL      },
	{ cm_command_panelcreate,            DUMMY,           MSG_CMENU_PANELCREATE   },
	{ cm_command_savepanel, 	         DUMMY,           MSG_CMENU_PANELSAVE     },
	{ cm_command_panelprefs, 	         DUMMY,           MSG_CMENU_PANELPREFS    },
	{ cm_command_format,                 "format",        MSG_CMENU_FORMAT        },
	{ cm_command_eject,                  "eject",         MSG_CMENU_EJECT         },
	{ cm_command_unmount,                "unmount",       MSG_CMENU_EJECT         },
	{ cm_command_help,                   DUMMY,           MSG_CMENU_HELP          },
	{ cm_command_information,            "info",          MSG_CMENU_INFO          },
	{ cm_command_invert,                 DUMMY,           MSG_CMENU_INVERT        },
	{ cm_command_locked,                 NULL,            MSG_CMENU_LOCKED        },
	{ cm_command_stayopen,               NULL,            MSG_CMENU_STAYOPEN      },
	{ cm_command_move,                   DUMMY,           MSG_CMENU_MOVE          },
	{ cm_command_newdrawer,              "makedir",       MSG_CMENU_NEWDRAWER     },
	{ cm_command_panel,                  DUMMY,           MSG_CMENU_PANEL         },
	{ cm_command_paste,                  "paste",         MSG_CMENU_PASTE         },
	{ cm_command_pasteinto,              "paste",         MSG_CMENU_PASTEINTO     },
	{ cm_command_pasteas,                "pasteas",       MSG_CMENU_PASTEAS       },
	{ cm_command_pasteasinto,            "pasteas",       MSG_CMENU_PASTEASINTO   },
	{ cm_command_properties,             "info",          MSG_CMENU_PROPERTIES    },
	{ cm_command_putaway,                "putaway",       MSG_CMENU_PUTAWAY       },
	{ cm_command_rename,                 "rename",        MSG_CMENU_RENAME        },
	{ cm_command_selectall,              DUMMY,           MSG_CMENU_SELECTALL     },
	{ cm_command_show,                   DUMMY,           MSG_CMENU_SHOW          },
	{ cm_command_snapshotwindow,         "window",        MSG_CMENU_SNAPSHOTWIN   },
	{ cm_command_unsnapshotwindow,       "window",        MSG_CMENU_UNSNAPSHOTWIN },
	{ cm_command_sort,                   DUMMY,           MSG_CMENU_SORT          },
	{ cm_command_sort_bydate,            NULL,            MSG_CMENU_SORT_BYDATE   },
	{ cm_command_sort_byname,            NULL,            MSG_CMENU_SORT_BYNAME   },
	{ cm_command_sort_bysize,            NULL,            MSG_CMENU_SORT_BYSIZE   },
	{ cm_command_sort_bytype,            NULL,            MSG_CMENU_SORT_BYTYPE   },
	{ cm_command_snapshot,               "snapshot",      MSG_CMENU_SNAPSHOT      },
	{ cm_command_unsnapshot,             "revert",        MSG_CMENU_UNSNAPSHOT    },
	{ cm_command_view,                   DUMMY,           MSG_CMENU_VIEW          },

	{ cm_command_listview_getsizes,      NULL,            MSG_CMENU_LISTTITLE_GETDIRECTORYSIZES    },
	{ cm_command_listview_defaultformat, NULL,            MSG_CMENU_LISTTITLE_RESETTODEFAULTFORMAT },

	{ cm_command_snapshot,               DUMMY,           MSG_CMENU_SNAPSHOT      },
	{ cm_command_window,                 NULL,            MSG_CMENU_WINDOW        },
	{ cm_command_all,                    NULL,            MSG_CMENU_ALL           },
	{ cm_command_icons,                  NULL,            MSG_CMENU_ICONS         },
	{ cm_command_open,                   NULL,            MSG_CMENU_MIME_OPEN     },

	{ cm_command_trash,                  DUMMY,           MSG_CMENU_TRASH         },
	{ cm_command_restore,                DUMMY,           MSG_CMENU_RESTORE       },
	{ cm_command_empty,                  DUMMY,           MSG_CMENU_EMPTY         },
	{ cm_command_openParent,             DUMMY,           MSG_CMENU_PANELOPENPARENT}, // bitRocky

	{ cm_command_netsettings,            DUMMY,           MSG_CMENU_NETWORKSSETTINGS },
	{ cm_command_netconnect,             DUMMY,           MSG_CMENU_NETWORKSCONNECT },

	{ NULL, NULL, 0 }
};

ULONG contextmenu_setuplabels(void)
{
	CONST struct cmenustrings_init *cmsi = cmenustrings_initarr;
	STRPTR buf;

	while ((buf = cmsi->buf))
	{
		STRPTR image     = cmsi->image;
		STRPTR localized = GSI(cmsi->lsid);

		if (image && _conf(misc_contextmenuimages))
		{
			if (image == DUMMY)
				image = "dummy";

			snprintf(buf, CM_BUFFERSIZE, "\033I[4:PROGDIR:images/menu/menu_%s.mbr] %s", image, localized);
		}
		else
		{
			snprintf(buf, CM_BUFFERSIZE, "%s", localized);
		}

		cmsi++;
	}

	return 0;
}

ULONG contextmenu_init(void)
{
	contextmenu_setuplabels();

	{
		struct command_shortcut *cs = command_shortcut;

		while(cs->allow)
		{
			if (cs->shortcut)
				cs->shortcut = GSI((ULONG)cs->shortcut);

			cs++;
		};
	}

	return TRUE;
}

/*
 * Dummy...
 */
void contextmenu_cleanup(void)
{
}

/*
 * Sets/clears a menu item's check.
 */
void contextmenu_item_check(APTR pm, STRPTR name, ULONG action)
{
	APTR o;

	if ((o = (APTR)DoMethod(pm, MUIM_FindUData, name)))
	{
		set(o, MUIA_Menuitem_Checked, action);
	}
}


static struct command_menu * contextmenu_create_cm(STRPTR name, STRPTR args)
{
	struct command_menu *cm;

	if ((cm = malloc(sizeof(*cm))))
	{
		if ((cm->name = malloc(strlen(name) + 1)))
		{
			strcpy(cm->name, name);

			if ((cm->args = malloc(strlen(args) + 1)))
			{
				strcpy((STRPTR)cm->args, args);

				cm->type = AC_INTERNAL;
				cm->allow = AS_ANY,
				cm->flags = AF_ENABLED,
				cm->mutex = 0;
				cm->actionnode = NULL;
				cm->shortcut_msg_id = 0;

				return (cm);
			}
			free(cm->name);
		}
		free(cm);
	}
	return (NULL);
}

static APTR contextmenu_get_from_strip(APTR cmenu)
{
	struct MinList *l = (struct MinList *) getv(cmenu, MUIA_Family_List);
	APTR lptr = l->mlh_Head;

	return NextObject(&lptr);
}

void contextmenu_delete_cm(APTR cm)
{
	struct command_menu *icm = (struct command_menu *)cm;
	ASSERT(icm);

	if(icm)
	{
		if(icm->args)
			free((APTR)icm->args);

		ASSERT(icm->name);
		if(icm->name)
			free(icm->name);

		free(icm);
	}
}

/*
 * XXX: This one is breaking whole modular concept of views,
 * but had to be done because current method caused lots of whining:)
 * It adds special entries to 'Views' submenu instead of adding
 * entries to 'Mode' submenu.
 */

static ULONG contextmenu_addmodes_fileview(APTR obj, APTR pm, ULONG index UNUSED)
{
	struct viewnode *vn;
	ULONG flags;
	APTR mmenu, cm, m;
	ULONG viewmode;
	STRPTR viewmodename;
	ULONG /*icon,*/ allfiles, thumbs;

	vn = viewapi_findbyid( getv( _view( obj ), MA_Viewgroup_ViewIndex ) );
	viewmode = getv( _view( obj ), MA_Viewgroup_ViewModeIndex );

	if ( vn == NULL )
		return FALSE;

	viewmodename = (STRPTR)tags_nth_tagdata(AVIEW_Query_Viewmode_RexxName, (ULONG)NULL, vn->querytagarray, viewmode + 1);

	if ( viewmodename == NULL )
		return FALSE;

	flags = viewapi_getflags( vn );
	
	if ( !(flags & VF_FILEVIEW) )
		return FALSE;

	mmenu = (APTR)DoMethod(pm, MUIM_FindUData, PLACEHOLDER_VIEWS);

	if ( mmenu == NULL )
		return FALSE;

	/*
	 * Add special toggle modes.
	 */

	if ((m = NewObject(getmenuitemclass(), NULL,
			MA_Menuitem_Command, NULL,
			MA_Menuitem_FreeCommand, FALSE,
			MA_Menuitem_MenuType, 0,
			MA_Menuitem_SubType, MV_Menuitem_SubType_None,
			MUIA_Menuitem_Title, NM_BARLABEL,
			End))
		{
			DoMethod(mmenu, OM_ADDMEMBER, m);
		}

	//icon = !strcmp( viewmodename, "ICON" ) ? TRUE : FALSE;
	allfiles = !strcmp( viewmodename, "ALL" ) ? TRUE : FALSE;
	thumbs = !strcmp( viewmodename, "THUMBS" ) ? TRUE : FALSE;

	if ((cm = contextmenu_create_cm(GSI( MSG_VIEW_ALLFILES ), !allfiles ? "ALL" : "ICON")))
	{
		CONST_STRPTR s = NULL;

		if ((m = NewObject(getmenuitemclass(), NULL,
			MA_Menuitem_Command, cm,
			MA_Menuitem_FreeCommand, TRUE,
			MA_Menuitem_MenuType, CM_VIEW,
			MA_Menuitem_SubType, MV_Menuitem_SubType_Mode,
			MUIA_Menuitem_Title, GSI( MSG_VIEW_ALLFILES ),
			MUIA_Menuitem_Toggle, TRUE,
			MUIA_Menuitem_Checkit, TRUE,
			MUIA_Menuitem_Checked, allfiles | thumbs,
			MUIA_Menuitem_Exclude, 1,
			MUIA_Menuitem_Shortcut, s,
			End))
		{
			DoMethod(mmenu, OM_ADDMEMBER, m);
		}
	}

	if ((cm = contextmenu_create_cm(GSI( MSG_VIEW_THUMB ), thumbs ? "ALL" : "THUMBS")))
	{
		CONST_STRPTR s = NULL;

		if ((m = NewObject(getmenuitemclass(), NULL,
			MA_Menuitem_Command, cm,
			MA_Menuitem_FreeCommand, TRUE,
			MA_Menuitem_MenuType, CM_VIEW,
			MA_Menuitem_SubType, MV_Menuitem_SubType_Mode,
			MUIA_Menuitem_Title, GSI( MSG_VIEW_THUMB ),
			MUIA_Menuitem_Toggle, TRUE,
			MUIA_Menuitem_Checkit, TRUE,
			MUIA_Menuitem_Checked, thumbs,
			MUIA_Menuitem_Exclude, 2,
			MUIA_Menuitem_Shortcut, s,
			End))
		{
			DoMethod(mmenu, OM_ADDMEMBER, m);
		}
	}

	/*
	 * Remove modes submenu (empty)
	 */

	mmenu = (APTR)DoMethod(pm, MUIM_FindUData, PLACEHOLDER_MODES);

	if ( mmenu )
	{
		DoMethod(pm, OM_REMMEMBER, mmenu);
		MUI_DisposeObject(mmenu);
	}

	return TRUE;
}

ULONG contextmenu_addmodes(APTR obj, APTR pm, ULONG index)
{
	APTR m, mmenu;
	struct command_menu *cm;
	struct viewnode *vn;

	if ( contextmenu_addmodes_fileview(obj, pm, index) )
		return TRUE;

	vn = viewapi_findbyid( getv( _view( obj ), MA_Viewgroup_ViewIndex ) );

	if (vn && vn->querytagarray)
	{
		ULONG total = 0;
		ULONG added = FALSE;

		FORTAG(vn->querytagarray)
		{
			case AVIEW_Query_Viewmode_Name:
				total++;
				break;
		}
		NEXTTAG

		ASSERT(total <= 32); /* XXX: we should do something better :) */

		if ((mmenu = (APTR)DoMethod(pm, MUIM_FindUData, PLACEHOLDER_MODES)))
		{
			if (total > 1)
			{
				ULONG i = 0;
				ULONG j;
				ULONG bits;

				FORTAG(vn->querytagarray)
				{
					case AVIEW_Query_Viewmode_Name:
					{
						added = TRUE;
						DB(("Adding new one:%s\n",tag->ti_Data));
						if ((cm = contextmenu_create_cm((STRPTR)tag->ti_Data, (STRPTR)tags_nth_tagdata(AVIEW_Query_Viewmode_RexxName, tag->ti_Data, vn->querytagarray, i + 1))))
						{
							CONST_STRPTR s = NULL;

							if (cm->shortcut_msg_id)
							{
								s = GSI(cm->shortcut_msg_id);
								if (*s == '\0')
									s = NULL;
							}

							j = total;
							bits = 0;

							while (j--)
							{
								if (j != i)
								{
									bits |= 1 << j;
								}
							}

							if ((m = NewObject(getmenuitemclass(), NULL,
								MA_Menuitem_Command, cm,
								MA_Menuitem_FreeCommand, TRUE,
								MA_Menuitem_MenuType, CM_VIEW,
								MA_Menuitem_SubType, MV_Menuitem_SubType_Mode,
								MUIA_Menuitem_Title, cm->name,
								MUIA_Menuitem_Toggle, TRUE,
								MUIA_Menuitem_Checkit, TRUE,
								(i == index) ? MUIA_Menuitem_Checked : TAG_IGNORE, TRUE,
								MUIA_Menuitem_Exclude, bits,
								MUIA_Menuitem_Shortcut, s,
								End))
							{
								DoMethod(mmenu, OM_ADDMEMBER, m);
							}
							else
							{
								contextmenu_delete_cm(cm);
								return (FALSE);
							}
								/* XXX: hm.. maybe I should add togglemodes and so on */
						}
						else
						{
							return (FALSE);
						}
						i++;
					}
					break;
				}
				NEXTTAG
			}
		}

		if (!added)
		{
			DoMethod(pm, OM_REMMEMBER, mmenu);
			MUI_DisposeObject(mmenu);
		}
	}
	return (TRUE);
}


ULONG contextmenu_addviews(APTR obj, APTR pm)
{
	struct viewnode *vn_current, *vn;
	APTR mmenu;

	//vn_current = viewapi_findbyclass(OCLASS(obj));
	vn_current = viewapi_findbyid( getv( _view(obj), MA_Viewgroup_ViewIndex ) );

	vn = NULL;

	if ((mmenu = (APTR)DoMethod(pm, MUIM_FindUData, PLACEHOLDER_VIEWS))) /* XXX: mmenu == pm here.. why? it's wrong.. */
	{
		ULONG added = FALSE;
		STRPTR mimetype;

		if ((mimetype = (STRPTR)getv(obj, MA_View_MIME)))
		{
			struct command_menu *cm;
			APTR m;
			ULONG total = 0;
			ULONG i = 0;
			ULONG j;
			ULONG bits;

			while ((vn = viewapi_nextviewmime(vn, mimetype)))
			{
				total++; /* this really suck. I hate the current menu system */
			}

			vn = NULL;

			if (total > 1)
			{
				added = TRUE;

				while ((vn = viewapi_nextviewmime(vn, mimetype)))
				{
					if ((cm = contextmenu_create_cm(vn->label, vn->name)))
					{
						CONST_STRPTR s = NULL;

						if (cm->shortcut_msg_id)
						{
							s = GSI(cm->shortcut_msg_id);
							if (*s == '\0')
								s = NULL;
						}

						j = total;
						bits = 0;

						while (j--)
						{
							if (j != i)
							{
								bits |= 1 << j;
							}
						}

						if ((m = NewObject(getmenuitemclass(), NULL,
							MA_Menuitem_Command, cm,
							MA_Menuitem_FreeCommand, TRUE,
							MA_Menuitem_MenuType, CM_VIEW,
							MA_Menuitem_SubType, MV_Menuitem_SubType_View,
							MUIA_Menuitem_Title, cm->name,
							MUIA_Menuitem_Toggle, TRUE,
							MUIA_Menuitem_Checkit, TRUE,
							(vn == vn_current) ? MUIA_Menuitem_Checked : TAG_IGNORE, TRUE,
							MUIA_Menuitem_Exclude, bits,
							MUIA_Menuitem_Shortcut, s,
							End))
						{
							DoMethod(mmenu, OM_ADDMEMBER, m);
						}
						else
						{
							contextmenu_delete_cm(cm);
							return (FALSE);
						}
					}
					else
					{
						return (FALSE);
					}
					i++;
				}
			}
		}

		if (!added)
		{
			DoMethod(pm, OM_REMMEMBER, mmenu);
			MUI_DisposeObject(mmenu);
		}
	}
	return (TRUE);
}


static ULONG context_menu_default(APTR pm, ULONG menumode, ULONG allow)
{
	ULONG i = 0;
	const struct command_menu *cm = cm; /* shut up gcc */
	struct MinList l;
	struct tempobj *to;
	ULONG depth = 0;
	APTR current_obj = pm;
	APTR current_obj_lurking = current_obj_lurking; /* shut up gcc, really */
	ULONG current_depth = 0;
	APTR obj_barlabel;

	BOOL enable;

	NEWLIST(&l);

	switch (menumode)
	{
		case CM_ICON:
		case CM_LIST:
			cm = icon_default_menu;
			break;

		case CM_ICONGROUP:
			cm = icongroup_default_menu;
			break;

		case CM_ICONVIEW:
		case CM_LISTVIEW:
			cm = iconview_default_menu;
			break;

		case CM_LISTGROUP:
			cm = listgroup_default_menu;
			break;

		case CM_APPICON:
			cm = appicon_default_menu;
			break;

		case CM_PANELGROUP:
			cm = panelgroup_default_menu;
			break;

		case CM_PANELGROUP_DRAG:
			cm = panelgroup_drag_default_menu;
			break;
		
		case CM_PANELGROUP_SUBPANEL:
			cm = panelgroup_subpanel_default_menu;
			break;
		
		case CM_PANELGROUP_DIRPANEL:
			cm = panelgroup_dirpanel_default_menu;
			break;

		case CM_PANELBUTTON:
			cm = panelbutton_default_menu;
			break;

		case CM_PANELSPACER:
			cm = panelspacer_default_menu;
			break;

		case CM_PANELSEPARATOR:
			cm = panelseparator_default_menu; 
			break;

		case CM_PANELPOPUP:
			cm = panelpopup_default_menu;
			break;

		case CM_PANELGROUP_SUB:
			cm = panelgroup_sub_default_menu;
			break;

		case CM_VIEW:
			cm = view_default_menu;
			break;

		#ifdef DEBUG
		default:
			PDB(("out of bounds\n"));
			break;
		#endif
	}

	obj_barlabel = (APTR)1;

	for (i = 0; cm->name; i++, cm++)
	{
		if ((!cm->allow || (cm->allow & allow) || (cm->allow & AS_DISABLED)) && (!cm->allowfunc || cm->allowfunc(allow)))
		{
			CONST_STRPTR s;
			ULONG is_bar;
			APTR m;

			is_bar = cm->name && !*cm->name;

			if (is_bar && obj_barlabel)
			{
				continue;
			}

			depth = FLAGS_DEPTH(cm->flags);

			if (depth > current_depth) /* -> */
			{
				if ((to = malloc(sizeof(*to))))
				{
					to->obj = current_obj = current_obj_lurking;
					ADDTAIL(&l, to);
				}
				else
				{
					errormsg(ERR_NOMEM);
					context_menu_cleanup(&l);
					return (FALSE);
				}
			}
			else if (depth < current_depth) /* <- */
			{
				ULONG b = current_depth - depth;

				while (b--)
				{
					to = REMTAIL(&l);
					free(to);
				}

				if (ISLISTEMPTY(&l))
				{
					current_obj = pm;
				}
			}

			if(cm->enablefunc)
			{
				enable  = cm->enablefunc();
			}
			else
			{
				enable = TRUE;
			}

			s = NULL;

			if (cm->shortcut_msg_id)
			{
				s = GSI(cm->shortcut_msg_id);
				if (*s == '\0')
					s = NULL;
			}

			if ((m = NewObject(getmenuitemclass(), NULL,
				MUIA_Menuitem_Title, is_bar ? NM_BARLABEL : cm->name,
					MUIA_Menuitem_Checkit, cm->flags & AF_CHECKIT,
					MUIA_Menuitem_Toggle, cm->flags & AF_TOGGLE,
					MUIA_Menuitem_Enabled, (cm->flags & AF_ENABLED) && enable && !(cm->allow & AS_DISABLED && !(cm->allow & allow) ),
					MUIA_Menuitem_Checked, cm->flags & AF_CHECKED,
					cm->mutex ? MUIA_Menuitem_Exclude : TAG_IGNORE, cm->mutex,
					is_bar ? TAG_IGNORE : MA_Menuitem_Command, cm,
					MA_Menuitem_MenuType, menumode,
					MUIA_Menuitem_Shortcut, s,
				End))
			{
				obj_barlabel = is_bar ? m : NULL;
				current_obj_lurking = m;
				current_depth = depth;

				DoMethod(current_obj, OM_ADDMEMBER, m);
			}
			else
			{
				break; /* XXX: ouch */
			}
		}
	}

	if (obj_barlabel && obj_barlabel != (APTR)1)
	{
		/* Undo bottom barlabel */

		DoMethod(current_obj, OM_REMMEMBER, obj_barlabel);
		MUI_DisposeObject(obj_barlabel);
	}

	context_menu_cleanup(&l);
	return (i);
}


/*
 * Builds a context menu and returns its structure.
 */
APTR contextmenu_build(ULONG menumode, ULONG allow)
{
	APTR ms, m;
	STRPTR title;
	ULONG titleid;

	MAINTASK;

	switch (menumode)
	{
		case CM_ICON:
			titleid = MSG_CMENUTITLE_ICON;
			break;

		case CM_ICONVIEW:
		case CM_ICONGROUP:
			titleid = MSG_CMENUTITLE_ICONS;
			break;

		case CM_LIST:
			titleid = MSG_CMENUTITLE_FILE;
			break;

		case CM_LISTGROUP:
		case CM_LISTVIEW:
			titleid = MSG_CMENUTITLE_FILES;
			break;

		case CM_APPICON:
			titleid = MSG_CMENUTITLE_APPICON;
			break;

		case CM_PANELGROUP:
			titleid = MSG_CMENUTITLE_PANELGROUP;
			break;
		
		case CM_PANELGROUP_DRAG:
			titleid = MSG_CMENUTITLE_PANELGROUP;
			break;

		case CM_PANELGROUP_SUBPANEL:
			titleid = MSG_CMENUTITLE_PANELGROUP;
			break;
		
		case CM_PANELGROUP_DIRPANEL:
			titleid = MSG_CMENUTITLE_PANELGROUP;
			break;
		
		case CM_PANELBUTTON:
			titleid = MSG_CMENUTITLE_PANELBUTTON;
			break;

		case CM_PANELSPACER:
			titleid = MSG_CMENUTITLE_PANELSPACER;
			break;

		case CM_PANELSEPARATOR:
			titleid = MSG_CMENUTITLE_PANELSEPARATOR;
			break;

		case CM_PANELPOPUP:
			titleid = MSG_CMENUTITLE_PANELPOPUP;
			break;

		case CM_PANELGROUP_SUB:
			titleid = MSG_CMENUTITLE_PANELGROUPSUB;
			break;

		default:
		#ifdef DEBUG
			PDB(("out of bounds\n"));
		#endif
		case CM_VIEW:
			titleid = MSG_CMENUTITLE_VIEW;
			break;
	}

	title = GSI( titleid );

	if ((ms = NewObject(getmenustripclass(), NULL, TAG_DONE)))
	{
		if ((m = NewObject(getmenuclass(), NULL, MUIA_Menu_Title, title, TAG_DONE)))
		{
			DoMethod(ms, OM_ADDMEMBER, m);

			context_menu_default(m, menumode, allow);

			return (ms);
		}
		MUI_DisposeObject(ms);
	}
	return (NULL);
}
/*
 * Same as contextmenu_build, but returns just menu object, not strip
 */
APTR contextmenu_build_simple(ULONG menumode, ULONG allow)
{
	APTR m;
	STRPTR title;
	ULONG titleid;

	MAINTASK;

	switch (menumode)
	{
		case CM_ICON:
		case CM_APPICON:
			titleid = MSG_CMENUTITLE_ICON;
			break;

		case CM_ICONVIEW:
		case CM_ICONGROUP:
			titleid = MSG_CMENUTITLE_ICONS;
			break;

		case CM_LIST:
			titleid = MSG_CMENUTITLE_FILE;
			break;

		case CM_LISTGROUP:
		case CM_LISTVIEW:
			titleid = MSG_CMENUTITLE_FILES;
			break;

		case CM_PANELGROUP:
			titleid = MSG_CMENUTITLE_PANELGROUP;
			break;
	   
		case CM_PANELGROUP_DRAG:
			titleid = MSG_CMENUTITLE_PANELGROUP;
			break;

		case CM_PANELGROUP_SUBPANEL:
			titleid = MSG_CMENUTITLE_PANELGROUP;
			break;

		case CM_PANELGROUP_DIRPANEL:
			titleid = MSG_CMENUTITLE_PANELGROUP;
			break;

		case CM_PANELBUTTON:
			titleid = MSG_CMENUTITLE_PANELBUTTON;
			break;

		case CM_PANELSPACER:
			titleid = MSG_CMENUTITLE_PANELSPACER;
			break;

		case CM_PANELSEPARATOR:
			titleid = MSG_CMENUTITLE_PANELSEPARATOR;
			break;

		case CM_PANELPOPUP:
			titleid = MSG_CMENUTITLE_PANELPOPUP;
			break;

		case CM_PANELGROUP_SUB:
			titleid = MSG_CMENUTITLE_PANELGROUPSUB;
			break;

		default:
		#ifdef DEBUG
			PDB(("out of bounds\n"));
		#endif
		case CM_VIEW:
			titleid = MSG_CMENUTITLE_VIEW;
			break;
	}

	title = GSI( titleid );

	if ((m = NewObject(getmenuclass(), NULL, MUIA_Menu_Title, title, TAG_DONE)))
	{
		context_menu_default(m, menumode, allow);
		return (m);
	}

	return (NULL);
}


ULONG contextmenu_add_mime( APTR cmenu, STRPTR uri, APTR mimetype )
{
	APTR mmenu;
	ULONG spatial = _conf( toolbar_browsermode ) ? 0 : ACTION_FLAG_SPATIALMODE;
	if (!(mmenu = contextmenu_get_from_strip(cmenu)))
		return FALSE;

	if ( mimetype )
	{
		struct MinList *action_list = ((struct internal_mimetype_node*)mimetype)->action_list;

		set(cmenu, MUIA_Menu_Title, ((struct internal_mimetype_node*)mimetype)->description);

		/*
		 * Traverse actions list.
		 */

		if ( action_list && !ISLISTEMPTY( action_list ) )
		{
			APTR man;

			/*
			 * ... First add the separator...
			 */
			{
				APTR m = NewObject(getmenuitemclass(), NULL,
					MUIA_Menuitem_Title, NM_BARLABEL,
				TAG_DONE);

				if ( m )
				{
					DoMethod( mmenu, OM_ADDMEMBER, m );
				}
			}

			ITERATELIST( man, action_list )
			{
				ULONG event = (ULONG)actionnode_getattr( man, ACTIONNODETAG_EVENT );
				ULONG flags = (ULONG)actionnode_getattr( man, ACTIONNODETAG_FLAGS );
				ULONG qualifier = (ULONG)actionnode_getattr( man, ACTIONNODETAG_QUALIFIER );
				ULONG skip = FALSE;

				/*
				 * There might be also spatial version of an action. For now we assume it's right after
				 * nonspatial one to ease some things. Check and skp/use if needed.
				 */

				if ( spatial )
				{
					struct mimetype_action_node *man_next = NEXTNODE( man );

					/* next action is for spatial mode */

					if ( man_next && ( (ULONG)actionnode_getattr( man_next, ACTIONNODETAG_FLAGS ) & ACTION_FLAG_SPATIALMODE ) )
					{
						/* check action (event and qualifier) */

						ULONG event_next = (ULONG)actionnode_getattr( man_next, ACTIONNODETAG_EVENT );
						ULONG qualifier_next = (ULONG)actionnode_getattr( man_next, ACTIONNODETAG_QUALIFIER );

						if ( event == event_next && qualifier == qualifier_next )
						{
							skip = TRUE;
						}
					}
				}
				else
				{
					/*
					 * in browser mode we simply skip spatial actions.
					 */

					if ( flags & ACTION_FLAG_SPATIALMODE )
					{
						skip = TRUE;
					}
				}

				if ( !skip && ( event == ACTION_EVENT_DOUBLECLICK || event == ACTION_EVENT_MENU ) )
				{
					APTR m = NULL;
					struct command_menu *cm = malloc( sizeof (*cm ) );

					if(cm)
					{
						ULONG def = FALSE;

						if ( event == ACTION_EVENT_DOUBLECLICK && qualifier == ACTION_QUALIFIER_NONE )
							def = TRUE;

						/*
						 * We assign this action nodeto this entry. will be processed when selected.
						 */

						cm->name = malloc( strlen( actionnode_getattr( man, ACTIONNODETAG_NAME) ) + 1 + ( def ? 3 : 0 ) );

						if(cm->name)
						{
							if ( def )
								sprintf( cm->name, "\33b%s", (STRPTR)actionnode_getattr( man, ACTIONNODETAG_NAME) );
							else
								sprintf( cm->name, "%s", (STRPTR)actionnode_getattr( man, ACTIONNODETAG_NAME) );

							cm->type = 0;					/* we don't care about it */
							if ( uri != NULL )
								cm->args = name_build( uri );	/* we pass URI here */
							else
								cm->args = NULL;			/* null means for all selected....*/
							cm->actionnode = man;			/* action to execute on this URI */

							if(cm->args || uri == NULL)
							{
								APTR tmenu;
								STRPTR menuname = NULL;

								if(event == ACTION_EVENT_MENU && (menuname = actionnode_getattr( man, ACTIONNODETAG_MENU_NAME)))
								{
									TEXT placeholder[512];
									snprintf(placeholder, sizeof(placeholder), "PLACEHOLDER_%s", menuname);

									tmenu = (APTR)DoMethod(mmenu, MUIM_FindUData, placeholder);

									if(!tmenu)
									{
										struct command_menu * cm2 = malloc( sizeof (*cm2 ) );

										if(cm2)
										{
											cm2->name = (STRPTR) malloc(strlen(menuname)+1);

											if(cm2->name)
											{
												strcpy(cm2->name, menuname);

												cm2->args = (STRPTR) malloc(strlen(placeholder)+1);

												if(cm2->args)
												{
													strcpy((STRPTR) cm2->args, placeholder);

													m = NewObject(getmenuitemclass(), NULL,
													MA_Menuitem_FreeCommand, TRUE,
													MUIA_Menuitem_Title, cm2->name,
													MA_Menuitem_Command, cm2,
													TAG_DONE);

													if ( m )
													{
														tmenu = m;
														DoMethod( mmenu, OM_ADDMEMBER, m );
													}
												}
											}
										}

										if(!m)
										{
											contextmenu_delete_cm(cm);
										}	 
									}	 
								}
								else
								{
									tmenu = mmenu;
								}

								if(tmenu)
								{
									m = NewObject(getmenuitemclass(), NULL,
									MA_Menuitem_FreeCommand, TRUE,
									MUIA_Menuitem_Title, cm->name,
									MA_Menuitem_Command, cm,
									TAG_DONE);

									if ( m )
									{
										DoMethod( tmenu, OM_ADDMEMBER, m );
									}
								}
							}
						}

						if(!m)
						{
							contextmenu_delete_cm(cm);
						}
					}
				}
			}
		}
	}

	return (TRUE);
}

ULONG contextmenu_add_listview_options(APTR obj, APTR cmenu, ULONG(*callback)(APTR obj, int i, struct menuitem_state *state))
{
	APTR m;
	const struct command_menu * cm;
	int i;

	set(cmenu, MUIA_Menu_Title, GSI( MSG_CMENUTITLE_PROPERTIES ));

	for (cm=listview_properties_menu,i=0; cm->name; cm++,i++)
	{
		struct menuitem_state state;
		ULONG checked = FALSE;

		if(callback(obj, i, &state))
		{
			checked = state.checked;
		}

		if ((m = NewObject(getmenuitemclass(), NULL,
			MA_Menuitem_Command, cm,
			MUIA_Menuitem_Title, *cm->name ? cm->name : NM_BARLABEL,
			MUIA_Menuitem_Checkit, cm->flags & AF_CHECKIT,
			MUIA_Menuitem_Toggle, cm->flags & AF_TOGGLE,
			(cm->flags&AF_CHECKIT)?MUIA_Menuitem_Checked:TAG_IGNORE, checked,
			End))
		{
			DoMethod(cmenu, OM_ADDMEMBER, m);
		}
	}
	return (TRUE);
}

ULONG contextmenu_add_global( APTR cmenu, STRPTR uri, ULONG type )
{
	APTR mmenu;

	struct MinList * action_list;
	struct internal_mimetype_node * imn = NULL;

	switch(type)
	{
		case MENU_GLOBALACTION_DIRECTORY:
			imn = mimetype_find_by_mimetype("internal/x-morphos-globalaction-directory");
			break;
		case MENU_GLOBALACTION_DEVICE:
			imn = mimetype_find_by_mimetype("internal/x-morphos-globalaction-device");
			break;
		case MENU_GLOBALACTION_FILE:
			imn = mimetype_find_by_mimetype("internal/x-morphos-globalaction-file");
			break;
	}

	if(!imn)
		return FALSE;

	if (!(mmenu = contextmenu_get_from_strip(cmenu)))
		return FALSE;

	/*
	 * Traverse actions list.
	 */

	action_list = imn->action_list;

	if ( action_list && !ISLISTEMPTY( action_list ) )
	{
		APTR man;

		/*
		 * ... First add the separator...
		 */
		{
			APTR m = NewObject(getmenuitemclass(), NULL,
				MUIA_Menuitem_Title, NM_BARLABEL,
			TAG_DONE);

			if ( m )
			{
				DoMethod( mmenu, OM_ADDMEMBER, m );
			}
		}

		ITERATELIST( man, action_list )
		{
			ULONG event = (ULONG)actionnode_getattr( man, ACTIONNODETAG_EVENT );
			//ULONG flags = (ULONG)actionnode_getattr( man, ACTIONNODETAG_FLAGS );
			//ULONG qualifier = (ULONG)actionnode_getattr( man, ACTIONNODETAG_QUALIFIER );

			if ( event == ACTION_EVENT_MENU )
			{
				APTR m = NULL;
				struct command_menu *cm = malloc( sizeof (*cm ) );

				/*
				 * We assign this action node to this entry. will be processed when selected.
				 */

				if(cm)
				{
					cm->name = malloc( strlen( actionnode_getattr( man, ACTIONNODETAG_NAME) ) + 1 );

					if(cm->name)
					{
						STRPTR menuname;
						APTR tmenu;

						strcpy( cm->name, (STRPTR)actionnode_getattr( man, ACTIONNODETAG_NAME) );

						cm->type = 0;					/* we don't care about it */
						if ( uri != NULL )
							cm->args = name_build( uri );	/* we pass URI here */
						else
							cm->args = NULL;			/* null means for all selected....*/
						cm->actionnode = man;			/* action to execute on this URI */

						if(cm->args || uri == NULL)
						{
							if((menuname = actionnode_getattr( man, ACTIONNODETAG_MENU_NAME)))
							{
								TEXT placeholder[512];
								snprintf(placeholder, sizeof(placeholder), "PLACEHOLDER_%s", menuname);

								tmenu = (APTR)DoMethod(mmenu, MUIM_FindUData, placeholder);

								if(!tmenu)
								{
									struct command_menu * cm2 = malloc( sizeof (*cm2 ) );

									if(cm2)
									{
										cm2->name = (STRPTR) malloc(strlen(menuname)+1);

										if(cm2->name)
										{
											strcpy(cm2->name, menuname);

											cm2->args = (STRPTR) malloc(strlen(placeholder)+1);

											if(cm2->args)
											{
												strcpy((STRPTR) cm2->args, placeholder);

												m = NewObject(getmenuitemclass(), NULL,
												MA_Menuitem_FreeCommand, TRUE,
												MUIA_Menuitem_Title, cm2->name,
												MA_Menuitem_Command, cm2,
												TAG_DONE);

												if ( m )
												{
													tmenu = m;
													DoMethod( mmenu, OM_ADDMEMBER, m );
												}
											}
										}
									}

									if(!m)
									{
										contextmenu_delete_cm(cm2);
									}
								}
							}
							else
							{
								tmenu = mmenu;
							}

							if(tmenu)
							{
								m = NewObject(getmenuitemclass(), NULL,
								MA_Menuitem_FreeCommand, TRUE,
								MUIA_Menuitem_Title, cm->name,
								MA_Menuitem_Command, cm,
								TAG_DONE);

								if ( m )
								{
									DoMethod( tmenu, OM_ADDMEMBER, m );
								}
							}
						}
					}

					if(!m)
					{
						contextmenu_delete_cm(cm);
					}
				}
			}
		}
	}

	return (TRUE);
}


void contextmenu_execute_shortcut(APTR obj, TEXT c, ULONG mask)
{
	struct command_shortcut *cs = command_shortcut;

	do
	{
		if (cs->allow & mask && cs->shortcut && *cs->shortcut == c)
		{
			execute_command_objarray(obj, AC_INTERNAL, cs->args);
			break;
		}

		cs++;
	}
	while (cs->allow);
}


void contextmenu_execute(APTR obj, APTR parent, APTR cmobj, ULONG grouped)
{
	struct command_menu *cm;

	if ((cm = (struct command_menu *)getv(cmobj, MA_Menuitem_Command)))
	{

		if (cm->actionnode == NULL && cm->name && *cm->name)
		{
			if (cm->flags & AF_NO_GROUPING)
			{
				execute_command(parent, cm->type, cm->args, NULL);
			}
			else if (grouped)
			{
				/* some internal commands expect an objlist, will be difficult with aline entries, hm.
				 * should we create a dummy object, having MA_Icon_Path attribute for each entry ?
				 * let's do that for now...
				 */

				execute_command_objarray(parent, cm->type, cm->args);
			}
			else
			{
				execute_command(obj, cm->type, cm->args, NULL); /* XXX: I think.. well no.. we need the object */
			}
		}

		if (cm->actionnode)
		{
			/*
			 * This is action we selected. Needs to be parsed.
			 */

			APTR dispatcher = (APTR)DoMethod( _app(obj), MM_Application_CreateActionDispatcher );

			if ( dispatcher )
			{
				SetAttrs(dispatcher,
					MA_ActionDispatcher_SrcURI, getv(parent, MA_View_Path),
					MA_ActionDispatcher_SrcID, getv(_win( obj ), MA_Window_ID),
					MA_ActionDispatcher_RefWin, _win(obj),
					MA_ActionDispatcher_Action, cm->actionnode,
					TAG_DONE
				);

				if ( cm->args )
				{
					/* single entry mode */

					DoMethod( dispatcher, MM_ActionDispatcher_AddURI, cm->args, TRUE );
				}
				else
				{
					APTR *selection	= (APTR*)DoMethod( parent, MM_View_PickSelected );
					ULONG i = 0;

					if ( selection != NULL )
					{
						while( selection[ i ] != NULL )
						{
							DoMethod( dispatcher, MM_ActionDispatcher_AddURI, getv( selection[ i ], MA_Icon_Path ), TRUE );
							i++;
						};

						FreeVecTaskPooled( selection );
					}

				}
				DoMethod( dispatcher, MM_ActionDispatcher_Execute );
			}
		}
	}
}
