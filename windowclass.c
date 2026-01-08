/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: windowclass.c,v 1.43 2023/01/18 02:53:09 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <devices/rawkeycodes.h>
#include <intuition/intuition.h>
#include <graphics/layers.h>
#include <libraries/gadtools.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <workbench/workbench.h> /* XXX: for NO_ICON_POSITION.. sucks */
#include <libraries/mui.h> // needed for PropObject, don't know why...

/* private */
#include "args.h"
#include "gfx_bitmap.h"
#include "menus.h"
#include "mui_func.h"
#include "threads.h"
#include "methodstack.h"
#include "prefs.h"
#include "dosnotify.h"
#include "iconview.h"
#include "screen.h"
#include "common_picture.h"
#include "background.h"
#include "rexx.h"
#include "command.h"
#include "viewapi.h"
#include "vfs.h"
#include "deficonpool.h"
#include "trashcan.h"
#include "prefs_advanced.h" // bitRocky: for checking "differentInactiveWindowTitles"
#include "networksfs.h"

struct Data {
	APTR viewobj;
	STRPTR wintitle;
	LONG  wintitlesize;
	ULONG type;
	ULONG closing;
	ULONG id;
	APTR nr;
	ULONG backfill_enabled;
	struct Hook *bfhook;
	ULONG xo;
	ULONG yo;
	APTR dtp;
	ULONG bgmode;
	ULONG bgpen;
	ULONG browser;
	APTR toolbar;
	APTR toolbar_separator;
	APTR statusbar;
	ULONG is_root;
	APTR menustrip;

	/* iconify stuff */

	LONG  window;
	LONG  win_top;
	LONG  win_left;
	ULONG win_width;
	ULONG win_height;

	ULONG is_iconified;
	APTR  iconify_obj;

	struct MinList *handlerlist;
};

struct backfill_msg {
	struct Layer *layer;
	struct Rectangle Bounds;
	LONG OffsetX;
	LONG OffsetY;
};


static LONG bffunc_GATE(void);
static const struct EmulLibEntry bffunc = {
	TRAP_LIB, 0, (void(*)(void))bffunc_GATE
};
static LONG bffunc_GATE(void)
{
	struct Hook *h = (struct Hook *)REG_A0;
	struct RastPort *rp = (struct RastPort *)REG_A2;
	struct backfill_msg *bfm = (struct backfill_msg *)REG_A1;

	struct Data *data = (struct Data *)h->h_Data;

	APTR bm = data->dtp ? picture_getattr( data->dtp, PICTURE_BITMAP ) : 0;

	int so_x = (data->xo + bfm->OffsetX) % (bm ? gfx_bitmap_width(bm) : 0);
	int so_y = (data->yo + bfm->OffsetY) % (bm ? gfx_bitmap_height(bm) : 0);

	if ( data->backfill_enabled ) /* XXX: don't forget to set it when needed :) */
	{
		background_blit(data->bgmode, bm, data->bgpen, so_x, so_y, rp,
			bfm->Bounds.MinX, bfm->Bounds.MinY,
			bfm->Bounds.MaxX, bfm->Bounds.MaxY
		);
	}
	return (0);
}

static ULONG add_iconified_window(struct Data * data, CONST_STRPTR path, ULONG viewid)
{
	APTR o;
	ULONG res = FALSE;

	if(path && viewid > 1)
	{
		if ( (o = (APTR)methodstack_push_sync(app, 3, MM_Application_CreateIcon, TRUE, MV_ViewID_Root)) )
		{
			if (deficonpool_apply_default_icon(o, MV_Icon_Type_View, NULL, NULL))
			{
				/* XXX: find something smart for position */
				ULONG x = NO_ICON_POSITION, y = NO_ICON_POSITION;

				if (x != NO_ICON_POSITION && y != NO_ICON_POSITION)
				{
					methodstack_push(o, 3, MUIM_Set, MA_Icon_X, x);
					methodstack_push(o, 3, MUIM_Set, MA_Icon_Y, y);
					methodstack_push(o, 3, MUIM_Set, MA_Icon_HasPos, TRUE);
				}

				methodstack_push(o, 3, MUIM_Set, MA_Icon_Fade, TRUE);
				methodstack_push(o, 3, MUIM_Set, MA_Icon_Type, MV_Icon_Type_View);

				methodstack_push(o, 3, MUIM_Set, MA_Icon_AppID, viewid); /* abusing appid to store viewid */

				if(path[0])
				{
					methodstack_push_sync(o, 3, MUIM_Set, MA_Icon_Path, path);
				}
				else
				{
					methodstack_push_sync(o, 3, MUIM_Set, MA_Icon_Path, data->wintitle);
				}

				methodstack_push(app, 8, MM_Application_RootDoMethodByAttr, NULL, NULL,
							  MM_Window_DoView, NULL,
							  MM_Iconview_AddIcon, o, FALSE);

				methodstack_push_sync(app, 6, MM_Application_RootDoMethodByAttr, NULL, NULL,
							  MM_Window_DoView, NULL,
							  MM_Iconview_DoLayout);

				data->iconify_obj = o;
				res = TRUE;
			}
			else
			{
				methodstack_push(o, 1, OM_RELEASE);
			}
		}
	}

	return res;
}

