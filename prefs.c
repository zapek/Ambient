/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2016 Ambient Open Source Team
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
 * $Id: prefs.c,v 1.28 2025/07/23 22:53:31 geit Exp $
 */

#include "ambient.h"

/* public */
#include <clib/alib_protos.h>

/* private */
#include "prefs.h"
#include "ambient_cat.h"
#include "mui_func.h"
#include "updatelist.h"
#include "textbox.h"
#include "background.h"
#include "listviewclass.h"
#include "paneltags.h"


APTR mainprefspool;
APTR cloneprefspool;
struct global_prefs *gprefs;

static APTR backupprefspool;


ULONG prefs_init(void)
{
	if ( (gprefs = malloc(sizeof(*gprefs))) )
	{
		memset(gprefs, 0, sizeof(*gprefs));

		if ( (mainprefspool = prefspool_create(0)) )
		{
			return (TRUE);
		}
	}
	return (FALSE);
}


void prefs_cleanup(void)
{
	if (backupprefspool)
	{
		prefspool_delete(backupprefspool);
	}

	if (mainprefspool)
	{
		prefspool_delete(mainprefspool);
	}

	if (cloneprefspool)
	{
		prefspool_delete(cloneprefspool);
	}

	if (gprefs)
	{
		free(gprefs);
	}
}


/*
 * Set prefs with variable datasize.
 */
void setprefs_ctx(APTR ctx, ULONG id, ULONG size, CONST_APTR data)
{
	prefspool_item_add(ctx, NULL, id, data, size); /* XXX: out of mem ? */
}

/* same without overwrite */
void setprefs_default_ctx(APTR ctx, ULONG id, ULONG size, CONST_APTR data)
{
	if (!prefspool_item_get(ctx, NULL, id, NULL, NULL))
	{
		prefspool_item_add(ctx, NULL, id, data, size); /* XXX: out of mem ? */
	}
}


/*
 * Set prefs string.
 */
void setprefsstr_ctx(APTR ctx, ULONG id, CONST_STRPTR data)
{
	prefspool_item_add(ctx, NULL, id, data, strlen(data) + 1); /* XXX: out of mem ? */
}

/* same without overwrite */
void setprefsstr_default_ctx(APTR ctx, ULONG id, CONST_STRPTR data)
{
	if (!prefspool_item_get(ctx, NULL, id, NULL, NULL))
	{
		prefspool_item_add(ctx, NULL, id, data, strlen(data) + 1); /* XXX: out of mem ? */
	}
}

/*
 * Set prefs long.
 */
void setprefslong_ctx(APTR ctx, ULONG id, ULONG v)
{
	prefspool_item_add(ctx, NULL, id, &v, sizeof(v)); /* XXX: out of mem ? */
}

/* same without overwrite */
void setprefslong_default_ctx(APTR ctx, ULONG id, ULONG v)
{
	if (!prefspool_item_get(ctx, NULL, id, NULL, NULL))
	{
		prefspool_item_add(ctx, NULL, id, &v, sizeof(v)); /* XXX: out of mem ? */
	}
}


/*
 * Get prefs string.
 */
STRPTR getprefsstr_ctx(APTR ctx, ULONG id)
{
	STRPTR p;

	if (prefspool_item_get(ctx, NULL, id, (APTR)&p, NULL))
	{
		return (p);
	}
	else
	{
		return (NULL);
	}
}


/*
 * Get prefs, without default fallback.
 */
APTR getprefs_ctx(APTR ctx, ULONG id)
{
	APTR p;

	if (prefspool_item_get(ctx, NULL, id, &p, NULL))
	{
		return (p);
	}
	else
	{
		return (NULL);
	}
}


/*
 * Get prefs long.
 */
ULONG getprefslong_ctx(APTR ctx, ULONG id)
{
	ULONG *v;

	if (prefspool_item_get(ctx, NULL, id, (APTR)&v, NULL))
	{
		return (*v);
	}
	else
	{
		return (0);
	}
}


