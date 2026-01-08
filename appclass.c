/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber
 * Copyright 2005-2018 Ambient Open Source Team
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
 * $Id: appclass.c,v 1.118.2.1 2024/01/20 02:56:53 piru Exp $
 */

#include "ambient.h"

/* public */
#include <exec/execbase.h>
#include <exec/rawfmt.h> // bitRocky: used in RXCMD_Settings
#include <graphics/rpattr.h>
#include <workbench/workbench.h>
#include <intuition/extensions.h>
#include <intuition/monitorclass.h>
#include <dos/dostags.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/wb.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/dos.h>

#if !USE_LEGACY
#include <proto/log.h>
#endif

/* private */
#include "ambient_cat.h"
#include "copyright.h"
#include "mui_func.h"
#include "prefs_desktop.h"
#include "prefs_startup.h"
#include "threads.h"
#include "prefswin.h"
#include "fonts.h"
#include "prefs.h"
#include "screen.h"
#include "methodstack.h"
#include "rexx.h"
#include "smartreq.h"
#include "name.h"
#include "wbstart.h"
#include "iconview.h"
#include "memtrack.h"
#include "cx.h"
#include "common_picture.h"
#include "datatypes_picture.h"
#include "reggae_picture.h"
#include "panelprefs.h"
#include "sizes.h"
#include "random.h"
#include "mimeuri.h"
#include "background.h"
#include "vars.h"
#include "dosreq.h"
#include "file_func.h"
#include "playsound.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "datatypes.h"
#include "multimedia.h"
#include "command.h"
#include "mimetype.h"
#include "appclass.h"
#include "clipboard.h"
#include "dragdrop.h"
#include "viewapi.h"
#include "appicon.h"
#include "wbarg.h"
#include "ipc.h"
#include "storage.h"
#include "deficon_getpath.h"
#include "deficon.h"
#include "deficonpool.h"
#include "time_func.h"
#include "eject.h"
#include "device_func.h"
#include "prefs_advanced.h"
#include "legacy.h"
#include "cache.h"
#include "keyshortcuts.h"
#include "clipboarddevice.h"
#include "doslistcache.h"
#include "bookmarks.h"
#include "panelitem.h"
#include "deficonpool.h"
#include "soundwin.h"
#include "contextmenu.h"
#include "menus.h"
#include "paneltags.h"
#include "preferences_update.h"
#include "trashcan.h"
#include "networksfs.h"

void dprintf(char *, ...) __attribute__ ((format (printf, 1, 2)));

#if USE_OS4
struct Library *OS4LoaderBase;
#endif


STRPTR screentitle;
#define SCREENTITLESIZE 2048

extern CONST_STRPTR __screentitle_additional; /* created on-the-fly for each build (revstring.o) */


struct dialog_delay
{
	struct MinNode node;
	APTR           win;
	ULONG          launchtime;
};

struct Data {
	ULONG  id;

	/*  backgrounds
	 */
	APTR   dtp_root_old;
	APTR   dtp_root_new;
	APTR   dtp_root;
	APTR   dtp_window;

	APTR   rootwin;  /* root window object (mostly for speed reasons but also to check if rootwin is around) */
	STRPTR screentitle_backup;

	/*  intuition sucks so much that we need 2 buffers to swap
	 */
	STRPTR scrbuf[2];
	ULONG  scrtick;
	ULONG  pushid;

	ULONG  wasiconified;

	/*  images cache
	 */
	APTR   image_cache;

	APTR   panelclasslist;
	
	/*  views counter
	 */

	/* */
	ULONG  cleanup_pending;

	/* background randomizer input handler
	 */
	ULONG bgrandom_timer_count;
	ULONG bgrandom_timer_wait;

	ULONG bgrandom_inputhandler_added;
	struct MUI_InputHandlerNode bgrandom_ihnode;

	/* cache cleanup counter
	 */
	ULONG cachecleanup_timer_count;
	ULONG cachecleanup_timer_wait;

	ULONG cachecleanup_inputhandler_added;
	struct MUI_InputHandlerNode cachecleanup_ihnode;

	/* Delayed dialogs */
	struct MinList delaylist;
	struct MUI_InputHandlerNode delay_ihnode;

	struct MUI_InputHandlerNode transtimer;
	UBYTE bg_transition_active;
	UBYTE mixlevel;
};

ULONG xget(Object *object, ULONG attr)
{
	ULONG val = 0;
	if (get(object, attr, &val))
		return val;
	return 0;
}

/*
 * Used MUI classes (well, we don't use any yet.. maybe Textinput makes sense).
 */
static const CONST_STRPTR classlist[] = {
#if 0
	"Textinput.mcc",
#endif
	"Listtree.mcc",
	NULL
};


DEFNEW
{
	struct Data *data;

	obj = DoSuperNew(cl, obj,
		MUIA_Application_Title,       APPNAME,
		MUIA_Application_Version,     "$VER: "APPNAME" "LVERTAG,
		MUIA_Application_Copyright,   "? 2001-2005 by David Gerber, ? 2005-" COPYRIGHTYEAR_END " by Ambient Open Source Team, All Rights Reserved.",
		MUIA_Application_Author,      "David Gerber, Ambient Open Source Team",
		MUIA_Application_UsedClasses, classlist,
		MUIA_Application_Description, "The MorphOS desktop",
		MUIA_Application_Base,        "AMBIENT",
		#if USE_MULTIPLE_DESKTOP
		MUIA_Application_SingleTask,  FALSE,
		#else
		MUIA_Application_SingleTask,  TRUE,
		#endif
		MUIA_Application_UseRexx,     FALSE,
		MUIA_Application_NoIconify,   TRUE,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (0);
	}

	data = INST_DATA(cl, obj);

	data->id = 1;
	data->cleanup_pending = FALSE;

	if (!(data->scrbuf[0] = malloc(SCREENTITLESIZE)) || !(data->scrbuf[1] = malloc(SCREENTITLESIZE))) /* should be enough for everyone (tm) */
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (0);
	}

	screentitle = data->scrbuf[0];
	NewRawDoFmt("Ambient Screen%s", NULL, screentitle, __screentitle_additional);

	if (!(data->screentitle_backup = malloc(SCREENTITLESIZE)))
	{
		CoerceMethod(cl, obj, OM_DISPOSE);
		return (0);
	}

	data->screentitle_backup[0] = '\0';

	data->bgrandom_timer_count = 0;
	data->bgrandom_timer_wait = 0;

	data->bgrandom_inputhandler_added = FALSE;
	set(obj, MA_Application_BackgroundRefreshDelay, getprefslong(DSI_BACKGROUND_ROOT_REFRESH_DELAY));

#if MORPHOS_BETABUILD
	/* cache cleanup handler (invoke once a minute to trigger any lingering bugs) */

	data->cachecleanup_timer_count = 1;
	data->cachecleanup_timer_wait = 1;
#else
	/* cache cleanup handler (invoke cleanup routine every 30 minutes) */

	data->cachecleanup_timer_count = 30;
	data->cachecleanup_timer_wait = 30;
#endif

	data->cachecleanup_ihnode.ihn_Object = obj;
	data->cachecleanup_ihnode.ihn_Flags = MUIIHNF_TIMER;
	data->cachecleanup_ihnode.ihn_Millis = 60 * 1000;
	data->cachecleanup_ihnode.ihn_Method = MM_Application_CleanupCache;
	DoMethod(obj, MUIM_Application_AddInputHandler, &data->cachecleanup_ihnode);
	data->cachecleanup_inputhandler_added = TRUE;

	NEWLIST(&data->delaylist);
	data->delay_ihnode.ihn_Object = obj;
	data->delay_ihnode.ihn_Flags  = MUIIHNF_TIMER;
	data->delay_ihnode.ihn_Millis = 1000;
	data->delay_ihnode.ihn_Method = MM_Application_CheckDelayedDialog;

	data->transtimer.ihn_Object = obj;
	data->transtimer.ihn_Millis = 50;
	data->transtimer.ihn_Flags = MUIIHNF_TIMER;
	data->transtimer.ihn_Method = MM_Application_UpdateTransitionEffect;

	set(obj, MUIA_Application_Iconified, FALSE);
	data->panelclasslist =  NewObject(getpanelclasslistclass(),NULL,TAG_DONE);
	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;

	if (_conf(misc_remember_windows) || _conf(misc_remember_documents))
	{
		sprefs_save();
	}

	sprefs_cleanup();

	if(data->bgrandom_inputhandler_added)
	{
		DoMethod(obj, MUIM_Application_RemInputHandler, &data->bgrandom_ihnode);
	}

	if(data->cachecleanup_inputhandler_added)
	{
		DoMethod(obj, MUIM_Application_RemInputHandler, &data->cachecleanup_ihnode);
	}

	/*
	 * Close all windows to make sure none of them
	 * uses the things we're going to free now.
	 */
	FORCHILD(obj, MUIA_Application_WindowList)
	{
		set(child, MUIA_Window_Open, FALSE);
	}
	NEXTCHILD

	if (_conf(root_font))
	{
		font_close(_conf(root_font));
		_conf(root_font) = NULL;
	}

	if (_conf(root_font_small))
	{
		font_close(_conf(root_font_small));
		_conf(root_font_small) = NULL;
	}

	if (_conf(window_font))
	{
		font_close(_conf(window_font));
		_conf(window_font) = NULL;
	}

	if (_conf(fl_lister_font))
	{
		font_close(_conf(fl_lister_font));
		_conf(fl_lister_font) = NULL;
	}

	picture_delete(data->dtp_root_old);
	picture_delete(data->dtp_root_new);
	picture_delete(data->dtp_root);
	picture_delete(data->dtp_window);

	if (data->scrbuf[0])
	{
		free(data->scrbuf[0]);
	}
	if (data->scrbuf[1])
	{
		free(data->scrbuf[1]);
	}

	if (data->screentitle_backup)
	{
		free(data->screentitle_backup);
	}
	if(data->panelclasslist)
	{
		MUI_DisposeObject(data->panelclasslist);
	}
	return (DOSUPER);
}

/*
 * Add a dialog that is opened with delay.
 */
static void AddDelayedDialog(struct IClass *cl, APTR obj, APTR win, ULONG delay)
{
	GETDATA;
	struct dialog_delay *dd = AllocTaskPooled(sizeof(*dd));

	if (dd)
	{
		LONG was_empty = ISLISTEMPTY(&data->delaylist);

		dd->win = win;
		dd->launchtime = MAX(delay, 50) + timedm();

		ADDTAIL(&data->delaylist, dd);

		if (was_empty)
		{
			DoSuperMethod(cl, obj, MUIM_Application_AddInputHandler, &data->delay_ihnode);
		}
	}
}


static void RemoveDelayedDialog(struct IClass *cl, APTR obj, APTR win)
{
	GETDATA;
	struct dialog_delay *dd, *next;
	LONG was_empty = ISLISTEMPTY(&data->delaylist);

	ITERATELISTSAFE(dd, next, &data->delaylist)
	{
		if (dd->win == win)
		{
			REMOVE(dd);
			FreeTaskPooled(dd, sizeof(*dd));
			break;
		}
	}

	if (!was_empty && ISLISTEMPTY(&data->delaylist))
		DoSuperMethod(cl, obj, MUIM_Application_RemInputHandler, &data->delay_ihnode);
}


/*
 * Launch dialogs. Use time_now = (ULONG)-1 to fire all pending dialogs immediately.
 */
static void CheckDelayedDialogs(struct IClass *cl, APTR obj, ULONG time_now)
{
	GETDATA;
	struct dialog_delay *dd, *next;
	LONG was_empty = ISLISTEMPTY(&data->delaylist);

	ITERATELISTSAFE(dd, next, &data->delaylist)
	{
		if (dd->launchtime <= time_now)
		{
			REMOVE(dd);
			set(dd->win, MUIA_Window_Open, TRUE);
			FreeTaskPooled(dd, sizeof(*dd));
		}
	}

	if (!was_empty && ISLISTEMPTY(&data->delaylist))
		DoSuperMethod(cl, obj, MUIM_Application_RemInputHandler, &data->delay_ihnode);
}

DEFTMETHOD(Application_DisplayDelayedDialog)
{
	CheckDelayedDialogs(cl, obj, (ULONG)-1);
	return 0;
}

DEFTMETHOD(Application_CheckDelayedDialog)
{
	CheckDelayedDialogs(cl, obj, timedm());
	return 0;
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Application_NextID:
			*msg->opg_Storage = data->id++;
			return (TRUE);
		case MA_Application_PanelClassList:
			*msg->opg_Storage = (ULONG)data->panelclasslist;
			return (TRUE);
	}

	return (DOSUPER);
}

DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MUIA_Application_Iconified:
			if (tag->ti_Data)
			{
				/*
				 * Clear the backgrounds.
				 */
				DoMethod(app, MM_Application_LoadBackground, MF_Application_LoadBackground_ClearRoot | MF_Application_LoadBackground_ClearWindow);

				data->wasiconified = TRUE;
			}
			else
			{
				if (data->wasiconified)
				{
					ULONG rc;

					rc = DOSUPER;

					/*
					 * Reload the background
					 */
					DoMethod(app, MM_Application_LoadBackground, MF_Application_LoadBackground_Root | MF_Application_LoadBackground_Window);

					data->wasiconified = FALSE;

					return (rc);
				}
			}
			break;

		case MA_Application_BackgroundRefreshDelay:
		{
			if(data->bgrandom_inputhandler_added)
			{
				DoMethod(obj, MUIM_Application_RemInputHandler, &data->bgrandom_ihnode);
				data->bgrandom_inputhandler_added = FALSE;
			}

			if(tag->ti_Data)
			{
				data->bgrandom_timer_wait = data->bgrandom_timer_count = (ULONG)tag->ti_Data;

				data->bgrandom_ihnode.ihn_Object = obj;
				data->bgrandom_ihnode.ihn_Flags = MUIIHNF_TIMER;
				data->bgrandom_ihnode.ihn_Millis = 60 * 1000;
				data->bgrandom_ihnode.ihn_Method = MM_Application_ReloadBackgrounds;

				DoMethod(obj, MUIM_Application_AddInputHandler, &data->bgrandom_ihnode);

				data->bgrandom_inputhandler_added = TRUE;

			}
		}
	}
	NEXTTAG

	return (DOSUPER);
}



//#define CHECKID(x) (msg->id == MV_Application_LoadPrefs_All || msg->id == (x))

/* ok it sucks, but actually, all _conf() members need to be updated IMO, as there can be
 * some reallocations in prefs for some id, not appearing in update list, which means old _conf()
 * values point to an invalid address if they aren't updated. At least, that's what i think... (fab)
 */
#define CHECKID_OLD(x) ( msg->id == MV_Application_LoadPrefs_All || msg->id == (x) )
#define CHECKID(x) (1)

static ULONG get_icon_size(ULONG size)
{
	switch (size)
	{
		case ICON_SIZE_MICRO:
			return (SIZE_MICRO);

		case ICON_SIZE_SMALL:
			return (SIZE_SMALL);

		case ICON_SIZE_MEDIUM:
			return (SIZE_MEDIUM);

		case ICON_SIZE_LARGE:
			return (SIZE_LARGE);

		case ICON_SIZE_HUGE:
			return (SIZE_HUGE);

		#ifdef DEBUG
		default:
			PDB(("eek, out of bound: %ld\n", size));
			break;
		#endif
	}
	return (0); /* shut up gcc */
}