static void remove_iconified_window(struct Data * data)
{
	methodstack_push(app, 7, MM_Application_RootDoMethodByAttr, NULL, NULL,
				  MM_Window_DoView, NULL,
				  MM_Iconview_DeleteIcon, data->iconify_obj);

	methodstack_push_sync(app, 6, MM_Application_RootDoMethodByAttr, NULL, NULL,
				  MM_Window_DoView, NULL,
				  MM_Iconview_DoLayout);

	data->iconify_obj = NULL;
}

// bitRocky: needed for MM_Window_UpdateBackground, because we need to delay it at first
// startup of Ambient afte boot else the background color will be black (for centered/tiled backgrounds)!
ULONG _firstWin = TRUE;
// bitRocky: these two are set in OM_NEW and used in MUIM_Setup, only for the first startup!
ULONG _bgmode = 0;
APTR _dtp = NULL;

DEFNEW
{
	struct Data *data;
	ULONG type = MV_Window_Type_Unknown; /* shut up gcc */
	APTR viewobj, root;
	APTR toolbar = NULL;
	APTR toolbar_separator = NULL;
	APTR statusbar = NULL;
	APTR mimectx = NULL;
	APTR dtp = NULL;
	APTR menu;
	APTR sb_seperator = NULL;
	ULONG bgmode = 0; /* XXX: put a proper define.. */
	ULONG browser = _conf(toolbar_browsermode);
	ULONG viewid = getv(app, MA_Application_NextID); /* used for notifies and identifications */

	struct Hook *bfhook = NULL;

	FORTAG(INITTAGS)
	{
		case MA_Window_Type:
			type = tag->ti_Data;
			break;

		case MA_Window_MIMEctx:
			mimectx = (APTR)tag->ti_Data;
			break;

		case MA_Window_DTP:
			dtp = (APTR)tag->ti_Data;
			break;

		case MA_Window_BGMode:
			bgmode = tag->ti_Data;
			break;

		case MA_Window_Browser:
			browser	= tag->ti_Data;
			break;
	}
	NEXTTAG

	ASSERT(type);

	if (!(bfhook = malloc(sizeof(*bfhook))))
 	{
		return (size_t)(NULL);
 	}

	viewobj = root = NewObject(getviewgroupclass(), NULL, MA_Viewgroup_ID, viewid, MA_View_IsRoot, (type == MV_Window_Type_Rootview), MA_Viewgroup_MIMEctx, mimectx, TAG_DONE);

	if (type != MV_Window_Type_Rootview)
	{
		root = VGroup,
			MUIA_Group_VertSpacing, 0,
			InnerSpacing(0, 0),
			NoFrame,
			Child, toolbar = NewObject(gettoolbarclass(), NULL, MA_Toolbar_Viewobj, viewobj, MUIA_ShowMe, browser ? TRUE : FALSE, TAG_DONE),
			Child, toolbar_separator = RectangleObject, MUIA_Rectangle_HBar, TRUE, MUIA_Weight, 0, MUIA_ShowMe, browser ? TRUE : FALSE, End,
			Child, root,
			Child, sb_seperator = RectangleObject, MUIA_Rectangle_HBar, TRUE, MUIA_Weight, 0, End,
			Child, statusbar = NewObject(getstatusbarclass(), NULL, TAG_DONE),
		End;
	}

	obj = DoSuperNew(cl, obj,
		MUIA_Window_Menustrip,   menu = MUI_MakeObject(MUIO_MenustripNM, (IPTR)newmenus, 0),
		MUIA_Window_Screen, get_screen(),
		MUIA_Window_ShowIconify, TRUE,
		MUIA_Window_ShowPrefs, FALSE,
		MUIA_Window_ShowJump, FALSE,
		MUIA_Window_ShowSnapshot, FALSE,
		MUIA_Window_ShowPopup, FALSE,
		MUIA_Window_ShowAbout, FALSE,
		MUIA_Window_ScreenTitle, screentitle,
		MUIA_Window_AllowTopMenus, FALSE,
		(type == MV_Window_Type_Rootview) ? TAG_IGNORE : MUIA_Window_UseRightBorderScroller, TRUE, /* XXX: is that true for every viewmode ? don't think so.. */
		(type == MV_Window_Type_Rootview) ? TAG_IGNORE : MUIA_Window_UseBottomBorderScroller, TRUE,
		MUIA_Window_BackfillHook, bfhook,
		MUIA_Window_RootObject, root,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		if (bfhook)
		{
			free(bfhook);
		}
		return (size_t)(NULL);
	}

	data = INST_DATA(cl, obj);

	data->viewobj = viewobj;
	data->type = type;
	data->toolbar = toolbar;
	data->toolbar_separator = toolbar_separator;
	data->statusbar = statusbar;
	data->id = viewid;
	data->nr = NULL;
	data->browser = browser;
	data->is_root = type == MV_Window_Type_Rootview;
	data->is_iconified = FALSE;
	data->iconify_obj = NULL;
	data->menustrip = menu;

	DoMethod(data->statusbar, MUIM_Notify, MUIA_ShowMe, MUIV_EveryTime, sb_seperator, 3, MUIM_Set, MUIA_ShowMe, MUIV_TriggerValue);

	DoSuperMethod(cl, obj, MUIM_Notify, MUIA_Window_MenuAction, MUIV_EveryTime,
		obj, 2, MM_Window_MenuAction, MUIV_TriggerValue);
		
	data->bfhook = bfhook;
	data->bfhook->h_Entry = (APTR)&bffunc;
	data->bfhook->h_Data = (APTR)data;

	/* insert first entry */

	if (type != MV_Window_Type_Rootview)
	{
		DoSuperMethod(cl, obj, MUIM_Notify, MUIA_Window_CloseRequest, TRUE,
			MUIV_Notify_Application, 5, MUIM_Application_PushMethod, obj, 2, MM_Window_Close, TRUE
		);

		if (toolbar)
		{
			struct viewnode *vn = viewapi_findbymime( (APTR)getv( data->viewobj, MA_View_MIME ) );
			char *mode = viewapi_getmodename( vn, getv(data->viewobj, MA_View_ModeIndex) );
			char *path = (STRPTR)getv(data->viewobj, MA_View_Path);

			DoMethod(toolbar, MM_Toolbar_HistoryAddURI, path, mode );
		}

		/* setup initial DOS notification */

		data->nr = dosnotify_start( (STRPTR)getv(data->viewobj, MA_View_Path), data->id );

		// bitRocky
		DoSuperMethod(cl, obj, MUIM_Notify, MUIA_Window_Activate, MUIV_EveryTime,
			obj, 2, MM_Window_IsActive, MUIV_TriggerValue
		);
	}
	else
	{
		set(FINDMENU(MENU_VIEW_NEWDRAWER), MUIA_Menuitem_Enabled, FALSE);
		set(FINDMENU(MENU_VIEW_LIST), MUIA_Menuitem_Enabled, FALSE);
		set(FINDMENU(MENU_VIEW_NETWORKSCONNECT), MUIA_Menuitem_Enabled, FALSE);
	}

	if (args.wbstartup)
	{
		menus_shutdown(menu);
	}

	#ifdef DEBUG
	menus_bind_debug(menu);
	#endif

	menus_user_add_to_view(obj);

	/* setup background */
DB(("_firstWin = %ld, _dtp = %p, dtp = %p\n", _firstWin, _dtp, dtp))
	if (dtp)
	{
		/*
		 * The window wants a background before it opens.
		 */
		if (_firstWin)
		{
			_dtp = dtp; _bgmode = bgmode;
			// bitRocky: this has to be done only for the first start after boot!
			// MM_Window_UpdateBackground will be done in MUIM_Setup, cause we need to delay it a bit
		}
		else
		{
			DoMethod(obj, MM_Window_UpdateBackground, dtp, bgmode);
		}
	}
	return ((ULONG)obj);
}


DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MA_Window_LeftOffset:
			{
				data->xo = tag->ti_Data;
			}
			break;

		case MA_Window_TopOffset:
			{
				data->yo = tag->ti_Data;
			}
			break;

		case MUIA_Window_CloseRequest:
			{
				if (data->type == MV_Window_Type_Rootview)
				{
					tag->ti_Tag = TAG_IGNORE; /* swallow it */
				}
			}
			break;

		case MA_Window_BgPen:
			{
				data->bgpen = tag->ti_Data;
				if (data->type == MV_Window_Type_Rootview)
				{
					data->bgmode = getprefslong(DSI_BACKGROUND_ROOT_BGRENDER);

				}
				else
				{
					data->bgmode = _conf(window_bgrender);
				}
			}
			break;

		case MA_Window_DoBackfill:
			{
				data->backfill_enabled = tag->ti_Data;
			}
			break;

		case MA_Window_EnableDOSNotify:
			{
				if ( tag->ti_Data )
				{
					if ( !data->nr )
						data->nr = dosnotify_start( (STRPTR)getv(data->viewobj, MA_View_Path), data->id );
				}
				else
				{
					if ( data->nr )
						dosnotify_stop( data->nr );
					data->nr = NULL;
				}

			}
			break;

		case MA_Window_MIMEctx:
			{
				ASSERT(data->viewobj);

				/* update current history node */

				DoMethod( obj, MM_Window_UpdateURI );

				set(data->viewobj, MA_Viewgroup_MIMEctx, tag->ti_Data);

				/* update dosnotify */

				dosnotify_stop( data->nr );
				data->nr = dosnotify_start( (STRPTR)getv(data->viewobj, MA_View_Path), data->id );

				/* add to history */

				if (data->toolbar)
				{
					struct viewnode *vn = viewapi_findbymime( (APTR)getv( data->viewobj, MA_View_MIME ) );
					char *mode = viewapi_getmodename( vn, getv(data->viewobj, MA_View_ModeIndex) );
					char *path = (STRPTR)getv(data->viewobj, MA_View_Path);

					DoMethod(data->toolbar, MM_Toolbar_HistoryAddURI, path, mode );
				}

				/* update new current history node will refresh strings if AddURI didn't do it */

				DoMethod( obj, MM_Window_UpdateURI );

			}
			break;

		case MA_Window_IsIconified:
			{
				data->is_iconified = tag->ti_Data;
			}
			break;

		case MUIA_Window_Open:
			{
				if(tag->ti_Data && getv(obj, MA_Window_IsIconified))
				{
					remove_iconified_window(data);
					set(obj, MA_Window_IsIconified, FALSE);

					if(data->window)
					{
						SetAttrs(obj, MUIA_Window_LeftEdge, data->win_left,
									  MUIA_Window_TopEdge, data->win_top,
									  MUIA_Window_Width, data->win_width,
									  MUIA_Window_Height, data->win_height,
									  TAG_DONE);
					}
				}
			}
			break;
	}
	NEXTTAG

	return (DOSUPER);
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Window_ID:
			*msg->opg_Storage = data->id;
			return (TRUE);

		case MA_Window_Path:
			*msg->opg_Storage = getv(data->viewobj, MA_View_Path);
			return (TRUE);

		case MA_Window_Type:
			*msg->opg_Storage = data->type;
			return (TRUE);

		case MA_View_HasBackground:
			{
				ULONG val = FALSE;

				get(data->viewobj, MA_View_HasBackground, &val);
				*msg->opg_Storage = val;
			}
			return (TRUE);

		case MA_Window_Viewobj:
			*msg->opg_Storage = (ULONG)data->viewobj;
			return (TRUE);

		case MA_Window_Browser:
			*msg->opg_Storage = data->browser;
			return (TRUE);

		/* Don't fetch this attribute unless you can update on background change. */
		case MA_Window_DTP:
			*msg->opg_Storage = (ULONG) data->dtp;
			return (TRUE);

		/* Don't fetch this attribute unless you can update on background change. */
		case MA_Window_BGMode:
			*msg->opg_Storage = data->bgmode;
			return (TRUE);

		/* Don't fetch this attribute unless you can update on background change. */
		case MA_Window_BgPen:
			*msg->opg_Storage = data->bgpen;
			return (TRUE);

		case MA_Window_IsIconified:
			*msg->opg_Storage = data->is_iconified;
			return (TRUE);
	}
	return (DOSUPER);
}