/*
 * Shortcuts for prefspool_item_add()
 */
ULONG setprefsstr_lp(APTR ctx, APTR pitem, ULONG id, CONST_STRPTR s)
{
	return ((ULONG)prefspool_item_add(ctx, pitem, id, s, strlen(s) + 1));
}


ULONG setprefslong_lp(APTR ctx, APTR pitem, ULONG id, ULONG v)
{
	return ((ULONG)prefspool_item_add(ctx, pitem, id, &v, sizeof(v)));
}

/*
 * Prefs clone system.
 */


/*
 * Call to setup the prefsclone.
 */
ULONG prefsclone_init(void)
{
	return ((ULONG)(cloneprefspool = prefspool_create(0))); // bitRocky: added some parenthesis because GCC5 complains
}


/*
 * Call when you're done with the prefsclone. If
 * writeback is true, it's written to disk, otherwise
 * it's ignored (eg. Use/Cancel buttons).
 */
void prefsclone_cleanup(ULONG writeback, ULONG remove)
{
	if (cloneprefspool)
	{
		if (writeback)
		{
			prefspool_copy(cloneprefspool, mainprefspool, updatelist_add); /* XXX: retval ? */
		}
		
		if (remove)
		{
			prefspool_delete(cloneprefspool);
			cloneprefspool = NULL;
		}
	}
}


static APTR getprefsclone(ULONG id)
{
	APTR p;

	if (prefspool_item_get(cloneprefspool, NULL, id, &p, NULL))
	{
		return (p);
	}
	else if (prefspool_item_get(mainprefspool, NULL, id, &p, NULL))
	{
		return (p);
	}
	else
	{
		return (NULL);
	}
}

STRPTR getprefsstr_clone(ULONG id)
{
	return ((STRPTR)getprefsclone(id));
}


APTR getprefs_clone(ULONG id)
{
	return (getprefsclone(id));
}


ULONG getprefslong_clone(ULONG id)
{
	ULONG *v = (ULONG *)getprefsclone(id);

	if (v)
	{
		return (*v);
	}
	else
	{
		return (0); /* sigh */
	}
}


/*
 * Backup system for 'Test' button. The
 * current prefs are stored in the backupprefspool.
 */
ULONG backup_prefs(void)
{
	if (backupprefspool)
	{
		prefspool_flush(backupprefspool);
	}
	else
	{
		backupprefspool = prefspool_create(0);
	}

	if (backupprefspool)
	{
		prefspool_copy(mainprefspool, backupprefspool, NULL); /* XXX: hm.. */
	}
	return (TRUE); /* XXX */
}


void restore_prefs(void)
{
	if (backupprefspool)
	{
		prefspool_flush(mainprefspool);

		prefspool_copy(backupprefspool, mainprefspool, NULL);

		prefspool_delete(backupprefspool);
		backupprefspool = NULL;
	}
}


/*
 * Pen presets.
 */
static const struct MUI_PenSpec pcol_black = {
	"r00000000,00000000,00000000"
};

static const struct MUI_PenSpec pcol_white = {
	"rffffffff,ffffffff,ffffffff"
};

static const struct MUI_PenSpec pcol_red = {
	"rffffffff,10101010,10101010"
};
#if 0
static const struct MUI_PenSpec pcol_yellow = {
	"rec44ec44,eeeeeeee,22f422f4"
};
#endif
static const struct MUI_PenSpec pcol_green_fluo = {
	"r16731673,eeeeeeee,00000000"
};

static const struct MUI_PenSpec pcol_pink = {
	"reeeeeeee,00000000,b380b380"
};

static const struct MUI_PenSpec pcol_blue = {
	"r50395039,81828182,ffffffff"
};

static const struct MUI_PenSpec pcol_mshine = {
	"m0"
};

static const struct MUI_PenSpec pcol_mback = {
	"m2"
};

static const struct MUI_PenSpec pcol_mtext = {
	"m5"
};
#if 0
static const struct MUI_PenSpec pcol_mfill = {
	"m6"
};
#endif
/* the default desktop color */
static const struct MUI_PenSpec pcol_desktop = {
	"r49494949,8c8c8c8c,a7a7a7a7"
};