DEFSMETHOD(Application_LoadPrefs)
{
	ASSERT(msg->id);

	if (!msg->quiet && CHECKID_OLD(DSI_FONT_ROOT))
	{
		do_action(obj, TA_Font_Load,
			TT_Font_Load_Path, getprefsstr(DSI_FONT_ROOT),
			TT_Font_Load_Type, TV_Font_Load_Type_Root,
		TAG_DONE);
	}


	if (!msg->quiet && CHECKID_OLD(DSI_FONT_ROOT_SMALL))
	{
		do_action(obj, TA_Font_Load,
			TT_Font_Load_Path, getprefsstr(DSI_FONT_ROOT_SMALL),
			TT_Font_Load_Type, TV_Font_Load_Type_Root_Small,
		TAG_DONE);
	}


	if (!msg->quiet && CHECKID_OLD(DSI_FONT_WINDOW))
	{
		do_action(obj, TA_Font_Load,
			TT_Font_Load_Path, getprefsstr(DSI_FONT_WINDOW),
			TT_Font_Load_Type, TV_Font_Load_Type_Window,
		TAG_DONE);
	}

	if (!msg->quiet && CHECKID_OLD(DSI_FASTLIST_FONT))
	{
		do_action(obj, TA_Font_Load,
			TT_Font_Load_Path, getprefsstr(DSI_FASTLIST_FONT),
			TT_Font_Load_Type, TV_Font_Load_Type_Lister,
		TAG_DONE);
	}

	if (CHECKID(DSI_FONT_ROOT_EFFECT))
	{
		_conf(root_font_effect) = getprefslong(DSI_FONT_ROOT_EFFECT);
	}

	if (CHECKID(DSI_FONT_WINDOW_EFFECT))
	{
		_conf(window_font_effect) = getprefslong(DSI_FONT_WINDOW_EFFECT);
	}

	if (CHECKID(DSI_BACKGROUND_WINDOW_BGRENDER))
	{
		_conf(window_bgrender) = getprefslong(DSI_BACKGROUND_WINDOW_BGRENDER);
	}

	if (CHECKID(DSI_BACKGROUND_WINDOW_BGCOLOR))
	{
		memcpy(&_conf(window_bgcolor), getprefs(DSI_BACKGROUND_WINDOW_BGCOLOR), sizeof(_conf(window_bgcolor)));
	}

	if (CHECKID(DSI_COLOR_ROOT))
	{
		memcpy(&_conf(root_pen1), getprefs(DSI_COLOR_ROOT), sizeof(_conf(root_pen1)));
	}

	if (CHECKID(DSI_COLOR_WINDOW))
	{
		memcpy(&_conf(window_pen1), getprefs(DSI_COLOR_WINDOW), sizeof(_conf(window_pen1)));
	}

	if (CHECKID(DSI_FONT_ROOTSPACE))
	{
		_conf(root_spaceline) = getprefslong(DSI_FONT_ROOTSPACE);
	}

	if (CHECKID(DSI_FONT_WINDOWSPACE))
	{
		_conf(window_spaceline) = getprefslong(DSI_FONT_WINDOWSPACE);
	}

	if (CHECKID(DSI_COLOR_ROOT2))
	{
		memcpy(&_conf(root_pen2), getprefs(DSI_COLOR_ROOT2), sizeof(_conf(root_pen2)));
	}

	if (CHECKID(DSI_COLOR_WINDOW2))
	{
		memcpy(&_conf(window_pen2), getprefs(DSI_COLOR_WINDOW2), sizeof(_conf(window_pen2)));
	}

	if (CHECKID(DSI_COLOR_LASSO))
	{
		memcpy(&_conf(lasso_pen), getprefs(DSI_COLOR_LASSO), sizeof(_conf(lasso_pen)));
	}

	if (CHECKID(DSI_ICON_MINSIZE))
	{
		if ( getprefslong(DSI_ICON_MINSIZE) > ICON_SIZE_HUGE )
			setprefslong(DSI_ICON_MINSIZE, ICON_SIZE_HUGE );

		_conf(icon_minsize) = get_icon_size(getprefslong(DSI_ICON_MINSIZE));
	}

	if (CHECKID(DSI_ICON_MAXSIZE))
	{
		if ( getprefslong(DSI_ICON_MAXSIZE) > ICON_SIZE_HUGE )
			setprefslong(DSI_ICON_MAXSIZE, ICON_SIZE_HUGE );

		_conf(icon_maxsize) = get_icon_size(getprefslong(DSI_ICON_MAXSIZE));
	}

	/*
		Hovering enable/disable config item.
	*/
	if (CHECKID(DSI_ICON_HOVER))
	{
		_conf(icon_hover) = getprefslong(DSI_ICON_HOVER);
	}
	
	if (CHECKID(DSI_ICON_SEPARATEFILES))
	{
		_conf(icon_separatefiles) = getprefslong(DSI_ICON_SEPARATEFILES);
	}

	if (CHECKID(DSI_ICON_DEFGHOSTED))
	{
		_conf(icon_defghosted) = getprefslong(DSI_ICON_DEFGHOSTED);
	}

	if (CHECKID(DSI_ICON_FADE))
	{
		_conf(icon_fade) = getprefslong(DSI_ICON_FADE);
	}

	if (CHECKID(DSI_ICON_SHORTCUTSIDENTIFIER))
	{
		_conf(icon_shortcutsidentifier) = getprefslong(DSI_ICON_SHORTCUTSIDENTIFIER);
	}

	if (CHECKID(DSI_ICON_APPICONSIDENTIFIER))
	{
		_conf(icon_appiconsidentifier) = getprefslong(DSI_ICON_APPICONSIDENTIFIER);
	}

	if (CHECKID(DSI_ICON_AUTOSNAPSHOT))
	{
		_conf(icon_autosnapshot) = getprefslong(DSI_ICON_AUTOSNAPSHOT);
	}

	if (CHECKID(DSI_ICON_DUALPNG))
	{
		_conf(icon_dualpng) = getprefslong(DSI_ICON_DUALPNG);
	}

	/*
	 * Miscellaneous.
	 */

	if (CHECKID(DSI_MISC_REMEMBER_WINDOWS))
	{
		_conf(misc_remember_windows) = getprefslong(DSI_MISC_REMEMBER_WINDOWS);
	}

	if (CHECKID(DSI_MISC_REMEMBER_DOCUMENTS))
	{
		_conf(misc_remember_documents) = getprefslong(DSI_MISC_REMEMBER_DOCUMENTS);
	}

	if (CHECKID(DSI_MISC_MYMORPHOSICON))
	{
		_conf(misc_mymorphosicon) = getprefslong(DSI_MISC_MYMORPHOSICON);
	}

	if (CHECKID(DSI_MISC_TRAPMORE))
	{
		_conf(misc_trapmore) = getprefslong(DSI_MISC_TRAPMORE);
	}

	if (CHECKID(DSI_MISC_TRAPMULTIVIEW))
	{
		_conf(misc_trapmultiview) = getprefslong(DSI_MISC_TRAPMULTIVIEW);
	}

	if (CHECKID(DSI_MISC_CONTEXTMENUIMAGES))
	{
		_conf(misc_contextmenuimages) = getprefslong(DSI_MISC_CONTEXTMENUIMAGES);
		contextmenu_setuplabels();
	}

	if (CHECKID(DSI_MISC_CREATEICONFORNEWDRAWER))
	{
		_conf(misc_createiconfornewdrawer) = getprefslong(DSI_MISC_CREATEICONFORNEWDRAWER);
	}

	if (CHECKID(DSI_MISC_DESKTOPDOUBLECLICK))
	{
		_conf(misc_desktopdoubleclick) = getprefslong(DSI_MISC_DESKTOPDOUBLECLICK);
	}

	if (CHECKID(DSI_MISC_DRIVE_INFO))
	{
		_conf(misc_driveinfo) = getprefslong(DSI_MISC_DRIVE_INFO);
	}

	if (CHECKID(DSI_MISC_SMARTFILEOPERATIONS))
	{
		_conf(misc_smartfileoperations) = getprefslong(DSI_MISC_SMARTFILEOPERATIONS);
	}

	/*
	 * CLI
	 */

	if (CHECKID(DSI_CLI_DEVICE))
	{
		_conf(cli_device) = getprefsstr(DSI_CLI_DEVICE);
	}

	if (CHECKID(DSI_CLI_STACK))
	{
		_conf(cli_stack) = getprefslong(DSI_CLI_STACK);
	}

	#if USE_DROP_EFFECT_PREFS
	if (CHECKID(DSI_DRAGDROP_DROPEFFECT))
	{
		_conf(dragdrop_dropeffect) = getprefslong(DSI_DRAGDROP_DROPEFFECT);
	}
	#endif

	if (CHECKID(DSI_DRAGDROP_TINTVAL))
	{
		memcpy(&_conf(dragdrop_tintval), getprefs(DSI_DRAGDROP_TINTVAL), sizeof(_conf(dragdrop_tintval)));
	}

	#if USE_DROP_EFFECT_PREFS
	if (CHECKID(DSI_DRAGDROP_TINTFADEVAL))
	{
		memcpy(&_conf(dragdrop_tintfadeval), getprefs(DSI_DRAGDROP_TINTFADEVAL), sizeof(_conf(dragdrop_tintfadeval)));
	}

	if (CHECKID(DSI_DRAGDROP_BRIGHTENVAL))
	{
		_conf(dragdrop_brightenval) = getprefslong(DSI_DRAGDROP_BRIGHTENVAL);
	}

	if (CHECKID(DSI_DRAGDROP_DARKENVAL))
	{
		_conf(dragdrop_darkenval) = getprefslong(DSI_DRAGDROP_DARKENVAL);
	}
	#endif

	/* Icon options */
	#if USE_DROP_EFFECT_PREFS
	if (CHECKID(DSI_ICON_SELECTEFFECT))
	{
		_conf(icon_selecteffect) = getprefslong(DSI_ICON_SELECTEFFECT);
	}
	#endif

	if (CHECKID(DSI_ICON_TINTVAL))
	{
		memcpy(&_conf(icon_tintval), getprefs(DSI_ICON_TINTVAL), sizeof(_conf(icon_tintval)));
	}

	#if USE_DROP_EFFECT_PREFS
	if (CHECKID(DSI_ICON_TINTFADEVAL))
	{
		memcpy(&_conf(icon_tintfadeval), getprefs(DSI_ICON_TINTFADEVAL), sizeof(_conf(icon_tintfadeval)));
	}

	if (CHECKID(DSI_ICON_BRIGHTENVAL))
	{
		_conf(icon_brightenval) = getprefslong(DSI_ICON_BRIGHTENVAL);
	}

	if (CHECKID(DSI_ICON_DARKENVAL))
	{
		_conf(icon_darkenval) = getprefslong(DSI_ICON_DARKENVAL);
	}
	#endif

	/* Fastlist */
	if (CHECKID(DSI_FASTLIST_COLOR_FILE_FG))
	{
		memcpy(&_conf(fl_color_file_fg), getprefs(DSI_FASTLIST_COLOR_FILE_FG), sizeof(_conf(fl_color_file_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_FILE_BG))
	{
		memcpy(&_conf(fl_color_file_bg), getprefs(DSI_FASTLIST_COLOR_FILE_BG), sizeof(_conf(fl_color_file_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_FILE_SEL_FG))
	{
		memcpy(&_conf(fl_color_file_sel_fg), getprefs(DSI_FASTLIST_COLOR_FILE_SEL_FG), sizeof(_conf(fl_color_file_sel_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_FILE_SEL_BG))
	{
		memcpy(&_conf(fl_color_file_sel_bg), getprefs(DSI_FASTLIST_COLOR_FILE_SEL_BG), sizeof(_conf(fl_color_file_sel_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_DIRECTORY_FG))
	{
		memcpy(&_conf(fl_color_directory_fg), getprefs(DSI_FASTLIST_COLOR_DIRECTORY_FG), sizeof(_conf(fl_color_directory_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_DIRECTORY_BG))
	{
		memcpy(&_conf(fl_color_directory_bg), getprefs(DSI_FASTLIST_COLOR_DIRECTORY_BG), sizeof(_conf(fl_color_directory_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_DIRECTORY_SEL_FG))
	{
		memcpy(&_conf(fl_color_directory_sel_fg), getprefs(DSI_FASTLIST_COLOR_DIRECTORY_SEL_FG), sizeof(_conf(fl_color_directory_sel_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_DIRECTORY_SEL_BG))
	{
		memcpy(&_conf(fl_color_directory_sel_bg), getprefs(DSI_FASTLIST_COLOR_DIRECTORY_SEL_BG), sizeof(_conf(fl_color_directory_sel_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_SOFTLINK_FG))
	{
		memcpy(&_conf(fl_color_softlink_fg), getprefs(DSI_FASTLIST_COLOR_SOFTLINK_FG), sizeof(_conf(fl_color_softlink_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_SOFTLINK_BG))
	{
		memcpy(&_conf(fl_color_softlink_bg), getprefs(DSI_FASTLIST_COLOR_SOFTLINK_BG), sizeof(_conf(fl_color_softlink_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_HARDLINK_FG))
	{
		memcpy(&_conf(fl_color_hardlink_fg), getprefs(DSI_FASTLIST_COLOR_HARDLINK_FG), sizeof(_conf(fl_color_hardlink_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_HARDLINK_BG))
	{
		memcpy(&_conf(fl_color_hardlink_bg), getprefs(DSI_FASTLIST_COLOR_HARDLINK_BG), sizeof(_conf(fl_color_hardlink_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_VOLUME_FG))
	{
		memcpy(&_conf(fl_color_volume_fg), getprefs(DSI_FASTLIST_COLOR_VOLUME_FG), sizeof(_conf(fl_color_volume_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_VOLUME_BG))
	{
		memcpy(&_conf(fl_color_volume_bg), getprefs(DSI_FASTLIST_COLOR_VOLUME_BG), sizeof(_conf(fl_color_volume_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_ASSIGN_FG))
	{
		memcpy(&_conf(fl_color_assign_fg), getprefs(DSI_FASTLIST_COLOR_ASSIGN_FG), sizeof(_conf(fl_color_assign_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_ASSIGN_BG))
	{
		memcpy(&_conf(fl_color_assign_bg), getprefs(DSI_FASTLIST_COLOR_ASSIGN_BG), sizeof(_conf(fl_color_assign_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_SOURCE_FG))
	{
		memcpy(&_conf(fl_color_source_fg), getprefs(DSI_FASTLIST_COLOR_SOURCE_FG), sizeof(_conf(fl_color_source_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_SOURCE_BG))
	{
		memcpy(&_conf(fl_color_source_bg), getprefs(DSI_FASTLIST_COLOR_SOURCE_BG), sizeof(_conf(fl_color_source_bg)));
	}

	if (CHECKID(DSI_FASTLIST_COLOR_DESTINATION_FG))
	{
		memcpy(&_conf(fl_color_destination_fg), getprefs(DSI_FASTLIST_COLOR_DESTINATION_FG), sizeof(_conf(fl_color_destination_fg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_DESTINATION_BG))
	{
		memcpy(&_conf(fl_color_destination_bg), getprefs(DSI_FASTLIST_COLOR_DESTINATION_BG), sizeof(_conf(fl_color_destination_bg)));
	}
	if (CHECKID(DSI_FASTLIST_COLOR_COLUMN_FG))
	{
		memcpy(&_conf(fl_color_column_fg), getprefs(DSI_FASTLIST_COLOR_COLUMN_FG), sizeof(_conf(fl_color_column_fg)));
	}

	if (CHECKID(DSI_FASTLIST_WINDOW_BG))
	{
		_conf(fl_window_bg) = getprefslong(DSI_FASTLIST_WINDOW_BG);
	}

	if (CHECKID(DSI_FASTLIST_COMPACT_SIZE_DISPLAY))
	{
		_conf(fl_compact_size_display) = getprefslong(DSI_FASTLIST_COMPACT_SIZE_DISPLAY);
	}

	if (CHECKID(DSI_FASTLIST_BOLD_DIRECTORIES))
	{
		_conf(fl_bold_directories) = getprefslong(DSI_FASTLIST_BOLD_DIRECTORIES);
	}

	if (CHECKID(DSI_FASTLIST_ASSIGN_DISPLAY))
	{
		_conf(fl_assign_display) = getprefslong(DSI_FASTLIST_ASSIGN_DISPLAY);
	}

	if (CHECKID(DSI_FASTLIST_SELECTION_MODE))
	{
		_conf(fl_selection_mode) = getprefslong(DSI_FASTLIST_SELECTION_MODE);
	}

	if (CHECKID(DSI_FASTLIST_DEFAULT_FORMAT_FILES))
	{
		_conf(fl_default_format_files) = getprefsstr(DSI_FASTLIST_DEFAULT_FORMAT_FILES);
	}

	if (CHECKID(DSI_FASTLIST_DEFAULT_FORMAT_DEVICES))
	{
		_conf(fl_default_format_devices) = getprefsstr(DSI_FASTLIST_DEFAULT_FORMAT_DEVICES);
	}

	if (CHECKID(DSI_FASTLIST_DEFAULT_MODE_FILES))
	{
		_conf(fl_default_mode_files) = getprefsstr(DSI_FASTLIST_DEFAULT_MODE_FILES);
	}

	if (CHECKID(DSI_FASTLIST_DEFAULT_MODE_DEVICES))
	{
		_conf(fl_default_mode_devices) = getprefsstr(DSI_FASTLIST_DEFAULT_MODE_DEVICES);
	}

	if (CHECKID(DSI_PANEL_ZIPSPEED))
	{
		_conf(panel_zipspeed) = getprefslong(DSI_PANEL_ZIPSPEED);
	}


	if (CHECKID(DSI_TOOLBAR_BROWSERMODE))
	{
		_conf(toolbar_browsermode) = getprefslong(DSI_TOOLBAR_BROWSERMODE);
	}

	if (CHECKID(DSI_TOOLBAR_DISPLAYMODE))
	{
		_conf(toolbar_displaymode) = getprefslong(DSI_TOOLBAR_DISPLAYMODE);
	}

	if (CHECKID(DSI_TOOLBAR_DEFINITION))
	{
		_conf(toolbar_definition) = getprefsstr(DSI_TOOLBAR_DEFINITION);
	}

	if (CHECKID(DSI_TOOLBAR_BUTTONFRAMESPEC))
	{
		_conf(toolbar_framespec) = getprefsstr(DSI_TOOLBAR_BUTTONFRAMESPEC);
	}

	if (CHECKID(DSI_TOOLBAR_BUTTONIMAGESPEC))
	{
		_conf(toolbar_imagespec) = getprefsstr(DSI_TOOLBAR_BUTTONIMAGESPEC);
	}

	if (CHECKID(DSI_WINDOW_DEFAULT_WIDTH))
	{
		_conf(window_default_width) = getprefslong(DSI_WINDOW_DEFAULT_WIDTH);
	}

	if (CHECKID(DSI_WINDOW_DEFAULT_HEIGHT))
	{
		_conf(window_default_height) = getprefslong(DSI_WINDOW_DEFAULT_HEIGHT);
	}

	if (CHECKID(DSI_WINDOW_DEFAULT_TOP))
	{
		_conf(window_default_top) = getprefslong(DSI_WINDOW_DEFAULT_TOP);
	}

	if (CHECKID(DSI_WINDOW_DEFAULT_LEFT))
	{
		_conf(window_default_left) = getprefslong(DSI_WINDOW_DEFAULT_LEFT);
	}

	if (CHECKID(DSI_WINDOW_DEFAULT_VIEW))
	{
		_conf(window_default_view) = getprefslong(DSI_WINDOW_DEFAULT_VIEW);
	}

	if (CHECKID(DSI_WINDOW_DEFAULT_VIEWMODE))
	{
		_conf(window_default_viewmode) = getprefslong(DSI_WINDOW_DEFAULT_VIEWMODE);
	}

	if (CHECKID(DSI_WINDOW_INHERIT_VIEWMODE))
	{
		_conf(window_inherit_viewmode) = getprefslong(DSI_WINDOW_INHERIT_VIEWMODE);
	}

	if (CHECKID(DSI_WINDOW_STATUSBARFORMAT))
	{
		_conf(window_statusbarformat) = getprefsstr(DSI_WINDOW_STATUSBARFORMAT);
	}

	if(CHECKID(DSI_LISTPOOL_KEYSHORTCUT_CHANGED))
	{
		keyshortcuts_reload();
	}

	if(CHECKID(DSI_BACKGROUND_ROOT_REFRESH_DELAY))
	{
		set(obj, MA_Application_BackgroundRefreshDelay, getprefslong(DSI_BACKGROUND_ROOT_REFRESH_DELAY));
	}

	if (CHECKID(DSI_BOOKMARKS_NAMESONLY))
	{
		_conf(bookmarks_showonly) = getprefslong(DSI_BOOKMARKS_SHOWONLY);
	}
	
	if (CHECKID(DSI_WINDOW_STATUSBARCOLOR)) // bitRocky
	{
		memcpy(&_conf(window_statusbarcolor), getprefs(DSI_WINDOW_STATUSBARCOLOR), sizeof(_conf(window_statusbarcolor)));		
	}

	return (0);
}


DEFSMETHOD(Application_SetFont)
{
	APTR old_font = NULL;

	/*
		Postponed closing of font to after DoMethod().
		Looks like some values are not propagated fast enough.
		This should be fixed properly later.
	*/

	switch (msg->type)
	{
		case TV_Font_Load_Type_Root:
			old_font = _conf(root_font);

			_conf(root_font) = msg->at;
			DoMethod(app, MM_Application_DisplayUpdate, MF_Application_DisplayUpdate_Fonts | MF_Application_DisplayUpdate_Root);

			break;

		case TV_Font_Load_Type_Root_Small:
			old_font = _conf(root_font_small);

			_conf(root_font_small) = msg->at;
			DoMethod(app, MM_Application_DisplayUpdate, MF_Application_DisplayUpdate_Fonts | MF_Application_DisplayUpdate_Root);

			break;

		case TV_Font_Load_Type_Window:
			old_font = _conf(window_font);

			_conf(window_font) = msg->at;
			DoMethod(app, MM_Application_DisplayUpdate, MF_Application_DisplayUpdate_Fonts | MF_Application_DisplayUpdate_Windows);

			break;

		case TV_Font_Load_Type_Lister:
			old_font = _conf(fl_lister_font);

			_conf(fl_lister_font) = msg->at;
			DoMethod(app, MM_Application_DisplayUpdate, MF_Application_DisplayUpdate_Fonts | MF_Application_DisplayUpdate_Windows);

			break;

		#ifdef DEBUG
		default:
			PDB(("unknown font type %ld\n", msg->type));
			break;
		#endif
	}

	if ( old_font )
	{
		font_close( old_font );
	}

	return (0);

}


DEFTMETHOD(Application_Cleanup)
{
	GETDATA;

	data->cleanup_pending = TRUE;

	return (threads_finish(FALSE));
}


#if USE_DOSNOTIFY
DEFSMETHOD(Application_DOSNotify)
{
	APTR wo;

	ASSERT(msg->id != 0 || msg->name != NULL);

	if (msg->id != 0 && (wo = (APTR)DoMethod(obj, MM_Application_FindWindowByID, msg->id)))
	{
		if (getv(wo, MA_Window_IsIconified) == FALSE)
			DoMethod(app, MUIM_Application_PushMethod, wo, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE, MM_Window_Reload);
	}

	/*
	 * Handle special files here.
	 * NOTE: strstr to not need to handle paths. only filename.
	 */

	if (msg->name != NULL)
	{
		if (strstr(msg->name, RECOGNITION_FILE) || strstr(msg->name, FILETYPE_DIR))
		{
			DB(("Reload recognition database\n"));
			mimetype_invalidate(NULL, NULL);
			mimetype_load_database(NULL);
		}

		if (strstr( msg->name, ADVANCED_FILE))
		{
			DB(("Reload Advanced.conf\n"));
			prefs_advanced_refresh(FALSE);
		}

		if (strstr( msg->name, DEFICONPATH_FILE))
		{
			DB(("Deficons path changed\n"));
			deficon_updatepath();
		}

		if (strstr( msg->name, TRASHCAN_INFO))
		{
			trashcan_updatediskinfo();
		}
	}

	return (0);
}
#endif


/*
 * Remove idle threads.
 */
DEFTMETHOD(Application_ReclaimThreads)
{
	threads_reclaim();

	return (0);
}


/*
 * Given an attribute and a value,
 * apply the method on the selected
 * windows.
 */
DEFSMETHOD(Application_WindowDoMethodByAttr)
{
	ULONG v;
	ASSERT(msg->attr);

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if (get(child, msg->attr, &v))
		{
			if (v == msg->val)
			{
				if (getv(child, MA_Window_Type) != MV_Window_Type_Rootview)
				{
					DoMethodA(child, (Msg)&msg->args);
				}
			}
		}
	}
	NEXTCHILD

	return (0);
}


/*
 * Ditto for rootwin. If attr is NULL it means it'll do the
 * method in any case.
 */
DEFSMETHOD(Application_RootDoMethodByAttr)
{
	GETDATA;
	ULONG v;

	/*
	 * data->rootwin is not immediately available.
	 * If it's there we can use it as a faster way
	 * to access rootwin.
	 */
	if (data->rootwin)
	{
		if (!msg->attr || (get(data->rootwin, msg->attr, &v) && (v == msg->val)))
		{
			DoMethodA(data->rootwin, (Msg)&msg->args);
		}
	}
	else
	{
		FORCHILD(obj, MUIA_Application_WindowList)
		{
			if (!msg->attr || (get(child, msg->attr, &v) && (v == msg->val)))
			{
				if (getv(child, MA_Window_Type) == MV_Window_Type_Rootview)
				{
					DoMethodA(child, (Msg)&msg->args);
					break;
				}
			}
		}
		NEXTCHILD
	}

	return (0);
}


/*
 * Returns Action Dispatcher object
 */

DEFTMETHOD(Application_CreateActionDispatcher)
{
	APTR dispatcher;

	/*
	 * TODO: Add object to application and to list of
	 * working dispatchers.
	 */

	dispatcher = NewObject(getactiondispatcherclass(), NULL, TAG_DONE);

	return (ULONG)dispatcher;
}

/*
 * Deletes Action Dispatcher object
 */

DEFSMETHOD(Application_DeleteActionDispatcher)
{
	/*
	 * TODO: delete object from application and from list of
	 * working dispatchers.
	 */

	MUI_DisposeObject(msg->dispatcher);

	return 0;
}

/*
 * Given an ID, returns the
 * window object.
 */
DEFSMETHOD(Application_FindWindowByID)
{
	ULONG id;

	ASSERT(msg->id);

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if (get(child, MA_Window_ID, &id))
		{
			if (id == msg->id)
			{
				return ((ULONG)child);
			}
		}
		#ifdef DEBUG
		else
		{
			PDB(("argh, window %p (win: %p) has no proper attributes\n", child, _window(child)));
		}
		#endif
	}
	NEXTCHILD

	return (ULONG)NULL;
}


/*
 * Given a name and a type, returns
 * the window object.
 */
DEFSMETHOD(Application_FindWindowByName)
{
	ULONG type;
	STRPTR name;

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if (get(child, MA_Window_Type, &type))
		{
			if (type == msg->type)
			{
				name = (STRPTR)getv(child, MA_Window_Path);

				if ((msg->name == (APTR)MV_Window_Path_Any) || (name && !stricmp(name, msg->name)))
				{
					return ((ULONG)child);
				}
			}
		}
		#ifdef DEBUG
		else
		{
			PDB(("argh, window %p (win: %p) has no proper attributes\n", child, _window(child)));
		}
		#endif
	}
	NEXTCHILD

	return (ULONG)NULL;
}

DEFSMETHOD(Application_FindWindowByType)
{
	ULONG type;

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if (get(child, MA_Window_Type, &type))
		{
			if (type == msg->type)
			{
				return ((ULONG)child);
			}
		}
		#ifdef DEBUG
		else
		{
			PDB(("argh, window %p (win: %p) has no proper attributes\n", child, _window(child)));
		}
		#endif
	}
	NEXTCHILD

	return (ULONG)NULL;
}

/*
 * Enable/Disable DOS Notifications for all windows with specified path
 */

#if USE_DOSNOTIFY
DEFSMETHOD(Application_EnableDOSNotify)
{
	ULONG  type;
	STRPTR path;
	STRPTR mainpath;
	STRPTR ptr;
	STRPTR vpath;

	mainpath = name_build(msg->path);

	ptr = PathPart( mainpath );

	if ( mainpath && ptr && mainpath != ptr)
	{
		DB(("Modify mainpath (original:%s)\n", mainpath));
		mainpath[ ptr - mainpath ] = 0;
	}

	DB(("Supressing:%s(%s)\n",mainpath, msg->enable ? "No" : "Yes" ));

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if (get(child, MA_Window_Type, &type))
		{
			if (type == MV_Window_Type_View)
			{
				path = (STRPTR)getv(child, MA_Window_Path);

				if(path)
				{
					vpath = name_build(path);

					if(vpath)
					{
						int len = strlen(vpath);

						if(len && vpath[len - 1] == '/')
						{
							vpath[len - 1] = '\0';
						}

						if ((msg->path == (APTR)MV_Window_Path_Any) || (!stricmp(vpath, mainpath)))
						{
							set( child, MA_Window_EnableDOSNotify, msg->enable );
						}

						name_delete( vpath );
					}
				}
			}
		}
		#ifdef DEBUG
		else
		{
			PDB(("argh, window %p (win: %p) has no proper attributes\n", child, _window(child)));
		}
		#endif
	}
	NEXTCHILD

	if ( mainpath )
	{
		name_delete( mainpath );
	}

	return (ULONG)NULL;
}
#endif

/*
 * Given a type and a userdata, returns
 * the window object (note: this is not mandatory
 * for every window but just the given 'type').
 */
DEFSMETHOD(Application_FindWindowByUserData)
{
	ULONG type;
	ULONG ud;

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if (get(child, MA_Window_Type, &type))
		{
			if (type == msg->type)
			{
				ud = getv(child, MA_Window_UserData);

				if (ud == msg->userdata)
				{
					return ((ULONG)child);
				}
			}
		}
		#ifdef DEBUG
		else
		{
			PDB(("argh, window %p (win: %p) has no proper attributes\n", child, _window(child)));
		}
		#endif
	}
	NEXTCHILD
	return (ULONG)NULL;
}


DEFTMETHOD(Application_Open_AboutWindow)
{
	if (!aboutwin)
	{
		if ( (aboutwin = NewObject(getaboutclass(), NULL, TAG_DONE)) )
		{
			DoMethod(app, OM_ADDMEMBER, aboutwin);
		}
		else
		{
			errormsg(ERR_NOMEM);
		}
	}

	if (aboutwin)
	{
		set(aboutwin, MUIA_Window_Open, TRUE);
	}

	return (0);
}


DEFSMETHOD(Application_Open_AboutMorphOSWindow)
{
	if (msg->open)
	{
		if (!aboutmoswin)
		{
			#ifndef USE_LEGACY
			aboutmoswin	= MUI_NewObject("Aboutmorphos.mcc",
				MUIA_Window_ID, MAKE_ID('A','M','A','M'), 
			TAG_DONE);		
			#else
			aboutmoswin = NewObject(getaboutmosclass(), NULL, TAG_DONE);
			#endif

			if ( aboutmoswin )
			{
				DoMethod(app, OM_ADDMEMBER, aboutmoswin);
				
				#ifndef USE_LEGACY
				DoMethod(aboutmoswin, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
					obj, 2, MM_Application_Open_AboutMorphOSWindow, FALSE);
				#endif
			}
			else
			{
				/*  Does a display beep in case Panthon.mcc isn't found
				 *  for non-legacy builds.
				 */
				errormsg(ERR_NOMEM);
			}
		}

		if (aboutmoswin)
		{
			set(aboutmoswin, MUIA_Window_Open, TRUE);
		}
	}
	#ifndef USE_LEGACY
	else
	{
		PDB(("Dispose About MorphOS window\n"));
		methodstack_push(obj, 2, MM_Application_DisposeWindow, aboutmoswin);
		aboutmoswin = NULL;
	}
	#endif

	return (0);
}


DEFTMETHOD(Application_Open_CxWindow)
{
	if (!cxwin)
	{
		if ( (cxwin = NewObject(getcxwinclass(), NULL, TAG_DONE)) )
		{
			DoMethod(app, OM_ADDMEMBER, cxwin);
		}
		/*
		 * No error. It means there's already
		 * an Exchange running.
		 */
	}

	if (cxwin)
	{
		set(cxwin, MUIA_Window_Open, TRUE);
		DoMethod(cxwin, MM_Cxwin_Rescan);
	}

	return (0);
}


DEFTMETHOD(Application_Open_SystemInfoWindow)
{
	if (!systeminfowin)
	{
		if ( (systeminfowin = NewObject(getsysinfowinclass(), NULL, TAG_DONE)) )
		{
			DoMethod(obj, OM_ADDMEMBER, systeminfowin);
		}
		else
		{
			errormsg(ERR_NOMEM);
		}
	}

	if (systeminfowin)
	{
		set(systeminfowin, MUIA_Window_Open, TRUE);
	}

	return (0);
}

#if !USE_LEGACY
DEFTMETHOD(Application_Open_SystemLog)
{
	LogShowWindow();
	return (0);
}
#endif

DEFTMETHOD(Application_Open_ExecuteWindow)
{
	if (!executewin)
	{
		if ( (executewin = NewObject(getexecuteclass(), NULL, TAG_DONE)) )
		{
			DoMethod(app, OM_ADDMEMBER, executewin);
		}
		else
		{
			errormsg(ERR_NOMEM);
		}
	}

	if (executewin)
	{
		set(executewin, MUIA_Window_Open, TRUE);
	}

	return (0);

}


DEFTMETHOD(Application_NewShell)
{
	BPTR l;
	BPTR oldcd = (BPTR)NULL;
	TEXT buf[PATH_SIZE + 10];

	snprintf(buf, sizeof(buf), "newshell %s",getprefsstr(DSI_CLI_NEWSHELL));

	if ( (l = Lock("RAM:", ACCESS_READ)) )
	{
		oldcd = CurrentDir(l);
	}

	systemtags(buf,
		SYS_Asynch,  TRUE,
		SYS_Input,   NULL,
		SYS_Output,  NULL,
		NP_Priority, 0,     /* XXX: make that settable */
		/* NP_StackSize, getprefslong( DSI_CLI_STACK),
		NP_PPCStackSize, getprefslong( DSI_CLI_STACK) * 2, */
		NP_PPCStackSize, getprefslong( DSI_CLI_STACK),
	TAG_DONE);

	if (l)
	{
		CurrentDir(oldcd);
	}

	return (0);
}

/*
 * Structure, remember to use LONG or STRPTR.
 * Nothing else.
 */
struct RX_Delete {
	ULONG noicon;
	STRPTR *path;
};

struct RX_Format {
	STRPTR drive;
};

struct RX_Eject {
	STRPTR drive;
	ULONG eject;
	ULONG inject;
	ULONG toggle;
};

struct RX_Unmount {
	STRPTR drive;
};

struct RX_IconInfo {
	STRPTR path;
	LONG wait;
};

struct RX_Makedir {
	STRPTR path;
	LONG noicon;
};

struct RX_MakeLink {
	STRPTR from, to;
};

struct RX_Panel {
	LONG create;
	LONG config;
	LONG additem;
	LONG lock;
	LONG unlock;
	LONG togglelock;
	LONG removeitem;
	LONG moveitem;
	LONG del;
	LONG save;
	LONG stayopen;
	LONG * id;
	STRPTR itemtype;
	STRPTR uri;
	STRPTR image;
	LONG openParent; // bitRocky
};

/*  XXX: merge with RX_Sound
 */
struct RX_PlaySound {
	STRPTR path;
	STRPTR mode;
	LONG opencontrol;
};

struct RX_ParseURI {
	STRPTR uri;
	STRPTR stem;
};

struct RX_Sound {
	LONG stop;
	LONG pause;
	LONG opencontrol;
	LONG closecontrol;
};

struct RX_Snapshot {
	STRPTR path;
	LONG icons;
	LONG window;
	LONG selection;
};

struct RX_Unsnapshot {
	STRPTR path;
	LONG icons;
	LONG window;
	LONG selection;
};

struct RX_Rename {
	STRPTR *from;
	STRPTR to;
	LONG *viewid;
};

struct RX_Run {
	STRPTR path;
	STRPTR arg;
};

struct RX_Select {
	LONG all;
	LONG none;
	LONG pattern;
	LONG *viewid;
	STRPTR name;
};

struct RX_Settings {
	STRPTR page;
};

struct RX_Shortcut {
	STRPTR path;
	LONG add;
	LONG remove;
};

struct RX_Sort {
	STRPTR path;
	LONG alpha;
	LONG size;
	LONG type;
	LONG date;
	LONG reverse;
};

struct RX_Viewmode {
	STRPTR mode;
	LONG *viewid;
};

struct RX_LoadURI {
	STRPTR uri;
	LONG newwin;
	LONG reload;
	LONG force;
	LONG *browser;
	LONG *viewid;
	LONG nonewwin;
	LONG iconified;
	LONG tofront;
};

struct RX_Move {
	STRPTR to;
	STRPTR from;
	LONG *viewid; /* Reference view. The one progress will be visible in */
	ULONG rename; /* new name will be asked */
};

struct RX_Copy {
	STRPTR to;
	STRPTR *from;
	LONG *viewid; /* Reference view. The one progress will be visible in */
	ULONG rename; /* new name will be asked */
};

struct RX_ClipboardCut {
	STRPTR *path;
};

struct RX_ClipboardCopy {
	STRPTR *path;
};

struct RX_ClipboardAdd {
	STRPTR *path;
};

struct RX_ClipboardPaste {
	STRPTR to;
	LONG *viewid;  /* Reference view. The one progress will be visible in */
	LONG rename;   /* new name will be asked */
};

struct RX_ViewList {
	STRPTR stem;
};

struct RX_GetSelectedNames {
	STRPTR stem;
	LONG *viewid;

};

struct RX_LoadBackground {
	STRPTR path;
	STRPTR type;
	STRPTR mode;
};

struct RX_Parent {
	LONG *viewid;
};

struct RX_HistoryPrev {
	LONG *viewid;
};

struct RX_HistoryNext {
	LONG *viewid;
};

struct RX_Find {
	STRPTR *location;
	STRPTR name;
	STRPTR text;
	LONG execute;
};

struct RX_ViewClose {
	LONG *viewid;
};

struct RX_DropFile {
	STRPTR to;
	STRPTR *from;
	LONG *viewid; /* Reference view. The one progress will be visible in */
	LONG invert;
	ULONG rename;
};

struct RX_CopySelectionToClipboard {
	LONG *viewid; /* Reference view. The one progress will be visible in */
	LONG fullpath;
};

struct RX_AddBookmark {
	STRPTR uri;
	STRPTR name;
	LONG *viewid; /* Reference view. Used to get uri and position window */
	LONG permanent;
};

struct RX_GetBookmarks {
	STRPTR stem;
	STRPTR mode;
};

struct RX_RemoveBookmark {
	STRPTR pattern;
	STRPTR mode;
};

struct RX_EditMimeType {
	STRPTR path;
};

struct RX_Menu {
	LONG add;
	LONG remove;
	STRPTR id;
	STRPTR title;
	STRPTR shortcut;
	STRPTR type;
	STRPTR parentid;
	STRPTR command;
	STRPTR commandtype;
};

struct RX_GetMimeType {
	STRPTR path;
};

struct RX_GetDeficonPath {
	STRPTR path;
	STRPTR mimetype;
};

struct RX_SetPrefs {
	STRPTR key;
	STRPTR value;
};

struct RX_GetPrefs {
	STRPTR key;
};

struct RX_NetworksSettings {
	STRPTR path;
};

struct RX_NetworksConnect {
};

/* XXX: hm, check the /F issue.. could be useful. also check dopus again and remove all the not needed PATH (really..) */

const struct ambient_command rexxcmds[] = {
	{"Delete",            2, "NOICON/S,PATH/M",                                                                           RXCMD_Delete},
	{"Exchange",          0, "",                                                                                          RXCMD_Exchange},
	{"Format",            1, "DRIVE",                                                                                     RXCMD_Format},
	{"Eject",             4, "DRIVE,EJECT/S,INJECT/S,TOGGLE/S",                                                           RXCMD_Eject},
	{"Flash",             0, "",                                                                                          RXCMD_Flash},
	{"IconInfo",          2, "PATH,WAIT/S",                                                                               RXCMD_IconInfo},
	{"LoadURI",           9, "URI/U,NEW=NEWWIN/S,RELOAD/S,FORCE/S,BROWSER/N,VIEWID/N,NONEWWIN/S,ICONIFIED/S,TOFRONT/S",   RXCMD_LoadURI},
	{"Makedir",           2, "PATH,NOICON/S",                                                                             RXCMD_Makedir},
	{"MakeLink",          1, "FROM,TO",                                                                                   RXCMD_MakeLink},
	{"ParseURI",          2, "URI/U,STEM",                                                                                RXCMD_ParseURI},
	{"Panel",             16, "CREATE/S,CONFIG/S,ADDITEM/S,LOCK/S,UNLOCK/S,TOGGLELOCK/S,REMOVEITEM/S,MOVEITEM/S,DELETE/S,SAVE/S,STAYOPEN/S,ID/N,ITEMTYPE/K,URI/K,IMAGE/K,OPENPARENT/S", RXCMD_Panel}, // bitRocky
	{"PlaySound",         3, "PATH,MODE,OPENCONTROL/S",                                                                   RXCMD_PlaySound},
	{"Snapshot",          4, "PATH,ICONS/S,WINDOW/S,SELECTION/S",                                                         RXCMD_Snapshot},
	{"Sound",             4, "STOP/S,PAUSE/S,OPENCONTROL/S,CLOSECONTROL/S",                                               RXCMD_Sound},
	{"Unsnapshot",        4, "PATH,ICONS/S,WINDOW/S,SELECTION/S",                                                         RXCMD_Unsnapshot},
	{"RefreshVars",       0, "",                                                                                          RXCMD_RefreshVars},
	{"Random",            0, "",                                                                                          RXCMD_Random},
	{"Rename",            3, "FROM/M,TO,VIEWID/N",                                                                        RXCMD_Rename},
	{"Run",               2, "PATH,ARG/F",                                                                                RXCMD_Run},
	{"ScreenToBack",      0, "",                                                                                          RXCMD_ScreenToBack},
	{"ScreenToFront",     0, "",                                                                                          RXCMD_ScreenToFront},
	{"Select",            5, "ALL/S,NONE/S,PATTERN/S,VIEWID/N,NAME",                                                      RXCMD_Select},
	{"Settings",          1, "PAGE",                                                                                      RXCMD_Settings},
	{"Shortcut",          3, "PATH,ADD/S,REMOVE/S",                                                                       RXCMD_Shortcut},
	{"Sort",              6, "PATH,ALPHA/S,SIZE/S,TYPE/S,DATE/S,REVERSE/S",                                               RXCMD_Sort}, /* XXX: path makes no sense here */
	{"TBH",               0, "",                                                                                          RXCMD_TBH},
	{"Version",           0, "",                                                                                          RXCMD_Version},
	{"Viewmode",          2, "MODE,VIEWID/N",                                                                             RXCMD_Viewmode},
	{"Move",              4, "TO,FROM/M,VIEWID/N,AS/S",                                                                   RXCMD_Move},
	{"Copy",              4, "TO,FROM/M,VIEWID/N,AS/S",                                                                   RXCMD_Copy},
	{"ClipboardCut",      1, "PATH/M",                                                                                    RXCMD_ClipboardCut},
	{"ClipboardCopy",     1, "PATH/M",                                                                                    RXCMD_ClipboardCopy},
	{"ClipboardAdd",      1, "PATH/M",                                                                                    RXCMD_ClipboardAdd},
	{"ClipboardPaste",    3, "TO,VIEWID/N,AS/S",                                                                          RXCMD_ClipboardPaste},
	{"ViewList",          1, "STEM",                                                                                      RXCMD_ViewList},
	{"GetSelectedNames",  2, "STEM,VIEWID/N",                                                                             RXCMD_GetSelectedNames},
	{"LoadBackground",    3, "PATH,TYPE/K,MODE/K",                                                                        RXCMD_LoadBackground},
	{"Parent",            1, "VIEWID/N",                                                                                  RXCMD_Parent},
	{"HistoryPrev",       1, "VIEWID/N",                                                                                  RXCMD_HistoryPrev},
	{"HistoryNext",       1, "VIEWID/N",                                                                                  RXCMD_HistoryNext},
	{"Find",              4, "LOCATION/M,NAME,TEXT,EXECUTE/S",                                                            RXCMD_Find},
	{"ViewClose",         1, "VIEWID/N",                                                                                  RXCMD_ViewClose},
	{"DropFile",          5, "TO,FROM/M,VIEWID/N,INVERT/S,AS/S",                                                          RXCMD_DropFile},
	{"CopySelectionToClipboard",   2, "VIEWID/N,FULLPATH/S",                                                              RXCMD_CopySelectionToClipboard},
	{"AddBookmark",       4, "URI,NAME,VIEWID/N,PERMANENT/S",                                                             RXCMD_AddBookmark},
	{"GetBookmarks",      2, "STEM,MODE",                                                                                 RXCMD_GetBookmarks},
	{"RemoveBookmark",    2, "PATTERN,MODE",                                                                              RXCMD_RemoveBookmark},
	{"EditMimeType",      1, "PATH",                                                                                      RXCMD_EditMimeType},
	{"Menu",              9, "ADD/S,REMOVE/S,ID,TITLE,SHORTCUT,TYPE,PARENTID,COMMAND,COMMANDTYPE",                        RXCMD_Menu},
	{"IconSelect",        0, "",                                                                                          RXCMD_IconSelect},
	{"GetMimeType",       1, "PATH",                                                                                      RXCMD_GetMimeType},
	{"GetDeficonPath",    2, "PATH,MIMETYPE",                                                                             RXCMD_GetDeficonPath},
	{"SetPrefs",          2, "KEY,VALUE/F",                                                                               RXCMD_SetPrefs},
	{"GetPrefs",          1, "KEY",                                                                                       RXCMD_GetPrefs},
	{"SavePrefs",         0, "",                                                                                          RXCMD_SavePrefs},
	{"Trash",             2, "NOICON/S,PATH/M",                                                                           RXCMD_Trash},
	{"Restore",           2, "NOICON/S,PATH/M",                                                                           RXCMD_RestoreFromTrash},
	{"EmptyTrashcan",     0, "",                                                                                          RXCMD_EmptyTrashcan},
	{"Unmount",           1, "DRIVE",                                                                                     RXCMD_Unmount},
	{"NetworksSettings",       1, "PATH",                                                                                 RXCMD_NetworksSettings},
	{"NetworksConnect",   0, "",                                                                                          RXCMD_NetworksConnect},
	{ NULL,0,NULL,0}
};

#define REPORT_SUCCESS (rc = TRUE)
#define REPORT_ERROR(x) ({rc = TRUE; retval = (x);})
#define REPORT_RESULT(x,str) ({STRPTR t; \
	rc = TRUE; \
	if( (t = malloc(strlen((str)) + 1)) ) \
	{ \
		strcpy(t, (str)); \
	} \
	retstr = t; retval = (x);})

/*
 * Async passing structure helpers.
 */
struct rx_delete_async {
	ULONG id;
	STRPTR *dirlist;
	BOOL noicon;
};


static void setvar_stem(ULONG id, STRPTR stem, STRPTR name, STRPTR val)
{
	TEXT t[256]; /* should be enough for everyone (tm) */
	STRPTR p;

	snprintf(t, sizeof(t), "%s.%s", stem, name);
	p = t;

	while (*p)
	{
		*p = toupper(*p);
		p++;
	}
	rx_setrexxvar_id(id, t, val);
}

static int is_valid_pathlist(STRPTR *paths)
{
	ULONG i;

	for (i = 0; paths[i]; i++)
	{
		if (!strchr(paths[i], ':'))
		{
			return 0;
		}
	}

	return 1;
}

/* itix: We need rexx_path and rexx_pathlist because some REXX commands take only single file argument
 *       but take an objectlist...
 *
 * pathcount == 0 if there wasnt any path given...
 */
static STRPTR *build_pathlist(APTR obj, STRPTR rexx_path, STRPTR *rexx_pathlist, APTR *objlist, ULONG is_internal, LONG *pathcount, BOOL noicon, BOOL info)
{
	STRPTR *dirlist = NULL;
	ULONG count = 0, mode = 0;

	if (rexx_pathlist && rexx_pathlist[0])
	{
		mode = 0;

		while (rexx_pathlist[count])
		{
			/* Relative paths are not accepted */
			if (!strchr(rexx_pathlist[count], ':'))
			{
				count = 0;
				break;
			}

			count++;
		}
	}
	else if (rexx_path)
	{
		mode = 3;
		count = 1;
	}
	else if (is_internal)
	{
		if (objlist)
		{
			mode = 1;

			while (objlist[count])
			{
				count++;
			}
		}
		else if (obj)
		{
			mode = 2;
			count = 1;
		}
	}

	if (count)
	{
		dirlist = malloc((count+1) * sizeof(ULONG));

		if (dirlist)
		{
			ULONG i, error = 0;

			for (i = 0; i < count; i++)
			{
				STRPTR s = NULL;

				switch (mode)
				{
					case 0: // rexx_pathlist
						s = rexx_pathlist[i];
						break;

					case 1: // objlist
						s = (STRPTR)getv(objlist[i], MA_Icon_Path);
						break;

					case 2: // single icon
						s = (STRPTR)getv(obj, MA_Icon_Path);
						break;

					case 3: // single rexx path
						s = rexx_path;
						break;
				}

				if (s)
				{
					int modify = stricmp(FilePart(s), ".info");

					if (info && modify)
					{
						s = name_build_noinfo(s);
					}
					else if (!noicon && modify)
					{
						s = name_build(s);
						name_truncateinfo(s);
					}
					else
					{
						s = name_build(s);
					}
				}

				dirlist[i] = s;

				if (!s)
				{
					error = 1;
				}
			}

			dirlist[count] = NULL;

			if (error)
			{
				// don't call free_pathlist() because list is not complete

				for (i = 0; i < count; i++)
				{
					if (dirlist[i])
						name_delete(dirlist[i]);
				}

				free(dirlist);
				dirlist = NULL;
			}
		}
	}

	*pathcount = count;

	return dirlist;
}


static void free_pathlist(STRPTR *dirlist)
{
	ULONG i = 0;

	ASSERT(dirlist);
	ASSERT(dirlist[0]);

	do
	{
		name_delete(dirlist[i]);
		i++;
	}
	while (dirlist[i]);

	free(dirlist);
}

#define FORWARD_TO_MAINTASK \
if( ! IS_MAINTASK )\
{\
	return methodstack_push_sync(app, 9, MM_Application_DoRexx,\
								 msg->internal,\
								 msg->obj,\
								 msg->str,\
								 msg->retval,\
								 msg->retstr,\
								 msg->id,\
								 msg->objlist,\
								 msg->sync);\
}

/*
 * If retval is not NULL, it means there's an immediate reply pending
 * and *msg->retval or *msg->retstr are valid.
 */

LONG application_dorexx( Class *cl, Object *obj, struct MP_Application_DoRexx *msg )
{
	ULONG cmdlen;
	STRPTR p;
	ULONG rc = 0;
	LONG retval = 0;
	STRPTR retstr = NULL; /* XXX: hm.. or perhaps it should be "" ? */
    			
#if 0
	if(msg->obj)
	{
		PDB(("obj class = <%s>\n", OCLASS(msg->obj)->cl_ID));
	}
#endif

	/* XXX: we should perhaps remove empty front spaces.. or do that at an earlier stage */

	cmdlen = strlen(msg->str);

	if ( (p = strchr(msg->str, ' ')) )
	{
		cmdlen = p - msg->str;

		while (*p == ' ')
		{
			p++;
		}
	}

	if (cmdlen)
	{
		const struct ambient_command *cmd;

		for (cmd = rexxcmds; cmd && cmd->name && strnicmp(msg->str, cmd->name, cmdlen); cmd++);

		if (cmd && cmd->name)
		{
			ULONG *array = NULL;

			if (!cmd->args || cmd->argnum == 0 || (array = malloc(cmd->argnum * sizeof(ULONG))))
			{
				struct RDArgs *rda = NULL;

				if (array)
				{
					memset(array, 0, cmd->argnum * sizeof(ULONG));
				}

				if (!array || (rda = readargsstring(p, cmd->args, array)))
				{
					#if 0
					{
						int i;
						DB(("Array:0x%x\n",array));
						for (i=0; i<cmd->argnum;i++)
							DB(("Argument 0x%x:0x%x (<%s)\n", array+i,array[i],array[i]));
					}
					#endif
				
					if (!array || rda)
					{
						switch (cmd->id)
						{
							/*
							 * Errors: see rexx.h
							 * Set 'rc' to TRUE if an immediate reply
							 * is possible. Set also retval (and retstr if
							 * applicable) in that case.
							 */

							/*
							 *  Command:
							 *  - Foobar
							 * Synopsis:
							 *  - Creates foobar
							 * Parameters:
							 *  - BLA: plop
							 * Example:
							 *  - foobar plop
							 * Result:
							 *  - A string containing crap
							 * RC:
							 *  - 0 : Ok
							 *  - 10: The universe imploded
							 * Bugs:
							 *  - This command reboots the machine
							 * XXX:
							 *  - some crap that the parser will strip out
							 */

							/*
							 * Command:
							 *  - Exchange
							 * Synopsis:
							 *  - Lets popup Exchange window (useful for launching Exchange from 3rd
							 *    party apps like Deluxe or Toolmanager)
							 * RC:
							 *  - 0: Ok
							 */
							case RXCMD_Exchange:
								{
									FORWARD_TO_MAINTASK;

									DoMethod(app, MM_Application_Open_CxWindow);
									REPORT_SUCCESS;
								}
								break;

							/*
							 * Command:
							 *  - Delete
							 * Synopsis:
							 *  - Deletes files
							 * Parameters:
							 *  - PATH: path of file to delete, if not supplied, files are taken
							 *          from the selected entries in the SRC lister or the
							 *          selected object
							 *	- NOICON: Act as if .info where not special file and there fore not
							 *			  'linked' to the file. -> Useful to not delete 'myfile' when
							 *			  selection 'myfile.info' in a list
							 *
							 * RC:
							 *  - 0 : Ok
							 *  - 5 : Aborted by user
							 *  - 10: Aborted during execution
							 *  - 20: No path supplied and nothing selected in the lister
							 *
							 * XXX:
							 *  - ask when delecting subdirs
							 *  - add a global option for confirmation
							 *  - add some 'FORCE' option for deep cleanups
							 *  - add some 'partially deleted' returncode
							 *  - PATH/M ?
							 */
							case RXCMD_Delete:
								{
									struct RX_Delete *arg = (struct RX_Delete *)array;
									STRPTR *dirlist;
									LONG count;
									ULONG handleicons = FALSE;
									BOOL noicon = FALSE;

									methodstack_push_sync( msg->obj, 3, OM_GET, MA_View_HandleIcons, &handleicons);

									if(!handleicons)
									{
										noicon = TRUE;
									}

									dirlist = build_pathlist(msg->obj, NULL, arg->path, msg->objlist, msg->internal, &count, noicon, FALSE);

									DB(("dirlist 0x%p, noicon=%d, count=%d\n", dirlist, noicon, count));

									if (dirlist)
									{
										struct rx_delete_async *rxdel;

										/* i don't think it's smart to try to delete devices that way :) */
										if(isdevicename(dirlist[0]))
										{
											count = 0;
											free_pathlist(dirlist);
											dirlist = NULL;
										}
										else
										{
											if ( (rxdel = malloc(sizeof(*rxdel))) )
											{
												TEXT buttons[32];
												rxdel->id = msg->id;
												rxdel->dirlist = dirlist;
												rxdel->noicon = noicon;

												/* maybe make this requester (and its default active button)
												 * conditional/optional, depending on paranoia level :)
												 */

												snprintf(buttons, sizeof(buttons), "*%s", GSI(MSG_YESNO));

												smartreq_request(
													NULL,
													NULL,
													GSI(MSG_DELETE_TITLE),
													MM_Application_Delete_Ok,
													(LONG)rxdel, buttons,
													MV_Notification_Warning,
													count == 1 ? GSI(MSG_DELETE_CONFIRM) : GSI(MSG_DELETE_CONFIRM_MULTIPLE),
													count == 1 ? (ULONG)rxdel->dirlist[0] : count);
											}
											else
											{
												free_pathlist(dirlist);
												dirlist = NULL;
											}
										}
									}

									if (!dirlist)
									{
										if (!count)
											REPORT_ERROR(20);
										else
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
									}
								}
								break;

							/*
							 * Command:
							 *  - Format
							 * Synopsis:
							 *  - Opens the format window
							 * Parameters:
							 *  - DRIVE: name of the drive, eg. dh0:, dh1:, df0:, etc..
							 * RC:
							 *  - 0: Ok
							 */
							case RXCMD_Format:
								{
									struct RX_Format *arg = (struct RX_Format *)array;
									APTR formatwin = NULL;

									FORWARD_TO_MAINTASK;

									if (arg->drive)
									{
										/* XXX: to ponder..hm, should be /F probably and accept volume names also, and block upon completion (WAIT/S), so more errors */

										if (isdevicename(arg->drive))
										{
											if (!(formatwin = (APTR)DoMethod(app, MM_Application_FindWindowByName, MV_Window_Type_Format, arg->drive, TAG_DONE)))
											{
												if ( (formatwin = NewObject(getformatwinclass(), NULL, MA_Window_Path, arg->drive, TAG_DONE)) )
												{
													DoMethod(app, OM_ADDMEMBER, formatwin);
												}
											}
										}
									}
									else
									{
										if (msg->internal && msg->obj)
										{
											if (!(formatwin = (APTR)DoMethod(app, MM_Application_FindWindowByName, MV_Window_Type_Format, getv(msg->obj, MA_Icon_Path), TAG_DONE)))
											{
												if ( (formatwin = NewObject(getformatwinclass(), NULL, MA_Window_Path, getv(msg->obj, MA_Icon_Path), TAG_DONE)) )
												{
													DoMethod(app, OM_ADDMEMBER, formatwin);
												}
											}
										}
										else
										{
											if ( (formatwin = NewObject(getformatwinclass(), NULL, TAG_DONE)) )
											{
												DoMethod(app, OM_ADDMEMBER, formatwin);
											}
										}
									}

									if (formatwin)
									{
										set(formatwin, MUIA_Window_Open, TRUE);

										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
									}
								}
								break;
							/*
							 * Command:
							 *  - Eject
							 * Synopsis:
							 *  - Ejects media
							 * Parameters:
							 *  - DRIVE: name of the drive, eg. dh0:, dh1:, df0:, etc..
							 *  - INJECT: Inject media
							 *  - EJECT:  Eject media
							 *  - TOGGLE: Inject or Eject media depending on prior state
							 * RC:
							 *  - 0: Ok
							 */
							case RXCMD_Eject:
								{
									struct RX_Eject *arg = (struct RX_Eject *)array;
									STRPTR name;
									struct device_info *di;
									ULONG mode = EJECT_TOGGLE;

									FORWARD_TO_MAINTASK;

									if ( arg->drive)
									{
										name = arg->drive;
										if( arg->inject )
										{
											mode = EJECT_LOAD;
										}
										else
										{
											if( arg->eject )
											{
												mode = EJECT_EJECT;
											}
										}
									}
									else
									{
										if (msg->internal && msg->obj)
										{
											name = (STRPTR) getv(msg->obj, MA_Icon_Path);
										}
										else
										{
											REPORT_ERROR(5);
											break;
										}
									}

									if( (di = deviceinfo_build( name, TRUE)) )
									{
										eject( di->devname, di->unit, di->flags, mode );

										deviceinfo_delete( di );

										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(5);
									}
								}
								break;

							/*
							 * Command:
							 *  - Unmount
							 * Synopsis:
							 *  - Unmounts media
							 * Parameters:
							 *  - DRIVE: name of the drive, eg. dh0:, dh1:, df0:, etc..
							 * RC:
							 *  - 0: Ok
							 */
							case RXCMD_Unmount:
								{
									struct RX_Unmount *arg = (struct RX_Unmount *)array;
									STRPTR name;
									struct device_info *di;

									FORWARD_TO_MAINTASK;

									if ( arg->drive)
									{
										name = arg->drive;
									}
									else
									{
										if (msg->internal && msg->obj)
										{
											name = (STRPTR) getv(msg->obj, MA_Icon_Path);
										}
										else
										{
											REPORT_ERROR(5);
											break;
										}
									}

									if( (di = deviceinfo_build( name, FALSE )) )
									{
										unmount( di->name );

										deviceinfo_delete( di );

										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(5);
									}
								}
								break;

							/*
							 * Command:
							 *  - Flash
							 * Synopsis:
							 *  - Flashes the screen
							 * RC:
							 *  - 0: Ok
							 */
							case RXCMD_Flash:
								{
									DisplayBeep(NULL);

									REPORT_SUCCESS;
								}
								break;

							/*
							 * Command:
							 *  - IconInfo
							 * Synopsis:
							 *  - Opens the icon information requester
							 * Parameters:
							 *  - PATH: path to the icon filename (with or without .info)
							 *  - WAIT/S: wait until the window is closed
							 * RC:
							 *  - 0: Ok
							 *  - 5: User canceled the window
							 *  - 10: Couldn't open the window
							 *  - 20: No path supplied and nothing selected in the lister
							 *  - 21: Couldn't write the icon to disk
							 *
							 */
							case RXCMD_IconInfo:
								{
									struct RX_IconInfo *arg = (struct RX_IconInfo *)array;
									STRPTR *dirlist;
									LONG count;
									ULONG ret = FALSE;
									

									dirlist = build_pathlist(msg->obj, arg->path, NULL, msg->objlist, msg->internal, &count, FALSE, TRUE);

									if (dirlist)
									{
										APTR infowin;
										STRPTR *dl = dirlist;
										LONG match = 0;
										BOOL async = FALSE;

										/* Scan through entries and reopen window if present */

										while (*dl)
										{
											PDB(("!%s! !%s\n", (arg->path) ? arg->path : "arg->path is NULL", *dl));
											if ( (infowin = (APTR)DoMethod(obj, MM_Application_FindWindowByName, MV_Window_Type_Info, *dl)) )
											{
												*dl[0] = '\0';   /* Mark this entry unused */
												match++;

												set(infowin, MUIA_Window_Open, TRUE); /* bring to front */

												if (arg->wait)
												{
													set(infowin, MA_Infowin_RXID, msg->id);
													async = TRUE;
												}
												else
												{
													REPORT_SUCCESS;
												}
											}

											dl++;
										}

										if (match == count)
										{
											/* Windows existed, we are done and ready to quit */
											ret = TRUE;
										}

										if (match != count)
										{
											if ( msg->sync )
											{
												ret = do_action_sync(obj, TA_Infowin_Open,
													TT_Infowin_Open_PathList, dirlist,
													arg->wait ? TT_Infowin_Open_Wait : TAG_IGNORE, TRUE,
													arg->wait ? TT_Infowin_Open_RexxID : TAG_IGNORE, msg->id,
												TAG_DONE);
											}
											else
											{
												ret = do_action(obj, TA_Infowin_Open,
													TT_Infowin_Open_PathList, dirlist,
													arg->wait ? TT_Infowin_Open_Wait : TAG_IGNORE, TRUE,
													arg->wait ? TT_Infowin_Open_RexxID : TAG_IGNORE, msg->id,
												TAG_DONE);
											}
											/* If we passed the rexx msg to the infowin, do NOT reply it here! */
											if (ret && arg->wait)
											{
												async = TRUE;
											}
										}
										if (async)
										{
											/* Do NOT reply the message, it's replied by the infowindow */
											rc = FALSE;
											break;
										}
									}

									if (!ret)
									{
										if (!count)
											REPORT_ERROR(20);
										else
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
									}
									else
									{
										REPORT_SUCCESS;
									}
								}
								break;

							/*
							 * Command:
							 *  - LoadURI
							 * Synopsis:
							 *  - Loads an URI
							 * Parameters:
							 *  - URI: absolute URI to open
							 *  - NEW=NEWWIN/S: opens the URI in a new window, if applicable
							 *  - RELOAD/S: reloads the URI
							 *  - FORCE/S: forces full reloading of the URI no matter what
							 *  - BROWSER/N: opens window in chosen browser mode (0-disabled, 1-enabled). if not present then use user setup
							 *  - VIEWID/N: specifies view/window ID for which command is executed. It overrides msg->obj param.
							 * RC:
							 *  - 0: Ok
							 *  - 5: Error
							 * XXX: change those errors. add a 'WAIT' to know until the URI is fully loaded
							 */
							case RXCMD_LoadURI:
								{
									struct RX_LoadURI *arg = (struct RX_LoadURI *)array;
									ULONG isroot = FALSE;
									ULONG uriid = 0;
									ULONG newwin = arg->newwin;
									APTR ctx;
									APTR object = obj;
									ULONG browser;

									/* XXX: we should honour the flags */

									if (msg->internal && object)
									{
										APTR viewobj;

										if ( (viewobj = _view(msg->obj)) ) /* XXX: hm.. isn't there another way ? */
										{
											isroot = getv(viewobj, MA_View_IsRoot);
											uriid = getv(viewobj, MA_Viewgroup_ID);
											newwin = getv(viewobj, MA_View_NewWin); /* XXX: and the new view should have a say too.. sigh */

										}
										/* XXX */
									}

									if ( arg->viewid && *arg->viewid > 0 )
									{
										DB(("Override ViewID to %d\n", *arg->viewid));
										uriid = *arg->viewid;
										newwin = 0;
									}

									/*
									 * Check what browser setup was selected.
									 */

									if ( arg->browser )
									{
										browser = *arg->browser;
									}
									else
									{
										browser = _conf( toolbar_browsermode );
									}

									/*
									 * Check if new win is forced by user config.
									 */

									if ( uriid == 0 && !_conf( toolbar_browsermode ) )
									{
										newwin = 1;
									}

									if ( uriid > 0 )
									{
										APTR wo = (APTR)methodstack_push_sync( app, 2, MM_Application_FindWindowByID, uriid );

										if ( wo )
										{
											APTR viewobj;

											if ( methodstack_push_sync( wo,3, OM_GET, MA_Window_Viewobj, &viewobj ) )
											{
												methodstack_push_sync( viewobj,3, OM_GET, MA_View_IsRoot, &isroot );

												/* try to honour view policy regarding newwin, if it couldn't be set before */
												if(!newwin && !arg->nonewwin)
												{
													methodstack_push_sync( viewobj,3, OM_GET, MA_View_NewWin, &newwin );
												}
											}
										}
									}

									/* XXX: do we need an isroot really ? */
									/*
									 * We open new URI. For root windows it always opens new window, for other
									 * we check in mimectx if new content has NEWWIN flag set to TRUE.
									 * TODO: take care for current view's NewWin (newwin var) value.
									 */

									if ( arg->uri && (ctx = mimeuri_create()) )
									{
										ULONG ret, addMode=FALSE, addView=FALSE;
										STRPTR uri=NULL;

										D(REXX, bug("arg->tofront = %ld\n", arg->tofront)); // bitRocky
										D(REXX, bug("arg->uri = '%s'\n", arg->uri)); 
// jDc: I have disabled this as it breaks listers set into All Files mode. Break this again and I'll break you.
										if (0 && !strcasestr(arg->uri, "DEVICES://"))
										{
											addMode = !strcasestr(arg->uri, "MODE="); // check if "mode=" is used
											addView = !strcasestr(arg->uri, "VIEW="); // check if "view=" is used
											D(REXX, bug("addMode = %ld, addView = %ld\n", addMode, addView));
											if ( addMode || addView )
											{
												ULONG len=strlen(arg->uri)+1+1; // 1 for the "?", 1 for the \0
												if (addMode) len += 5+6; // len of "mode=" and ICONS, THUMBS or ALL
												if (addView) len += 5+4; // len of "view=" and LIST or ICON
												if (addMode && addView) len += 1; // for the "&"
												D(REXX,bug("len = %ld, arg->uri len = %ld\n", len, strlen(arg->uri)));
												if ((uri = malloc(len)))
												{
													static const CONST_STRPTR modes[] = {"ICONS", "ALL", "THUMBS"};
													static const CONST_STRPTR views[] = {"ICON", "LIST"}; 
													
													sprintf(uri, "%s%s%s%s%s%s%s", arg->uri,
														strstr(arg->uri, "?") ? "&" : "?", //((addMode && !addView) || (addView && !addMode)) ? "&":"?",
														addMode ? "MODE=" : "", addMode ? modes[_conf(window_default_viewmode)] : "",
														(addMode && addView) ? "&":"",
														addView ? "VIEW=" : "", addView ? views[_conf(window_default_view)] : "");
													D(REXX, bug("uri = '%s'\n", uri));
												}
											}
										}
										
										// why check if IS_MAINTASK? both do the same????
										if ( IS_MAINTASK )
										{
											/*
											 * Called from a thread already.
											 */
											ret	= do_action(obj, TA_URI_Load,
												TT_URI_Load_URI, uri ? uri : arg->uri,
												TT_URI_Load_ID, uriid,
												TT_URI_Load_Ctx, ctx,
												TT_URI_Load_Newwin, isroot ? 1 : ( newwin ? TRUE : (ULONG)mimetype_getattr( ctx, MIMETYPETAG_NEWWIN ) ),
												TT_URI_Load_Browser, browser,
												TT_URI_Load_Iconified, arg->iconified,
												(msg->internal && !arg->tofront) ? TAG_IGNORE : TT_URI_Load_ToFront, TRUE,
											TAG_DONE);
										}
										else
										{
											ret	= do_action(obj, TA_URI_Load,
												TT_URI_Load_URI, uri ? uri : arg->uri,
												TT_URI_Load_ID, uriid,
												TT_URI_Load_Ctx, ctx,
												TT_URI_Load_Newwin, isroot ? 1 : ( newwin ? TRUE : (ULONG)mimetype_getattr( ctx, MIMETYPETAG_NEWWIN ) ),
												TT_URI_Load_Browser, browser,
												TT_URI_Load_Iconified, arg->iconified,
												(msg->internal && !arg->tofront) ? TAG_IGNORE : TT_URI_Load_ToFront, TRUE,
											TAG_DONE);
										}
										
										if (uri) free(uri);

										if ( ret )
										{
											REPORT_SUCCESS;
										}
										else
										{
											mimeuri_delete(ctx);
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
										}
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
									}
								}
								break;

							/*
							 * Command:
							 *  - Makedir
							 * Synopsis:
							 *  - Creates a directory
							 * Parameters:
							 *  - PATH: path of the directory to create. If it contains multiple components, the last
							 *          one is taken as the directory name
							 *  - NOICON/S: do not create an icon
							 * RC:
							 *  - 0: Ok
							 *  - 5: Error
							 * XXX: check if PATH is absolute, and if so create the directory, if not just create it in
							 * the SRC lister
							 */
							case RXCMD_Makedir:
								{
									struct RX_Makedir *arg = (struct RX_Makedir *)array;
									APTR makedirwin = NULL;

									FORWARD_TO_MAINTASK;

									if (arg->path)
									{
										makedirwin = NewObject(getmakedirwinclass(), NULL, MA_Makedirwin_Path, arg->path, MA_Makedirwin_Icon, !arg->noicon, TAG_DONE);
									}
									else
									{
										if (msg->internal && msg->obj)
										{
											makedirwin = NewObject(getmakedirwinclass(), NULL, MA_Makedirwin_Path, getv(msg->obj, MA_DragDrop_Path/* XXX: yeah yeah.. name is wrong :) */), MA_Makedirwin_Icon, !arg->noicon, TAG_DONE);
										}
										else
										{
											/* XXX: complain ? */
											//REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}

									if (makedirwin)
									{
										DoMethod(app, OM_ADDMEMBER, makedirwin);
										set(makedirwin, MUIA_Window_Open, TRUE);
										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 * Command:
							 *  - MakeLink
							 * Synopsis:
							 *  - Creates a link
							 * Parameters:
							 *  - FROM:
							 *  - TO:
							 * RC:
							 *  - 0: Ok
							 *  - 5: Error

							 */
							case RXCMD_MakeLink:
								{
									struct RX_MakeLink *arg = (struct RX_MakeLink *)array;
									APTR makelinkwin = NULL;

									FORWARD_TO_MAINTASK;

									/* XXX: from and to? think about it */

									if (arg->from)
									{
										if ( (makelinkwin = NewObject(getmakelinkwinclass(), NULL, MA_Makelinkwin_From, arg->from, TAG_DONE)) )
										{
											DoMethod(app, OM_ADDMEMBER, makelinkwin);
										}
									}
									else
									{
										if (msg->internal && msg->obj)
										{
											if ( (makelinkwin = NewObject(getmakelinkwinclass(), NULL, MA_Makelinkwin_From, getv(msg->obj, MA_DragDrop_Path/* XXX: yeah yeah.. name is wrong :) */), TAG_DONE)) )
											{
												DoMethod(app, OM_ADDMEMBER, makelinkwin);
											}
										}
										else
										{
											/* XXX: complain ? */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}

									if (makelinkwin)
									{
										set(makelinkwin, MUIA_Window_Open, TRUE);

										REPORT_SUCCESS;
									}
								}
								break;

							/*
							 *  Command:
							 *  - ParseURI
							 * Synopsis:
							 *  - Parses an URI/URL into a stem
							 * Parameters:
							 *  - URI: full URI/URL
							 *  - STEM: stem name to store the result to
							 * Example:
							 *  - ParseURI http://foo@plop.com:80 urlres
							 * Result:
							 *  - A stem var with the following fields:
							 *    - stem.uri: full URI after processing. Same as input most of the time.
							 *    - stem.scheme: scheme (http, ftp, etc..)
							 *    - stem.host: host or "authority"
							 *    - stem.port: port number. Either a default or specified in the URI
							 *    - stem.username: username
							 *    - stem.password: password
							 *    - stem.path: URI path
							 *    - stem.args: URI arguments or "query"
							 *    - stem.fragment: URI fragment
							 *    - stem.local: set to 1 if the URI is local, 0 otherwise
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Couldn't parse URI
							 * XXX:
							 *  - hm, we should return when the URI is invalid..
							 */
							case RXCMD_ParseURI:
								{
									struct RX_ParseURI *arg = (struct RX_ParseURI *)array;

									if (arg->uri && arg->stem)
									{
										APTR ctx;

										if ( (ctx = mimeuri_create()) )
										{
											if (mimeuri_gather(ctx, arg->uri,
												MIMEURIGATHERTAG_Extension, FALSE,
												MIMEURIGATHERTAG_Protocol,  FALSE,
												MIMEURIGATHERTAG_FileIO,    FALSE,
												MIMEURIGATHERTAG_Network,   FALSE,
											TAG_DONE
											))
											{
												TEXT t[6];

												setvar_stem(msg->id, arg->stem, "uri",      mimeuri_getattr(ctx, MIMEURIATTR_URI));
												setvar_stem(msg->id, arg->stem, "scheme",   mimeuri_getattr(ctx, MIMEURIATTR_SCHEME));
												setvar_stem(msg->id, arg->stem, "host",     mimeuri_getattr(ctx, MIMEURIATTR_HOST));
												setvar_stem(msg->id, arg->stem, "username", mimeuri_getattr(ctx, MIMEURIATTR_USERNAME));
												setvar_stem(msg->id, arg->stem, "password", mimeuri_getattr(ctx, MIMEURIATTR_PASSWORD));
												setvar_stem(msg->id, arg->stem, "path",     mimeuri_getattr(ctx, MIMEURIATTR_PATH));
												setvar_stem(msg->id, arg->stem, "args",     mimeuri_getattr(ctx, MIMEURIATTR_ARGS));
												setvar_stem(msg->id, arg->stem, "fragment", mimeuri_getattr(ctx, MIMEURIATTR_FRAGMENT));
												snprintf(t, sizeof(t), "%lu", (ULONG)mimeuri_getattr(ctx, MIMEURIATTR_PORT));
												setvar_stem(msg->id, arg->stem, "port",     t);
												setvar_stem(msg->id, arg->stem, "local",    mimeuri_getattr(ctx, MIMEURIATTR_LOCAL) ? "1" : "0");

												REPORT_SUCCESS;
											}
											else
											{
												REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY); /* XXX: for now.. */
											}
											mimeuri_delete(ctx);
										}
										else
										{
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
										}
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
									}
								}
								break;

							/*
							 * Command:
							 *  - PlaySound
							 * Synopsis:
							 *  - plays an audio file with ambients internal player
							 * Parameters:
							 *  - PATH: path to an audio file
							 *  - MODE: DATATYPE, MPEGA, VORBIS or MULTIMEDIA, else it will try to autodetect
							 *  - OPENCONTROL/S: opens the control window
							 * RC:
							 *  - 0: Ok
							 *  - 5: Error
							 */
							case RXCMD_PlaySound:
								{
									struct RX_PlaySound *arg = (struct RX_PlaySound *)array;
									ULONG mode = PS_UNKNOWN;

									if(arg->mode)
									{
										if(!stricmp("datatype", arg->mode))
											mode = PS_DATATYPES;

										if(!stricmp("mpega", arg->mode))
											mode = PS_MPEGA;

										if(!stricmp("vorbis", arg->mode))
											mode = PS_VORBIS;

										if(!stricmp("multimedia", arg->mode))
											mode = PS_MULTIMEDIA;
									}

									if (arg->path)
									{
										if (do_action(obj, TA_Sound_Play,
											TT_Sound_Play_Path, arg->path,
											TT_Sound_Play_Mode, mode | PSF_QUEUED_IMMEDIATE,
										TAG_DONE))
										{
											if (arg->opencontrol)
											{
												methodstack_push_sync(app, 2, MM_Application_SoundControl, MV_Application_SoundControl_OpenControl);
											}
											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
										}
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
									}
								}
								break;

							/*
							 * Command:
							 *  - Panel
							 * Synopsis:
							 *  - Sends a command to a panel
							 * Parameters:
							 *  - CREATE/S: create a panel. This parameter is mutually exclusive
							 *  - CONFIG/S: open the configuration window. This parameter is mutually exclusive
							 *  - ADDITEM/S: add an item. This parameter is mutually exclusive
							 *  - LOCK/S: lock the panel. This parameter is mutually exclusive
							 *  - UNLOCK/S: unlocksthe panel. This parameter is mutually exclusive
							 *  - TOGGLELOCK/S: toggle the locking of the panel. This parameter is mutually exclusive
							 *  - REMOVEITEM/S: remove an item. This parameter is mutually exclusive
							 *  - DELETE/S: delete a panel. This parameter is mutually exclusive
							 * RC:
							 *  - 0: Ok
							 *  - 5: Error
							 */
							case RXCMD_Panel:
								{
									struct RX_Panel *arg = (struct RX_Panel *)array;
									APTR panelwin = NULL;

									FORWARD_TO_MAINTASK;

									/*
									 * Arguments are mutually exclusive.
									 */
									if (arg->create + arg->config + arg->additem + arg->lock + arg->unlock + arg->togglelock + arg->removeitem + arg->moveitem + arg->del > 1)
									{
										REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
										break;
									}

									if (arg->create)
									{
										if ( (panelwin = NewObject(getpanelwinclass(), NULL, TAG_DONE)) )
										{
											TEXT buffer[16];

											DoMethod(app, OM_ADDMEMBER, panelwin);

											snprintf(buffer, sizeof(buffer), "%ld", getv(panelwin, MA_Window_ID));
											REPORT_RESULT(0, buffer);
										}
									}
								/*	  else if (arg->config)
									{
										if (msg->obj)
										{
											if (!(panelwin = (APTR)DoMethod(app, MM_Application_FindWindowByUserData, MV_Window_Type_Panelconfig, msg->obj)))
											{
												if ( (panelwin = NewObject(getpanelconfigwinclass(), NULL, MA_Panelconfigwin_Parent, msg->obj, TAG_DONE)) )
												{
													DoMethod(app, OM_ADDMEMBER, panelwin);
													set(msg->obj, MA_Panelgroup_Configwin, panelwin);
												}

											}
										}
										else
										{
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}       */
									else if (arg->additem)
									{
										if(arg->id && *arg->id && arg->itemtype)
										{
											if ((panelwin = (APTR)DoMethod(app, MM_Application_FindWindowByID, *arg->id)))
											{
												switch( PanelItem_NameToType( arg->itemtype ) )
												{
													case MV_Panel_Type_Button:
														if(arg->uri && arg->image)
														{
															APTR panelgroup = (APTR) getv(panelwin, MA_Panelwin_Group);
															APTR icon = (APTR) methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, MV_ViewID_Panel);

															if(icon)
															{
																SetAttrs(icon, MA_Icon_Path, arg->uri, MA_Icon_PathInfo, arg->image, TAG_DONE);

																#warning do it properly!
																DoMethod(panelgroup, MUIM_DragDrop, icon, 0, 0, 0);

																methodstack_push_sync(app, 2, MM_Application_DisposeObject, icon);

																REPORT_SUCCESS;
															}
														}
														break;

													default:
														REPORT_ERROR(20);
												}
											}
											else
											{
												REPORT_ERROR(20);
											}

										}
										else if(msg->obj)
										{
										/*	  if (!(panelwin = (APTR)DoMethod(app, MM_Application_FindWindowByName, MV_Window_Type_Panelitem, MV_Window_Path_Any)))
											{
												if ( (panelwin = NewObject(getpanelitemwinclass(), NULL, TAG_DONE)) )
												{
													DoMethod(app, OM_ADDMEMBER, panelwin);
												}
											}*/

											REPORT_SUCCESS;			   
										}
										else
										{
											REPORT_ERROR(20);
										}
									}
									else if (arg->del)
									{
										if (msg->obj)
										{
											DoMethod(app, MUIM_Application_PushMethod, msg->obj, 1, MM_Panelgroup_Delete);

											REPORT_SUCCESS;
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}
									else if (arg->lock)
									{
										if (msg->obj)
										{
											DoMethod(app, MUIM_Application_PushMethod, msg->obj, 2, MM_Panelgroup_Lock, TRUE);

											REPORT_SUCCESS;
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}
									else if (arg->unlock)
									{
										if (msg->obj)
										{
											DoMethod(app, MUIM_Application_PushMethod, msg->obj, 2, MM_Panelgroup_Lock, FALSE);

											REPORT_SUCCESS;
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}
									else if (arg->togglelock)
									{
										if (msg->obj)
										{
											DoMethod(app, MUIM_Application_PushMethod, msg->obj, 1, MM_Panelgroup_ToggleLock);

											REPORT_SUCCESS;
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}
									else if (arg->removeitem)
									{
										if (msg->obj)
										{
											DoMethod(app, MUIM_Application_PushMethod, msg->obj, 2, MM_Panelgroup_RemoveItem, MV_Panelgroup_RemoveItem_Current);
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}
									else if (arg->moveitem)
									{
										PDB(("%x\n",msg->obj));
										if (msg->obj)
										{
										//	DoMethod(app, MUIM_Application_PushMethod, msg->obj, 4, MM_Panelgroup_MoveMode_Start, MV_Panelgroup_MoveMode_Start_Standalone, 0, 0);
											DoMethod(msg->obj, MM_Panelgroup_MoveMode_Start, MV_Panelgroup_MoveMode_Start_Standalone, 0, 0);
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}
									else if(arg->save)
									{
										if ((msg->obj)&&(_win(msg->obj)))
										{
											DoMethod(app, MUIM_Application_PushMethod, _win(msg->obj), 4, MM_Panelwin_SaveConfig,0 , 0, 0);
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}
									else if (arg->stayopen)
									{

										if (msg->obj)
										{
											DoMethod(app, MUIM_Application_PushMethod, _win(msg->obj), 1, MM_Panelsubwin_ToggleStayOpen);

											REPORT_SUCCESS;
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}
									}
									else if (arg->openParent) // bitRocky
									{
										if (msg->obj)
										{
											DoMethod(app, MUIM_Application_PushMethod, msg->obj, 1, MM_Panelgroup_OpenParent);
										}
										else
										{
											/* XXX: for now */
											REPORT_ERROR(AMBIENT_RXERR_NYI);
										}										
									}
									else
									{
										/* XXX: for now */
										REPORT_ERROR(AMBIENT_RXERR_NYI);
									}

									if (panelwin)
									{
										set(panelwin, MUIA_Window_Open, TRUE);

										REPORT_SUCCESS;
									}
								}
								break;

							/*
							 * Command:
							 *  - Snapshot
							 * Synopsis:
							 *  - Snapshots the icon
							 * Parameters:
							 *  - PATH: path of the icon to snapshot, if not supplied, path is taken from
							 *          the selected entries in the SRC lister or the selected object
							 *  - ICONS/S: snapshot the icon placement
							 *  - WINDOW/S: snapshot the window placement and size
							 *  - SELECTION/S: if ICON is set, when called from view, only selected icons are snapshot.
							 * RC:
							 *  - 0: Ok
							 * XXX:
							 *  - no retcode? no WAIT/S?
							 */
							case RXCMD_Snapshot:
								{
									struct RX_Snapshot *arg = (struct RX_Snapshot *)array;

									if (msg->internal && msg->obj) /* XXX: handle the other cases.. */
									{
										ULONG dummy;
										/* really sucky: iconview has some special handling to snapshot shortcuts */
										if(GetAttr(MA_Iconview_IsRoot, _view(msg->obj), &dummy))
										{
											if(dummy)
											{
												/* rootview will save shortcuts that way */
												DoMethod(_parent(msg->obj), MM_Iconview_SnapshotIcon, msg->obj);
											}
											else
											{
												DoMethod(msg->obj, MM_Rexx_Snapshot, arg->icons, arg->window, arg->selection);
											}
										}
										else
										{
											DoMethod(msg->obj, MM_Rexx_Snapshot, arg->icons, arg->window, arg->selection);
										}

										REPORT_SUCCESS; /* XXX: should depend on the above */
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_NYI);
									}
								}
								break;

							/*
							 * Command:
							 *  - Sound
							 * Synopsis:
							 *  - Controls Ambient's sound system
							 * Parameters:
							 *  - STOP/S: stops the sound
							 *  - PAUSE/S: pauses the sound, acts as a toggle
							 *  - OPENCONTROL/S: opens the control window
							 * RC:
							 *  - 0: Ok
							 * XXX:
							 *  - needs more commands and a way to address separate sounds
							 */
							case RXCMD_Sound:
								{
									struct RX_Sound *arg = (struct RX_Sound *)array;

									FORWARD_TO_MAINTASK;

									if (arg->opencontrol && arg->closecontrol)
									{
										REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
										break;
									}

									if (arg->stop)
									{
										sound_stop = TRUE;
										REPORT_SUCCESS;
									}
									else if (arg->pause)
									{
										sound_pause ^=1;
										REPORT_SUCCESS;
									}

									if (arg->opencontrol)
									{
										DoMethod(app, MM_Application_SoundControl, MV_Application_SoundControl_OpenControl);
										REPORT_SUCCESS;
									}

									if (arg->closecontrol)
									{
										REPORT_ERROR(AMBIENT_RXERR_NYI);
									}

								}
								break;

							/*
							 * Command:
							 *  - Unsnapshot
							 * Synopsis:
							 *  - Unsnapshots the icon
							 * Parameters:
							 *  - PATH: path of the icon to unsnapshot, if not supplied, path is taken from
							 *          the selected entries in the SRC lister or the selected object
							 *  - ICONS/S: unsnapshot the icon placement
							 *  - WINDOW/S: unsnapshot the window placement and size
							 * RC:
							 *  - 0: Ok
							 * XXX:
							 *  - no retcode? no WAIT/S?
							 */
							case RXCMD_Unsnapshot:
								{
									struct RX_Unsnapshot *arg = (struct RX_Unsnapshot *)array;

									FORWARD_TO_MAINTASK;

									if (msg->internal && msg->obj) /* XXX: handle the other cases.. */
									{
										ULONG dummy;
										/* really sucky: iconview has some special handling to snapshot shortcuts */
										if(GetAttr(MA_Iconview_IsRoot, _view(msg->obj), &dummy))
										{
											if(dummy)
											{
												/* rootview will save shortcuts that way */
												DoMethod(_parent(msg->obj), MM_Iconview_UnsnapshotIcon, msg->obj);
											}
											else
											{
												DoMethod(msg->obj, MM_Rexx_Unsnapshot, arg->icons, arg->window, arg->selection);
											}
										}
										else
										{
											DoMethod(msg->obj, MM_Rexx_Unsnapshot, arg->icons, arg->window);
										}

										REPORT_SUCCESS; /* XXX: should depend on the above */
									}
									else if(arg->path && *arg->path)
									{
										if(arg->window)
										{
											do_action(NULL, TA_Window_Snapshot,
												TT_Window_Snapshot_Path, arg->path,
												TT_Window_Snapshot_X, NO_ICON_POSITION,
												TT_Window_Snapshot_Y, NO_ICON_POSITION,
												TT_Window_Snapshot_XS, 0,
												TT_Window_Snapshot_YS, 0,
												TT_Window_Snapshot_Flags, 0,
											TAG_DONE);
										}

										if(arg->icons)
										{
											do_action(NULL, TA_Icon_Unsnapshot,
												TT_Icon_Snapshot_Path, arg->path,
											TAG_DONE);
										}

										REPORT_SUCCESS;
									}
								}
								break;

							/*
							 * Command:
							 *  - Random
							 * Synopsis:
							 *  - Returns a random unsigned 32-bit value
							 * Result:
							 *  - A string containing a 32-bit decimal unsigned value
							 * RC:
							 *  - 0 : Ok
							 */
							case RXCMD_Random:
								{
									TEXT rndbuf[14];

									snprintf(rndbuf, sizeof(rndbuf), "%lu", random_ulong());

									REPORT_RESULT(0, rndbuf);
								}
								break;

							/*
							 * Command:
							 *  - RefreshVars
							 * Synopsis:
							 *  - Re-reads SYS:Prefs/Ambient/vars variables and
							 *    updates them
							 * RC:
							 *  - 0 : Ok
							 */
							case RXCMD_RefreshVars:
								{
									refresh_vars();
									REPORT_SUCCESS;
								}
								break;

							/*
							 * Command:
							 *  - Rename
							 * Synopsis:
							 *  - Rename an icon/file/dir
							 * Parameters:
							 *  - FROM: path to rename from
							 *  - TO: filename to rename to
							 * Result:
							 * RC:
							 *  - 0: Ok
							 */

							case RXCMD_Rename:
								{
									struct RX_Rename *arg = (struct RX_Rename *)array;
									APTR renamewin = NULL;
									ULONG handleicons = FALSE;
									ULONG noicon = FALSE;
									STRPTR * dirlist = NULL;
									LONG count;

									FORWARD_TO_MAINTASK;

									methodstack_push_sync( msg->obj, 3, OM_GET, MA_View_HandleIcons, &handleicons);

									if(!handleicons)
									{
										noicon = TRUE;
									}

									dirlist = build_pathlist(msg->obj, NULL, arg->from, msg->objlist, msg->internal, &count, noicon, FALSE);

									/* If only 1 entry is selected, fallback to standard rename */
									if(dirlist && count <= 1) 
									{
										free_pathlist(dirlist);
										dirlist = NULL;
									}

									/* Use Pattern Rename for multiple files */
									if(dirlist && !arg->viewid)
									{
										if(do_action(obj, TA_File_Rename,
											TT_File_Rename_PathList, dirlist,
											TT_File_Rename_NoIcon, noicon,
										TAG_DONE))
										{
											REPORT_SUCCESS;
										}
										else
										{
											free_pathlist(dirlist);
											REPORT_ERROR(20);
										}
									}
									else if (arg->from && arg->from[0] && !arg->to && !arg->viewid)
									{
										TEXT buffer[PATH_SIZE];

										stccpy(buffer, arg->from[0], sizeof(buffer));

										if(isdevicename(arg->from[0]))
										{
											ULONG len = strlen(buffer);
											if(len > 0)
												buffer[len-1] = 0;
										}

										/* XXX: hm, dummy filetype */
										if (!(renamewin = (APTR)DoMethod(app, MM_Application_FindWindowByName, MV_Window_Type_Rename, arg->from, MA_Renamewin_FileType, MV_Icon_FileType_File, TAG_DONE)))
										{
											if ( (renamewin = NewObject(getrenamewinclass(), NULL,
												MA_Renamewin_Path, arg->from[0],
												MA_Renamewin_Name, FilePart(buffer),
												MA_Renamewin_FileType, MV_Icon_FileType_File,
												MA_Renamewin_NoIcon, noicon,
												TAG_DONE)) )
											{
												DoMethod(app, OM_ADDMEMBER, renamewin);
											}
										}
									}
									else if (arg->from && arg->from[0] && arg->to && !arg->viewid)
									{
										/* XXX: should be cleaner perhaps.. have some window ? */
										if(do_action(obj, TA_File_Rename,
											TT_File_Rename_Path, arg->from[0],
											TT_File_Rename_Name, arg->to,
											TT_File_Rename_NoIcon, noicon,
										TAG_DONE))
										{
											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(20);
										}
									}
									else
									{
										LONG viewid = 0;

										if(arg->viewid)
										{
											viewid = *arg->viewid;

											if (viewid > 0)
											{
												APTR wo;
												wo = (APTR)methodstack_push_sync( app, 2, MM_Application_FindWindowByID, viewid);

												if(wo)
												{
													get(wo, MA_Window_Viewobj, &wo);

													if(wo)
													{
														if (DoMethod(wo, MM_Rexx_Rename))
														{
															REPORT_SUCCESS;
														}
														else
														{
															REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
														}														 
													}
												}
											}
											else
											{
												REPORT_ERROR(20);
											}
										}
										//else if ((msg->internal && msg->obj) && !_ap_bool(popuprename))
										#warning "until inline rename for icons works (see iconclass.c Rexx_Rename method), use RenamePopUpWindow in icon mode"
										else if ((msg->internal && msg->obj) && !_ap_bool(popuprename) && (getv(_view(msg->obj), MA_View_ViewMode) < IVM_ICON))
										{
											if (DoMethod(msg->obj, MM_Rexx_Rename))
											{
												REPORT_SUCCESS;
											}
											else
											{
												REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
											}
										}
										else
										{
											STRPTR name = (STRPTR)getv(msg->obj, MA_Icon_Name);

											#if 1 /* XXX: finish this mess.. */
											if (name && *name && !(renamewin = (APTR)DoMethod(app, MM_Application_FindWindowByName, MV_Window_Type_Rename, getv(msg->obj, MA_Icon_Path), MA_Renamewin_FileType, getv(msg->obj, MA_Icon_FileType), TAG_DONE)))
											{
												if ( (renamewin = NewObject(getrenamewinclass(), NULL,
													MA_Renamewin_Path, getv(msg->obj, MA_Icon_Path),
													MA_Renamewin_Name, name,
													MA_Renamewin_FileType, getv(msg->obj, MA_Icon_FileType),
													MA_Renamewin_NoIcon, noicon,
													MA_Renamewin_IconType, getv(msg->obj, MA_Icon_Type),
													TAG_DONE)) )
												{
													DoMethod(app, OM_ADDMEMBER, renamewin);
												}
											}
											#else
											REPORT_ERROR(AMBIENT_RXERR_NYI);
											#endif
										}
									}

									if (renamewin)
									{
										set(renamewin, MUIA_Window_Open, TRUE);
										REPORT_SUCCESS;
									}
								}
								break;

							/*
							 * Command:
							 *  - Run
							 * Synopsis:
							 *  - Executes a file
							 * Parameters:
							 *  - PATH: path of the file to execute
							 *  - ARG: optional parameters passed to the command
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Couldn't run the file
							 */
							case RXCMD_Run:
								{
									struct RX_Run *arg = (struct RX_Run *)array;

									if (arg->path)
									{
										/* FIXME: Passing the ARG in TT_WBStart_Argument is broken. */
										/* TODO: Parse ARG with ReadItem to TT_WBStart_ArgumentList. - Piru */

										if ( msg->sync )
										{
											if (do_action_sync(obj, TA_WBStart,
												TT_WBStart_Path, arg->path,
												TT_WBStart_Argument, ( arg->arg && *arg->arg ) ? arg->arg : NULL,
											TAG_DONE))
											{
												REPORT_SUCCESS;
											}
											else
											{
												REPORT_ERROR(20);
											}
										}
										else
										{
											if (do_action(obj, TA_WBStart,
												TT_WBStart_Path, name_build(arg->path),
												TT_WBStart_Argument, ( arg->arg && *arg->arg ) ? name_build(arg->arg) : NULL,
												TT_WBStart_FreeNames, TRUE,
											TAG_DONE))
											{
												REPORT_SUCCESS; /* XXX: wrong.. it's async.. but do we care ? */
											}
											else
											{
												REPORT_ERROR(20);
											}
										}
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_NYI);
									}
								}
								break;

							/*
							 * Command:
							 *  - ScreenToBack
							 * Synopsis:
							 *  - Put Ambient's screen to back
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Ambient's window is closed
							 */
							case RXCMD_ScreenToBack:
								{
									GETDATA;

									if (data->rootwin && getv(data->rootwin, MUIA_Window_Open))
									{
										DoMethod(data->rootwin, MUIM_Window_ScreenToBack);
										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 * Command:
							 *  - ScreenToFront
							 * Synopsis:
							 *  - Put Ambient's screen to front
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Ambient's window is closed
							 */
							case RXCMD_ScreenToFront:
								{
									GETDATA;

									if (data->rootwin && getv(data->rootwin, MUIA_Window_Open))
									{
										DoMethod(data->rootwin, MUIM_Window_ScreenToFront);
										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 * Command:
							 *  - Select
							 * Synopsis:
							 *  - Selects objects. If no parameter is supplied, it inverts the
							 *    current selection.
							 * Parameters:
							 *  - ALL/S: selects all the objects
							 *  - NONE/S: unselects all the objects
							 *  - PATTERN/S: popups a selection window for pattern
							 * RC:
							 *  - 0: Ok
							 */

							case RXCMD_Select:
								{
									struct RX_Select *arg = (struct RX_Select *)array;

									APTR viewobj = NULL;

									FORWARD_TO_MAINTASK;

									if ( arg->viewid && *arg->viewid > 0 )
									{
										APTR wo = (APTR)methodstack_push_sync( app, 2, MM_Application_FindWindowByID, *arg->viewid );

										if ( wo )
										{
											methodstack_push_sync( wo,3, OM_GET, MA_Window_Viewobj, &viewobj );
										}
									}
									else if (msg->internal && msg->obj)
									{
										viewobj = _view(msg->obj);
									}

									if (viewobj)
									{
										ULONG selectmode;

										if(arg->pattern)
										{
											APTR selectwin = NULL;

											if ( (selectwin = NewObject(getselectwinclass(), NULL,
																	  /*MA_Selectwin_Pattern, "*", */
																	  MA_Viewgroup_ID, getv(viewobj, MA_Viewgroup_ID),
																	  TAG_DONE)) )
											{
												DoMethod(app, OM_ADDMEMBER, selectwin);
											}

											if (selectwin)
											{
												set(selectwin, MUIA_Window_Open, TRUE);

												REPORT_SUCCESS;
											}
											else
											{
												REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
											}
										}
										else
										{
											if (arg->all && arg->none)
											{
												REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
											}
											else
											{
												STRPTR name = NULL;

												if (arg->all)
												{
													selectmode = MV_View_Select_All;
												}
												else if (arg->none)
												{
													selectmode = MV_View_Select_None;
												}
												else if (arg->name && arg->name[0])
												{
													selectmode = MV_View_Select_Name;
													name = arg->name;
												}
												else
												{
													selectmode = MV_View_Select_Invert;
												}

												DoMethod(viewobj, MM_View_Select, selectmode, name);

												REPORT_SUCCESS; /* XXX: I think.. how could that fail anyway.. */
											}
										}
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY); /* XXX: I should invent new errors */
									}
								}
								break;

							/*
							 * Command:
							 *  - Settings
							 * Synopsis:
							 *  - Opens the settings window
							 * Parameters:
							 *  - PAGE: Optional name of the page to select, Ambient pages
							 *          are "INTERNAL name" (for example "INTERNAL Backgrounds", system pages are "name"
							 *          (for example "AHI") or "SYSTEM" for the system page only. For reasons of
							 *          script interoperability with different locales, please always use english
							 *          strings here. Case sensitivity for system settings is important, so use
							 *          "Serial" and not "serial".
							 * Example:
							 *  - Settings Serial
							 *  - Settings "INTERNAL Backgrounds"
							 * RC:
							 *  - 0: Ok
							 */
							case RXCMD_Settings:
								{
									struct RX_Settings *arg = (struct RX_Settings *)array;

									if ((arg->page && !strnicmp("INTERNAL ", arg->page, 9)) || !arg->page)
									{
										if (!prefswin)
										{
											if ( (prefswin = NewObject(getprefswin_mainclass(), NULL, TAG_DONE)) )
											{
												DoMethod(app, OM_ADDMEMBER, prefswin);
											}
										}

										if (prefswin)
										{
											STRPTR p;

											if ( arg->page && (p = strchr(arg->page, ' ')) )
											{
												while (*p == ' ') p++;

												if (*p)
												{
													/* XXX: trailing spaces will fuck up.. who cares.. */
													DoMethod(prefswin, MM_Prefswin_Main_SetPage, p);
												}
											}

											set(prefswin, MUIA_Window_Open, TRUE);

											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
										}
									}
									else if (arg->page)
									{
										ULONG anypage;
										//TEXT buf[PATH_SIZE];
										STRPTR buf=NULL; // bitRocky
										APTR w;

										if (!stricmp("SYSTEM", arg->page))
										{
											anypage = TRUE;
										}
										else
										{
											anypage = FALSE;
										}

										w = dosreq_disable();

										if (!anypage)
										{
											ULONG len=0;
											//PDB(("arg->page = '%s'\n", arg->page));
											// bitRocky: allocate a buffer for the string, instead of using a local buffer, because TA_WBStart call is aynchron here!
											NewRawDoFmt("MOSSYS:Prefs/mprefs/%s.mprefs", (APTR)RAWFMTFUNC_COUNT, (STRPTR)&len, arg->page); //bitRocky: removed the quotes, it caused trouble in v_wbstart()
											if ((len > 0) && (buf = malloc(len)))
											{
												NewRawDoFmt("MOSSYS:Prefs/mprefs/%s.mprefs", (APTR)RAWFMTFUNC_STRING, buf, arg->page);
											}
											else // set anypage to TRUE if allocation the buffer failed
											{
												anypage = TRUE;
											}
											PDB(("len = %ld, buf = '%s'\n", len, buf ? buf : "NULL"));
										}

										if (do_action(obj, TA_WBStart,
											TT_WBStart_Path, "MOSSYS:Prefs/Preferences",
											anypage ? TAG_IGNORE : TT_WBStart_Argument, buf, // bitRocky: will be free()d in tr_wbstart()!
											anypage ? TAG_IGNORE : TT_WBStart_FreeNames, TRUE, // bitRocky: free() "buf" argument!
										TAG_DONE))
										{
											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
										}

										dosreq_enable(w);
									}
								}
								break;

							/*
							 * Command:
							 *  - Shortcut
							 * Synopsis:
							 *  - Adds/Removes desktop shortcuts
							 * Parameters:
							 *  - PATH: path of the shortcut to add/remove. If not supplied, path is taken from
							 *          the selected object
							 *  - ADD/S: add a shortcut. This parameter is mutually exclusive
							 *  - REMOVE/S: remove a shortcut. This parameter is mutually exclusive
							 * Result:
							 * RC:
							 *  - 0 : Ok
							 *  - 10: Shortcut doesn't exist
							 *  - 20: No path supplied
							 * XXX:
							 *  - fix those RCs.. all above too
							 *  - implement some WAIT/S for retcode?
							 *  - mention that default is 'ADD' above
							 */
							case RXCMD_Shortcut:
								{
									#if USE_SHORTCUTS
									struct RX_Shortcut *arg = (struct RX_Shortcut *)array;
									STRPTR p = NULL;
									TEXT device[ 64 ];

									FORWARD_TO_MAINTASK;

									if (arg->add + arg->remove > 1)
									{
										REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
										break;
									}

									if (/*!msg->internal &&*/ arg->path)
									{
										p = arg->path;
									}
									else if (msg->internal && msg->obj && arg->add)
									{
										p = (STRPTR)getv(msg->obj, MA_Icon_Path);

										/*
										 * XXX: Because of overal deficons mess, some icons get different (volume vs device)
										 * names for _Path and _Name. This confuses name_build_info() and other functions.
										 * This workaround seems to fix few problems, especialy with Ram Disk.
										 * NOTE: Will this case even happen??
										 */

										if (isdevicename( p ))
										{
											strcpy( device, (STRPTR)getv(msg->obj, MA_Icon_Name) );
											strcat( device, ":" );
											p = device;
										}
									}
									else if (msg->internal && (msg->obj != NULL || msg->objlist != NULL) && arg->remove)
									{
										/*
										 * Avoid using lookup to ensure we remove the icon we really want.
										 * Delay icon deletion so that its context menu returns correctly.
										 */

										if (msg->objlist != NULL)
										{
											LONG ind = 0;
											while (msg->objlist[ind])
											{
												DoMethod(obj, MUIM_Application_PushMethod, obj, 2 | MUIV_PushMethod_Delay(100), MM_Application_RemoveShortcut, msg->objlist[ind]);
												ind++;
											}
										}
										else
										{
											DoMethod(obj, MUIM_Application_PushMethod, obj, 2 | MUIV_PushMethod_Delay(100), MM_Application_RemoveShortcut, msg->obj);
										}

										REPORT_SUCCESS;
									}

									if (p)
									{
										STRPTR q;

										if ( (q = name_build_info(p)) )
										{
											GETDATA;

											if (arg->remove)
											{
												APTR o;

												ASSERT(data->rootwin);

												if ( (o = (APTR)DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_FindShortcut, q)) ) /* XXX: replace that NULL by some iconview ID/string */
												{
													/* Delay icon deletion so that its context menu returns correctly */
													DoMethod(obj, MUIM_Application_PushMethod, obj, 2 | MUIV_PushMethod_Delay(100), MM_Application_RemoveShortcut, o);

													REPORT_SUCCESS;
												}
												else
												{
													REPORT_ERROR(10);
												}
											}
											else
											{
												if (DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_AddShortcut, p))
												{
													REPORT_SUCCESS;
												}
												else
												{
													REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
												}
											}
											name_delete(q);
										}
										else
										{
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
										}
									}
									else
									{
										REPORT_ERROR(20);
									}
									#else
									REPORT_ERROR(AMBIENT_RXERR_NYI);
									#endif
								}
								break;

							/*
							 * Command:
							 *  - Sort
							 * Synopsis:
							 *  - Changes the sorting order of an iconview
							 * Parameters:
							 *  - PATH: path of the directory/lister to sort icons/entries from, if not supplied, path
							 *         is taken from the selected object
							 *  - ALPHA/S: sort in alphabetical order
							 *  - SIZE/S: sort by size
							 *  - TYPE/S: sort by type
							 *  - DATE/S: sort by date
							 *  - REVERSE/S: reverse the sorting order
							 * Result:
							 * RC:
							 *  - 0: Ok
							 * XXX: hm.. perhaps we should allow multiple sorting modes and process them all in order ?
							 */
							case RXCMD_Sort:
								{
									struct RX_Sort *arg = (struct RX_Sort *)array;
									ULONG mode = 0;

									if (arg->alpha)
									{
										mode = IVS_NAME;
									}
									else if (arg->size)
									{
										mode = IVS_SIZE;
									}
									else if (arg->type)
									{
										mode = IVS_TYPE;
									}
									else if (arg->date)
									{
										mode = IVS_DATE;
									}
									#ifdef DEBUG
									else
									{
										PDB(("error, not defined\n"));
									}
									#endif

									if (mode && msg->internal && msg->obj) /* XXX */
									{
										if (DoMethod(msg->obj, MM_Iconview_DoSort, mode, arg->reverse ? IVSO_DECREMENTAL : IVSO_INCREMENTAL))
										{
											REPORT_SUCCESS;
										}
										/* XXX: erm.. */
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_NYI);
									}
								}
								break;

							case RXCMD_TBH: /* The single greatest feature in Ambient IMO. - piru */
								{
									/* tbh stuff by Bart King <bart@bart666.com> */
									#define TBH_MAX_WORDS 7
									#define TBH_SMILEY_RATIO 3 /* 1 : SMILEY_RATIO chance of appearing */
									#define TBH_BUF_SIZE 256

									static const CONST_STRPTR tbh_words[] = {
										"tbh", /* to be honest */
										"fs",  /* fucks sake */
										"omfg", /* oh my fucking god */
										"stfu", /* shut the fuck up */
										"ffs", /* for fucks sake */
										"wtf", /* what the fuck */
										"omg", /* oh my god */
										"tbfh", /* to be fucking honest */
										"nfw", /* no fucking way */
										"plz", /* please */
										"n00b", /* newbie */
										"hax", /* hacks */
										"lol", /* laugh out loud */
										NULL
									};

									static const CONST_STRPTR tbh_smilies[] = {
										":D",
										":P",
										":/",
										":)",
										NULL
									};

									STRPTR tbh_buf;

									if ( (tbh_buf = malloc(TBH_BUF_SIZE)) )
									{
										ULONG num_words, num_smilies;

										for (num_words = 0; tbh_words[num_words]; num_words++);
										for (num_smilies = 0; tbh_smilies[num_smilies]; num_smilies++);

										if (num_words)
										{
											ULONG len;
											ULONG i;
											CONST_STRPTR w;
											CONST_STRPTR lastw = NULL;
											ULONG charlen = 0;

											len = random_ulong() % TBH_MAX_WORDS + 1;

											tbh_buf[0] = '\0';

											for (i = 0, charlen = 0; i < len; i++)
											{
												w = tbh_words[random_ulong() % num_words];

												if (w == lastw) continue; /* don't use the same word as before */

												charlen += strlen(w) + 1; /* space */

												if (charlen + 1 > TBH_BUF_SIZE) break;

												if (lastw) strcat(tbh_buf, " ");

												strcat(tbh_buf, w);
												lastw = w;
											}

											/* add a smiley ? */
											if (num_smilies && !(random_ulong() % TBH_SMILEY_RATIO))
											{
												strcat(tbh_buf, " ");
												strcat(tbh_buf, tbh_smilies[random_ulong() % num_smilies]);
											}
											REPORT_RESULT(0, tbh_buf);
										}
										else
										{
											REPORT_ERROR(AMBIENT_RXERR_BADDEFINITION);
										}
										free(tbh_buf);
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
									}
								}
								break;

							/*
							 * Command:
							 *  - Version
							 * Synopsis:
							 *  - Returns the version and the revision.
							 * Result:
							 *  - A string containing the version, a dot and the revision.
							 * RC:
							 *  - 0 : Ok
							 */
							case RXCMD_Version:
								{
									TEXT verbuf[16];

									snprintf(verbuf, sizeof(verbuf), "%lu.%lu", (ULONG)VERSION, (ULONG)REVISION);

									REPORT_RESULT(0, verbuf);
								}
								break;

							/*
							 * Command:
							 *  - Viewmode
							 * Synopsis:
							 *  - Changes the visual presentation of a view.
							 * Parameters:
							 *  - MODE: name of the mode
							 *  - VIEWID: id of calling view (needed when called from actiondispatcher)
							 * Example:
							 *  - Viewmode "ICON ALL"
							 * Result:
							 * RC:
							 *  - 0: Ok
							 * XXX: provide a result perhaps. especially when the mode cannot be applied (mimetype not supporting it or so)
							 */
							case RXCMD_Viewmode:
								{
									struct RX_Viewmode *arg = (struct RX_Viewmode *)array;
									APTR viewobj = NULL;

									FORWARD_TO_MAINTASK;

									if (arg->mode)
									{
										/* use viewid if supplied */
										if(arg->viewid && *arg->viewid > 0)
										{
											APTR wo = (APTR) DoMethod(app, MM_Application_FindWindowByID, *arg->viewid );
											viewobj = (APTR) getv(wo, MA_Window_Viewobj);
										}
										else if (msg->internal && msg->obj)
										{
											viewobj = _view(msg->obj);
										}

										if(viewobj && !getv(viewobj, MA_View_IsRoot)) /* better filter rootview */
										{
											struct viewnode *vn;
											STRPTR p;
											TEXT buf[64];
											TEXT newmode[64]="";

											stccpy(buf, arg->mode, sizeof(buf));

											if ((p = strchr(buf, ' ')))
											{
												*p = 0;
											}
											/* if mode or submode are set to "next", then we will cycle to next mode/submode */
											if(!stricmp(buf, "next") || (p && *(p+1) && !stricmp(p+1, "next")))
											{
												vn = viewapi_findbyid( getv( viewobj, MA_Viewgroup_ViewIndex ) );

												if(vn)
												{
													/* get next suitable mode for this view */
													if(!stricmp(buf, "next"))
													{
														vn = viewapi_nextviewmime(vn, (STRPTR) getv(viewobj, MA_View_MIME));
													}

													if(vn == NULL)
													{
														vn = viewapi_nextviewmime(NULL, (STRPTR) getv(viewobj, MA_View_MIME));
													}

													if(vn)
													{
														strncat(newmode, vn->name, sizeof(newmode));

														if(p && *(++p))
														{
															ULONG mode;
															STRPTR m = NULL;

															mode = getv( viewobj, MA_Viewgroup_ViewModeIndex );

															/* get next submode for this view */
															if(!stricmp(p, "next"))
															{
																m = viewapi_getmodename(vn, mode+1);
															}

															if(m == NULL || *m == 0)
															{
																if(!stricmp(p, "next"))
																{
																	mode = 0;
																}
																m = viewapi_getmodename(vn, mode);
															}


															if(m && *m)
															{
																strncat(newmode, " ", sizeof(newmode));
																strncat(newmode, m, sizeof(newmode));
															}
														}

														DoMethod(viewobj, MM_Viewgroup_ChangeView, newmode);
													}
												}
											}
											else
											{
												DoMethod(viewobj, MM_Viewgroup_ChangeView, arg->mode);
											}

											REPORT_SUCCESS; /* XXX: hm :) */
										}
										else
										{
											REPORT_ERROR(20);
										}
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
									}
								}
								break;

							/*
							 * Command:
							 *  - Move
							 * Synopsis:
							 *  - Move specified files to different path
							 * Parameters:
							 *  - TO - destination path
							 *  - FROM - files to be moved. Separated by spaces
							 *  - VIEWID - window id on which we are operating (for progress/blocking)
							 * Example:
							 *  - Move "ram:" "disk:some/file.txt" "disk:some/file2.txt"
							 * Result:
							 * RC:
							 *  - 0: Ok
							 */

							/*
							 * Command:
							 *  - Copy
							 * Synopsis:
							 *  - Copy specified files to different path
							 * Parameters:
							 *  - TO - destination path
							 *  - FROM - files to be copied. Separated by spaces
							 *  - VIEWID - window id on which we are operating (for progress/blocking)
							 * Example:
							 *  - Copy "ram:" "disk:some/file.txt" "disk:some/file2.txt"
							 * Result:
							 * RC:
							 *  - 0: Ok
							 */
							case RXCMD_DropFile:
							case RXCMD_Move:
							case RXCMD_Copy:
								{
									struct RX_Copy *arg = (struct RX_Copy *)array;
									APTR wo = NULL;
									struct MinList * ml;
									//ULONG res = TRUE; // bitRocky: its not used
									ULONG copy = TRUE;
									ULONG handleicons = FALSE;
									ULONG noicon = FALSE;
									ULONG rename = FALSE;

									if ( (ml = malloc(sizeof(*ml))) )
									{
										struct dragdropnode *ddn, *nextddn;
										LONG success;

										/* ml is freed by tr_movefile, unless do_action fails */
										NEWLIST(ml);

										if(arg->from && *arg->from)
										{
											STRPTR *filelist = arg->from;
											int i=0;

											while(filelist[i])
											{
												int len = strlen(filelist[i]) + 1;

												ddn	= (struct dragdropnode *) malloc(sizeof(*ddn)+ len);

												if(ddn)
												{
													strcpy(ddn->path, filelist[i]);
													ddn->type = MV_Icon_FileType_Directory; /* doesn't matter, if it's a file, movefile will be called later */

													ADDTAIL(ml, ddn);
												}/*
												else
												{
													res = FALSE;
												}*/

												i++;
											}

											switch(cmd->id)
											{
												case RXCMD_Move:
													copy = FALSE;
													rename = arg->rename;
													break;

												case RXCMD_Copy:
													copy = TRUE;
													rename = arg->rename;
													break;

												case RXCMD_DropFile:
												{
													struct RX_DropFile * arg2 = (struct RX_DropFile *) array;

													if(same_volume(filelist[0], arg->to))
														copy = FALSE;
													else
														copy = TRUE;

													if (arg2->invert)
													{
														copy = copy ? FALSE : TRUE;
													}

													rename = arg2->rename;
													break;
												}
											}
										}

										if ( arg->viewid && *arg->viewid > 0 )
										{
											wo = (APTR)methodstack_push_sync( app, 2, MM_Application_FindWindowByID, *arg->viewid );
										}

										DB(("Copy:<%s, ...> -> <%s> (sync = %s) rename = %d\n", *arg->from, arg->to, msg->sync ? "Yes" : "No", arg->rename ));

										methodstack_push_sync( msg->obj, 3, OM_GET, MA_View_HandleIcons, &handleicons);

										if(!handleicons)
										{
											noicon = TRUE;
										}

										if( msg->sync )
										{
											success = do_action_sync(NULL, TA_File_Move,
												TT_File_Move_SrcList, ml,
												TT_File_Move_DstPath, arg->to,
												TT_File_Move_Refwin, wo,
												TT_File_Move_Copy, copy,
												TT_File_Move_NoIcon, noicon,
												TT_File_Move_Rename, rename,
											TAG_DONE);
										}
										else
										{
											success = do_action(NULL, TA_File_Move,
												TT_File_Move_SrcList, ml,
												TT_File_Move_DstPath, arg->to,
												TT_File_Move_Refwin, wo,
												TT_File_Move_Copy, copy,
												TT_File_Move_NoIcon, noicon,
												TT_File_Move_Rename, rename,
											TAG_DONE);
										}

										if (!success)
										{
											ITERATELISTSAFE(ddn, nextddn, ml)
											{
												free(ddn);
											}
											free(ml);
										}
									}

									REPORT_SUCCESS;
								}
								break;

							/*
							 * Command:
							 *  - ClipboardCut/Copy/Add
							 * Synopsis:
							 *  - Cut/Copy/Add them to clipboard for cut. If not taken, use view selected files.
							 * Parameters:
							 *  - PATH - files to be cut/copied/added
							 * Example:
							 *  - ClipboardCut/Copy/Add "ram:t" "disk:some/file.txt" "disk:some/file2.txt"
							 * Result:
							 * RC:
							 *  - 0: Ok
							 */
							case RXCMD_ClipboardAdd:
							case RXCMD_ClipboardCut:
							case RXCMD_ClipboardCopy:
								{
									struct RX_ClipboardCopy *arg = (struct RX_ClipboardCopy *)array; /* structs are same for cut/copy/add */
									ULONG handleicons = FALSE;

									FORWARD_TO_MAINTASK;

									if(msg->obj)
									{
										handleicons = getv(msg->obj, MA_View_HandleIcons);
									}

									switch(cmd->id)
									{
										case RXCMD_ClipboardCut:
											clipboard_clear();
											clipboard_set_mode(CLIPBOARD_CUT, handleicons);
											break;

										case RXCMD_ClipboardCopy:
											clipboard_clear();
											clipboard_set_mode(CLIPBOARD_COPY, handleicons);
											break;

										case RXCMD_ClipboardAdd:
											if(clipboard_get_mode() == CLIPBOARD_VOID)
											{
												clipboard_set_mode(CLIPBOARD_COPY, handleicons);
											}
											break;
									}

									/* get selection... */

									/* can be obtained from arguments... */
									if(arg->path && *arg->path) /* rexx command, use arg */
									{
										STRPTR *dirlist = arg->path;

										if (is_valid_pathlist(dirlist))
										{
											ULONG i;

											for (i = 0; dirlist[i]; i++)
											{
												clipboard_add((STRPTR)dirlist[i]);
											}
										}
										else
										{
											REPORT_ERROR(20);
											break;
										}
									} /* or from selected objects */
									else if(msg->internal && msg->obj) /* internal command */
									{
										struct MinList ml;
										struct dragdropnode *ddn, *nextddn;

										NEWLIST(&ml);
										DoMethod(_view(msg->obj), MM_View_GetSelectionList, &ml);

										if(!ISLISTEMPTY(&ml)) /* we come from view menu or group menu */
										{
											ITERATELISTSAFE(ddn, nextddn, &ml)
											{
												clipboard_add(ddn->path);
												free(ddn);
											}
										}
										else  /* no selection -> an unselected icon with popup menu */
										{
											if (strcmp(OCLASS(msg->obj)->cl_ID, "iconviewclass")
											&&  strcmp(OCLASS(msg->obj)->cl_ID, "listviewclass")) /* XXX : megalame. We should probably just return NULL when getting MA_Icon_Path on iconview) */
											{
												clipboard_add((STRPTR)getv(msg->obj, MA_Icon_Path));
											}
										}
									}

									REPORT_SUCCESS;
								}
								break;
							/*
							 * Command:
							 *  - ClipboardPaste
							 * Synopsis:
							 *  - Copy specified files to different path
							 * Parameters:
							 *  - TO - destination path
							 *  - VIEWID - window id on which we are operating (for progress/blocking)
							 *  - RENAME - new name for each entry in clipboard will be asked (paste as)
							 * Example:
							 *  - ClipboardPaste TO "ram:" VIEWID 4
							 * Result:
							 * RC:
							 *  -  0: Ok
							 *  - 20: View or destination not found
							 */
							case RXCMD_ClipboardPaste:
								{
									struct RX_ClipboardPaste *arg = (struct RX_ClipboardPaste *)array;
									STRPTR dest = NULL;
									LONG viewid = 0;

									FORWARD_TO_MAINTASK;

									if(arg->viewid)
									{
										viewid = *arg->viewid;

										/* if destination is set, paste into it */
										if(arg->to && arg->to[0])
										{
											dest = arg->to;
										}
										else if (viewid > 0)
										{
											APTR wo;
											wo = (APTR)methodstack_push_sync( app, 2, MM_Application_FindWindowByID, viewid);

											if(wo)
											{
												get(wo, MA_Window_Viewobj, &wo);

												if(wo)
												{
													dest = (STRPTR) getv(wo, MA_View_Path);
												}
											}
										}
									}
									else if (msg->internal && msg->obj)
									{
										ULONG dummy;

										if (!GetAttr(MA_View_IsViewObject, msg->obj, (ULONG *)&dummy)) /* GetAttr() returns FALSE if tag not found */
										{
											dest = (STRPTR)getv(msg->obj, MA_Icon_Path);
										}
										else
										{
											dest = (STRPTR)getv(msg->obj, MA_View_Path);
										}

										viewid = getv(_view(msg->obj), MA_Viewgroup_ID);
									}

									if (dest == NULL || !strchr(dest, ':'))
									{
										REPORT_ERROR(20);
									}
									else
									{
										do_action(NULL, TA_Clipboard_Paste,
											TT_Clipboard_Paste_DestPath, dest,
											TT_Clipboard_Paste_ViewID, viewid,
											TT_Clipboard_Paste_As, arg->rename,
										TAG_DONE);

										REPORT_SUCCESS;
									}
								}
								break;
							/*
							 *  Command:
							 *  - ViewList
							 * Synopsis:
							 *  - return view list into a stem
							 * Parameters:
							 *  - STEM: stem name to store the result to
							 * Example:
							 *  - ViewList views
							 * Result:
							 *  - A stem var with the following fields:
							 *    - stem.n: <id,name> pair
							 *    - stem.count: list count
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Syntax Error
							 */
							case RXCMD_ViewList:
								{
									struct RX_ViewList *arg = (struct RX_ViewList *)array;

									if (arg->stem)
									{
										int i = 0;
										TEXT member[6];
										TEXT item[256];

										FORCHILD(app, MUIA_Application_WindowList)
										{
											if (getv(child, MA_Window_Type) == MV_Window_Type_View)
											{
												snprintf(member, sizeof(member), "%d", i);
												snprintf(item, sizeof(item), "%ld:%s", getv( child, MA_Window_ID ),  (STRPTR) getv( child, MA_Window_Path ) );

												setvar_stem(msg->id, arg->stem, member, item);

												i++;
											}
										}
										NEXTCHILD

										snprintf(item, sizeof(item), "%d", i);
										setvar_stem(msg->id, arg->stem, "count", item);

										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
									}
								}
								break;
							/*
							 *  Command:
							 *  - GetSelectedNames
							 * Synopsis:
							 *  - return current view selection in a stem
							 * Parameters:
							 *  - STEM: stem name to store the result to
							 * Example:
							 *  - GetSelectedNames files
							 * Result:
							 *  - A stem var with the following fields:
							 *    - stem.n: filename
							 *    - stem.count: list count
							 * RC:
							 *  - 0 : Ok
							 *  - 20: No files selected
							 */
							case RXCMD_GetSelectedNames:
								{
									struct RX_GetSelectedNames *arg = (struct RX_GetSelectedNames *)array;
									int i = 0;
									TEXT member[6];
									TEXT item[256];
									APTR wo = NULL;

									if(arg->viewid && *arg->viewid) /* get arg view */
									{
										wo = (APTR) methodstack_push_sync( app, 2, MM_Application_FindWindowByID, *arg->viewid );

										if(wo)
										{
											get(wo, MA_Window_Viewobj, &wo);
										}
									}
									else /* get current active view */
									{
										FORCHILD(app, MUIA_Application_WindowList)
										{
											if (getv(child, MUIA_Window_Activate) && getv(child, MA_Window_Type) == MV_Window_Type_View)
											{
												get(child, MA_Window_Viewobj, &wo);
												break;
											}
										}
										NEXTCHILD
									}


									if(wo)
									{
										struct MinList ml;
										struct dragdropnode *ddn, *nextddn;

										NEWLIST(&ml);
										DoMethod(wo, MM_View_GetSelectionList, &ml);

										ITERATELISTSAFE(ddn, nextddn, &ml)
										{
											if(arg->stem)
											{
												snprintf(member, sizeof(member), "%d", i);
												snprintf(item, sizeof(item), "%s", ddn->path );

												setvar_stem(msg->id, arg->stem, member, item);
											}
											i++;

											free(ddn);
										}
									}

									if(arg->stem)
									{
										snprintf(item, sizeof(item), "%d", i);
										setvar_stem(msg->id, arg->stem, "count", item);
										REPORT_SUCCESS;
									}
									else /* internal mode, we'll use retval to check if selection count is != 0 */
									{
										if(i>0)
										{
											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(20);
										}
									}
								}
								break;

							/*
							 *  Command:
							 *  - LoadBackground
							 * Synopsis:
							 *  - load a background
							 * Parameters:
							 *  - PATH,MODE/K,TYPE/K
							 * Example:
							 *  - LoadBackground ram:foo.png type=desktop mode=centered
							 * Result:
							 *
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Error
							 */
							case RXCMD_LoadBackground:
								{
									struct RX_LoadBackground *arg = (struct RX_LoadBackground *)array;

									ULONG mode = BGRENDER_Scaled;
									ULONG type = TV_Background_Load_Type_Root;

									if(arg->mode)
									{
										if(!stricmp("tiled", arg->mode))
											mode = BGRENDER_Tiled;

										if(!stricmp("centered", arg->mode))
											mode = BGRENDER_Centered;

										if(!stricmp("scaled", arg->mode))
											mode = BGRENDER_Scaled;

										if(!stricmp("zoomed", arg->mode))
											mode = BGRENDER_Zoomed;
									}

									if(arg->type)
									{
										if(!stricmp("window", arg->type))
											type = TV_Background_Load_Type_Window;

										if(!stricmp("desktop", arg->type))
											type = TV_Background_Load_Type_Root;
									}

									if(arg->path)
									{
										if(type == TV_Background_Load_Type_Root)
										{
											setprefsstr(DSI_BACKGROUND_ROOT, arg->path);
											setprefslong(DSI_BACKGROUND_ROOT_BGRENDER, mode);

											DoMethod(app, MM_Application_LoadBackground, MF_Application_LoadBackground_ClearRoot | MF_Application_LoadBackground_Root);
										}

										if(type == TV_Background_Load_Type_Window)
										{
											setprefsstr(DSI_BACKGROUND_WINDOW, arg->path);
											setprefslong(DSI_BACKGROUND_WINDOW_BGRENDER, mode);

											DoMethod(app, MM_Application_LoadBackground, MF_Application_LoadBackground_ClearWindow | MF_Application_LoadBackground_Window);
										}

										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 *  Command:
							 *  - ClickPathParent
							 * Synopsis:
							 *  - Go to parent path
							 * Parameters:
							 *  - VIEWID/N
							 * Example:
							 *  - ClickPathParent VIEWID 2
							 * Result:
							 *
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Error
							 */
							case RXCMD_Parent:
								{
									struct RX_Parent *arg = (struct RX_Parent *)array;

									if(arg->viewid && *arg->viewid && (*arg->viewid != 1))
									{
										APTR wo = NULL;

										wo = (APTR) methodstack_push_sync( app, 2, MM_Application_FindWindowByID, *arg->viewid );

										if(wo)
										{
											methodstack_push_sync(wo, 1, MM_Clickpath_Parent);
											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(20);
										}

									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 *  Command:
							 *  - ClickPathScrollForward
							 * Synopsis:
							 *  - Go to parent path
							 * Parameters:
							 *  - VIEWID/N
							 * Example:
							 *  - ClickPathScrollForward VIEWID 2
							 * Result:
							 *
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Error
							 */
							case RXCMD_HistoryPrev:
								{
									struct RX_HistoryPrev *arg = (struct RX_HistoryPrev *)array;

									if(arg->viewid && *arg->viewid && (*arg->viewid != 1))
									{
										APTR wo = NULL;
										wo = (APTR) methodstack_push_sync( app, 2, MM_Application_FindWindowByID, *arg->viewid );

										if(wo)
										{
											methodstack_push_sync(wo, 1, MM_Toolbar_HistoryPrev);
											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(20);
										}
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 *  Command:
							 *  - ClickPathScrollBack
							 * Synopsis:
							 *  - Go to parent path
							 * Parameters:
							 *  - VIEWID/N
							 * Example:
							 *  - ClickPathScrollForward VIEWID 2
							 * Result:
							 *
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Error
							 */
							case RXCMD_HistoryNext:
								{
									struct RX_HistoryNext *arg = (struct RX_HistoryNext *)array;

									if(arg->viewid && *arg->viewid && (*arg->viewid != 1))
									{
										APTR wo = NULL;
										wo = (APTR) methodstack_push_sync( app, 2, MM_Application_FindWindowByID, *arg->viewid );

										if(wo)
										{
											methodstack_push_sync(wo, 1, MM_Toolbar_HistoryNext);
											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(20);
										}
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 *  Command:
							 *  - Find
							 * Synopsis:
							 *  - Looks for file matching certain criteria
							 * Parameters:
							 *  - LOCATION: location (can be multiple paths) to be searched. When ommited all volumes are searched.
							 *  - NAME: look for file names matching it
							 *  - TEXT: look for files containing this phrase
							 * Example:
							 *  - Find LOCATION Sys: NAME "foo#?" TEXT "bar"
							 * Result:
							 *
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Error
							 */
							case RXCMD_Find:
								{
									struct RX_Find *arg = (struct RX_Find *)array;
									APTR findwin;

									FORWARD_TO_MAINTASK;

									findwin = NewObject(getfindwinclass(), NULL, TAG_DONE);
									if (findwin != NULL)
									{
										DoMethod(app, OM_ADDMEMBER, findwin);

										if(arg->location != NULL && arg->location[0] != NULL && *arg->location[0])
										{
											LONG i = 0;
											while(arg->location[i] != NULL)
											{
												DoMethod(findwin, MM_Find_AddLocation, arg->location[i]);
												i++;
											}
										}
										else
										{
											DoMethod(findwin, MM_Find_AddLocation, "[VOLUMES]");
										}

										SetAttrs(findwin,
											MA_Find_Pattern, arg->name,
											MA_Find_Text, arg->text,
											MUIA_Window_Open, TRUE,
											TAG_DONE
										);

										if (arg->execute)
										{
											DoMethod(findwin, MM_Find_Start);
										}

									}

									if (findwin)
									{
										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 *  Command:
							 *  - ViewClose
							 * Synopsis:
							 *  - Closes given view (root can't be closed)
							 * Parameters:
							 *  - VIEWID/N
							 * Example:
							 *  - ViewClose VIEWID 2
							 * Result:
							 *
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Error
							 */
							case RXCMD_ViewClose:
								{
									struct RX_ViewClose *arg = (struct RX_ViewClose *)array;

									if(arg->viewid && *arg->viewid && (*arg->viewid != 1))
									{
										APTR wo = NULL;

										wo = (APTR) methodstack_push_sync( app, 2, MM_Application_FindWindowByID, *arg->viewid );

										if(wo)
										{
											methodstack_push_sync(wo, 2, MM_Window_Close, FALSE);
											REPORT_SUCCESS;
										}
										else
										{
											REPORT_ERROR(20);
										}

									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 *  Command:
							 *  - CopySelectionToClipboard
							 * Synopsis:
							 *  - Closes given view (root can't be closed)
							 * Parameters:
							 *  - VIEWID/N
							 *  - FULLPATH/S
							 * Example:
							 *  - CopySelectionToClipboard VIEWID 2 FULLPATH
							 * Result:
							 *
							 * RC:
							 *  - 0 : Ok
							 *  - 5 : No view selected
							 *  - 10: Clipboard error
							 *  - 20: Error
							 */
							case RXCMD_CopySelectionToClipboard:
								{
									struct RX_CopySelectionToClipboard *arg = (struct RX_CopySelectionToClipboard *)array;
									APTR wo = NULL;

									FORWARD_TO_MAINTASK;

									if(arg->viewid && *arg->viewid) /* get arg view */
									{
										wo = (APTR) methodstack_push_sync( app, 2, MM_Application_FindWindowByID, *arg->viewid );

										if(wo)
										{
											get(wo, MA_Window_Viewobj, &wo);
										}
									}
									else /* get current active view */
									{
										FORCHILD(app, MUIA_Application_WindowList)
										{
											if (getv(child, MUIA_Window_Activate) && getv(child, MA_Window_Type) == MV_Window_Type_View)
											{
												get(child, MA_Window_Viewobj, &wo);
												break;
											}
										}
										NEXTCHILD
									}

									if(wo)
									{
										struct MinList ml;
										struct dragdropnode *ddn, *nextddn;
										ULONG len;
										STRPTR buffer;
										CONST_STRPTR str;

										NEWLIST(&ml);
										DoMethod(wo, MM_View_GetSelectionList, &ml);

										len = 1; /* terminating '\0' */
										ITERATELIST(ddn, &ml)
										{
											str = arg->fullpath ? (STRPTR)ddn->path : FilePart(ddn->path);
											len += strlen(str) + 1; /* + '\n' */
										}

										buffer = AllocVecTaskPooled(len);

										if(buffer)
										{
											struct IOClipReq * req;
											UBYTE *buf = buffer;

											ITERATELISTSAFE(ddn, nextddn, &ml)
											{
												str = arg->fullpath ? (STRPTR)ddn->path : FilePart(ddn->path);
												len = strlen(str);
												memcpy(buf, str, len);
												buf += len;
												*buf++ = '\n';

												free(ddn);
											}
											*buf = '\0';

											req = clipboard_open(0);

											if(req)
											{
												clipboard_write_ftxt(req, buffer);
												clipboard_close(req);
												REPORT_SUCCESS;
											}
											else
											{
												REPORT_ERROR(10);
											}

											FreeVecTaskPooled(buffer);
										}
										else
										{
											REPORT_ERROR(20);
										}
									}
									else
									{
										REPORT_ERROR(5);
									}
								}
								break;

							/*
							 * Command:
							 *  - AddBookmark
							 * Synopsis:
							 *  - Adds bookmark
							 * Parameters:
							 *  - URI: location the bookmark points to
							 *  - NAME: name that will be displayed for a location
							 *  - VIEWID: view identifier from which the uri will be taken. name will be asked in window
							 *  - PERMANENT: creates bookmark which is stored to disk instead of being temporary
							 * RC:
							 *  - 0: Ok
							 *  - 5: Error
							 */
							case RXCMD_AddBookmark:
								{
									struct RX_AddBookmark *arg = (struct RX_AddBookmark *)array;
									APTR addbookmarkwin = NULL;

									FORWARD_TO_MAINTASK;
									DB(("Got here\n"));
									if (arg->uri == NULL || arg->name == NULL)
									{
										if ( (addbookmarkwin = NewObject(getaddbookmarkwinclass(), NULL,
												MA_AddBookmarkwin_Uri, arg->uri,
												MA_AddBookmarkwin_Name, arg->name,
												MA_AddBookmarkwin_Permanent, arg->permanent,
												MA_Viewgroup_ID, arg->viewid,
												TAG_DONE)) )
										{
											DoMethod(app, OM_ADDMEMBER, addbookmarkwin);
										}
									}
									else
									{
										bookmarks_adduri(arg->uri, arg->name, arg->permanent);
									}

									if (addbookmarkwin)
									{
										set(addbookmarkwin, MUIA_Window_Open, TRUE);

										REPORT_SUCCESS;
									}
								}
								break;

							/*
							 * Command:
							 *  - EditMimeType
							 * Synopsis:
							 *  - Open mimetype editor window for a given file (if a mimetype exists).
							 * Parameters:
							 *  - PATH: path of the file
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Couldn't run the file
							 */
							case RXCMD_EditMimeType:
								{
									struct RX_EditMimeType *arg = (struct RX_EditMimeType *)array;

									FORWARD_TO_MAINTASK;

									if (arg->path)
									{
										APTR maobj;
										APTR mimetype = (APTR) DoMethod(obj, MM_Application_GetMimeType, arg->path);

										if(mimetype && !( ((struct internal_mimetype_node *)mimetype)->flags & MIMETYPEFLAG_INTERNAL))
										{
											if ((maobj = NewObject(getmimeadjustwinclass(), NULL,
															MA_Mimeadjustwin_MimeNode, mimetype,
															MA_Mimeadjustwin_MimeType, NULL,
															MA_Mimeadjustwin_Edit, TRUE,
															MA_Mimeadjustwin_Generic, FALSE,
															TAG_DONE)))
											{
												DoMethod(app, OM_ADDMEMBER, maobj);
												set(maobj, MUIA_Window_Open, TRUE);

												DoMethod(maobj, MUIM_Notify, MUIA_Window_Open, FALSE, app, 3, MM_Mimegroup_Mime_Ack, maobj, TRUE);

												REPORT_SUCCESS;
											}
											else
											{
												REPORT_ERROR(20);
											}
										}
										else
										{
											smartreq_info("Ambient mimetypes", MV_Notification_Warning, "No filetype found for\n%s.", arg->path);
											REPORT_ERROR(5);
										}
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 * Command:
							 *  - Menu
							 * Synopsis:
							 *  - Adds or removes a menu entry
							 * Parameters:
							 *  - ADD/REMOVE: add or removes a menu entry (REMOVE removes all menus for now)
							 *  - ID: entry identifier
							 *  - PARENTID: entry parent identifier
							 *  - TITLE: menu entry label
							 *  - SHORTCUT: menu entry shortcut key (no conflict check for now)
							 *  - TYPE: menu entry type: "menu" or "item"
							 *  - COMMAND: command string executed when entry is selected
							 *  - COMMANDTYPE: command type: "workbench" or "rexx" or "amigados" or "script" or "internal"
							 * RC:
							 *  - 0: Ok
							 *  - 5: Error
							 */
							case RXCMD_Menu:
								{
									struct RX_Menu *arg = (struct RX_Menu *)array;

									FORWARD_TO_MAINTASK;

									if(arg->add && arg->id)
									{
										ULONG commandtype = 0;

										if(arg->commandtype)
										{
											if(!stricmp(arg->commandtype, "Workbench"))
											{
												commandtype = AC_WORKBENCH;
											}
											else if(!stricmp(arg->commandtype, "Rexx"))
											{
												commandtype = AC_AREXX;
											}
											else if(!stricmp(arg->commandtype, "AmigaDOS"))
											{
	                                            commandtype = AC_AMIGADOS;
											}
											else if(!stricmp(arg->commandtype, "Internal"))
											{
												commandtype = AC_INTERNAL;
											}
											else if(!stricmp(arg->commandtype, "Script"))
											{
												commandtype = AC_SCRIPT;
											}
										}

										menus_user_add(arg->id, arg->title, arg->shortcut, arg->type, arg->parentid, arg->command, commandtype);

										REPORT_SUCCESS;
									}
									else if(arg->remove)
									{
										menus_user_remove();
										REPORT_SUCCESS;
									}
									else
									{
										REPORT_ERROR(5);
									}
								}
								break;

							/*
							 * Command:
							 *  - IconSelect
							 * Synopsis:
							 *  - Acts as an icon doubleclick
							 * Parameters:
							 * RC:
							 *  - 0: Ok
							 *  - 20: Error
							 */
							case RXCMD_IconSelect:
								{
									if(msg->internal && msg->obj)
									{
										DoMethod(msg->obj, MM_Icon_Select);
										REPORT_SUCCESS;
									}
									else
									{
                                        REPORT_ERROR(20);
									}
								}
								break;

							/*
							 * Command:
							 *  - GetMimeType
							 * Synopsis:
							 *  - Get the mimetype name of a given file
							 * Parameters:
							 *  - PATH: path of the file
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Couldn't run the file
							 */
							case RXCMD_GetMimeType:
								{
									struct RX_GetMimeType *arg = (struct RX_GetMimeType *)array;

									FORWARD_TO_MAINTASK;

									if (arg->path)
									{
										struct internal_mimetype_node *mimetype = (struct internal_mimetype_node *) DoMethod(obj, MM_Application_GetMimeType, arg->path);

										if(mimetype)
										{

											REPORT_RESULT(0, mimetype->mimetype);
										}
										else
										{
											REPORT_ERROR(5);
										}
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;

							/*
							 * Command:
							 *  - GetDeficonPath
							 * Synopsis:
							 *  - Get the deficon of a given file
							 * Parameters:
							 *  - PATH: path of the file
							 * RC:
							 *  - 0 : Ok
							 *  - 20: Couldn't run the file
							 */
							case RXCMD_GetDeficonPath:
								{
									struct RX_GetDeficonPath *arg = (struct RX_GetDeficonPath *)array;

									FORWARD_TO_MAINTASK;

									if (arg->path || arg->mimetype)
									{
										char *info = (char *) DoMethod(obj, MM_Application_GetDeficonPath, arg->path, arg->mimetype);

										if(info)
										{
											REPORT_RESULT(0, info);
											free(info);
										}
										else
										{
											REPORT_ERROR(5);
										}
									}
									else
									{
										REPORT_ERROR(20);
									}
								}
								break;
							/*
							 * Command:
							 * - SetPrefs
							 * Synopsis:
							 * - Set a preferences setting
							 * Parameters:
						 	 * - KEY: the setting to change
							 * - VALUE: the new value for the setting
							 * RC:
							 * -  0 : Ok
							 * - 10 : KEY doesn't exist
							 * - 20 : VALUE is wrong
							 */
							case RXCMD_SetPrefs:
								{
									struct RX_SetPrefs *arg = (struct RX_SetPrefs *)array;

									FORWARD_TO_MAINTASK;
									
									/* See preferences_update.c */
									rc = preferences_process_SetPrefs(obj, arg->key, arg->value);

									REPORT_SUCCESS;
									if(rc) REPORT_ERROR(rc);
								}
							break;
							/*
							 * Command:
							 * - GetPrefs
							 * Synopsis:
							 * - Get a preferences setting
							 * Parameters:
						 	 * - KEY: the setting to get
							 * RC:
							 * -  0 : Ok
							 * - 20 : KEY doesn't exist
							 */
							case RXCMD_GetPrefs:
								{
									STRPTR result;
									struct RX_GetPrefs *arg = (struct RX_GetPrefs *)array;

									FORWARD_TO_MAINTASK;
									
									/* See preferences_update.c */
									result = preferences_process_GetPrefs(arg->key);

									if(result != NULL)
									{
										REPORT_RESULT(0, result);
										free(result);
									}
									else
									{
										REPORT_ERROR(rc);
									}
								}
							break;
							/*
							 * Command:
							 * - SavePrefs
							 * Synopsis:
							 * - Save the settings
							 * Parameters:
							 * - None
							 * RC:
							 * -  0 : Ok
							 * Saving is threaded...
							 */
							case RXCMD_SavePrefs:
								{
									DoMethod(obj, MM_Application_SavePrefs);
									REPORT_SUCCESS;
								}
							break;
								
							case RXCMD_NetworksSettings:
								{
									struct RX_NetworksSettings *arg = (struct RX_NetworksSettings *)array;
									STRPTR *dirlist;
									LONG count;

									dirlist = build_pathlist(msg->obj, NULL, arg->path, msg->objlist, msg->internal, &count, TRUE, FALSE);

									if (dirlist)
									{
										for (LONG i = 0; i <count; i++)
										{
											switch (cmd->id)
											{
											case RXCMD_NetworksSettings:
												networksfs_settings(dirlist[i]);
												break;
											}
										}

										free_pathlist(dirlist);
										REPORT_SUCCESS;
									}

									if (!dirlist)
									{
										if (!count)
											REPORT_ERROR(20);
										else
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
									}
								}
							break;

							case RXCMD_NetworksConnect:
								networksfs_connect();
								REPORT_SUCCESS;
							break;

							case RXCMD_Trash:
								{
									struct RX_Delete *arg = (struct RX_Delete *)array;
									STRPTR *dirlist;
									LONG count;
									ULONG handleicons = FALSE;
									BOOL noicon = FALSE;

									methodstack_push_sync( msg->obj, 3, OM_GET, MA_View_HandleIcons, &handleicons);

									if(!handleicons)
									{
										noicon = TRUE;
									}

									dirlist = build_pathlist(msg->obj, NULL, arg->path, msg->objlist, msg->internal, &count, noicon, FALSE);

									DB(("dirlist 0x%p, noicon=%d, count=%d\n", dirlist, noicon, count));

									if (dirlist)
									{
										struct rx_delete_async *rxdel;

										/* i don't think it's smart to try to delete devices that way :) */
										if(isdevicename(dirlist[0]))
										{
											count = 0;
											free_pathlist(dirlist);
											dirlist = NULL;
										}
										else
										{
											if ( (rxdel = malloc(sizeof(*rxdel))) )
											{
												TEXT buttons[32];
												rxdel->id = msg->id;
												rxdel->dirlist = dirlist;
												rxdel->noicon = noicon;

												/* maybe make this requester (and its default active button)
												 * conditional/optional, depending on paranoia level :)
												 */

												snprintf(buttons, sizeof(buttons), "*%s", GSI(MSG_YESNO));

												smartreq_request(
													NULL,
													NULL,
													GSI(MSG_TRASH_TITLE),
													MM_Application_Trash_Ok,
													(LONG)rxdel, buttons,
													MV_Notification_Warning,
													count == 1 ? GSI(MSG_TRASH_CONFIRM) : GSI(MSG_TRASH_CONFIRM_MULTIPLE),
													count == 1 ? (ULONG)rxdel->dirlist[0] : count);
											}
											else
											{
												free_pathlist(dirlist);
												dirlist = NULL;
											}
										}
									}

									if (!dirlist)
									{
										if (!count)
											REPORT_ERROR(20);
										else
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
									}
								}
							break;
							
							case RXCMD_RestoreFromTrash:
								{
									struct RX_Delete *arg = (struct RX_Delete *)array;
									STRPTR *dirlist;
									LONG count;
									ULONG handleicons = FALSE;
									BOOL noicon = FALSE;

									methodstack_push_sync( msg->obj, 3, OM_GET, MA_View_HandleIcons, &handleicons);

									if(!handleicons)
									{
										noicon = TRUE;
									}

									dirlist = build_pathlist(msg->obj, NULL, arg->path, msg->objlist, msg->internal, &count, noicon, FALSE);

									DB(("dirlist 0x%p, noicon=%d, count=%d\n", dirlist, noicon, count));

									if (dirlist)
									{
										struct rx_delete_async *rxdel;

										/* i don't think it's smart to try to delete devices that way :) */
										if(isdevicename(dirlist[0]))
										{
											count = 0;
											free_pathlist(dirlist);
											dirlist = NULL;
										}
										else
										{
											if ( (rxdel = malloc(sizeof(*rxdel))) )
											{
												//TEXT buttons[32];
												rxdel->id = msg->id;
												rxdel->dirlist = dirlist;
												rxdel->noicon = noicon;

												do_action(obj, TA_File_Restore,
													TT_File_Delete_PathList, rxdel->dirlist,
													TT_File_Delete_RexxID,   rxdel->id,
													TT_File_Delete_RxDel,    rxdel,
													TT_File_Delete_NoIcon,   rxdel->noicon,
													TAG_DONE);
											}
											else
											{
												free_pathlist(dirlist);
												dirlist = NULL;
											}
										}
									}

									if (!dirlist)
									{
										if (!count)
											REPORT_ERROR(20);
										else
											REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
									}
								}
							break;
							
							case RXCMD_EmptyTrashcan:
								{
									trashcan_empty();
								}
							break;
						}
						freeargsstring(rda);
					}
					else
					{
						REPORT_ERROR(AMBIENT_RXERR_BADSYNTAX);
					}

					if (array)
					{
						free(array);
					}
				}
				else
				{
					REPORT_ERROR(AMBIENT_RXERR_OUTOFMEMORY);
				}
			}
			else
			{
				REPORT_ERROR(AMBIENT_RXERR_BADDEFINITION);
			}
		}
		else
		{
			REPORT_ERROR(AMBIENT_RXERR_UNKNOWNCOMMAND);
		}
	}

	if (rc)
	{
		if (msg->retval)
		{
			*msg->retval = retval;
		}

		if (msg->retstr)
		{
			*msg->retstr = retstr;
		}
	}

	return (rc);
}

DEFSMETHOD(Application_DoRexx)
{
	return application_dorexx( cl, obj, msg );
}

/* MM_Application_Delete_Ok
 *
 * This is called when delete requester is finished.
 */

DEFSMETHOD(Application_Delete_Ok)
{
	struct rx_delete_async *rxdel = (struct rx_delete_async *)msg->userdata;

	if (msg->butnum)
	{
		do_action(obj, TA_File_Delete,
			TT_File_Delete_PathList, rxdel->dirlist,
			TT_File_Delete_RexxID,   rxdel->id,
			TT_File_Delete_RxDel,    rxdel,
			TT_File_Delete_NoIcon,   rxdel->noicon,
		TAG_DONE); /* XXX: we should supply TA_REFWIN */
	}
	else
	{
		rx_set_result(rxdel->id, 5, NULL);
		rx_reply_id(rxdel->id);
		free_pathlist(rxdel->dirlist);
		free(rxdel);
	}

	return (0);
}

DEFSMETHOD(Application_Trash_Ok)
{
	struct rx_delete_async *rxdel = (struct rx_delete_async *)msg->userdata;

	if (msg->butnum)
	{
		do_action(obj, TA_File_Trash,
			TT_File_Delete_PathList, rxdel->dirlist,
			TT_File_Delete_RexxID,   rxdel->id,
			TT_File_Delete_RxDel,    rxdel,
			TT_File_Delete_NoIcon,   rxdel->noicon,
		TAG_DONE); /* XXX: we should supply TA_REFWIN */
	}
	else
	{
		rx_set_result(rxdel->id, 5, NULL);
		rx_reply_id(rxdel->id);
		free_pathlist(rxdel->dirlist);
		free(rxdel);
	}

	return (0);
}

/*
 * Save the prefs to disk.
 */
DEFSMETHOD(Application_SavePrefs)
{
	APTR ctx;

	if ( (ctx = prefspool_duplicate(mainprefspool)) )
	{
		do_action(obj, TA_Prefs_Save,
			TT_Prefs_Save_Ctx, ctx,
		TAG_DONE);
	}

	return (0);
}


STATIC APTR CenterBackgroundImage(struct Screen *scr, LONG pen, APTR dtp_root)
{
	if (scr != NULL && dtp_root != NULL)
	{
		int mode = getprefslong(DSI_BACKGROUND_ROOT_BGRENDER);
		int screen_width = scr->Width;
		int screen_height = scr->Height - (GetSkinInfoAttr(GetScreenDrawInfo(scr), SI_ScreenTitlebarHeight, TAG_DONE));
		int w = (int)picture_getattr(dtp_root, PICTURE_WIDTH);
		int h = (int)picture_getattr(dtp_root, PICTURE_HEIGHT);
		int newbm = FALSE;

		switch (mode)
		{
			default:
				if (w <= screen_width || h <= screen_height)
					break;
				/* fall through */
			case BGRENDER_Centered:
			case BGRENDER_Scaled:
				if (screen_width != w || screen_height != h)
					newbm = TRUE;
				break;
		}

		if (newbm)
		{
			APTR bm;
			ULONG compositing = FALSE;

			if(scr)
			{
				ASSERT(dtp_root);

				int reqvmem = getprefslong(DSI_BACKGROUND_TRANSITION);

				ULONG memory = 0;
				Object *monitor = NULL;
				GetAttr(SA_MonitorObject, (Object *)scr, (IPTR)&monitor);
				GetAttr(MA_MemorySize, monitor, &memory);
				GetAttr(SA_CompositingLayers,(Object *)scr,&compositing);

				if (memory <= MINIMUM_TRANSITIONS_VMEM)
					reqvmem = FALSE;

				// On systems with lots of vmem, allow the bitmap to vmem in this case to allow for accelerated transitions 
				if (memory > MINIMUM_TRANSITIONS_VMEM)
					compositing = FALSE;
				
				/*
				 * That one needs some post processing now.
				 */
				if ( (bm = gfx_bitmap_create(screen_width, screen_height, BITMAPDEPTH_Clone, BITMAPTAG_VMem, reqvmem ? TRUE : compositing ? FALSE : TRUE, BITMAPTAG_ScreenFriend, scr, TAG_DONE)) )
				{
					ULONG src_x, src_y;
					ULONG dst_x, dst_y;

					/*
					 * Do the background.
					 */
					if (w < screen_width || h < screen_height)
					{
						struct RastPort rp;

						InitRastPort(&rp);
						rp.BitMap = gfx_bitmap_bm(bm);

						SetRPAttrs(&rp,
							RPTAG_PenMode, FALSE,
							RPTAG_FgColor, pen,
							TAG_DONE
						);
						RectFill(&rp, 0, 0, screen_width - 1, screen_height - 1);
					}

					/*
					 * Then blit.
					 */
					if (h > screen_height)
					{
						src_y = (h - screen_height) / 2;
						dst_y = 0;
					}
					else
					{
						src_y = 0;
						dst_y = (screen_height - h) / 2;
					}

					if (w > screen_width)
					{
						src_x = (w - screen_width) / 2;
						dst_x = 0;
					}
					else
					{
						src_x = 0;
						dst_x = (screen_width - w) / 2;
					}

					gfx_blit(picture_getattr(dtp_root, PICTURE_BITMAP), bm,
						BLITTAG_SrcX, src_x,
						BLITTAG_SrcY, src_y,
						BLITTAG_DstX, dst_x,
						BLITTAG_DstY, dst_y,
					TAG_DONE);

					picture_set_bitmap(dtp_root, bm);
				}
				else
				{
					picture_delete(dtp_root);
					dtp_root = NULL;
					/* XXX: report out of memory error.. there's no picture at all */
				}
			}
		}
	}

	return (dtp_root);
}


static void remove_transition_effect(struct Data *data)
{
	if (data->bg_transition_active)
	{
		data->bg_transition_active = 0;
		DoMethod(app, MUIM_Application_RemInputHandler, &data->transtimer);
	}
}

DEFTMETHOD(Application_UpdateTransitionEffect)
{
	GETDATA;
	data->mixlevel += 5;

	if (data->mixlevel == 0xff)
	{
		remove_transition_effect(data);
	}

	return DoMethod(app, MM_Application_RootDoMethodByAttr, MA_View_HasBackground, TRUE, MM_Window_UpdateBackground, data->dtp_root, getprefslong(DSI_BACKGROUND_ROOT_BGRENDER));
}


/*
 * Processes the background by centering it
 * and generating a new one. It's a bit weird in the
 * sense it has to set the window's backfill bitmap again.
 *
 * This method is called whenever background is redrawn (from iconviewclass.c)
 */
DEFSMETHOD(Application_CenterBackground)
{
	GETDATA;
	BOOL dispose = TRUE;

	data->dtp_root = CenterBackgroundImage(msg->screen, msg->pen, data->dtp_root);

	if (data->dtp_root && data->dtp_root_new)
	{
		APTR bm = picture_getattr(data->dtp_root, PICTURE_BITMAP);
		int w = gfx_bitmap_width(bm);
		int h = gfx_bitmap_height(bm);
		struct Rectangle r = { 0, 0, w - 1, h - 1 };
		UBYTE v = data->mixlevel;

		if (v == 0xff || !data->bg_transition_active)
		{
			gfx_blit_tiled(picture_getattr(data->dtp_root_new, PICTURE_BITMAP), 0, 0, gfx_bitmap_bm(bm), &r);
		}
		else
		{
			data->dtp_root_old = CenterBackgroundImage(msg->screen, msg->pen, data->dtp_root_old);
			data->dtp_root_new = CenterBackgroundImage(msg->screen, msg->pen, data->dtp_root_new);

			if (data->dtp_root_old && data->dtp_root_new)
			{
				ULONG mixlevel = v << 24 | v << 16 | v << 8 | v;

				gfx_blit_tiled(picture_getattr(data->dtp_root_old, PICTURE_BITMAP), 0, 0, gfx_bitmap_bm(bm), &r);
				gfx_blit_tiled_alpha(picture_getattr(data->dtp_root_new, PICTURE_BITMAP), 0, 0, gfx_bitmap_bm(bm), &r, mixlevel);

				dispose = FALSE;
			}
		}
	}

	if (dispose)
	{
		picture_delete(data->dtp_root_old);
		picture_delete(data->dtp_root_new);
		data->dtp_root_old = NULL;
		data->dtp_root_new = NULL;
	}

	return 0;
}


/*
 * Holds the background for every subwindow (including the rootwindow)
 */
DEFSMETHOD(Application_AddBackground)
{
	GETDATA;

	switch (msg->type)
	{
		case TV_Background_Load_Type_Root:
		{
			APTR dtp = msg->dtp, tmp = data->dtp_root;

			picture_delete(data->dtp_root_old);
			picture_delete(data->dtp_root_new);
			data->dtp_root_old = NULL;
			data->dtp_root_new = NULL;

			data->dtp_root = dtp;
			remove_transition_effect(data);

			if (tmp && dtp && getprefslong(DSI_BACKGROUND_TRANSITION))
			{
				struct Screen *scr = get_screen();

				if (scr)
				{
					ULONG compositing = FALSE;
					ULONG memory = 0;
					Object *monitor = NULL;
					GetAttr(SA_CompositingLayers, (Object *)scr, &compositing);
					GetAttr(SA_MonitorObject, (Object *)scr, (IPTR)&monitor);
					GetAttr(MA_MemorySize, monitor, &memory);
					
					if (compositing && memory > MINIMUM_TRANSITIONS_VMEM)
					{
						dtp = picture_create(BITMAPWIDTH_Clone, BITMAPHEIGHT_Clone, BITMAPDEPTH_Clone, scr, TRUE);

						if (dtp)
						{
							data->bg_transition_active = 1;
							DoMethod(app, MUIM_Application_AddInputHandler, &data->transtimer);

							data->mixlevel = 0;
							data->dtp_root_old = tmp;
							data->dtp_root_new = msg->dtp;
							data->dtp_root = dtp;
							tmp = NULL;
						}
					}
				}
			}

			DoMethod(app, MM_Application_RootDoMethodByAttr, MA_View_HasBackground, TRUE,
				MM_Window_UpdateBackground, dtp, getprefslong(DSI_BACKGROUND_ROOT_BGRENDER)
			);

			picture_delete(tmp);
		}
		break;

		case TV_Background_Load_Type_Window:
		{
			picture_delete(data->dtp_window);
			data->dtp_window = msg->dtp;

			DoMethod(app, MM_Application_WindowDoMethodByAttr, MA_View_HasBackground, TRUE,
				MM_Window_UpdateBackground, data->dtp_window, getprefslong(DSI_BACKGROUND_WINDOW_BGRENDER)
			);
		}
		break;

		#ifdef DEBUG
		default:
			PDB(("unknown bgtype %ld\n", msg->type));
			break;
		#endif
	}

	return (0);
}


/*
 * That function is called to load backgrounds,
 * either on initialization or when the prefs
 * changes.
 */
DEFSMETHOD(Application_LoadBackground)
{
	GETDATA;

#if 0
	if (msg->flags & MF_Application_LoadBackground_ClearRoot)
	{
		DoMethod(app, MM_Application_RootDoMethodByAttr, MA_View_HasBackground, TRUE,
			MM_Window_UpdateBackground, NULL, NULL
		);

		picture_delete(data->dtp_root);
		data->dtp_root = NULL;
	}
#endif

	if (msg->flags & MF_Application_LoadBackground_ClearWindow)
	{
		DoMethod(app, MM_Application_WindowDoMethodByAttr, MA_View_HasBackground, TRUE,
			MM_Window_UpdateBackground, NULL, NULL
		);

		picture_delete(data->dtp_window);
		data->dtp_window = NULL;
	}

	if (msg->flags & MF_Application_LoadBackground_Root)
	{
		do_action(obj, TA_Background_Load,
			TT_Background_Load_Type, TV_Background_Load_Type_Root,
			TT_Background_Load_Mode, getprefslong(DSI_BACKGROUND_ROOT_BGRENDER),
			TT_Background_Load_Path, getprefsstr(DSI_BACKGROUND_ROOT),
		TAG_DONE);
	}

	if (msg->flags & MF_Application_LoadBackground_Window)
	{
		do_action(obj, TA_Background_Load,
			TT_Background_Load_Type, TV_Background_Load_Type_Window,
			TT_Background_Load_Mode, getprefslong(DSI_BACKGROUND_WINDOW_BGRENDER),
			TT_Background_Load_Path, getprefsstr(DSI_BACKGROUND_WINDOW),
		TAG_DONE);
	}

	return (0);
}


/*
 * Updates the display of root/normal windows.
 * Needed after a background change, font, etc..
 */
DEFSMETHOD(Application_DisplayUpdate)
{
	GETDATA;
	ULONG flags = 0;

	if (msg->flags & MF_Application_DisplayUpdate_Fonts)
	{
		flags |= MF_View_Refresh_Fonts;
	}
	if (msg->flags & MF_Application_DisplayUpdate_Size)
	{
		flags |= MF_View_Refresh_Size;
	}
	if (msg->flags & MF_Application_DisplayUpdate_Background)
	{
		flags |= MF_View_Refresh_Background;
	}

	if (msg->flags & MF_Application_DisplayUpdate_Root)
	{
		DoMethod(app, MM_Application_RootDoMethodByAttr, NULL, NULL,
			MM_Window_DoView, NULL, MM_View_Refresh, flags, data->dtp_root ? picture_getattr(data->dtp_root, PICTURE_BITMAP) : NULL
		);
	}

	if (msg->flags & MF_Application_DisplayUpdate_Windows)
	{
		DoMethod(app, MM_Application_WindowDoMethodByAttr, MA_Window_Type, MV_Window_Type_View, /* do to all views */
			MM_Window_DoView, NULL, MM_View_Refresh, flags, data->dtp_window ? picture_getattr(data->dtp_window, PICTURE_BITMAP) : NULL
		);

		DoMethod(app, MM_Application_WindowDoMethodByAttr, MA_Window_Type, MV_Window_Type_View,
			MM_Window_UpdateUI );
	}

	return (0);
}


/*
 * Create a window useable for a view. Either root
 * or normal.
 */
DEFSMETHOD(Application_CreateWindow)
{
	APTR o = o; /* shut up gcc */
	GETDATA;

	switch (msg->type)
	{
		case MV_Application_CreateWindow_Rootview:
			if (!data->rootwin)
			{
				data->rootwin = o = NewObject(getwindowclass(), NULL, /* we register data->rootwin because it's handy */
					MUIA_Window_Backdrop,    TRUE,
					MUIA_Window_Borderless,  TRUE,
					MUIA_Window_CloseGadget, FALSE,
					MUIA_Window_DepthGadget, FALSE,
					MUIA_Window_DragBar,     FALSE,
					MUIA_Window_SizeGadget,  FALSE,
					MA_Window_Type,          MV_Window_Type_Rootview,
					MA_Window_MIMEctx,       msg->mimectx,
					MA_Window_DTP,           data->dtp_root,
					MA_Window_BGMode,        getprefslong(DSI_BACKGROUND_ROOT_BGRENDER),
				TAG_DONE);
			}
			else
			{
				PDB(("there's already a rootview..\n"));
			}
			break;

		case MV_Application_CreateWindow_View:
			o = NewObject(getwindowclass(), NULL,
				MA_Window_Type,    MV_Window_Type_View,
				MA_Window_MIMEctx, msg->mimectx,
				MA_Window_DTP,     data->dtp_window,
				MA_Window_Browser, msg->browser,
				MA_Window_BGMode,  getprefslong(DSI_BACKGROUND_WINDOW_BGRENDER),
			TAG_DONE);
			break;

		#ifdef DEBUG
		default:
			PDB(("unknown window type %ld\n", msg->type));
			break;
		#endif
	}

	if (o)
	{
		DoMethod(app, OM_ADDMEMBER, o);
	}
	return ((ULONG)o);
}

/*
 * Closes and disposes window object (needed for threads).
 */
DEFSMETHOD(Application_DisposeWindow)
{
	if ( msg->win )
	{
		RemoveDelayedDialog(cl, obj, msg->win);

		#if 0
		/* redundant */
		if ( getv( msg->win, MUIA_Window_Open ) )
			set(msg->win, MUIA_Window_Open, FALSE);
		#endif

		DoMethod(obj, OM_REMMEMBER, msg->win);
		MUI_DisposeObject(msg->win);
	}

	return (TRUE);
}

/*
 * Creates an icon (needed for threads).
 */
DEFSMETHOD(Application_CreateIcon)
{

	return ((ULONG)NewObject(geticonclass(), NULL,
		MA_Icon_AFont,       msg->isroot ? _conf(root_font)       : _conf(window_font),
		MA_Icon_SmallFont,   msg->isroot ? _conf(root_font_small) : _conf(window_font),
		MA_Icon_FontSpace,   msg->isroot ? _conf(root_spaceline)  : _conf(window_spaceline),
		MA_Icon_ViewID,      msg->view,
		MUIA_CustomBackfill, TRUE,
		TAG_DONE
	));
}

/*
 * Disposes an object (needed for threads and pushmethod calls).
 */
DEFSMETHOD(Application_DisposeObject)
{
	if(msg->o)
	{
		MUI_DisposeObject(msg->o);
	}

	return (TRUE);
}

DEFSMETHOD(Application_DisposeObjectArray)
{

	if(msg->array)
	{
		GETDATA;

		ULONG done = FALSE;

		if ( !data->cleanup_pending && msg->threaded )
		{
			done = do_action(app, TA_Dispose_Objects,
				TT_Dispose_Objects_Array,     msg->array,
				TT_Dispose_Objects_FreeArray, msg->freearray,
				TT_Priority,                  -5,
			TAG_DONE);
		}

		if ( !done )
		{
			ULONG i;

			for(i=0; msg->array[ i ]; i++)
			{
				MUI_DisposeObject(msg->array[ i ]);
				msg->array[ i ] = NULL;	/* to make thread below work, it's only necessary to null first element... */
			}

			if ( msg->freearray )
			{
				free( msg->array );
			}
		}
	}

	return (TRUE);
}


/*
 * Creates an icon for the infowin (needed for threads).
 */
DEFTMETHOD(Application_CreateIconinfo)
{
	return ((ULONG)NewObject(geticonclass(), NULL,
		MA_Icon_ViewID, MV_ViewID_Info,
		MUIA_ShowSelState, FALSE,
		TAG_DONE
	));
}


/*
 * Creates an infowin (needed for threads).
 */
DEFSMETHOD(Application_CreateInfowin)
{
	APTR o;

	o = NewObject(getinfowinclass(), NULL,
		MA_Infowin_Iconobj,  msg->iconobj,
		MA_Infowin_RXID,     msg->rxid,
		MA_Infowin_Infodata, msg->infodata,
		TAG_DONE
	);

	if (o)
	{
		DoMethod(obj, OM_ADDMEMBER, o);
		set(o, MUIA_Window_Open, TRUE);
	}

	return ((ULONG)o);
}


/*
 * Creates a progress window (needed for threads).
 */
DEFSMETHOD(Application_CreateProgresswin)
{
	APTR o;

	o = NewObject(getprogresswinclass(), NULL,
			MA_Progresswin_Look,   msg->mode,
			MA_Progresswin_Thread, msg->thread,
			MA_Progresswin_Refwin, msg->refwin,
		TAG_DONE
	);

	if (o)
	{
		DoMethod(app, OM_ADDMEMBER, o);
		AddDelayedDialog(cl, obj, o, _aprefs(progresswindowdelay));
	}

	return ((ULONG)o);
}

/*
 * Creates a destination selector window (needed for threads).
 */
DEFSMETHOD(Application_CreateDestSelectorwin)
{
	APTR o;

	o = NewObject(getdestselectwinclass(), NULL,
			MA_DestSelector_Thread, msg->thread,
			MA_DestSelector_RefWin, msg->refwin,
		TAG_DONE
	);

	if (o)
	{
		DoMethod(app, OM_ADDMEMBER, o);
	}

	return ((ULONG)o);
}

/*
 * Creates a pattern rename window (needed for threads).
 */
DEFSMETHOD(Application_CreatePatternRenamewin)
{
	APTR o = NULL;

	if (!(o = (APTR)DoMethod(app, MM_Application_FindWindowByName, MV_Window_Type_PatternRename, msg->pathlist ? msg->pathlist[0] : msg->from, MA_PatternRenamewin_FileType, MV_Icon_FileType_File, TAG_DONE)))
	{
		TEXT buffer[PATH_SIZE+1];
		STRPTR ptr;

		ptr = msg->pathlist ? msg->pathlist[0] : msg->from;

		if(isdevicename(ptr))
		{
			ULONG len = strlen(ptr);

			stccpy(buffer, ptr, sizeof(buffer));
			if(len > 0)
				buffer[len-1] = 0;
		}
		else
		{
			stccpy(buffer, FilePart(ptr), sizeof(buffer));
		}

		if ( (o = NewObject(getpatternrenamewinclass(), NULL,
			msg->pathlist ? MA_PatternRenamewin_PathList : TAG_IGNORE, msg->pathlist,
			MA_PatternRenamewin_Path, msg->pathlist ? msg->pathlist[0] : msg->from,
			msg->pathlist ? MA_PatternRenamewin_OldName : TAG_IGNORE, msg->pathlist ? buffer : NULL,
			MA_PatternRenamewin_NewName, buffer,
			MA_PatternRenamewin_FileType, MV_Icon_FileType_File,
			MA_PatternRenamewin_Thread, msg->thread,
			TAG_DONE)) )
		{
			DoMethod(app, OM_ADDMEMBER, o);
			CheckDelayedDialogs(cl, obj, (ULONG)-1);
		}
	}

	return ((ULONG) o);
}

/*
 * Creates a panelwin.
 */
DEFSMETHOD(Application_CreatePanelwin)
{
	APTR o;

	PDB(("Application_CreatePanelwin\n"));

	if( ( o = NewObject( getpanelwinclass(), NULL,
			MA_Panelwin_Prefspool, msg->pctx,
			TAG_DONE ) ) )
	{
		DoMethod( app, OM_ADDMEMBER, o );
	}

	PDB(("Application_CreatePanelwin done\n"));

	return( (ULONG) o );
}


/*
 * Creates a panelitem.
 */
DEFSMETHOD(Application_CreatePanelitem)
{
	APTR o = NULL;
	APTR pl, pi = NULL; /* list, item */

	PDB(("Application_CreatePanelitem\n"));

	if( ( pl = prefspool_item_get( msg->prefspool, NULL, DSI_LISTPOOL_PANEL, NULL, NULL ) ) )
	{
		pi = prefspool_item_get( msg->prefspool, pl, msg->index | DSF_LISTPOOL, NULL, NULL );
	}

	PDB(("Application_CreatePanelitem 0x%08lx\n", pi ));

	switch( msg->type )
	{
		#ifdef DEBUG
		case MV_Panel_Type_Drag:
			PDB(("this is not supposed to happen\n"));
			break;
		#endif
		case MV_Panel_Type_Button:
			{
				STRPTR imagepath, paneluri;

				o = NewObject( getpanelcommandbuttonclass(), NULL,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath, NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ), imagepath,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_URI,       (APTR) &paneluri , NULL ) ) ? MA_Panel_URI       : TAG_IGNORE ), paneluri,
						TAG_DONE );
			//	PDB(("%s %s ",imagepath,paneluri));
			}
			break;

		case MV_Panel_Type_SubPanel:
			{
				APTR subpanel, group;
				STRPTR imagepath, paneluri, backdrop;
				ULONG *subpanelroot, *backmode, *backcolor;
				o = NewObject( getpanelsubpanelbuttonclass(), NULL,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH     , (APTR) &imagepath   , NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ),  imagepath,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_URI           , (APTR) &paneluri    , NULL ) ) ? MA_Panel_URI       : TAG_IGNORE ),  paneluri,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_SUBPANEL_ROOT , (APTR) &subpanelroot, NULL ) ) ? MA_SubPanel_ID     : TAG_IGNORE ), *subpanelroot,
						TAG_DONE );
						PDB(("Subpanelroot %d\n",*subpanelroot));
				subpanel = (APTR) getv( o, MA_Panelbutton_AttachedObject );
				group = (APTR) getv( subpanel, MA_Panelwin_Group );
				SetAttrs( group,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_PANELGROUP_BACKMODE , (APTR) &backmode , NULL ) ) ? MA_Panelgroup_BackMode  : TAG_IGNORE ), *backmode,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_PANELGROUP_BACKCOLOR, (APTR) &backcolor, NULL ) ) ? MA_Panelgroup_BackColor : TAG_IGNORE ), *backcolor,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_PANELGROUP_BACKDROP , (APTR) &backdrop , NULL ) ) ? MA_Panelgroup_Backdrop  : TAG_IGNORE ),  backdrop,
						TAG_DONE );
			}

			break;
		case MV_Panel_Type_DirPanel:
			{
				APTR subpanel, group;
				STRPTR imagepath, paneluri, backdrop;
				ULONG *backmode, *backcolor;

				o = NewObject( getpaneldirpanelbuttonclass(), NULL,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath, NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ), imagepath,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_URI      , (APTR) &paneluri,  NULL ) ) ? MA_Panel_URI       : TAG_IGNORE ), paneluri,
						TAG_DONE );

				subpanel = (APTR) getv( o       , MA_Panelbutton_AttachedObject );
				group    = (APTR) getv( subpanel, MA_Panelwin_Group );
				SetAttrs( group,
								( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_PANELGROUP_BACKMODE  , (APTR) &backmode , NULL ) ) ? MA_Panelgroup_BackMode  : TAG_IGNORE ), *backmode,
								( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_PANELGROUP_BACKCOLOR , (APTR) &backcolor, NULL ) ) ? MA_Panelgroup_BackColor : TAG_IGNORE ), *backcolor,
								( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_PANELGROUP_BACKDROP  , (APTR) &backdrop , NULL ) ) ? MA_Panelgroup_Backdrop  : TAG_IGNORE ),  backdrop,
								TAG_DONE );

			}
			break;

		case MV_Panel_Type_External:
			{
				APTR sobj;
				STRPTR classname;
				#define EXTCLASSPATH_SIZEOF 0x100
				TEXT extclasspath[ EXTCLASSPATH_SIZEOF ] = "MOSSYS:Classes/Panels";

				ASSERT( pi );

				sobj = NewObject( getpanelexternalsupportclass(), NULL, MA_Panelsupport_PPool, msg->prefspool, MA_Panelsupport_PItem, pi, TAG_DONE );
				prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_EXT_NAME, (APTR) &classname, NULL );
				AddPart( extclasspath, classname, EXTCLASSPATH_SIZEOF );
				if( !( o = MUI_NewObject( extclasspath, MA_Panelextern_SupportObject, sobj,MA_Panelsupport_PPool, msg->prefspool, MA_Panelsupport_PItem, pi, TAG_DONE ) ) ) {
					if( !( o = MUI_NewObject( &extclasspath[3], MA_Panelextern_SupportObject, sobj,MA_Panelsupport_PPool, msg->prefspool, MA_Panelsupport_PItem, pi,  TAG_DONE ) ) ) {
					}
				}
				
				if( o )
				{
					set( sobj, MA_Panelsupport_Object, o ); /* tell support object which object to support */
				}
			}
			break;

		case MV_Panel_Type_Spacer:
			{
				o = NewObject( getpanelspacerclass(), NULL, TAG_DONE );
			}
			break;

		case MV_Panel_Type_Separator:
			{
				o = NewObject( getpanelseparatorclass(), NULL, TAG_DONE );
			}
			break;

		case MV_Panel_Type_ViewWatcher:
			{
				STRPTR imagepath;
				o = NewObject(getpanelviewwatcherclass(), NULL,
						( ( pi && prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath, NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ), imagepath,
						TAG_DONE );
			}
			break;

		case MV_Panel_Type_Bookmarks:
			{
                STRPTR imagepath;
				o = NewObject(getpanelbookmarksclass(), NULL,
						( ( pi && prefspool_item_get(msg->prefspool, pi, DSI_LISTPOOL_PANEL_IMAGEPATH, (APTR) &imagepath, NULL ) ) ? MA_Panel_Imagepath : TAG_IGNORE ), imagepath,
						TAG_DONE );
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("wrong type\n"));
			break;
		#endif

	}

	if (o)
	{
		ULONG *sub;
		APTR subtbar;
		/* old files don't have DSI_LISTPOOL_PANEL_SUBPANEL so it must be a root panel*/
		if( ( prefspool_item_get( msg->prefspool, pi, DSI_LISTPOOL_PANEL_SUBPANEL , (APTR) &sub, NULL ) ) && ( *sub != 0 ) )
		{
			if( ( subtbar = (APTR) DoMethod( msg->obj, MM_Panelgroup_FindSubPanel, *sub ) ) )
			{
				DoMethod( subtbar, MUIM_Group_InitChange );
				DoMethod( subtbar, OM_ADDMEMBER, o );
				DoMethod( subtbar, MUIM_Group_ExitChange );
			}
		}
		else
		{
			DoMethod( msg->obj, MUIM_Group_InitChange ); /* needed because of our paneldrag repositioning */
			DoMethod( msg->obj, OM_ADDMEMBER, o );
			DoMethod( msg->obj, MUIM_Group_ExitChange );
		}
	}
	return ( (ULONG) o );
}         

/*
 * Creates a smartreq window (needed for threads).
 */
DEFSMETHOD(Application_CreateSmartReq)
{
	APTR o;

	o = NewObject(getsmartreqclass(), NULL,
		MUIA_Window_Title, "",
	TAG_DONE);

	if (o)
	{
		DoMethod(app, OM_ADDMEMBER, o);
	}

	return ((ULONG)o);
}


/* XXX: beware! there's no waiting list concept anymore */
DEFSMETHOD(Application_AddAppIcon)
{
	GETDATA;

	ASSERT(msg->o);

	D(ICONIO, bug("msg->o %p\n", msg->o));

	DB(("Adding appicon to view:0x%x\n", getv(msg->o, MA_Icon_AppAddress)));

	if (getv(msg->o, MA_Icon_HasPos) || getv(msg->o, MA_Icon_IsShortcut))
	{
		DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_AddIcon, msg->o, FALSE); /* fix that NULL, etc.. */
	}
	else
	{
		DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_AddIcon, msg->o, FALSE);
	}

	DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_DoLayout);

	return (0);
}


DEFSMETHOD(Application_ChangeScreenTitle)
{
	GETDATA;

	data->scrtick = (data->scrtick + 1) % 2;

	if (!data->pushid)
	{
		if (!data->screentitle_backup[0] /* XXX: add a SetPermanent flag or something.. */)
		{
			strcpy(data->screentitle_backup, screentitle);
		}
	}

	stccpy(data->scrbuf[data->scrtick], msg->title, SCREENTITLESIZE);
	screentitle = data->scrbuf[data->scrtick];

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		set(child, MUIA_Window_ScreenTitle, screentitle);
	}
	NEXTCHILD

	if (msg->flags & MF_Application_ChangeScreenTitle_Delayed)
	{
		if (!data->pushid)
		{
			data->pushid = DoMethod(app, MUIM_Application_PushMethod, obj, 1, MM_Application_RestoreScreenTitle);
		}
		if (data->pushid)
		{
			DoMethod(app, MUIM_Application_SetPushMethodDelay, data->pushid, 2000);
		}
	}

	return (0);
}


DEFTMETHOD(Application_RestoreScreenTitle)
{
	GETDATA;

	data->scrtick = (data->scrtick + 1) % 2;

	data->pushid = 0;

	strcpy(data->scrbuf[data->scrtick], data->screentitle_backup);
	screentitle = data->scrbuf[data->scrtick];

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		set(child, MUIA_Window_ScreenTitle, screentitle);
	}
	NEXTCHILD

	return (0);
}


#ifdef DEBUG
DEFSMETHOD(Application_SetDebug)
{
	setprefslong(db_a[msg->type].prefs, msg->value);
	db_a[msg->type].active = msg->value;

	return (0);
}


DEFSMETHOD(Application_DoDebug)
{
	switch (msg->type)
	{
		#if USE_MEMTRACK_MEMLIST
		case DBA_MEMCHECK:
			memcheck();
			break;

		case DBA_MEMSTATS:
			memstats(MEMSTATS_TOTAL);
			break;

		case DBA_MEMSTATS_ALL:
			memstats(MEMSTATS_ALL);
			break;
		#endif

		#if USE_MEMTRACK_RECORD
		case DBA_STARTMEMRECORD:
			memtrack_record_start();
			break;

		case DBA_STOPMEMRECORD:
			memtrack_record_stop();
			break;
		#endif

		default:
			PDB(("hum, no debug value there\n"));
			break;
	}

	return (0);
}
#endif /* DEBUG */


DEFTMETHOD(Application_CountWindows)
{
	ULONG wincnt = 0;

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if (getv(child, MUIA_Window_Open))
		{
			wincnt++;
		}
	}
	NEXTCHILD

	return (wincnt);
}


DEFSMETHOD(Application_DeleteAppIcon)
{
	GETDATA;

	return (DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_DeleteAppIcon, msg->address)); /* XXX: check for iconview instead of that NULL.. */
}


DEFSMETHOD(Application_Shutdown)
{
	/*  Push it gently, so the Quit requester has time to close (nifty extra 
	 *  handling for Piru) -- tokai
	 */
	{
		if (!msg->butnum)
		{
			return 0;
		}

		if (!((1<<31) & msg->butnum))
		{
			DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_Application_Shutdown, (msg->butnum | (1<<31)));
			return 0;
		}

		msg->butnum &= ~(1<<31);
	}

	if (msg->butnum == 1 || msg->butnum == 2)
	{
		if (_conf(misc_remember_windows) || _conf(misc_remember_documents))
		{
			sprefs_save();
		}

		sprefs_cleanup();
	}

	switch (msg->butnum)
	{
		case 0: /* cancel */
			break;

		case 1: /* shutdown */

			if ((SysBase->LibNode.lib_Version == 50 && 
					SysBase->LibNode.lib_Revision >= 60) || 
					SysBase->LibNode.lib_Version > 50)
			{
#if !USE_LEGACY
				ShutdownA(NULL);
				break;
#else
				SystemTags("MOSSYS:C/shutdown", SYS_Asynch, FALSE, SYS_Input, NULL, SYS_Output, NULL,TAG_DONE);
				break;
#endif
			}

		case 2: /* reboot */
			ColdReboot(); /* XXX: yeah well.. this is not really smart but doing a proper shutdown is a mess.. or not.. actually I should try exiting as much as I can then reboot (with some way to prevent disk writing) */
			break;

		#ifdef DEBUG
		default:
			PDB(("no action\n"));
			break;
		#endif
	}

	return (0);
}


DEFSMETHOD(Application_ReloadIcons)
{
	ULONG type;

	FORCHILD(obj, MUIA_Application_WindowList)
	{
		if (get(child, MA_Window_Type, &type))
		{
			if (type == MV_Window_Type_Rootview ||
			    (msg->onlyroot ? 1 == 2 : type ==  MV_Window_Type_View))
				DoMethod(child, MM_Window_Reload);
		}
	}
	NEXTCHILD

	return 0;
}


DEFSMETHOD(Thread_Finished)
{
	switch (msg->action)
	{
		case TA_Font_Load:
			{
				static ULONG fontbarrier;

				/*
				 * We need the fonts to be loaded
				 * before going further otherwise
				 * icons will show up fontless as they
				 * calc it in MUIM_AskMinMax and I don't
				 * want to reflow.
				 */
				if (fontbarrier < 3)
				{
					if (++fontbarrier == 3)
					{
						execute_command(app, AC_INTERNAL, "LoadURI root://", NULL);
					}
				}
			}
			break;

		case TA_Panels_LoadAll:
			panelprefs_loaded();
			break;

		case TA_Appicon_Read:
			{
				APTR o;

				if (msg->taglist)
				{
					o = (APTR)GetTagData(TT_Appicon_Read_Object, (ULONG)NULL, msg->taglist);
				}
				else
				{
					o = NULL;
				}

				if (msg->status)
				{
					APTR appicon;

					DoMethod(o, MM_Icon_End);
					DoMethod(app, MM_Application_AddAppIcon, o);

					appicon = (APTR)getv(o, MA_Icon_AppAddress);
					appicon_markasloaded(appicon);

				}
				else
				{
					/* failed, kill the object */
					PDB(("appicon failed\n"));
					if (o)
					{
						MUI_DisposeObject(o);
					}
				}
			}
			break;

		case TA_File_Delete:
		case TA_File_Restore:
		case TA_File_Trash:
			{
				struct rx_delete_async *rxdel;
				ULONG rxid;

				if (msg->taglist)
				{
					rxdel = (struct rx_delete_async *)GetTagData(TT_File_Delete_RxDel, (ULONG)NULL, msg->taglist);
					rxid = GetTagData(TT_File_Delete_RexxID, 0, msg->taglist);
				}
				else
				{
					rxdel = NULL;
					rxid = 0;
				}

				if (!msg->status)
				{
					if (rxid)
					{
						rx_set_result(rxid, 10, NULL);
					}
				}
				if (rxid)
				{
					rx_reply_id(rxid);
				}
				if (rxdel)
				{
					free_pathlist(rxdel->dirlist);
					free(rxdel);
				}
			}
			break;

		case TA_Infowin_Open:

			if (msg->status != ASYNC)
			{
				ULONG rxid;
				STRPTR * pathlist = (STRPTR *) GetTagData(TT_Infowin_Open_PathList, (ULONG)NULL, msg->taglist);

				if(pathlist)
				{
					free_pathlist(pathlist);
				}

				if (msg->taglist)
				{
					rxid = GetTagData(TT_Infowin_Open_RexxID, 0, msg->taglist);
				}
				else
				{
					rxid = 0;
				}

				if (!msg->status)
				{
					rx_set_result(rxid, 10, NULL);
				}

				rx_reply_id(rxid);
			}
			break;

		case TA_File_Rename:

			if (msg->status != ASYNC)
			{
				ULONG rxid = 0;
				STRPTR * pathlist = (STRPTR *) GetTagData(TT_File_Rename_PathList, (ULONG)NULL, msg->taglist);

				if(pathlist)
				{
					free_pathlist(pathlist);
				}

				if (!msg->status)
				{
					rx_set_result(rxid, 10, NULL);
				}

				rx_reply_id(rxid);
			}
			break;

		case TA_URI_Load:
			{
				ULONG use_ctx = FALSE;
				APTR  ctx;
				ULONG id;
				ULONG newwin;
				ULONG browser;
				ULONG iconified;
				ULONG tofront;

				if (msg->taglist)
				{
					ctx = (APTR)GetTagData(TT_URI_Load_Ctx, (ULONG)NULL, msg->taglist);
					id = GetTagData(TT_URI_Load_ID, 0, msg->taglist);
					newwin = GetTagData(TT_URI_Load_Newwin, FALSE, msg->taglist);
					browser	= GetTagData(TT_URI_Load_Browser, FALSE, msg->taglist);
					iconified = GetTagData(TT_URI_Load_Iconified, FALSE, msg->taglist);
					tofront = GetTagData(TT_URI_Load_ToFront, FALSE, msg->taglist);

					DB(("Fetched LoadURI tags, ctx;%08lx, id;%ld, "
					"newwin;%lx, browser;%lx, iconified;%lx, tofront;%lx\n", ctx, id, newwin, browser, iconified, tofront));
				}
				else
				{
					ctx       = NULL;
					id        = 0;
					newwin    = FALSE;
					browser   = FALSE;
					iconified = FALSE;
					tofront   = FALSE;
				}

				if (msg->status ) /* XXX: that is *wrong* I think. could be an abort */
				{
					STRPTR s;

					rescan:

					if ( (s = mimeuri_getattr(ctx, MIMEURIATTR_MIMETYPE)) )
					{
						ULONG action;

						action = (ULONG)mimeuri_getattr(ctx, MIMEURIATTR_MIME_ACTION);

						D(MIMEURI,bug("mimetype: <%s>, action: %ld\n", s, action));

						switch ((int)action)
						{
							case MIMEACTION_VIEW:
							{
								APTR wo;

								/*
								 * If mimetype request opening of new window, if LoadURI param passed NEWWIN=TRUE
								 * or it window with loaded URI doesn't exist we open a new window.
								 */
								//if (!id || newwin)// || mimeuri_getattr(luri->ctx, MIMEURIATTR_MIME_NEWWIN))
								if (!id || newwin || mimeuri_getattr(ctx, MIMEURIATTR_MIME_NEWWIN))
								{
									wo = NULL;
								}
								else
								{
									wo = (APTR)DoMethod(obj, MM_Application_FindWindowByID, id);

									if (id && !wo)
									{
										/*
										 * The user probably closed the window, we abort
										 * then.
										 */
										break;
									}
								}

								if (!wo)
								{
									if (!strcmp(s, MIMETYPE_INTERNAL_ROOTVIEW))
									{
										/*
										 * In case of root windows don't allow creating more than one.
										 */

										if (DoMethod(obj, MM_Application_FindWindowByType, MV_Window_Type_Rootview) == (ULONG)NULL)
											wo = (APTR)DoMethod(obj, MM_Application_CreateWindow, MV_Application_CreateWindow_Rootview, ctx, browser);
									}
									else
									{
										if ( !newwin )
										{
											STRPTR path;

											if ( ( path = mimeuri_getattr(ctx, MIMEURIATTR_PATH)) )
											{
												wo = (APTR)DoMethod(obj, MM_Application_FindWindowByName, MV_Window_Type_View, path);
											}
										}

										if (!wo)
										{
											wo = (APTR)DoMethod(obj, MM_Application_CreateWindow, MV_Application_CreateWindow_View, ctx, browser);
										}
									}
								}
								else
								{
									set(wo, MA_Window_MIMEctx, ctx);
								}

								if (wo)
								{
									ULONG opened;

									/* XXX: and in case we didn't create a window.. how do we tell it it has to change its view ? */
									use_ctx = TRUE;

									get(wo, MUIA_Window_Open, &opened);

									if (!opened)
									{
										set(wo, MUIA_Window_Open, TRUE);
									}
									else if (!iconified && tofront)
									{
										DB(("Put window to front and activate it!\n")); // bitRocky Test
										DoMethod(wo, MUIM_Window_ToFront);
										set(wo, MUIA_Window_Activate, TRUE);
									}

									/* XXX: window should probably not be opened if view is iconified, but it's more safe for now */
									if(iconified)
										DoMethod(wo, MM_Window_Iconify);
								}
								/* XXX */

							}
							break;

							#if USE_OS4
							case MIMEACTION_OS4:
								{
									ULONG skip = TRUE;

									if ( (OS4LoaderBase = OpenLibrary("os4loader.library", 0)) )
									{
										if (OS4LoaderBase->lib_OpenCnt > 1)
										{
											skip = FALSE;
										}
										CloseLibrary(OS4LoaderBase);
									}

									if (skip)
									{
										smartreq_info("Ambient mimetypes", MV_Notification_Warning, "OS4 emulator not installed.\nPlease check http://os4emu.amigazeux.net/", NULL);
										break;
									}
								}
								/* fallthrough */
							#endif

							case MIMEACTION_EXECUTE:
							case MIMEACTION_LOADSEG:
								{
									STRPTR path;

									if ( (path = mimeuri_getattr(ctx, MIMEURIATTR_PATH)) )
									{
										do_action(obj, TA_WBStart, TT_WBStart_Path, path, TAG_DONE);
									}
									/* XXX: also.. */
								}
								break;

							case MIMEACTION_PLAYSOUND:
								{
									#if USE_VORBIS
									if (!strcmp(s, MIMETYPE_INTERNAL_VORBIS))
									{
										do_action(obj, TA_Sound_Play,
											TT_Sound_Play_Path, mimeuri_getattr(ctx, MIMEURIATTR_PATH),
											TT_Sound_Play_Mode, (PS_VORBIS | PSF_QUEUED_IMMEDIATE),
										TAG_DONE); /* XXX */
									}
									#endif
									#if USE_MPEGA
									else if (!strcmp(s, MIMETYPE_INTERNAL_MPEGA))
									{
										do_action(obj, TA_Sound_Play,
											TT_Sound_Play_Path, mimeuri_getattr(ctx, MIMEURIATTR_PATH),
											TT_Sound_Play_Mode, (PS_MPEGA | PSF_QUEUED_IMMEDIATE),
										TAG_DONE); /* XXX */
									}
									#endif
									#if USE_MULTIMEDIA
									else if (!strcmp(s, MIMETYPE_INTERNAL_MULTIMEDIA)) /* XXX: hack! */
									{
										do_action(obj, TA_Sound_Play,
											TT_Sound_Play_Path, mimeuri_getattr(ctx, MIMEURIATTR_PATH),
											TT_Sound_Play_Mode, (PS_MULTIMEDIA | PSF_QUEUED_IMMEDIATE),
										TAG_DONE);
									}
									#endif
									#if USE_DATATYPES_SOUND
									else if (!strcmp(s, MIMETYPE_INTERNAL_DATATYPES)) /* XXX: hack! */
									{
										do_action(obj, TA_Sound_Play,
											TT_Sound_Play_Path, mimeuri_getattr(ctx, MIMEURIATTR_PATH),
											TT_Sound_Play_Mode, (PS_DATATYPES | PSF_QUEUED_IMMEDIATE),
										TAG_DONE);
									}
									#endif
									/* XXX: show up a message ? */
								}
								break;

							case MIMEACTION_NONE:
								smartreq_info("Ambient mimetypes", MV_Notification_Warning, "No action for for\n%s.", mimeuri_getattr(ctx, MIMEURIATTR_LOCAL) ? mimeuri_getattr(ctx, MIMEURIATTR_PATH) : mimeuri_getattr(ctx, MIMEURIATTR_URI));

								break;

							case MIMEACTION_DATATYPES:
								{
									STRPTR path;

									if ( (path = mimeuri_getattr(ctx, MIMEURIATTR_PATH)) )
									{
										switch ((LONG)mimeuri_getattr(ctx, MIMEURIATTR_MIME_SUBTYPE))
										{
											case DTTYPE_NONE:
												smartreq_info("Ambient mimetypes", MV_Notification_Warning, "No datatypes for\n%s.", mimeuri_getattr(ctx, MIMEURIATTR_LOCAL) ? path : mimeuri_getattr(ctx, MIMEURIATTR_URI));
												break;

											case DTTYPE_SOUND:
												//mimeuri_setattr(ctx, MIMEURIATTR_MIMETYPE, "sound/*");
												mimeuri_setattrs(ctx,
													MIMEURIATTR_MIMETYPE, MIMETYPE_INTERNAL_DATATYPES, /* XXX: hack */
													MIMEURIATTR_MIME_ACTION, MIMEACTION_PLAYSOUND,
													MIMEURIATTR_MIME_NEWWIN, FALSE,
												TAG_DONE);
												goto rescan;

											case DTTYPE_IMAGE:
												mimeuri_setattrs(ctx,
													MIMEURIATTR_MIMETYPE, "image/*",
													MIMEURIATTR_MIME_ACTION, MIMEACTION_VIEW,
													//MUIA_MIME_NEWWIN, FALSE,
												TAG_DONE);
												goto rescan;

											case DTTYPE_TEXT:
												mimeuri_setattrs(ctx,
													MIMEURIATTR_MIMETYPE, "text/*",
													MIMEURIATTR_MIME_ACTION, MIMEACTION_VIEW,
													//MIMEURIATTR_MIME_NEWWIN, TRUE,
												TAG_DONE);
												goto rescan;

											case DTTYPE_HEX:
												mimeuri_setattrs(ctx,
													MIMEURIATTR_MIMETYPE, "internal/x-morphos-hex-file",
													MIMEURIATTR_MIME_ACTION, MIMEACTION_VIEW,
													//MIMEURIATTR_MIME_NEWWIN, TRUE,
												TAG_DONE);
												goto rescan;

											case DTTYPE_BOOPSI:
												//smartreq_info("Ambient mimetypes", "No boopsiview yet for\n%s.", mimeuri_getattr(ctx, MIMEURIATTR_LOCAL) ? path : mimeuri_getattr(ctx, MIMEURIATTR_URI));
												mimeuri_setattrs(ctx,
													MIMEURIATTR_MIMETYPE, "boopsi/*",
													MIMEURIATTR_MIME_ACTION, MIMEACTION_VIEW,
												TAG_DONE);
												goto rescan;

											#ifdef DEBUG
											default:
												PDB(("no datatype action\n"));
												break;
											#endif
										}
									}
									/* XXX */
								}
								break;

							#if USE_MULTIMEDIA
							case MIMEACTION_MULTIMEDIA:
								{
									STRPTR path;
									if ( (path = mimeuri_getattr(ctx, MIMEURIATTR_PATH)) )
									{
										switch ((LONG)mimeuri_getattr(ctx, MIMEURIATTR_MIME_SUBTYPE))
										{
											case MULTIMEDIATYPE_NONE:
												/* XXX: just fail so it tries datatypes then.. */
												break;

											case MULTIMEDIATYPE_SOUND:
												mimeuri_setattrs(ctx,
													MIMEURIATTR_MIMETYPE, MIMETYPE_INTERNAL_MULTIMEDIA,  /* XXX: hack! */
													MIMEURIATTR_MIME_ACTION, MIMEACTION_PLAYSOUND,
													MIMEURIATTR_MIME_NEWWIN, FALSE,
												TAG_DONE);
												goto rescan;
											case MULTIMEDIATYPE_VIDEO:
												bug("MULTIMEDIATYPE_VIDEO\n");
												mimeuri_setattrs(ctx,
												// MIMEURIATTR_MIMETYPE, MIMETYPE_INTERNAL_MULTIMEDIA,  // XXX: hack! 
													MIMEURIATTR_MIMETYPE, "image/*",
													MIMEURIATTR_MIME_ACTION, MIMEACTION_VIEW,
												//	MIMEURIATTR_MIME_NEWWIN, TRUE,
												TAG_DONE);
												goto rescan;
											#ifdef DEBUG
											default:
												PDB(("no multimedia action\n"));
												break;
											#endif
										}
									}
									/* XXX */
								}
								break;
							#endif

							case MIMEACTION_USER:
								{
									STRPTR path;

									if ( (path = mimeuri_getattr(ctx, MIMEURIATTR_PATH)) )
									{
										APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

										if ( dispatcher )
										{
											TEXT buf[PATH_SIZE+256];

											SetAttrs(dispatcher,
												MA_ActionDispatcher_Event, ACTION_EVENT_DOUBLECLICK,
												MA_ActionDispatcher_SrcID, -1,
												TAG_DONE
											);

											{
												UBYTE encpath[mimeuri_encodepath(NULL, 0, path)];

												mimeuri_encodepath(encpath, sizeof(encpath), path);

												snprintf(buf, sizeof(buf), "file:///%s", encpath);
											}
											DoMethod( dispatcher, MM_ActionDispatcher_AddURI, buf, TRUE );
											DoMethod( dispatcher, MM_ActionDispatcher_Execute );
										}
									}
								}
								break;
						}
					}
					else
					{
						D(MIMEURI,bug("mimetype: No mimetype found for %08lx\n", ctx));
						//smartreq_info("Ambient mimetypes", MV_Notification_Warning, "No mimetype for\n%s.", mimeuri_getattr(ctx, MIMEURIATTR_LOCAL) ? mimeuri_getattr(ctx, MIMEURIATTR_PATH) : mimeuri_getattr(ctx, MIMEURIATTR_URI));
					}
				}
				/* XXX: put a requester ? */
				if (!use_ctx && ctx)
				{
					mimeuri_delete(ctx);
				}
			}
			break;
	}

	return (0);
}


struct DragData
{
	APTR src;
	struct Screen *screen;
	struct Window *win;
	LONG x;
	LONG y;
};

static struct Window * dd_FindWindow(struct DragData *dd)
{
	struct Window *appwin = NULL;
	struct Layer  *layer  = WhichLayer(&dd->screen->LayerInfo, dd->screen->MouseX, dd->screen->MouseY);

	if (layer)
	{
		if (AppWindowObtain(layer->Window))
		{
			appwin = dd->win = layer->Window;
		}
		else
		{
			appwin = NULL;
		}
		AppWindowRelease();
	}

	return(appwin);
}

/* No ambient object found for drop, so it's an external appwindow */
DEFMMETHOD(DragDrop)
{
	struct DragData dd;
	struct Window *win = NULL;

	dd.src    = msg->obj;
	dd.screen = _screen(msg->obj);
	dd.x      = _screen(msg->obj)->MouseX;
	dd.y      = _screen(msg->obj)->MouseY;

	win = dd_FindWindow(&dd);

	if (win)
	{
		struct ipc_appwindow *appwin;

		if ( (appwin = (APTR)AppWindowObtain(dd.win)) )
		{
			struct wbargs *wba;

			if ( (wba = wba_create(dd.src)) )
			{
				do_action(app, TA_AppMsg_Send,/* XXX: I *think* 'app' is ok here.. check */
					TT_AppMsg_Send_Type,      AMTYPE_APPWINDOW,
					TT_AppMsg_Send_Window,    dd.win,
					TT_AppMsg_Send_Path,      wba->basepath,
					TT_AppMsg_Send_ID,        appwin->id,
					TT_AppMsg_Send_Userdata,  appwin->userdata,
					TT_AppMsg_Send_NumArgs,   wba->count,
					TT_AppMsg_Send_WBArgList, wba->wba,
					TT_AppMsg_Send_MouseX,    dd.x,
					TT_AppMsg_Send_MouseY,    dd.y,
				TAG_DONE);

				wba_delete(wba);
			}
		}
		AppWindowRelease();
	}

	return (0);
}

/* delayed execution for shortcut icon removal (give time to contextmenu to return) */
DEFSMETHOD(Application_RemoveShortcut)
{
	GETDATA;
	STRPTR path;

	path = (STRPTR)getv(msg->icon, MA_Icon_Path);

	if (path && *path && path[strlen(path)-1] == ':')
	{
		/* Also in iconviewclass.c/MM_Iconview_RemoveByName */
		hiddendrives_updatedevice(obj, path, FALSE, getv(msg->icon, MA_Icon_X), getv(msg->icon, MA_Icon_Y));
	}

	DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_DeleteIcon, msg->icon);

	DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_DoLayout);

	DoMethod(data->rootwin, MM_Window_DoView, NULL, MM_Iconview_SaveShortcuts);

	return (0);
}


DEFTMETHOD(Application_CommitStorage)
{
	storage_commit();

	return (0);
}


DEFSMETHOD(Application_OpenDevicesWindow)
{
	TEXT buf[128];
	ULONG flags;
	LONG x, y, w, h;

	dprefs_mymorphos_window_get(&x, &y, &w, &h, &flags);

	if (msg->Moused)
		x = y = -1;

	snprintf(buf, sizeof(buf), "LoadURI devices://?left=%ld&top=%ld&width=%ld&height=%ld&view=%s NEWWIN", x, y, w, h, flags == MV_Icon_ViewMode_Lister ? "DLIST" : "ICONS");

	return (DoMethod(obj, MM_Application_DoRexx, TRUE, NULL, buf, NULL, NULL, 0, NULL));
}


DEFTMETHOD(Application_ReloadBackgrounds)
{
	GETDATA;
	ULONG flags = 0;

	if(--data->bgrandom_timer_count == 0)
	{
		data->bgrandom_timer_count = data->bgrandom_timer_wait;

		if(getprefslong(DSI_BACKGROUND_ROOT_BGRENDER) != BGRENDER_Color && isdir(getprefsstr(DSI_BACKGROUND_ROOT)))
		{
			flags |= MF_Application_LoadBackground_Root;
		}

		if(getprefslong(DSI_BACKGROUND_WINDOW_BGRENDER) != BGRENDER_Color && isdir(getprefsstr(DSI_BACKGROUND_WINDOW)))
		{
			flags |= MF_Application_LoadBackground_Window;
		}

		if(flags)
		{
			DoMethod(obj, MM_Application_LoadBackground, flags);
		}
	}

	return (0);
}

DEFTMETHOD(Application_CleanupCache)
{
	GETDATA;

	if(--data->cachecleanup_timer_count == 0)
	{
		data->cachecleanup_timer_count = data->cachecleanup_timer_wait;

		/*
		 * TODO: Make this one asynchronous? Probably good idea...
		 */

		/*
		 * Clean generic cache.
		 */

		cache_deleteunused();

		/*
		 * Clean deficon pool.
		 */

		deficonpool_flush();

	}

	return (0);
}

DEFSMETHOD(Application_SoundControl)
{
	switch(msg->command)
	{
		case MV_Application_SoundControl_Pause:
			execute_command(app, AC_INTERNAL, "Sound Pause", NULL);
			break;
		case MV_Application_SoundControl_Stop:
			execute_command(app, AC_INTERNAL, "Sound Stop", NULL);
			break;
		case MV_Application_SoundControl_OpenControl:
			soundwin_play();
			break;

	}

	return 0;
}

DEFSMETHOD(Application_GetMimeType)
{
	struct internal_mimetype_node *mimetype = NULL;

	if (msg->path)
	{
		/*
		 * Needs to be found. This is a bit problematic as it should be done on
		 * a thread but for now we do this workaround.
		 */

		LONG cnt = 0;
		Object *mimeTypeObject = DoMethod(NewObject(getmimetypeclass(), NULL, TAG_DONE), OM_RETAIN);

		if (do_action(obj, TA_MimeType_Scan,
						TT_MimeType_Scan_Path, msg->path,
						TT_MimeType_Scan_MimetypeObject, mimeTypeObject,
					TAG_DONE))
		{
			/*
			 * We give it max 1s to find the type.
			 */

			while( cnt < 25 && !xget(mimeTypeObject, MA_Mimetype_TypeResolved))
			{
				Delay(2);
				cnt++;
				methodstack_check(FALSE);
			}
		}
		
		mimetype = (struct internal_mimetype_node *)xget(mimeTypeObject, MA_Mimetype_Type);
		DoMethod(mimeTypeObject, OM_RELEASE);
	}

	return (ULONG) mimetype;
}

static ULONG application_name_build_info(STRPTR name, STRPTR info)
{
	ULONG len;
	ULONG found = FALSE;

	ASSERT(name);

	len = strlen(name);

	/* volume/assign name?... */
	if (len && name[len - 1] == ':')
	{
		stccpy(info, name, PATH_SIZE);
		strncat(info, "disk.info", PATH_SIZE);

		/* No disk.info, let's check if a def_thing exists */
		if (!exists(info))
		{
			STRPTR filename = deficon_build_devicename(name);
			if(filename)
			{
				found = TRUE;
				stccpy(info, filename, PATH_SIZE);
				name_delete(filename);
			}
		}
		else
		{
			found = TRUE;
		}
	}
	else /* ... or normal path? */
	{
		stccpy(info, name, PATH_SIZE);

		if (!(len > 5
			&& name[len - 5] == '.'
			&& (name[len - 4] == 'i' || name[len - 4] == 'I')
			&& (name[len - 3] == 'n' || name[len - 3] == 'N')
			&& (name[len - 2] == 'f' || name[len - 2] == 'F')
			&& (name[len - 1] == 'o' || name[len - 1] == 'O')
		))
		{
			strncat(info, ".info", PATH_SIZE);

			if(exists(info))
			{
				found = TRUE;
			}
		}
		else
		{
			found = TRUE;
		}
	}

	return(found);
}

static void application_get_icon_name(STRPTR path, APTR mimetype, STRPTR info, ULONG * isdefault)
{
	ULONG refine;

	if(path)
	{
		/* does icon exist ? */
		if(application_name_build_info(path, info))
		{
			return ;
		}

		*isdefault = TRUE;

		/* is it a directory ? */
		if(isdir(path))
		{
			deficon_getpath(info, PATH_SIZE, "def_drawer.info");
			return;
		}
	}

	if(path || mimetype)
	{
	/* use iconmime hack to get icon name */
		if(new_deficonpool_build_name(MV_ViewID_Unknown, path, mimetype, &refine, info, PATH_SIZE))
		{
			return;
		}
	}

	/* fallback to def_tool.info then */
	deficon_getpath(info, PATH_SIZE, "default.info");
}

#include <clib/debug_protos.h>

/* result must be freed by caller */
DEFSMETHOD(Application_GetDeficonPath)
{
	APTR mimetype = NULL;
	STRPTR info = NULL;

	if (msg->path)
	{
		mimetype = (APTR) DoMethod(obj, MM_Application_GetMimeType, msg->path);
	}
	else if(msg->mimetype)
	{
		mimetype = mimetype_find_by_mimetype(msg->mimetype);
	}

	if(mimetype)
	{
		char buffer[PATH_SIZE];
		int len = 0;
		ULONG isdefault = FALSE;

		application_get_icon_name(msg->path, mimetype, buffer, &isdefault);
		len = strlen(buffer) + 1;
		info = (char *) malloc(len);
		if(info)
			stccpy(info, buffer, len);
	}

	return (ULONG) info;
}

DEFSMETHOD(Mimegroup_Mime_Ack)
{
	DoMethod(msg->mimeobj, MM_Mimeadjustwin_Close);

	return (0);
}

ULONG tr_dispose_objects( APTR obj, APTR *array, ULONG freearray )
{
	ULONG rc = TRUE;
	ULONG i  = 0;
	ULONG t  = timedm();

	THREAD;

	/*
	 * For safety reasons, maybe we should increase priority
	 * if it's taking too long?
	 */

	DB(("Disposing objects...\n"));

	if ( !array )
	{
		return rc;
	}

	/*
	 * Tiny bit of explanation:) We release objects in groups starting from end of an array.
	 * This is to eliminate need for temporary arrays (null-terminated). Arrays become
	 * auto null-terminated because MM_Application_DisposeObjectArray nulls each entry,
	 * so next pushmethod will have properly terminated array to work on.
	 */

	while( array[ i ] )
	{
		i++;
	}

	while(1)
	{
		LONG first = i - 20;
		if ( first < 0 )
		{
			first = 0;
		}

		//DB(("Disposing range:%d - %d (0x%x,0x%x)\n", first, i,array[first],array[i]));

		DoMethod( obj, MUIM_Application_PushMethod, obj, 4, MM_Application_DisposeObjectArray, array + first, FALSE, FALSE );

		if ( first == 0 )
		{
			break;
		}

		i = first;

	};

	if ( freearray )
	{
		/* we can't free() array here as it's still used! */

		DoMethod( obj, MUIM_Application_PushMethod, obj, 4, MM_Application_DisposeObjectArray, array, FALSE, TRUE );

	}

	if ( threads_check_abort() )
	{
		rc = ABORTED;
	}

	DB(("Finished disposing objects (%dms)\n", timedm() - t ));

	return rc;
}

static ULONG dispatch(void);
static const struct EmulLibEntry GATE_dispatch =
{
	TRAP_LIB, 0, (void (*)(void)) dispatch
};
static ULONG dispatch(void) \
{
	struct IClass *cl = (struct IClass *)REG_A0;
	Msg msg = (Msg)REG_A1;
	Object *obj = (Object *)REG_A2;
	#ifdef DEBUG
	#if 1
	/*
	 * Damn stuntzi sends a MUIM_Applist_Find from asl.library to all mui tasks when
	 * the requester is freed. This method gets some MUIA_Application_Process/BrokerPort or so
	 * so we have to ignore OM_GET for the sanity check.
	 */
	if (msg->MethodID != MUIM_Application_PushMethod && msg->MethodID != OM_GET) MAINTASK;
	#else
	if (msg->MethodID != MUIM_Application_PushMethod)
	{
		if (FindTask(NULL) != dmt)
		{
			PDB(("not main: methodid: 0x%lx\n", msg->MethodID));
			showppcstackhistory((ULONG *) __builtin_frame_address(0),
										(ULONG *) ((UBYTE *)__builtin_frame_address(0) + 1024));
		}
	}
	#endif
	#endif
	switch (msg->MethodID)
	{
DECNEW
DECDISP
DECGET
DECSET
DECSMETHOD(Application_CheckDelayedDialog)
DECSMETHOD(Application_CreateWindow)
DECSMETHOD(Application_DisplayDelayedDialog)
DECSMETHOD(Application_DisposeWindow)
DECSMETHOD(Application_CreateIcon)
DECSMETHOD(Application_DisposeObject)
DECSMETHOD(Application_DisposeObjectArray)
DECTMETHOD(Application_CreateIconinfo)
DECSMETHOD(Application_CreateInfowin)
DECSMETHOD(Application_CreateProgresswin)
DECSMETHOD(Application_CreatePanelwin)
DECSMETHOD(Application_CreatePanelitem)
DECSMETHOD(Application_CreateSmartReq)
DECSMETHOD(Application_CreateDestSelectorwin)
DECSMETHOD(Application_CreatePatternRenamewin)
DECSMETHOD(Application_AddAppIcon)
DECSMETHOD(Application_LoadPrefs)
DECSMETHOD(Application_SetFont)
DECTMETHOD(Application_Cleanup)
DECTMETHOD(Application_ReclaimThreads)
DECSMETHOD(Application_WindowDoMethodByAttr)
DECSMETHOD(Application_RootDoMethodByAttr)
DECSMETHOD(Application_FindWindowByID)
DECSMETHOD(Application_FindWindowByName)
DECSMETHOD(Application_FindWindowByType)
DECSMETHOD(Application_FindWindowByUserData)
DECSMETHOD(Application_DisplayUpdate)
DECTMETHOD(Application_Open_AboutWindow)
DECSMETHOD(Application_Open_AboutMorphOSWindow)
DECTMETHOD(Application_Open_CxWindow)
DECTMETHOD(Application_Open_SystemInfoWindow)
#if !USE_LEGACY
DECTMETHOD(Application_Open_SystemLog)
#endif
DECTMETHOD(Application_Open_ExecuteWindow)
DECTMETHOD(Application_NewShell)
DECSMETHOD(Application_DoRexx)
DECSMETHOD(Application_Delete_Ok)
DECSMETHOD(Application_Trash_Ok)
DECSMETHOD(Application_SavePrefs)
DECSMETHOD(Application_ChangeScreenTitle)
DECTMETHOD(Application_RestoreScreenTitle)
#if USE_DOSNOTIFY
DECSMETHOD(Application_DOSNotify)
DECSMETHOD(Application_EnableDOSNotify)
#endif
DECTMETHOD(Application_CreateActionDispatcher)
DECSMETHOD(Application_DeleteActionDispatcher)
DECSMETHOD(Application_AddBackground)
DECSMETHOD(Application_LoadBackground)
DECSMETHOD(Application_CenterBackground)
DECTMETHOD(Application_CountWindows)
DECSMETHOD(Application_DeleteAppIcon)
DECSMETHOD(Application_Shutdown)
#if REFUSE_ICONIFY
DECMMETHOD(Application_RefreshScreen)
DECMMETHOD(Application_OpenPublic)
DECMMETHOD(Application_ClosePublic)
#endif
#ifdef DEBUG
DECSMETHOD(Application_SetDebug)
DECSMETHOD(Application_DoDebug)
#endif /* DEBUG */
DECSMETHOD(Application_ReloadIcons)
DECSMETHOD(Application_RemoveShortcut)
DECSMETHOD(Thread_Finished)
DECTMETHOD(Application_CommitStorage)
DECMMETHOD(DragDrop)
DECTMETHOD(Application_OpenDevicesWindow)
DECTMETHOD(Application_ReloadBackgrounds)
DECTMETHOD(Application_CleanupCache)
DECSMETHOD(Application_SoundControl)
DECSMETHOD(Application_GetMimeType)
DECSMETHOD(Application_GetDeficonPath)
DECSMETHOD(Mimegroup_Mime_Ack)
DECTMETHOD(Application_UpdateTransitionEffect)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Application, appclass)