DEFDISP
{
	GETDATA;

	D(PROC,bug("disposing window object..\n"));

	#if USE_DOSNOTIFY
	dosnotify_stop(data->nr);
	data->nr = NULL;
	#endif

	if (data->bfhook)
	{
		free(data->bfhook);
	}

	if (data->wintitle)
	{
		free(data->wintitle);
	}

	#if 0
	killpushedmethods(data->viewobj); /* XXX: sure ? I think so */
	killpushedmethods(obj); /* XXX: sure ? */
	#endif

	D(PROC,bug("Disposing super\n"));

	vfs_release(obj, (STRPTR) getv(data->viewobj, MA_View_Path));

	return(DOSUPER);
}


DEFMMETHOD(Window_AddEventHandler)
{
	GETDATA;
	DOSUPER;

	if (!data->handlerlist)
	{
		struct MinNode *n = &msg->ehnode->ehn_Node;

		while (n->mln_Pred)
			n = n->mln_Pred;

		data->handlerlist = (struct MinList *)n;
	}

	return (0);
}


DEFSMETHOD(Window_Close)
{
	GETDATA;

	if (data->type == MV_Window_Type_Rootview)
	{
		if (msg->fromroot)
		{
			D(PROC, bug("is root, closing..\n"));
			set(obj, MUIA_Window_Open, FALSE);
			DoMethod(app, MUIM_Application_PushMethod, app, 2, MUIM_Application_ReturnID, MUIV_Application_ReturnID_Quit);
		}
	}
	else
	{
		/*
		 * Defer til the (possible) process returns.
		 */

		set(obj, MUIA_Window_Open, FALSE);
		D(PROC, bug("defering close return of %p\n", obj));
		data->closing = TRUE;
		DoMethod(data->viewobj, MM_View_Abort); /* broadcast */
	}
	return (0);
}


DEFTMETHOD(Window_Aborted)
{
	GETDATA;

	if (data->closing)
	{
		set(obj, MUIA_Window_Open, FALSE); /* just to be sure */
		DoMethod(app, MUIM_Application_PushMethod, app, 2, MM_Application_DisposeWindow, obj );
		D(PROC, bug("sent dispose method\n"));
	}
	return (0);
}