/*
 * toolbar/window presets
 */
static const UBYTE toolbar_def[] =
{
	""
};


/*
 * The evil routine. During Ambient development I added new prefs
 * stuff. This routine is called after loading the prefs to try to figure
 * out some smart way of "upgrading" prefs, or at least figuring out
 * some new options correctly.
 */
static void prefs_fix(void)
{
	STRPTR str;

	/*
	 * Since the previous default was tiled mode
	 * we set them like that if there's a picture.
	 */
	if (!prefspool_item_get(mainprefspool, NULL, DSI_BACKGROUND_ROOT_BGRENDER, NULL, NULL))
	{
		str = getprefsstr(DSI_BACKGROUND_ROOT);

		if (str && str[0])
		{
			setprefslong(DSI_BACKGROUND_ROOT_BGRENDER, BGRENDER_Tiled);
		}
		else
		{
			setprefslong(DSI_BACKGROUND_ROOT_BGRENDER, BGRENDER_Color);
		}
	}

	if (!prefspool_item_get(mainprefspool, NULL, DSI_BACKGROUND_WINDOW_BGRENDER, NULL, NULL))
	{
		str = getprefsstr(DSI_BACKGROUND_WINDOW);

		if (str && str[0])
		{
			setprefslong(DSI_BACKGROUND_WINDOW_BGRENDER, BGRENDER_Tiled);
		}
		else
		{
			setprefslong(DSI_BACKGROUND_WINDOW_BGRENDER, BGRENDER_Color);
		}
	}

	str = getprefsstr(DSI_FASTLIST_DEFAULT_FORMAT_FILES);

	if(!listview_check_format_string(str, FLT_FILES))
	{
		setprefsstr(DSI_FASTLIST_DEFAULT_FORMAT_FILES, LVFORMAT_FILES);
	}

	str = getprefsstr(DSI_FASTLIST_DEFAULT_FORMAT_DEVICES);

	if(!listview_check_format_string(str, FLT_DEVICES))
	{
		setprefsstr(DSI_FASTLIST_DEFAULT_FORMAT_DEVICES, LVFORMAT_DEVICES);
	}

}


/*
 * Sets default prefs.
 */