DEFSMETHOD(Window_FillBackground)
{
	GETDATA;

	if (data->bfhook)
	{
		struct backfill_msg bfm;

		bfm.layer = NULL;
		bfm.Bounds.MinX = 0;
		bfm.Bounds.MinY = 0;
		bfm.Bounds.MaxX = msg->xs - 1;
		bfm.Bounds.MaxY = msg->ys - 1;
		bfm.OffsetX = msg->x;
		bfm.OffsetY = msg->y;

		CallHookA(data->bfhook, (Object *)msg->rp, &bfm);
	}
	return (0);
}


/*
 * Most importantly; the method below occurs when prefschange occurs
 * and an update is needed.
 */
DEFSMETHOD(Window_UpdateBackground)
{
	GETDATA;

	if (getv(data->viewobj, MA_View_NeedsBackfill))
	{
		data->bgmode = msg->bgmode;
		data->dtp = msg->dtp;
	}
	else
	{
		/*
		 * The view renders the background itself then.
		 */
		data->dtp = NULL;
	}

	DoMethod(data->viewobj, MM_View_Refresh, MF_View_Refresh_Background, msg->dtp );

	if(data->statusbar)
		DoMethod(data->statusbar, MM_Statusbar_UpdateBackground);

	return (0);
}


DEFSMETHOD(Window_DoView)
{
	GETDATA;

	ASSERT(data->viewobj);

	/* XXX: we should check if the view is right ! */
	return (DoMethod(data->viewobj, MM_View_DoMethod, (Msg)&msg->args));
}


DEFMMETHOD(Window_Setup)
{
	ULONG rc;

	flush_screen();

	if ((rc = DOSUPER))
	{
		GETDATA;

		DB(("_firstWin = %ld, _dtp = %p\n", _firstWin, _dtp))
		if (_firstWin && _dtp) // bitRocky
		{
			//DoMethod(_app(obj), MUIM_Application_PushMethod, obj, 3 | MUIV_PushMethod_Delay(100), MM_Window_UpdateBackground, _dtp, _bgmode);
			// it has to be PushMethod, else the surrounding color of the background is black at startup!
			DoMethod(_app(obj), MUIM_Application_PushMethod, obj, 3, MM_Window_UpdateBackground, _dtp, _bgmode);
			_dtp = NULL; _bgmode = 0;
			_firstWin = FALSE;
		}
		// else: MM_Window_UpdateBackground is done in OM_NEW!
		if (_firstWin) _firstWin = FALSE;

		if (getv(data->viewobj, MA_View_NeedsBackfill))
		{
			data->backfill_enabled = TRUE;
		}
		else
		{
			data->backfill_enabled = FALSE;
		}

		if (!data->is_root)
			DoMethod(obj, MM_Window_UpdateStatusbar);
	}

	return (rc);
}


DEFMMETHOD(Window_Cleanup)
{
	flush_screen();
	return (DOSUPER);
}


#define WINTITLESIZE 256 /* minimum window title size */

DEFSMETHOD(Window_SetTitle)
{
	GETDATA;
	LONG size;

	if(data->is_root)
	{
		return (0);
	}

	/* NULL means clear the window title.. */
	if (!msg->name)
	{
		msg->name = "";
	}

	size = strlen(msg->name) + 1;
	size = max(size, WINTITLESIZE);

	if (data->wintitlesize < size)
	{
		if (data->wintitle)
		{
			free(data->wintitle);
		}
		data->wintitle = malloc(size);
		data->wintitlesize = size;
	}

	if (data->wintitle)
	{
		stccpy(data->wintitle, msg->name, size);
		if (_aprefs(differentinactivewindowtitles))
			DoMethod(obj, MM_Window_IsActive, getv(obj, MUIA_Window_Activate));
		else
			set(obj, MUIA_Window_Title, data->wintitle);
	}
	/* XXX */

	return (0);
}

DEFSMETHOD(Window_LoadURI)
{
	GETDATA;
	char *cmd = NULL;
	STRPTR path = msg->path;
	ULONG optionslen = 0;

	if(msg->options)
	{
		optionslen = strlen(msg->options);
	}

	if ( path && path[0] && msg->mode )
	{
		/* build URI from both path and mode */

		cmd = malloc( sizeof("LoadURI") - 1 + strlen( path ) + strlen( msg->mode ) + optionslen + 32 );

		if ( cmd )
			sprintf(cmd, "LoadURI \"%s?mode=%s\" VIEWID %ld %s", path, msg->mode, data->id, (msg->options) ? msg->options : (STRPTR) "" );

	}
	else if ( path && path[0] )
	{
		/* build URI from only path */

		cmd = malloc( sizeof("LoadURI") - 1 + strlen( path ) + optionslen + 32 );

		if ( cmd )
			sprintf(cmd, "LoadURI \"%s\" VIEWID %ld %s", path, data->id, (msg->options) ? msg->options : (STRPTR) "" );

	}
	else if(path)
	{
		cmd = malloc( sizeof("LoadURI") - 1 + sizeof("devices://") - 1 + optionslen + 32 );

		if ( cmd )
			sprintf(cmd, "LoadURI devices:// VIEWID %ld %s", data->id,(msg->options) ? msg->options : (STRPTR) "" );
	}

	if ( cmd )
	{
		/* load new URI */

		execute_command(data->viewobj, AC_INTERNAL, cmd, NULL);

		free( cmd );
		return (TRUE);
	}

	return (FALSE);
}


DEFTMETHOD(Window_UpdateURI)
{
	GETDATA;

	/* Get data from viewobject and update uri in toolbar */

	if (data->toolbar)
	{
		char *mode = NULL;
		char *path = (STRPTR) getv(data->viewobj, MA_View_Path);

		/* Don't try to inherit device list mode */
		if(!getv(data->viewobj, MA_View_ShowDevices) && path && *path != 0)
		{
			struct viewnode *vn = viewapi_findbymime( (APTR)getv( data->viewobj, MA_View_MIME ) );
			mode = viewapi_getmodename( vn, getv(data->viewobj, MA_View_ModeIndex) );
		}

		DoMethod(data->toolbar, MM_Toolbar_HistoryUpdateURI, path, mode );
	}

	return 0;
}


DEFTMETHOD(Window_Reload)
{
	GETDATA;

	/* Get data from viewobject and update uri in toolbar */
	struct viewnode *vn = viewapi_findbymime( (APTR)getv( data->viewobj, MA_View_MIME ) );
	STRPTR path = (STRPTR)getv(data->viewobj, MA_View_Path);
	STRPTR options = "NONEWWIN"; /* NONEWWIN prevents loaduri from opening a new win in spatial mode */

	/* XXX: Hmm... is this enough? */
	if (!path || !*path)
	{
		do_action(data->viewobj, TA_Devices_RemoveAll, TAG_DONE);
	}
	else
	{
		STRPTR mode = viewapi_getmodename( vn, getv(data->viewobj, MA_View_ModeIndex) );
		DB(("Refresh due to DOS notification?(%s,%s)\n", path, mode));
		DoMethod(obj, MM_Window_LoadURI, path, mode, options);
	}

	return 0;
}


DEFTMETHOD(Window_UpdateStatusbar)
{
	GETDATA;
	ULONG type;

	/* If no statusbar, we bail out. */
	if (!data->statusbar)
		return 0;

	type = getv(data->viewobj, MA_View_Type);

	/* Set statusbar to right background & state depending on type... */
	switch (type)
	{
		case MV_View_Type_Icon:
			set(data->statusbar, MUIA_ShowMe, TRUE);
			DoMethod(data->statusbar, MM_Statusbar_SetMode, MV_Statusbar_SetMode_Icon);
			break;

		case MV_View_Type_List:
			set(data->statusbar, MUIA_ShowMe, TRUE);
			DoMethod(data->statusbar, MM_Statusbar_SetMode, MV_Statusbar_SetMode_List);
			break;

		/* We currently dont use these, so lets hide em'! */
		case MV_View_Type_Image:
		case MV_View_Type_Boopsi:
		case MV_View_Type_Text:
		case MV_View_Type_ToolTypes:
			set(data->statusbar, MUIA_ShowMe, FALSE);
			break;
	}

	return 0;
}


DEFTMETHOD(Window_UpdateUI)
{
	GETDATA;

	if (data->is_root == FALSE)
	{
		/* rebuild toolbar */

		if (data->toolbar != NULL)
		{
			SetAttrs(data->toolbar,
				MA_Toolbargroup_Definition, _conf(toolbar_definition),
				MUIA_ShowMe, _conf(toolbar_browsermode),
				TAG_DONE
			 );
			set(data->toolbar_separator, MUIA_ShowMe, _conf(toolbar_browsermode));
		}

		/* update statusbars */

		if (data->statusbar != NULL)
		{
			DoMethod(data->statusbar, MM_Statusbar_Refresh);
		}

	}

	return 0;
}


DEFTMETHOD(Toolbar_HistoryPrev)
{
	GETDATA;
	return DoMethod(data->toolbar, MM_Toolbar_HistoryPrev);
}


DEFTMETHOD(Toolbar_HistoryNext)
{
	GETDATA;
	return DoMethod(data->toolbar, MM_Toolbar_HistoryNext);
}


DEFTMETHOD(Clickpath_Parent)
{
	GETDATA;
	return DoMethod(data->toolbar, MM_Clickpath_Parent);
}


static ULONG send_method(struct IClass *cl, APTR o, Msg msg)
{
	ULONG rc;

	if (cl)
	{
		rc = CoerceMethodA(cl, o, msg) & MUI_EventHandlerRC_Eat;
	}
	else
	{
		cl = OCLASS(o);

		do
		{
			rc = CoerceMethodA(cl, o, msg) & MUI_EventHandlerRC_Eat;
			cl = cl->cl_Super;
		}
		while (!rc && cl);
	}

	return rc;
}