void set_default_prefs(void)
{
	prefs_fix();

	/*
	 * Background
	 */
	setprefsstr_default(DSI_BACKGROUND_ROOT, "");
	setprefsstr_default(DSI_BACKGROUND_WINDOW, "");
	setprefslong_default(DSI_BACKGROUND_ROOT_BGRENDER, BGRENDER_Color);
	setprefslong_default(DSI_BACKGROUND_WINDOW_BGRENDER, BGRENDER_Color);
	
	setprefs_default(DSI_BACKGROUND_ROOT_BGCOLOR, sizeof(pcol_desktop), (APTR) &pcol_desktop);
	setprefs_default(DSI_BACKGROUND_WINDOW_BGCOLOR, sizeof(pcol_desktop), (APTR)&pcol_desktop);

	setprefslong_default(DSI_BACKGROUND_ROOT_REFRESH_DELAY, 0);

	/*
	 * Fonts
	 */
	setprefsstr_default(DSI_FONT_ROOT, "XHelvetica/11");
	setprefsstr_default(DSI_FONT_ROOT_SMALL, "XHelvetica/9");
	setprefsstr_default(DSI_FONT_WINDOW, "XHelvetica/11");
	setprefslong_default(DSI_FONT_ROOT_EFFECT, TBRENDER_AlphaShadow);
	setprefslong_default(DSI_FONT_WINDOW_EFFECT, TBRENDER_AlphaShadow);

	/*
	 * Colors
	 */
	setprefs_default(DSI_COLOR_ROOT, sizeof(pcol_white), &pcol_white);
	setprefs_default(DSI_COLOR_WINDOW, sizeof(pcol_white), &pcol_white);
	setprefs_default(DSI_COLOR_ROOT2, sizeof(pcol_black), &pcol_black);
	setprefs_default(DSI_COLOR_WINDOW2, sizeof(pcol_black), &pcol_black);
	setprefs_default(DSI_COLOR_LASSO, sizeof(pcol_red), &pcol_red);

	/*
	 * Space
	 */
	setprefslong_default(DSI_FONT_ROOTSPACE, 2);
	setprefslong_default(DSI_FONT_WINDOWSPACE, 2);

	/*
	 * Miscellaneous
	 */
	setprefsstr_default(DSI_MISC_WBSTARTUP_PATH, "SYS:WBStartup");
	setprefslong_default(DSI_MISC_DRIVE_INFO, 2L);

	setprefslong_default(DSI_MISC_MYMORPHOSICON, TRUE);
	setprefslong_default(DSI_MISC_DESKTOPDOUBLECLICK, TRUE);
	setprefslong_default(DSI_MISC_SMARTFILEOPERATIONS, TRUE);
	setprefslong_default(DSI_MISC_TRAPMORE, TRUE);
	setprefslong_default(DSI_MISC_TRAPMULTIVIEW, TRUE);
	setprefslong_default(DSI_MISC_CONTEXTMENUIMAGES, TRUE);
	setprefslong_default(DSI_MISC_CREATEICONFORNEWDRAWER, TRUE);

	/*
	 * Icon options
	 */

	setprefslong_default(DSI_ICON_SELECTEFFECT, 0); // tint
	setprefs_default(DSI_ICON_TINTVAL, sizeof(pcol_blue), &pcol_blue);
	setprefs_default(DSI_ICON_TINTFADEVAL, sizeof(pcol_blue), &pcol_blue);
	setprefslong_default(DSI_ICON_BRIGHTENVAL, 0x40);
	setprefslong_default(DSI_ICON_DARKENVAL, 0x40);

	setprefslong_default(DSI_ICON_MINSIZE, 1); /* micro icons (16x16, allows user to see icons smaller than 64x64 unscaled/ filtered */
	setprefslong_default(DSI_ICON_MAXSIZE, 5); /* huge icons  (64x64, morphos default icon size) <- do not change this or icon designers will burn you in hell! :) */

	setprefslong_default(DSI_ICON_HOVER, FALSE);

	setprefslong_default(DSI_ICON_SEPARATEFILES, TRUE);
	setprefslong_default(DSI_ICON_SHORTCUTSIDENTIFIER, TRUE);
	setprefslong_default(DSI_ICON_APPICONSIDENTIFIER, FALSE);
	setprefslong_default(DSI_ICON_DEFGHOSTED, TRUE);
	setprefslong_default(DSI_ICON_AUTOSNAPSHOT, FALSE);
	setprefslong_default(DSI_ICON_DUALPNG, FALSE);
	setprefslong_default(DSI_ICON_FADE, TRUE);

	/*
	 * CLI launching
	 */
	setprefsstr_default(DSI_CLI_DEVICE, "CON:////Ambient Output Window/CLOSE/AUTO/WAIT");
	setprefslong_default(DSI_CLI_STACK, 16384);
	setprefsstr_default(DSI_CLI_NEWSHELL, "WINDOW=\"CON:200/240/640/480/Ambient Shell/CLOSE\"");

	/*
	 * Drag & Drop
	 */
	#if USE_SOLIDDRAG
	setprefslong_default(DSI_DRAGDROP_DISPLAY, 0); // solid
	#endif
	setprefslong_default(DSI_DRAGDROP_DROPEFFECT, 0); // tint
	setprefs_default(DSI_DRAGDROP_TINTVAL, sizeof(pcol_red), &pcol_red);
	setprefs_default(DSI_DRAGDROP_TINTFADEVAL, sizeof(pcol_red), &pcol_red);
	setprefslong_default(DSI_DRAGDROP_BRIGHTENVAL, 0x40);
	setprefslong_default(DSI_DRAGDROP_DARKENVAL, 0x40);

	/*
	 * Format
	 */
	setprefslong_default(DSI_FORMAT_ICON, TRUE);
	setprefslong_default(DSI_FORMAT_SFS_CASE, FALSE);
	setprefslong_default(DSI_FORMAT_SFS_RECYCLED, TRUE);
	setprefslong_default(DSI_FORMAT_SFS_SHOWRECYCLED, TRUE);

	/*
	 * Lister
	 */
	setprefs_default(DSI_FASTLIST_COLOR_FILE_FG, sizeof(pcol_mtext), &pcol_mtext);
	setprefs_default(DSI_FASTLIST_COLOR_FILE_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_DIRECTORY_FG, sizeof(pcol_mshine), &pcol_mshine);
	setprefs_default(DSI_FASTLIST_COLOR_DIRECTORY_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_FILE_SEL_FG, sizeof(pcol_mshine), &pcol_mshine);
	setprefs_default(DSI_FASTLIST_COLOR_FILE_SEL_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_DIRECTORY_SEL_FG, sizeof(pcol_mshine), &pcol_mshine);
	setprefs_default(DSI_FASTLIST_COLOR_DIRECTORY_SEL_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_SOURCE_FG, sizeof(pcol_mshine), &pcol_mshine);
	setprefs_default(DSI_FASTLIST_COLOR_SOURCE_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_DESTINATION_FG, sizeof(pcol_mshine), &pcol_mshine);
	setprefs_default(DSI_FASTLIST_COLOR_DESTINATION_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_VOLUME_FG, sizeof(pcol_mshine), &pcol_mshine);
	setprefs_default(DSI_FASTLIST_COLOR_VOLUME_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_ASSIGN_FG, sizeof(pcol_mtext), &pcol_mtext);
	setprefs_default(DSI_FASTLIST_COLOR_ASSIGN_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_SOFTLINK_FG, sizeof(pcol_green_fluo), &pcol_green_fluo);
	setprefs_default(DSI_FASTLIST_COLOR_SOFTLINK_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_HARDLINK_FG, sizeof(pcol_pink), &pcol_pink);
	setprefs_default(DSI_FASTLIST_COLOR_HARDLINK_BG, sizeof(pcol_mback), &pcol_mback);

	setprefs_default(DSI_FASTLIST_COLOR_COLUMN_FG, sizeof(pcol_mtext), &pcol_mtext);

	setprefslong_default(DSI_FASTLIST_WINDOW_BG, TRUE);
	setprefslong_default(DSI_FASTLIST_HANDLE_ICONS, FALSE);

	setprefslong_default(DSI_FASTLIST_COMPACT_SIZE_DISPLAY, FALSE);
	setprefslong_default(DSI_FASTLIST_ASSIGN_DISPLAY, TRUE);

	setprefsstr_default(DSI_FASTLIST_DEFAULT_FORMAT_DEVICES, LVFORMAT_DEVICES);
	setprefsstr_default(DSI_FASTLIST_DEFAULT_FORMAT_FILES,   LVFORMAT_FILES);
	setprefsstr_default(DSI_FASTLIST_DEFAULT_MODE_FILES,     LVMODE_DEFAULT_FILES);
	setprefsstr_default(DSI_FASTLIST_DEFAULT_MODE_DEVICES,   LVMODE_DEFAULT_DEVICES);

	setprefslong_default(DSI_FASTLIST_BOLD_DIRECTORIES, FALSE);
	setprefslong_default(DSI_FASTLIST_HILIGHTED_SORTING_COLUMN, FALSE);
	setprefslong_default(DSI_FASTLIST_ALTERNATED_ROWS, FALSE);

	setprefsstr_default(DSI_FASTLIST_FONT, "");

	/*
	 * Panels
	 */
	setprefslong_default(DSI_PANEL_ZIPSPEED, MV_Panel_ZipSpeed_Medium);
	setprefslong_default(DSI_PANEL_AUTOSAVE_DROP,TRUE);
	setprefslong_default(DSI_PANEL_AUTOSAVE_DELETE,TRUE);
	setprefslong_default(DSI_PANEL_AUTOSAVE_MOVE,TRUE);
	setprefslong_default(DSI_PANEL_AUTOSAVE_WINDOWPOS,FALSE);
	setprefslong_default(DSI_PANEL_HIGHLIGHT_EFFECT,PANEL_EFFECT_CLONE_ICONVIEW);
	setprefslong_default(DSI_PANEL_DRAGDROP_EFFECT,PANEL_EFFECT_CLONE_ICONVIEW);
	setprefslong_default(DSI_PANEL_SELECTED_EFFECT,PANEL_EFFECT_CLONE_ICONVIEW);
	setprefslong_default(DSI_PANEL_HIGHLIGHT_BRIGHTEN,0x80);
	setprefslong_default(DSI_PANEL_HIGHLIGHT_DARKEN,0x80);
	setprefslong_default(DSI_PANEL_DRAGDROP_BRIGHTEN,0x80);
	setprefslong_default(DSI_PANEL_DRAGDROP_DARKEN,0x80);
	setprefslong_default(DSI_PANEL_SELECTED_BRIGHTEN,0x80);
	setprefslong_default(DSI_PANEL_SELECTED_DARKEN,0x80);
	setprefslong_default(DSI_PANEL_LAYOUT_GRID,TRUE);
	setprefs_default(DSI_PANEL_HIGHLIGHT_TINT, sizeof(pcol_red), &pcol_red);
	setprefs_default(DSI_PANEL_DRAGDROP_TINT, sizeof(pcol_blue), &pcol_blue);
	setprefs_default(DSI_PANEL_HIGHLIGHT_TINTFADE, sizeof(pcol_red), &pcol_red);
	setprefs_default(DSI_PANEL_DRAGDROP_TINTFADE, sizeof(pcol_blue), &pcol_blue);
	setprefs_default(DSI_PANEL_SELECTED_TINT, sizeof(pcol_red), &pcol_red);
	setprefs_default(DSI_PANEL_SELECTED_TINTFADE, sizeof(pcol_red), &pcol_red);
	/*
	 * Toolbar
	 */
	setprefslong_default(DSI_TOOLBAR_BROWSERMODE, FALSE);
	setprefslong_default(DSI_TOOLBAR_DISPLAYMODE, DM_IMAGETEXT);
	setprefsstr_default(DSI_TOOLBAR_DEFINITION, (STRPTR) toolbar_def);
	setprefsstr_default(DSI_TOOLBAR_BUTTONFRAMESPEC, "!");
	setprefsstr_default(DSI_TOOLBAR_BUTTONIMAGESPEC, "!");

	setprefslong_default(DSI_WINDOW_DEFAULT_WIDTH,  400);
	setprefslong_default(DSI_WINDOW_DEFAULT_HEIGHT, 300);
	setprefslong_default(DSI_WINDOW_DEFAULT_TOP,    50);
	setprefslong_default(DSI_WINDOW_DEFAULT_LEFT,   50);
	
	setprefslong_default(DSI_WINDOW_INHERIT_VIEWMODE, TRUE);

	setprefsstr_default(DSI_WINDOW_STATUSBARFORMAT, GSI(MSG_STATUSBAR_PATTERN_NORMAL));

	setprefs_default(DSI_WINDOW_STATUSBARCOLOR, sizeof(pcol_black), &pcol_mtext); // bitRocky

	/*
	 * Dirty flag for hotkeys listpool
	 */
	setprefslong_default(DSI_LISTPOOL_KEYSHORTCUT_CHANGED, 0);

	/*
	 * Bookmarks
	 */

	setprefslong_default(DSI_BOOKMARKS_SHOWONLY, 0);

	/*
	 * Debugging
	 */
	#ifdef DEBUG
	{
		ULONG i;

		for (i = 0; db_a[i].prefs; i++)
		{
			setprefslong_default(db_a[i].prefs, FALSE);
		}
	}
	#endif /* DEBUG */
}