static void send_muikey(struct Data *data, APTR obj, LONG muikey)
{
	if (data->handlerlist)
	{
		struct MUIP_HandleEvent event;
		struct MUI_EventHandlerNode *ehn;
		struct IntuiMessage imsg;
		ULONG ivalue = 0;
		APTR aobj, defobj;

		imsg.Class = IDCMP_RAWKEY;
		imsg.Code  = muikey == MUIKEY_COPY ? RAWKEY_C : muikey == MUIKEY_CUT ? RAWKEY_X : RAWKEY_V;
		imsg.Qualifier = IEQUALIFIER_RCOMMAND;
		imsg.IAddress  = &ivalue;
		imsg.IDCMPWindow = NULL;

		event.MethodID = MUIM_HandleEvent;
		event.imsg     = &imsg;
		event.muikey   = muikey;

		aobj = (APTR)getv(obj, MUIA_Window_ActiveObject);

		if (aobj)
		{
			ITERATELIST(ehn, data->handlerlist)
			{
				if (ehn->ehn_Object == aobj && ehn->ehn_Events & IDCMP_RAWKEY)
				{
					if (send_method(ehn->ehn_Class, aobj, (Msg)&event))
						return;
				}
			}
		}

		defobj = (APTR)getv(obj, MUIA_Window_DefaultObject);

		if (defobj && aobj != defobj)
		{
			ITERATELIST(ehn, data->handlerlist)
			{
				if (ehn->ehn_Object == defobj && ehn->ehn_Events & IDCMP_RAWKEY)
				{
					if (send_method(ehn->ehn_Class, defobj, (Msg)&event))
						return;
				}
			}
		}

		ITERATELIST(ehn, data->handlerlist)
		{
			if (ehn->ehn_Object != aobj && ehn->ehn_Object != defobj && ehn->ehn_Events & IDCMP_RAWKEY)
			{
				if (send_method(ehn->ehn_Class, ehn->ehn_Object, (Msg)&event))
					return;
			}
		}
	}
}


DEFSMETHOD(Window_MenuAction)
{
	GETDATA;
	CONST_STRPTR args = NULL;
	LONG target_icon = 0;
	LONG no_grouping = 0;
	LONG sortmenu = 0;
	APTR target = (APTR)getv(data->viewobj, MA_Viewgroup_CurrentView);

	/* Target Icon
	 *
	 * Some actions are done to an icon only. That is, the action is targetted to
	 * a single icon. Normally actions are done on group level.
	 */

	switch (msg->Action)
	{
		default: menus_execute(_app(obj), target, msg->Action); break;

		case MENU_EDIT_PASTE    :
		case MENU_EDIT_CUT      :
		case MENU_EDIT_COPY     :
			send_muikey(data, obj, msg->Action == MENU_EDIT_PASTE ? MUIKEY_PASTE : msg->Action == MENU_EDIT_COPY ? MUIKEY_COPY : MUIKEY_CUT);
			break;

		//case MENU_EDIT_PASTEINTO  : args = "ClipboardPaste"; target_icon = 1; no_grouping = 1; break;
		case MENU_EDIT_SELECTALL  : args = "Select All"; break;
		case MENU_EDIT_INVERT     : args = "Select"; break;

		case MENU_VIEW_NEWDRAWER       : args = "Makedir"; no_grouping = 1; break;
		case MENU_VIEW_NETWORKSCONNECT : args = "NetworksConnect"; no_grouping = 1; break;
		case MENU_VIEW_SORT_BYNAME: args = "Sort Alpha"; sortmenu = 1; break;
		case MENU_VIEW_SORT_BYTYPE: args = "Sort Type"; sortmenu = 2; break;
		case MENU_VIEW_SORT_BYSIZE: args = "Sort Size"; sortmenu = 3; break;
		case MENU_VIEW_SORT_BYDATE: args = "Sort Date"; sortmenu = 4;break;

		case MENU_ICONS_INFORMATION: args = "IconInfo"; target_icon = 1; break;
		case MENU_ICONS_PUTAWAY    : args = "Shortcut Remove"; target_icon = 1; break;
		case MENU_ICONS_EJECT      : args = "Eject"; target_icon = 1; /* no_grouping = 1;*/ break;
		case MENU_ICONS_RENAME     : args = "Rename"; target_icon = 1; break;
		case MENU_ICONS_DELETE     : args = "Delete"; target_icon = 1; break;
		case MENU_ICONS_TRASH      : if (trashcan_is_running() && !is_networksfs((CONST_STRPTR)getv(target, MA_View_Path))) { args = "Trash"; target_icon = 1; }; break;
		case MENU_ICONS_RESTORE    : if (is_trashcan((CONST_STRPTR)getv(target, MA_View_Path))) { args = "Restore"; target_icon = 1; }; break;
		case MENU_ICONS_EMPTY      : if (is_trashcan((CONST_STRPTR)getv(target, MA_View_Path))) { args = "EmptyTrashcan"; target_icon = 1; no_grouping = 1;}; break;
		case MENU_ICONS_FORMAT     : args = "Format"; target_icon = 1; no_grouping = 1; break;

		case MENU_VIEW_LIST :
			{
				/* No idea how this bullshit should work */

				struct viewnode *vn;
				STRPTR path, mode;

				path = (STRPTR)getv(target, MA_View_Path);
				vn   = viewapi_findbymime((APTR)getv(target, MA_View_MIME));
				mode = viewapi_getmodename(vn, getv(target, MA_View_ModeIndex));

				DoMethod(obj, MM_Window_LoadURI, path, mode, NULL);
			}
			break;
	}

	if (args)
	{
		ULONG grouped;

		if (sortmenu)
		{
			/* XXX: little hacky */
			LONG i, id;
			APTR menu = data->menustrip;

			for (i = 1; i <= 4; i++)
			{
				if (i != sortmenu)
				{
					switch (i)
					{
						default:
						case 1: id = MENU_VIEW_SORT_BYNAME; break;
						case 2: id = MENU_VIEW_SORT_BYTYPE; break;
						case 3: id = MENU_VIEW_SORT_BYSIZE; break;
						case 4: id = MENU_VIEW_SORT_BYDATE; break;
					}

					set(FINDMENU(id), MUIA_Menuitem_Checked, FALSE);
				}
			}
		}

		grouped = getv(data->viewobj, MA_View_NumSelected) > 1;

		if (grouped && !no_grouping)
		{
			execute_command_objarray(target, AC_INTERNAL, args);
		}
		else
		{
			if (!target_icon || (target = (APTR)getv(target, MA_View_SubObject)))
			{
				execute_command(target, AC_INTERNAL, args, NULL);
			}
		}
	}

	return (0);
}

DEFTMETHOD(Window_Iconify)
{
	GETDATA;

	if(!getv(obj, MA_Window_IsIconified))
	{
		struct Window * window = (struct Window *) getv(obj, MUIA_Window_Window);

		if(window)
		{
			data->win_top    = window->TopEdge;
			data->win_left   = window->LeftEdge;
			data->win_width  = window->Width;
			data->win_height = window->Height;

			data->window = TRUE;
		}
		else
		{
			data->window = FALSE;
		}

		/* XXX: Adding icon should be done in a thread */
		if(add_iconified_window(data, (CONST_STRPTR) getv(obj, MA_Window_Path), getv(obj, MA_Window_ID)))
		{
			DB(("Setting open to false\n"));
			SetAttrs(obj, MA_Window_IsIconified, TRUE, MUIA_Window_Open, FALSE, TAG_DONE);
		}
	}

	return (0);
}

DEFMMETHOD(Window_ActionIconify)
{
	return DoMethod(obj, MM_Window_Iconify);
}

DEFSMETHOD(Window_IsActive) // bitRocky
{
	GETDATA;

	if (_aprefs(differentinactivewindowtitles))
	{
		STRPTR path;
		path = (STRPTR)getv(data->viewobj, MA_View_Path);
		//DB(("MM_Window_IsActive: MA_View_Path = '%s', active = %ld\n", path, msg->active));

		set(obj, MUIA_Window_Title, (msg->active || !(path && path[0])) ? data->wintitle : path);
	}

	return(0);
}

DEFSMETHOD(Window_ZoomIcons) // bitRocky
{
	GETDATA;

	APTR currview = (APTR)getv(data->viewobj, MA_Viewgroup_CurrentView);
	//LONG isa = getv(currview, MA_Icon_SizeAdjustment);
	
	SetAttrs(currview,
		MUIA_Group_Forward, FALSE,
		MA_Icon_SizeAdjustment, msg->level,
	TAG_DONE);

	return(0);
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECSMETHOD(Window_Close)
DECSMETHOD(Window_FillBackground)
DECTMETHOD(Window_Aborted)
DECSMETHOD(Window_UpdateBackground)
DECSMETHOD(Window_DoView)
DECMMETHOD(Window_AddEventHandler)
DECMMETHOD(Window_Setup)
DECMMETHOD(Window_Cleanup)
DECSMETHOD(Window_SetTitle)
DECSMETHOD(Window_LoadURI)
DECTMETHOD(Window_UpdateURI)
DECTMETHOD(Window_Reload)
DECTMETHOD(Window_UpdateUI)
DECTMETHOD(Window_UpdateStatusbar)
DECSMETHOD(Window_MenuAction)
DECTMETHOD(Toolbar_HistoryPrev)
DECTMETHOD(Toolbar_HistoryNext)
DECTMETHOD(Clickpath_Parent)
DECMMETHOD(Window_ActionIconify)
DECTMETHOD(Window_Iconify)
DECSMETHOD(Window_IsActive) // bitRocky
DECSMETHOD(Window_ZoomIcons) // bitRocky
ENDMTABLE

DECSUBCLASS_NC(MUIC_Window, windowclass)
