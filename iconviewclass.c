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
 * $Id: iconviewclass.c,v 1.69.2.1 2024/01/20 02:56:53 piru Exp $
 */

#include "ambient.h"

/* public */
#include <devices/rawkeycodes.h>
#include <workbench/workbench.h> /* XXX: for NO_ICON_POSITION */
#include <graphics/regions.h>
#include <graphics/rpattr.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <proto/locale.h>
#include <proto/wb.h>
#include <hardware/atomic.h>
#include <intuition/pointerclass.h>
#include <proto/thumbnails.h>
#include <libraries/thumbnails.h>

/* private */
#include "ambient_cat.h"
#include "locale.h"
#include "mimeuri.h"
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_pen.h"
#include "gfx_alpha.h"
#include "gfx_mask.h"
#include "gfx_color.h"
#include "prefs.h"
#include "threads.h"
#include "args.h"
#include "iconmem.h"
#include "dragdrop.h"
#include "colormap.h"
#include "contextmenu.h"
#include "command.h"
#include "rexx.h"
#include "iconview.h"
#include "layout.h"
#include "snapshotlist.h"
#include "panelprefs.h"
#include "lasso.h"
#include "shortcuts.h"
#include "movelist.h"
#include "pointer.h"
#include "vars.h"
#include "icondata.h"
#include "textbox.h"
#include "notify.h"
#include "name.h"
#include "thumbs.h"
#include "capacity.h"
#include "actiondispatcherclass.h"
#include "mimetype.h"
#include "typescanner.h"
#include "vfs.h"
#include "doslistcache.h"
#include "file_func.h"
#include "viewapi.h"
#include "prefs_advanced.h"
#include "prefs_desktop.h"
#include "prefs_startup.h"
#include "time_func.h"
#include "viewclass.h"
#include "methodstack.h"
#include "iconio.h"
#include "screen.h"
#include "info64.h"
#include "avcodec.h"

#include <clib/debug_protos.h>
#define BUBBLEDELAY 10 /* intuiticks */

extern void SetWindowTitle(struct IClass *cl, Object *obj, ULONG flags); // from listviewclass.c

/*
 * We always fallback to autolayout
 * when there's more than that number
 * of icons for performance reasons.
 *
 * Snapshoted icons is a thing of the
 * past and is responsible for the
 * downfall of the Amiga. Everyone thinks
 * it was Commodore's management but that's
 * not true.
 */
#define TILELAYOUT_MAX 128


#define IV_MINWIDTH 80
#define IV_MINHEIGHT 44

/* TODO: verify tags and move to mui_internal.h */
#ifndef MUIM_GoActive
#define MUIM_GoActive           0x8042491a
struct MUIP_GoActive { ULONG MethodID; };
#endif
#ifndef MUIM_GoInactive
#define MUIM_GoInactive         0x80422c0c
struct MUIP_GoInactive { ULONG MethodID; };
#endif

#define MUIA_Window_DisableKeys 0x80424c36 /* V15 isg ULONG */


struct Process;

struct icon_node {
	struct MinNode n;
	APTR obj;
};

struct Data {
	ULONG isroot;
	ULONG show_devices;
	ULONG startup;
	ULONG dimensions;

	ULONG clip_x;
	ULONG clip_y;
	ULONG clip_xs;
	ULONG clip_ys;

	ULONG clip_x2;
	ULONG clip_y2;
	ULONG clip_xs2;
	ULONG clip_ys2;

	LONG prev_x; /* since jaca implemented opaqueresize from any border I have to put that prev_x/prev_y kludge. XXX: flickers */
	LONG prev_y;
	LONG prev_xs;
	LONG prev_ys;
	LONG prev_virt_x;
	LONG prev_virt_y;
	LONG prev_virt_xs;
	LONG prev_virt_ys;

	LONG vx;        /* cached vleft/vtop positions */
	LONG vy;

	ULONG prev_child_count;
	ULONG refresh;
	ULONG nobackfill;
	ULONG noflicker; /* performs drawing operations without overdraw */

	LONG icon_text_color;
	LONG icon_text_bgcolor;

	ULONG objcount;         /* number of positioned objects */

	/* layout context */
	APTR lctx;

	/* notify context */
	APTR nctx;

	/* icons without position */
	struct MinList addlist;
	struct MinList removelist;

	ULONG pos_x;
	ULONG pos_y;

	/* event handler */
	struct MUI_EventHandlerNode ehnode;

	/* mouse position */

	LONG mousex;
	LONG mousey;

	/* mouse clicks */
	ULONG seconds;
	ULONG micros;

	LONG numselected; /* number of selected icons */
	LONG numunscanned;
	LONG typescanner_running;
	LONG typescanner_methodid;

	/* view mode */
	LONG viewmode;
	LONG viewmode_postpone;

	/* for devices view */
	ULONG devices_postpone;

	/* mouse click */
	LONG click_x;
	LONG click_y;
	LONG click_offx;
	LONG click_offy;
	LONG click_vx;
	LONG click_vy;
	LONG click_prevx;
	LONG click_prevy;

	/* lasso */
	LONG click_xt;
	LONG click_yt;
	ULONG lasso;
	APTR lasso_ctx;

	/* panels (XXX: only for root.. that's ugly) */
	ULONG panels_loaded;

	/* init window sizes */
	LONG win_xs;
	LONG win_ys;

	/* mouse over changes */
	APTR lastobject;
	APTR lastmouseobject; /* must NOT be dereferenced, only used for change comparison */

	/* bubble help */
	ULONG bubblecnt;
	APTR bubbleobj;

	ULONG sortmode;     /* to make sorting faster, used by layout_sort() */

	UQUAD diskusage;
	UQUAD selecteddiskusage;

	ULONG is_active;

	LONG lasso_pending; /* when scrolling do not draw lasso */

	ULONG driveinfo;

	ULONG qualifier;    /* make sure it's uptodate. used for drag&drop and action execution */

	/* optimization for GetEntry method */

	APTR *objectarray;
	ULONG objectarray_dirty;

	/* this icon was lately selected */
	APTR selected_iconobj;

	TEXT path[PATH_SIZE];

	/* initial selection/focus */

	LONG focuspos;
	STRPTR *initialselection;

	/* size adjustment */

	LONG iconsizeadjustment;


	/* videopreview timer
	 */
	ULONG videopreview_restart;
	ULONG videopreview_running;
	ULONG videopreview_abort;
	APTR  videopreview_iconobj;

	/* mymorphos object */

	APTR mymorphosicon;

	/* MUI config items */
	LONG display_bubbles;
};

enum {
	LASSO_OFF,
	LASSO_STARTING,
	LASSO_DRAWING,
};

/*
 * Clipping mode.
 */
#define REFRESH_CLIP_ICON (1 << 0)
#define REFRESH_TRASHED   (1 << 1)

/*
 * Return true if object is a iconclass one
 */
static int is_icon_object(Object *o)
{
	return o && OCLASS(o) == geticonclass();
}

MUI_HOOK(layoutfunc, APTR grp, struct MUI_LayoutMsg *lm)
{
	struct Data *data = INST_DATA( OCLASS(grp), grp);

	switch (lm->lm_Type)
	{
		case MUILM_MINMAX:
			lm->lm_MinMax.MinWidth = IV_MINWIDTH;
			lm->lm_MinMax.MinHeight = IV_MINHEIGHT;
			lm->lm_MinMax.DefWidth = 256;
			lm->lm_MinMax.DefHeight = 256;
			lm->lm_MinMax.MaxWidth = MUI_MAXMAX;
			lm->lm_MinMax.MaxHeight = MUI_MAXMAX;

			return (0);

		case MUILM_LAYOUT:
			{
				struct IconData *idata;
				Object *cstate = (Object *)lm->lm_Children->mlh_Head;
				Object *child;
				//Object *lastchild = 0;
				ULONG max_x = 0;
				ULONG max_y = 0;
				ULONG child_count = 0;

				if ( data->isroot )
				{
					layout_setattrs(data->lctx,
						LAYOUTTAG_Width, _mwidth(grp), /* XXX: I think.. */
						LAYOUTTAG_Height, _mheight(grp), /* XXX: ditto */
						/* XXX: implement GridSize, sortmode, etc.. */
					TAG_DONE);
				}
				else
				{
					layout_setattrs(data->lctx,
						LAYOUTTAG_Width, _mwidth(grp), /* XXX: I think.. */
						//LAYOUTTAG_Height, _mheight(grp), /* XXX: ditto */
						/* XXX: implement GridSize, sortmode, etc.. */
					TAG_DONE);
				}

				while ( (child = NextObject(&cstate)) )
				{
					if (_isfloating(child))
					{
						if (!MUI_Layout(child, _left(child) + getv(grp, MUIA_Virtgroup_Left), _top(child) + getv(grp, MUIA_Virtgroup_Top), min(_width(grp), _maxwidth(child)), min(_height(grp), _maxheight(child)), 0))
						{
							return (FALSE);
						}
					}
					else
					{
						idata = (struct IconData *)muiUserData(child);

						D(LAYOUT, bug("layouting child %p (name: %s), x: %ld, y: %ld\n", child, (STRPTR)getv(child, MA_Icon_Name), ICON_GETICON_LEFT(child), ICON_GETICON_TOP(child)));

						/*
						 * The rootwin is a virtualgroup also.
						 * We don't want any object outside, even
						 * if bigfoot finds it funny.
						 */
						if (data->isroot)
						{
							if (ICON_GETOBJ_LEFT(child) < 0)
							{
								ICON_SETICON_LEFT_NOGRID( ICON_GETOBJ_WIDTH(child) / 2 - ICON_GETICON_WIDTH(child) / 2);
							}
							else if (ICON_GETOBJ_LEFT(child) + ICON_GETOBJ_WIDTH(child) > _width(grp))
							{
								ICON_SETICON_LEFT_NOGRID(_width(grp) - ICON_GETOBJ_WIDTH(child));
							}

							if (ICON_GETOBJ_TOP(child) < 0)
							{
								ICON_SETICON_TOP( 0 );
							}
							else if (ICON_GETOBJ_TOP(child) + ICON_GETOBJ_HEIGHT(child) > _height(grp))
							{
								ICON_SETICON_TOP_NOGRID(_height(grp) - ICON_GETICON_HEIGHT(child));
							}
						}

						if (!layout_icon_add(data->lctx, child))
						{
							PDB(("layout failed!\n"));
							return (FALSE);
						}
						else
						{
							/* some code might change these (thumbnailing code does) so we reset here */

							_addtop(child) = 0;
							_subheight(child) = 0;
						}

						child_count++;
						//lastchild = child;

						#if 0
						if (!((struct _Object *)((struct _Object *)cstate)->o_Node.mln_Succ))
						{
							if (child_count != data->prev_child_count)
							{
								if (child_count > data->prev_child_count)
								{
									idata = (struct IconData *)muiUserData(child);
									/*
									 * We are the last object and have just been
									 * added.
									 */
									layout_add_icon(data->lctx, child);
									data->clip_x = ICON_GETX;
									data->clip_y = ICON_GETY;
									data->clip_xs = _minwidth(child) - 1;
									data->clip_ys = _minheight(child) - 1;

									data->refresh |= REFRESH_CLIP_ICON;
								}
								/* XXX */
							}
						}
						#endif

						max_x = max(max_x, ICON_GETOBJ_LEFT(child) + ICON_GETOBJ_WIDTH(child));
						max_y = max(max_y, ICON_GETOBJ_TOP(child) + ICON_GETOBJ_HEIGHT(child));
					}
				}

				if (layout_getattr(data->lctx, LAYOUTTAG_Changed))
				{
					data->refresh |= REFRESH_TRASHED;
				}

				layout_done(data->lctx);

				#if 0
				if (child_count == data->prev_child_count &&
					lastchild)
				{
					/* XXX: I think this belongs there.. does it ? hm.. */
					layout_set_boundaries_prefered(data->lctx, _mwidth(grp), _mheight(grp));
				}
				#endif

				data->prev_child_count = child_count;

				lm->lm_Layout.Width = max_x;
				lm->lm_Layout.Height = max_y + 10;

				return (TRUE);
			}
	}
	return (MUILM_UNKNOWN);
}

static ULONG objectarray_check( APTR obj, struct Data *data )
{
	if ( data->objectarray_dirty )
	{
		ULONG i = 0;

		/* rebuild */

		if ( data->objectarray )
			free( data->objectarray );

		data->objectarray = malloc( data->objcount * sizeof( APTR ) );
		if ( data->objectarray )
		{
			FORCHILD( obj, MUIA_Group_ChildList )
			{
				data->objectarray[ i++ ] = child;
			}
			NEXTCHILD

			data->objectarray_dirty = FALSE;
		}

	}

	return !data->objectarray_dirty;
}

/* enable or disable dosnotify for a given path */
static void iconview_enable_dosnotify(CONST_STRPTR spath, ULONG enable)
{
	TEXT to[PATH_SIZE];

	/* Enable dos notify */
	/* EnableDOSNotify makes parent from it, so we have to trick it. */

	if (spath && *spath)
	{
		CONST_STRPTR path;
		ULONG len;

		len = strlen(spath);
		path = spath;

		if ( spath[len - 1] != ':' && spath[len - 1] != '/' )
		{
			path = to;
			stccpy(to, spath, sizeof(to));
			if (len + 1 < sizeof(to))
			{
				to[len++] = '/';
				to[len] = '\0';
			}

		}

		methodstack_push_sync( app, 3, MM_Application_EnableDOSNotify, path, enable );
	}
}

/* function creating regions from objects in group */

static APTR buildregionfromicons(APTR obj )
{
	/*
	 * Iterate through icons and mask them from backfill.
	 * NOTE: For now only for root windows. generic case needs optimizations.
	 */

	struct Region *clipregion = NewRegion();

	if ( clipregion != NULL )
	{
		struct Rectangle r;

		FORCHILD( obj, MUIA_Group_ChildList )
		{
			r.MinX = _left( child );
			r.MinY = ICON_GETIMG_TOP( child );
			r.MaxX = r.MinX + _mwidth( child ) - 1;
			r.MaxY = _top( child ) + _mheight( child ) - 1;

			OrRectRegion( clipregion, &r );

		}
		NEXTCHILD
	}

	return clipregion;
}

static void initializenoflicker(APTR obj, struct Data *data)
{
	if ( muiRenderInfo(obj) != NULL && data->isroot )
	{
		data->noflicker = TRUE;
	}

}

static void finalizenoflicker(APTR obj UNUSED, struct Data *data)
{
	if ( data->noflicker )
	{
		data->noflicker = FALSE;
	}
}

static int bufferedlayers = FALSE;

static void updatebufferedlayersinfo(APTR obj)
{
	bufferedlayers = getv(_screen(obj), SA_CompositingLayers);
}

static void update_diskusage(struct IClass *cl UNUSED, APTR obj)
{
	UQUAD diskusage = 0;
	UQUAD selecteddiskusage = 0;

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		struct IconData *idata = (struct IconData *)muiUserData(child);
		diskusage += idata->filesize;
		if (idata->selected)
			selecteddiskusage += idata->filesize;
	}
	NEXTCHILD

	SetAttrs(obj, MUIA_Group_Forward, FALSE,
					MA_View_DiskUsage, &diskusage,
					MA_View_SelectedDiskUsage, &selecteddiskusage,
					TAG_DONE);
}

DEFNEW
{
	struct Data *data;
	LONG isroot = GetTagData( MA_View_IsRoot, FALSE, INITTAGS );

	obj = DoSuperNew(cl, obj,
		MUIA_CustomBackfill, TRUE,
		MUIA_Group_LayoutHook, (ULONG)&layoutfunc_hook,
		//MUIA_Virtgroup_Smooth, TRUE, /* XXX: unfortunately MUIA_Virtgroup_Top/Left aren't updated during the effect so we can't update our backfilling either.. have to find a better way */
		MUIA_CycleChain, TRUE,
		MUIA_Dropable, TRUE,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		MUIA_Virtgroup_Input, isroot ? FALSE : TRUE,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (size_t)(obj);
	}

	data = INST_DATA(cl, obj);

	data->isroot = isroot;
	data->show_devices = isroot;

	NEWLIST(&data->addlist);
	NEWLIST(&data->removelist);

	data->objectarray = NULL;
	data->objectarray_dirty = TRUE;
	data->devices_postpone = FALSE;
	data->numunscanned = 0;
	data->typescanner_running = FALSE;
	data->typescanner_methodid = 0;

	/*
	 * Initial viewmode. Bit sucky as it needs knowledge about modes indexes.
	 * Maybe can be checked with URI, but then it nullifies point of passing
	 * MA_View_ModeIndex.
	 */

	switch(	GetTagData( MA_View_ModeIndex, IVM_ICON, INITTAGS ) )
	{
		case LVM_ICONS:
			data->viewmode = IVM_ICON;
			break;
		case LVM_SHOWALL:
			data->viewmode = IVM_SHOWALL;
			break;
		case LVM_THUMBS:
			data->viewmode = IVM_THUMBS;
			break;
	}

	if (!(data->nctx = notify_create()) || !(data->lctx = layout_create(obj, LAYOUTTAG_Auto, data->isroot ? FALSE : TRUE, TAG_DONE)))
	{
		CoerceMethod(cl, obj, OM_RELEASE);
		return ((ULONG)NULL);
	}

	data->click_x = -1;
	data->click_y = -1;
	
	data->click_prevx = -1;
	data->click_prevy = -1;

	/* XXX: fixme */
	//data->driveinfo = _conf(misc_driveinfo);

	return ((ULONG)obj);
}


DEFDISP
{
	GETDATA;
	struct icon_node *in, *nextin;
	LONG i;

	/* initialselection might be still here */

	if ( data->initialselection != NULL )
	{
		for(i=0; data->initialselection[ i ] != NULL ;i++)
			name_delete( data->initialselection[ i ] );

		FreeVecTaskPooled( data->initialselection );
	}

	/* */

	iconview_enable_dosnotify(data->path, FALSE);

	set(obj, MA_View_NumSelected, 0);
	if ( data->typescanner_methodid )
		DoMethod(obj, MUIM_Application_KillPushMethod, NULL, data->typescanner_methodid, 0);

	if (data->nctx)
	{
		notify_delete(data->nctx);
	}

	if (data->lasso_ctx)
	{
		lasso_pixel(data->lasso_ctx);
		lasso_delete(data->lasso_ctx);
	}

	ITERATELISTSAFE(in, nextin, &data->addlist)
	{
		MUI_DisposeObject(in->obj);
		icon_free(in);
	}

	ITERATELISTSAFE(in, nextin, &data->removelist)
	{
		/* ignore locked state here. all threads should be gone */
		MUI_DisposeObject(in->obj);
		icon_free(in);
	}

	if (data->lctx)
	{
		layout_delete(data->lctx);
		data->lctx = NULL;
	}

	if ( data->objectarray )
	{
		free( data->objectarray );
	}

	/*
	 * If more than 50 children then we dispose them from
	 * separate thread to improve responsiveness.
	 */

	if ( data->objcount >= 50 )
	{
		ULONG i = 0;
		APTR *array;

		array = (APTR*)malloc( ( data->objcount + 1 ) * sizeof( APTR ) );


		if ( array )
		{
			DoMethod( obj, MUIM_Group_InitChange );

			FORCHILD(obj, MUIA_Group_ChildList)
			{
				/* remove reference from the icon, so it will be safe to dispose deficonpool */
				set(child, MA_Icon_Reference, NULL);
				DoMethod(obj, OM_REMMEMBER, child);
				array[ i++ ] = child;
			}
			NEXTCHILD

			array[ i ] = NULL;

			DoMethod( app, MM_Application_DisposeObjectArray, array, TRUE, TRUE );
			DoMethod( obj, MUIM_Group_ExitChange );
		}
	}

	/* if mymorphosicon is not displayed, we have to delete it explicitely here (better way?) */
	if(data->mymorphosicon && _conf(misc_mymorphosicon) == FALSE) 
	{
		MUI_DisposeObject(data->mymorphosicon);
	}

	return (DOSUPER);
}


DEFADDMEMBER
{
	GETDATA;

	data->objectarray_dirty = TRUE;

	if (!_isfloating(msg->opam_Object) && !data->sortmode)
	{
		data->objcount++;

		layout_setattrs(data->lctx, LAYOUTTAG_Num, data->objcount, TAG_DONE);

		/* XXX: hm.. later I could try to handle the layout addition there.. for consistency.. */
	}
	return (DOSUPER);
}


DEFREMMEMBER
{
	GETDATA;
	APTR o = msg->opam_Object;

	data->objectarray_dirty = TRUE;

	if (!_isfloating(o) && !data->sortmode)
	{
		if (getv(o, MA_Icon_Selected)) /* XXX: why not direct access ? hm.. not sure it's useful here */
		{
			data->selecteddiskusage -= *(UQUAD*)getv(o, MA_Icon_FileSize);
			DoSuperMethod(cl, obj, MM_View_IconSelect, MV_View_IconSelect_Unselect, getv(o, MA_Icon_Type), getv(o, MA_Icon_IsShortcut));
			SetAttrs(obj, MUIA_Group_Forward, FALSE,
						  MA_View_NumSelected, MV_View_NumSelected_Decrease,
						  MA_View_SelectedDiskUsage, &data->selecteddiskusage,
						  TAG_DONE);
		}

		if ( data->lctx )
		{
			layout_icon_remove(data->lctx, o);
		}

		data->objcount--;

		if ( data->lctx )
		{
			layout_setattrs(data->lctx, LAYOUTTAG_Num, data->objcount, TAG_DONE);
		}

		if ((o == data->lastobject))
		{
			data->lastobject = NULL; /* no unighlighting for that one since it's gone */
			#if USE_POINTER_HOVERING
			pointer_clear(_window(obj)); /* and no pointer either */
			#endif
		}

		if (o == data->lastmouseobject) /* technically updating this is not needed */
		{
			data->lastmouseobject = NULL;
		}

		if ((o == data->selected_iconobj))
		{
			data->selected_iconobj = NULL;
		}
	}
	return (DOSUPER);
}


DEFMMETHOD(Hide)
{
	DoMethod(obj, MM_Iconview_ClearBubble);
	return (DOSUPER);
}

DEFMMETHOD(Show)
{
	GETDATA;
	ULONG rc = DOSUPER;
	ULONG dimension = _mwidth(obj) * _mheight(obj);

	/* For root window reload shortcuts and stuff as screen resolution might have been just changed */

	DoMethod(obj, MUIM_GetConfigItem, 160, &data->display_bubbles);	// Check if we should display short help

	if (data->isroot && data->startup && (data->dimensions != dimension) && (data->dimensions != 0))
	{
		DoMethod(app, MUIM_Application_PushMethod, _win(obj), 1 | MUIF_PUSHMETHOD_SINGLE, MM_Window_Reload);
	}

	data->dimensions = dimension;

	return rc;
}

DEFSMETHOD(Iconview_AddIcon)
{
	struct icon_node *in;
	GETDATA;

	ASSERT(msg->o);

	if (msg->unique)
	{
		STRPTR s;

		s = (STRPTR)getv(msg->o, MA_Icon_Name);
		ASSERT(s);

		/*
		 * Readded icon will have same position as removed one.
		 */

		if ( data->isroot )
		{
			APTR original = (APTR)DoMethod(obj, MM_Iconview_FindShortcut, getv(msg->o, MA_Icon_PathInfo));

			if ( original )
			{
				SetAttrs(msg->o, MA_Icon_X, getv(original, MA_Icon_X),
								MA_Icon_Y, getv(original, MA_Icon_Y),
								MA_Icon_HasPos, getv(original, MA_Icon_HasPos),
								TAG_DONE);
			}
		}

		DoMethod(obj, MM_Iconview_RemoveByName, s);

		ITERATELIST(in, &data->addlist)
		{
			if (!stricmp(s, (STRPTR)getv(in->obj, MA_Icon_Name)))
			{
				ASSERT(in->obj);
				MUI_DisposeObject(in->obj);
				REMOVE(in);
				break;
			}
		}
	}

	if (data->isroot)
	{
		ULONG type = getv(msg->o, MA_Icon_Type);
		if (type == MV_Icon_Type_MyComputer && data->mymorphosicon == NULL)
		{
			data->mymorphosicon = msg->o;
			
			if (_conf(misc_mymorphosicon) == FALSE)
			{
				/* If mymorphos icon is not enabled then we don't add it to window but instead just store for further use */

				return (0);
			}
		}
		else if (type == MV_Icon_Type_MyComputer && data->mymorphosicon != msg->o)
		{
			DB(("Trying to readd MyMorphos icon to view!\n"));
		}

	}

	if ( (in = icon_malloc(sizeof(*in))) )
	{
		in->obj = msg->o;
		ADDTAIL(&data->addlist, in);
	}

	/* XXX */
	return (0);
}


DEFSMETHOD(Iconview_DeleteIcon)
{
	struct icon_node *in;
	GETDATA;

	ASSERT(msg->o);

	if ((in = icon_malloc(sizeof(*in))))
	{
		in->obj = msg->o;
		ADDTAIL(&data->removelist, in);
	}
	/* XXX */
	return (0);
}


STATIC VOID UpdateHandlers(struct IClass *cl, APTR obj, struct Data *data, ULONG show)
{
	if (muiRenderInfo(obj) && _win(obj))
	{
		if (data->ehnode.ehn_Object)
		{
			DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
		}

		data->ehnode.ehn_Object = obj;
		data->ehnode.ehn_Class = cl;
		data->ehnode.ehn_Events = (show ? (IDCMP_DISKINSERTED | IDCMP_DISKREMOVED) : 0L) | IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY | IDCMP_INACTIVEWINDOW | IDCMP_MOUSEOBJECT | IDCMP_MOUSEMOVE;
		data->ehnode.ehn_Priority = 51; /* priority over MUI's areaclass */
		data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
		DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);

		if (show && !data->isroot)
		{
			DoMethod(_win(obj), MM_Window_SetTitle, dprefs_mymorphos_name_get());
			SetAttrs(obj, MUIA_Group_Forward, FALSE,
						  MA_View_TotalFiles, 0, MA_View_TotalDirs, 0, MA_View_Links, 0,
						  MA_View_IconFiles, 0, MA_View_DiskUsage, &data->diskusage, MA_View_SelectedDiskUsage, &data->selecteddiskusage,
						  TAG_DONE);
		}
	}
}


DEFMMETHOD(Setup)
{
	ULONG rc;
	GETDATA;

	if ( (rc = DOSUPER) )
	{
		UpdateHandlers(cl, obj, data, data->show_devices);

		if (data->isroot && !data->panels_loaded)
		{
			panelprefs_loadall();
			data->panels_loaded = TRUE;
		}

		updatebufferedlayersinfo(obj);
	}

	return (rc);
}


DEFTMETHOD(View_Setup)
{
	DoMethod(obj, MM_Iconview_AllocPens);

	return (0);
}


DEFTMETHOD(Iconview_AbortLasso)
{
	GETDATA;

	data->lasso = LASSO_OFF;
	//data->ehnode.ehn_Events &= ~IDCMP_MOUSEMOVE;
	// why code above is commented out? didnt implement code below then either -itix
	//data->ehnode.ehn_Events &= ~IDCMP_INTUITICKS;

	if (data->lasso_ctx)
	{
		APTR handle;

		pointer_clear(_window(obj));

		/* itix: should we invoke MUIM_Draw instead? */
		handle = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));
		lasso_clear(data->lasso_ctx);
		MUI_RemoveClipping(muiRenderInfo(obj), handle);

		lasso_pixel(data->lasso_ctx);
		lasso_delete(data->lasso_ctx);
		data->lasso_ctx = NULL;
		return (TRUE);
	}
	return (FALSE);
}


#define _isinicon(_o,_x,_y) ({ \
	ULONG vx = getv(obj, MUIA_Virtgroup_Left); \
	ULONG vy = getv(obj, MUIA_Virtgroup_Top); \
	struct IconData *_idata = (struct IconData *)muiUserData(_o); \
	(_between(ICON_GETIMG_LEFT(_o),((_x)-_left(obj)+vx),ICON_GETIMG_LEFT(_o) + ICON_GETIMG_WIDTH(_o)) && _between(ICON_GETIMG_TOP(_o),((_y)-_top(obj)+vy),ICON_GETIMG_TOP(_o) + ICON_GETIMG_HEIGHT(_o))); \
   })

#define _isinicontext(_o,_x,_y) ({ \
	struct IconData *_idata = (struct IconData *)muiUserData(_o); \
	LONG xs = (_mwidth(_o) - textbox_getwidth(_idata->tbctx)) / 2; \
	ULONG ys = textbox_getheight(_idata->tbctx); \
	ULONG vx = getv((obj), MUIA_Virtgroup_Left); \
	ULONG vy = getv((obj), MUIA_Virtgroup_Top); \
	if (xs < 0) xs = 0; \
	(_between(_mleft(_o)+xs,((_x)-_left((obj))+vx),_mright(_o)-xs)) && (_between(_idata->y + _idata->addy + _idata->bm_max_ys + _idata->fontspace,((_y)-_top((obj))+vy),_idata->y + _idata->addy + _idata->bm_max_ys +_idata->fontspace + ys)); \
   })

#define _winmouse_x(_o) (_window(_o)->MouseX - _left(_o))
#define _winmouse_y(_o) (_window(_o)->MouseY - _top(_o))


/* this routine does both _isinicon() and _isinicontext() macros shown above */
static LONG check_iconarea(APTR obj, APTR o, LONG x, LONG y)
{
	struct IconData *_idata = (struct IconData *)muiUserData(o);
	LONG vx, vy, xs, ys, rc;

	vx = getv(obj, MUIA_Virtgroup_Left);
	vy = getv(obj, MUIA_Virtgroup_Top);

	rc = FALSE;

	if ((_between(ICON_GETIMG_LEFT(o), (x-_left(obj)+vx),ICON_GETIMG_LEFT(o) + ICON_GETIMG_WIDTH(o))) && (_between(ICON_GETIMG_TOP(o),(y-_top(obj)+vy), ICON_GETIMG_TOP(o) + ICON_GETIMG_HEIGHT(o))))
	{
		rc = TRUE;
	}
	else
	{
		ys = textbox_getheight(_idata->tbctx);
		xs = (_width(o) - (LONG)textbox_getwidth(_idata->tbctx) - 1) / 2;

		if (xs < 0)
			xs = 0;

		if ((_between(_left(o) + xs, (x-_left(obj)+vx), _right(o)-xs)) && (_between(_idata->y + _idata->addy + _idata->bm_max_ys + _idata->fontspace,((y)-_top((obj))+vy),_idata->y + _idata->addy + _idata->bm_max_ys +_idata->fontspace + ys)))
		{
			rc = TRUE;
		}
	}

	return rc;
}


static APTR find_nearest_obj(APTR obj, APTR selected_obj, LONG code, LONG selected_x, LONG selected_y, LONG isroot)
{
	APTR nearest_obj = NULL;
	LONG direction_x = 0;
	LONG direction_y = 0;

	if (!isroot && (code == RAWKEY_LEFT || code == RAWKEY_RIGHT))
	{
		APTR child, cstate, first, previous;

		cstate = (APTR)((struct MinList *)getv(obj, MUIA_Group_ChildList))->mlh_Head;
		first = child = NextObject(&cstate);
		previous = NULL;

		while (child)
		{
			if (child == selected_obj)
			{
				if (code == RAWKEY_LEFT && previous)
				{
					/* Previous object was the right one */
					return previous;
				}
				else if (code == RAWKEY_RIGHT)
				{
					child = NextObject(&cstate);
					return (child ? child : first);
				}
			}

			previous = child;
			child = NextObject(&cstate);
		}

		switch (code)
		{
			case RAWKEY_LEFT:
				return previous;

			case RAWKEY_RIGHT:
				return first;
		}
	}

	switch ( code )
	{
		case RAWKEY_UP:
			direction_x = 0;
			direction_y = -1;
			break;
		case RAWKEY_DOWN:
			direction_x = 0;
			direction_y = 1;
			break;
		case RAWKEY_LEFT:
			direction_x = -1;
			direction_y = 0;
			break;
		case RAWKEY_RIGHT:
			direction_x = 1;
			direction_y = 0;
			break;

		#if DEBUG
		default:
			PDB(("Invalid keycode\n"));
			return NULL;
		#endif
	}

	if ( direction_x || direction_y )
	{
		LONG div, min_dist = 0x7fffffff;

		switch (_conf(icon_minsize))
		{
			default:
			case ICON_SIZE_MICRO : div = SIZE_MICRO;  break;
			case ICON_SIZE_SMALL : div = SIZE_SMALL;  break;
			case ICON_SIZE_MEDIUM: div = SIZE_MEDIUM; break;
			case ICON_SIZE_LARGE : div = SIZE_LARGE;  break;
			case ICON_SIZE_HUGE  : div = SIZE_HUGE;   break;
		}

		/* divide to eliminate danger of overflows for large views */

		selected_x /= div;
		selected_y /= div;

		FORCHILD(obj, MUIA_Group_ChildList)
		{
			if ( child != selected_obj )
			{
				/* use center of object to eliminate influence of wide labels */

				int x = ( _left( child ) + _width( child ) / 2 ) / div;
				int y = ( _top( child ) + _height( child ) / 2 ) / div;

				int delta_x = x - selected_x;
				int delta_y = y - selected_y;

				#if 0
				/* add small threshold (dunno if needed, but works with it:) */

				if ( abs( delta_x )< 2 )
					delta_x = 0;
				#endif

				//PDB(("delta_x %ld, delta_y %ld (%s)\n", delta_x, delta_y, getv(child, MA_Icon_Name)));

				if (( delta_y * direction_y > 0 || direction_y == 0 ) && ( delta_x * direction_x > 0 || direction_x == 0 ))
				{
					LONG dist;

					if ( direction_x == 0 )
						delta_x *= 2;
					if ( direction_y == 0 )
					{
						delta_y *= 2;

						if (direction_x == 1 && delta_y < 0)
							delta_y *= 2;
					}

					dist = delta_x * delta_x + delta_y * delta_y;

					if ( dist < min_dist )
					{
						nearest_obj = child;
						min_dist = dist;
					}
				}
			}
		}
		NEXTCHILD;
	}

	return nearest_obj;
}


/* XXX: bah sucks.. I need an _isinicontext() too so direct icondata access + tracking and stuff.. won't be easy */

DEFMMETHOD(HandleEvent)
{
	struct IntuiMessage *imsg;

	if ( (imsg = msg->imsg) )
	{
		GETDATA;
		LONG MouseX, MouseY;

		MouseX = imsg->MouseX;
		MouseY = imsg->MouseY;
		data->mousex = imsg->MouseX;
		data->mousey = imsg->MouseY;

		switch (imsg->Class)
		{
			case IDCMP_MOUSEBUTTONS:
			{
				data->qualifier = imsg->Qualifier;    /* NOTE: Not every method has Qualifier set, so don't move it */

				switch (imsg->Code)
				{
					case SELECTDOWN:
					{
						if (_isinobject(MouseX, MouseY))
						{
							LONG iconhit;
							APTR o;

							o = (APTR)DoMethod(obj, MUIM_WhichObject, MouseX, MouseY);
							iconhit = o == obj ? FALSE : check_iconarea(obj, o, MouseX, MouseY);

							if ( (o == obj && imsg->Code == SELECTDOWN) || (!iconhit)) /* click on an empty area = start lasso */
							{
								data->selected_iconobj = NULL;  /* no icon selected atm */

								if (!(data->qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
								{
									ULONG found_selected = FALSE;

									/*
									 * Click on an empty area. Unselect all the
									 * possibly selected icons.
									 */
									FORCHILD(obj, MUIA_Group_ChildList)
									{
										if (getv(child, MA_Icon_Selected))
										{
											found_selected = TRUE;
											set(child, MA_Icon_Selected, FALSE);
										}
									}
									NEXTCHILD

									/*
									 * XXX: bah bah bah.. I would have to make a list of the icons already selected
									 * then walk that list to see if I have to select or deselect if shift was pressed,
									 * etc.. sucks
									 */

									if (found_selected)
									{
										DoMethod(obj, MM_Icon_UpdateBitMapBuffer); /* broadcast */
									}
								}

								if (data->isroot && (data->qualifier & (IEQUALIFIER_LALT|IEQUALIFIER_RALT) || _conf(misc_desktopdoubleclick)))
								{
									if (DoubleClick(data->seconds, data->micros, imsg->Seconds, imsg->Micros))
									{
										data->seconds = 0;
										data->micros = 0;

										/* relaxed double click area. */
										if(data->click_prevx != -1 && data->click_prevy != -1)
										{
											if(abs(data->click_prevx-MouseX) < 5 && abs(data->click_prevy-MouseY) < 5)
											{
												data->click_prevx = -1;
												data->click_prevy = -1; 
												DoMethod(_app(obj), MM_Application_OpenDevicesWindow, 1);
												return (MUI_EventHandlerRC_Eat);
											}
										}
									}
									else
									{
										data->click_prevx = MouseX;
										data->click_prevy = MouseY;
										data->seconds = imsg->Seconds;
										data->micros = imsg->Micros;
									}
								}

								data->vy = getv(obj, MUIA_Virtgroup_Top);
								data->click_x = MouseX;
								data->click_y = MouseY;
								data->click_vx = data->click_x + getv(obj, MUIA_Virtgroup_Left);
								data->click_vy = data->click_y + data->vy;

								data->lasso = LASSO_STARTING;
								data->ehnode.ehn_Events |= IDCMP_MOUSEMOVE | IDCMP_INTUITICKS;

								return (MUI_EventHandlerRC_Eat);
							}
							else if (o != obj)
							{
								if (iconhit)
								{
									ULONG was_selected = getv(o, MA_Icon_Selected);

									data->selected_iconobj = o;  /* last selected icon atm */

									DoMethod(o, MM_Icon_Click, imsg->Seconds, imsg->Micros);

									if (was_selected && getv(o, MA_Icon_DoubleClick))
									{
										/*
										 * Double click in an icon.
										 */

										//data->ehnode.ehn_Events &= ~IDCMP_MOUSEMOVE;

										DoMethod(o, MM_Icon_Click, 0, 0);
										DoMethod(o, MM_Icon_Select, data->click_x - _left(o), data->click_y - _top(o));

										return (MUI_EventHandlerRC_Eat);
									}
									else
									{
										/*
										 * Click in an icon.
										 */

										SetAttrs(o,
											MA_Icon_ClickPosX, MouseX - _left(o),
											MA_Icon_ClickPosY, MouseY - _top(o),
											MA_Icon_Selected, ((data->qualifier & IEQUALIFIER_SHIFTS) && was_selected) ? FALSE : TRUE, /* If shift pressed then toggle selection. */
										TAG_DONE);

										data->click_x = MouseX;
										data->click_y = MouseY;
										data->ehnode.ehn_Events |= IDCMP_MOUSEMOVE; /* try to detect a drag & drop */

										DoMethod(obj, MM_Iconview_ShowPreview, FALSE);

										/* XXX: also icon should have a MA_Icon_Draggable which is set when it compares the window click position from MA_Icon_Selected, TRUE.. */
									}

									if (!was_selected && !( data->qualifier & IEQUALIFIER_SHIFTS))
									{
										/*
										 * Unselect the rest, only if we are on an icon which
										 * isn't selected.
										 */
										DoMethod(obj, MM_Iconview_Unselect, o);
									}
									return (MUI_EventHandlerRC_Eat);
								}
							}
						}
					}
					break;

					case SELECTUP:
					{
						DoMethod(obj, MM_Iconview_AbortLasso); /* this automatically removes IDCMP_MOUSEMOVE for drag & drop as well */

						data->click_x = -1;
						data->click_y = -1;

					}
					break; /* XXX: or eat ? */

					case MENUDOWN:
					{
						data->click_x = -1;
						data->click_y = -1;

						if (DoMethod(obj, MM_Iconview_AbortLasso)) /* ditto */
						{
							/* XXX: and I should unselect what was selected.. sigh */
							return (MUI_EventHandlerRC_Eat);
						}

						/*
						 * Trap RMB on the window's titlebar..
						 */
						if (!data->isroot && _isinwinborder(MouseX, MouseY))
						{
							set(_win(obj), MUIA_Window_MouseObject, obj);
							DoMethod(_win(obj), MUIM_Window_HandleRMB, imsg);
							return (MUI_EventHandlerRC_Eat);
						}
					}
					break;

					case MIDDLEDOWN:
						if (!data->isroot && data->qualifier & IEQUALIFIER_CONTROL)
						{
							SetAttrs(obj, MUIA_Group_Forward, FALSE,
											MA_Icon_SizeAdjustment, 0,
											TAG_DONE);
						}
						break;
				}
			}
			break;

			case IDCMP_INACTIVEWINDOW:
				/* XXX: hm.. maybe we have to do some more stuff here.. */
				DoMethod(obj, MM_Iconview_AbortLasso);
				DoMethod(obj, MM_Iconview_ClearBubble);
				data->seconds = 0;
				data->micros = 0;

				data->click_x = -1;
				data->click_y = -1;

				break;


			case IDCMP_MOUSEMOVE:
			{
				data->qualifier = imsg->Qualifier;    /* NOTE: Not every method has Qualifier set, so don't move it */

				DoMethod(obj, MM_Iconview_ClearBubble);

				switch (data->lasso)
				{
					case LASSO_DRAWING:
						data->click_xt = MouseX + data->vx;
						data->click_yt = MouseY + data->vy;

						DoMethod(obj, MM_Iconview_SelectLasso, TRUE);
						MUI_Redraw(obj, MADF_DRAWUPDATE);
						DoMethod(obj, MM_Iconview_SelectLasso, FALSE);
						break;

					case LASSO_STARTING:
						if (abs(data->click_x - MouseX) > DRAGDROP_START_X || abs(data->click_y - MouseY) > DRAGDROP_START_Y)
						{
							if ( (data->lasso_ctx = lasso_create(_rp(obj), data->click_x, data->click_y, _left(obj), _top(obj), _right(obj), _bottom(obj), data->vx, data->vy, _screen(obj)->RastPort.BitMap, gfx_get_penspec_value(muiRenderInfo(obj), &_conf(lasso_pen)))) )
							{
								pointer_set(_window(obj), POINTER_CROSSAIR);
								data->lasso = LASSO_DRAWING;
							}
							/* keep trying if it fails */
						}
						break;

					case LASSO_OFF:
						{
							APTR o;

							if (_isinobject(MouseX, MouseY))
							{
								/* check for d&d */

								if ( data->click_x != -1 && data->click_y != -1 )
								{
									/* We should probably use object which was at place we clicked the mouse. */

									//o = (APTR)DoMethod(obj, MUIM_WhichObject, MouseX, MouseY);
									o = (APTR)DoMethod(obj, MUIM_WhichObject, data->click_x, data->click_y);

									if (o && o != obj)
									{
										if ( getv(o, MA_Icon_Selected ) )
										{
											if (abs(data->click_x - MouseX) > DRAGDROP_START_X || abs(data->click_y - MouseY) > DRAGDROP_START_Y)
											{
												APTR source = getv(obj, MA_View_NumSelected) > 1 ? obj : o;
												//data->ehnode.ehn_Events &= ~IDCMP_MOUSEMOVE;
												data->click_offx = data->click_x - getv(o, MA_Icon_Left) + data->vx;
												data->click_offy = data->click_y - getv(o, MA_Icon_Top) + data->vy;
												DoMethod(source, MUIM_DoDrag, data->click_x - ICON_GETOBJ_LEFT(o) + data->vx, data->click_y - ICON_GETOBJ_TOP(o) + data->vy, 0);

												/* mouseup meeesge won't be called after d&d, so we reset these here */
												data->click_x = data->click_y = -1;

												return (MUI_EventHandlerRC_Eat);
											}
										}
									}
								}

								/* hovering */

								if ( _conf(icon_hover) )
								{
									/* first try to exit as soon as possible to not eat cpu time */

									if ( data->lastobject )
									{
										/* check if we are in same object. in hovering mode we are sure that lastobject is an icon */

										LONG vx = getv( obj, MUIA_Virtgroup_Left );
										LONG vy = getv( obj, MUIA_Virtgroup_Top );

										LONG mx = MouseX - _left( obj ) + vx;
										LONG my = MouseY - _top( obj ) + vy;

										//struct IconData *idata = (struct IconData *)muiUserData(data->lastobject);

										LONG oy1 = ICON_GETIMG_TOP(data->lastobject);
										LONG oy2 = oy1 + ICON_GETIMG_HEIGHT(data->lastobject);

										if( my > oy1 && my < oy2 )
										{
											LONG ox1 = ICON_GETIMG_LEFT(data->lastobject);
											LONG ox2 = ox1 + ICON_GETIMG_WIDTH(data->lastobject);

											if( mx > ox1 && mx < ox2 )
											{
												break;
											}
										}
									}

									/* lookup object under mouse (TODO: optimize) */

									o = NULL;

									{
										LONG vx = getv( obj, MUIA_Virtgroup_Left );
										LONG vy = getv( obj, MUIA_Virtgroup_Top );

										LONG mx = MouseX - _left( obj ) + vx;
										LONG my = MouseY - _top( obj ) + vy;

										LONG yt = 0 + vy;
										LONG yb = getv( obj, MUIA_Height ) + vy;

										FORCHILD(obj, MUIA_Group_ChildList)
										{
											LONG ox1, ox2, oy1, oy2;
											//struct IconData *idata = (struct IconData *)muiUserData(child);

											oy1 = ICON_GETIMG_TOP(child);

											if ( oy1 >= yb )
											{
												/* won't find it anyway. */
												break;
											}

											oy2 = oy1 + ICON_GETIMG_HEIGHT(child);

											if ( oy2 > yt )
											{
												if( my > oy1 && my < oy2 )
												{
													ox1 = ICON_GETIMG_LEFT(child);
													ox2 = ox1 + ICON_GETIMG_WIDTH(child);

													if( mx > ox1 && mx < ox2 )
													{
														o = child;
														break;
													}
												}
											}
										}
										NEXTCHILD;
									}

									/* this is new object */

									if ( o && o != data->lastobject )
									{
										if ( data->lastobject )
											set(data->lastobject, MA_Icon_Highlighted, FALSE);

										data->lastobject = o;
										set(data->lastobject, MA_Icon_Highlighted, TRUE);
										DoMethod(obj, MM_Iconview_ShowPreview, TRUE);

										data->ehnode.ehn_Events |= IDCMP_INTUITICKS;   // enable intuiticks when mouse is over object

									}
									if ( !o && data->lastobject )
									{
										set(data->lastobject, MA_Icon_Highlighted, FALSE);
										DoMethod(obj, MM_Iconview_ShowPreview, FALSE);
										data->lastobject = NULL;

										data->ehnode.ehn_Events &= ~IDCMP_INTUITICKS;   // disable intuiticks when mouse is not over object

									}

									break;
								}
							}
						}
						break;
				}
			}
			break;

			case IDCMP_MOUSEOBJECT:
			{
				if (data->lasso == LASSO_OFF)
				{
					APTR o;

					/* if hovering is disabled we rely on this message to trigger helpbubbles */

					if ( !_conf(icon_hover) )
					{
						o = (APTR)DoMethod(obj, MUIM_WhichObject, MouseX, MouseY);

						if (o != data->lastmouseobject)
						{
							DoMethod(obj, MM_Iconview_ClearBubble);

							if (o)
							{
								if (o == obj)
								{
									data->ehnode.ehn_Events &= ~IDCMP_INTUITICKS;   // disable intuiticks when mouse is over nothing
								}
#warning "XXX: we could use is_icon_object(o) here?"
								else /* XXX: this is wrong. we must check if the object is an icon or a statusbar might annoy us later */
								{
									data->ehnode.ehn_Events |= IDCMP_INTUITICKS;
								}
							}
						}
						data->lastobject = o == obj ? NULL : o;
						data->lastmouseobject = o;
						DoMethod(obj, MM_Iconview_ShowPreview, (o && o != obj) ? TRUE : FALSE);
					}
				}
			}
			break;

			case IDCMP_INTUITICKS:
			{
				if (data->lasso == LASSO_DRAWING)
				{
					LONG delta = 0;

					if (MouseY < _top(obj))
					{
						delta = _top(obj) - MouseY;
					}
					else if (MouseY > _bottom(obj))
					{
						delta = _bottom(obj) - MouseY;
					}

					delta /= 2;

					if (delta)
						set(obj, MUIA_Virtgroup_Top, data->vy - delta);
				}
				else
				{
					if (data->display_bubbles) // Boolean
					{
						APTR o;

						/* XXX: check if coords are -1/-1 and add some special tag. then I know I can unactivate the data->lastobject. OR perhaps not if IDCMP_MOUSEOBJECT handles it */

						o = (APTR)DoMethod(obj, MUIM_WhichObject, MouseX, MouseY);

#warning "XXX: we could use is_icon_object(o) here?"
						if (o != obj && o) /* XXX: wrong as well.. we must check it's an icon */
						{
							if (!data->bubbleobj && !data->videopreview_running)
							{
								if (data->bubblecnt == BUBBLEDELAY)
								{
									STRPTR path;

									data->bubblecnt++; /* only one bubble */

									if( (path = (STRPTR)getv(o, MA_Icon_Path)) )
									{
										struct internal_mimetype_node * imn = (APTR)getv(o, MA_Icon_MimeType);
										CONST_STRPTR descr = imn ? imn->description : NULL;

										do_action(obj, TA_Metadata_Gather,
											TT_Metadata_Gather_Path, path,
											descr ? TT_Metadata_Gather_MimeTypeDescription : TAG_DONE, descr,
										TAG_DONE);
									}
								}
								else
								{
									data->bubblecnt++;
								}
							}
						}
					}
				}
			}
			break;

			case IDCMP_DISKINSERTED:
				/* XXX: TA_Devices_RemoveAll doesnt work really -itix */
				if (!data->isroot)
				{
					/* kiero: _removeall triggers _all when successfull so abort both of them. solves
					 *        problem when lots of messages start flowing. 
					 */
					threads_abort(obj, TA_Devices_Add, NULL);
					threads_abort(obj, TA_Devices_RemoveAll, NULL);
				}
				do_action(obj, data->isroot ? TA_Devices_Add : TA_Devices_RemoveAll, TAG_DONE);
				//do_action(obj, TA_Devices_Add, TAG_DONE);
				break;

			case IDCMP_DISKREMOVED:
				/* XXX: TA_Devices_RemoveAll doesnt work really -itix */
				if (!data->isroot)
				{
					/* This is some black magic: Doing this here appears to solve the "duplicate"
					 * icons appearing on "My MorphOS" views. No idea how and why. This really
					 * should be fixed properly instead of such black magic. - Piru
					 */
					threads_abort(obj, TA_Devices_RemoveAll, NULL);
				}
				do_action(obj, data->isroot ? TA_Devices_Remove : TA_Devices_RemoveAll, TAG_DONE);
				//do_action(obj, TA_Devices_Remove, TAG_DONE);
				break;

			case IDCMP_RAWKEY:
			{
				ULONG code = imsg->Code;

				data->qualifier = imsg->Qualifier;    /* NOTE: Not every method has Qualifier set, so don't move it */

				/* Utilize numpad keys so Amiga keyboards work too */

				switch (code)
				{
					case RAWKEY_NM_WHEEL_UP:
						if (!data->isroot && data->qualifier & IEQUALIFIER_CONTROL)
						{
							if (data->iconsizeadjustment < 128)
							{
								PDB(("obj = 0x%08lx\n", obj)); // bitRocky
								SetAttrs(obj, MUIA_Group_Forward, FALSE,
												MA_Icon_SizeAdjustment, data->iconsizeadjustment + 10,
												TAG_DONE);
							}
							return (MUI_EventHandlerRC_Eat);
						}
						break;

					case RAWKEY_NM_WHEEL_DOWN:
						if (!data->isroot && data->qualifier & IEQUALIFIER_CONTROL)
						{
							if (data->iconsizeadjustment - 10 > -(LONG)_conf(icon_minsize))
							{
								SetAttrs(obj, MUIA_Group_Forward, FALSE,
												MA_Icon_SizeAdjustment, data->iconsizeadjustment - 10,
												TAG_DONE);
							}
							return (MUI_EventHandlerRC_Eat);
						}
						break;

					case RAWKEY_HOME:
					case RAWKEY_KP_7:
						set(obj, MUIA_Virtgroup_Top, 0);
						return (MUI_EventHandlerRC_Eat);

					case RAWKEY_END:
					case RAWKEY_KP_1:
						set(obj, MUIA_Virtgroup_Top, 0x7fffffff);
						return (MUI_EventHandlerRC_Eat);

					case RAWKEY_PAGEUP:
					case RAWKEY_KP_9:
						set(obj, MUIA_Virtgroup_Top, data->vy - _mheight(obj) / 4 * 3);
						return (MUI_EventHandlerRC_Eat);

					case RAWKEY_PAGEDOWN:
					case RAWKEY_KP_3:
						set(obj, MUIA_Virtgroup_Top, data->vy + _mheight(obj) / 4 * 3);
						return (MUI_EventHandlerRC_Eat);
				}

				if (code == RAWKEY_TAB)
				{
					if (!data->is_active)
						break;

					code = imsg->Qualifier & (IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT) ? RAWKEY_LEFT : RAWKEY_RIGHT;
				}

				if ( code == RAWKEY_UP || code == RAWKEY_DOWN || code == RAWKEY_LEFT || code == RAWKEY_RIGHT )
				{
					if (data->lasso != LASSO_OFF)
					{
						if (code == RAWKEY_UP)
							set(obj, MUIA_Virtgroup_Top, data->vy - 16);
						else if (code == RAWKEY_DOWN)
							set(obj, MUIA_Virtgroup_Top, data->vy + 16);
					}
					else
					{
						int selected_x = 0;
						int selected_y = - getv( obj, MUIA_Virtgroup_Top ) - SIZE_HUGE;
						APTR nearest_obj = NULL;
						APTR selected_obj = NULL;

						/* disable active object, as enter would 'execute' it */

						set(_win(obj), MUIA_Window_ActiveObject, NULL );

						/* Find first selected object */

						FORCHILD(obj, MUIA_Group_ChildList)
						{
							if ( getv( child, MA_Icon_Selected ) )
							{
								selected_obj = child;
								break;
							}
						}
						NEXTCHILD;

						if ( selected_obj )
						{
							selected_x = _left( selected_obj ) + _width( selected_obj ) / 2;
							selected_y = _top( selected_obj ) + _height( selected_obj ) / 2;
						}

						nearest_obj = find_nearest_obj(obj, selected_obj, code, selected_x, selected_y, data->isroot);

						if ( !nearest_obj  )
						{
							/* try again */

							switch (code)
							{
								case RAWKEY_UP:
									code = RAWKEY_LEFT;
									selected_y = _mbottom(obj) + 50;
									break;
								case RAWKEY_DOWN:
									code = RAWKEY_UP;
									selected_y = -10000;
									break;
								case RAWKEY_LEFT:
									code = RAWKEY_UP;
									selected_x = _mwidth(obj) + 50;
									selected_y -= 20;
									break;
								case RAWKEY_RIGHT:
									code = RAWKEY_DOWN;
									selected_x = -50;
									selected_y += 20;
									break;
							}

							nearest_obj = find_nearest_obj(obj, selected_obj, code, selected_x, selected_y, data->isroot);
						}

						if ( nearest_obj )
						{
							/*
							 * Focus on new icon first. If it's done after selection, for some reason
							 * image is not correct.
							 */

							if ( selected_obj )
							{
								set( selected_obj, MA_Icon_Selected, FALSE );

								/*
								 * Due to some problem with refresh, we have
								 * to invalidate icon so it will be really
								 * back to default.
								*/

								DoMethod( selected_obj, MM_Icon_UpdateBitMapBuffer );
							}

							DoMethod( obj, MM_Iconview_FocusIcon, nearest_obj );

							set( nearest_obj, MA_Icon_Selected, TRUE );
							data->selected_iconobj = nearest_obj;
						}
						else
						{
							if (selected_obj)
							{
								set(selected_obj, MA_Icon_Selected, FALSE);
								data->selected_iconobj = NULL;
							}

							set(_win(obj), MUIA_Window_ActiveObject, (code == RAWKEY_DOWN || code == RAWKEY_RIGHT) ? MUIV_Window_ActiveObject_Next : MUIV_Window_ActiveObject_Prev);
						}

					}

					return (MUI_EventHandlerRC_Eat);
				}
				else if ( code == RAWKEY_RETURN )
				{
					/* Find first selected object (or, why not 'click' them all?) */

					FORCHILD(obj, MUIA_Group_ChildList)
					{
						if ( getv( child, MA_Icon_Selected ) )
						{
							DoMethod( child, MM_Icon_Click, 0, 0 );
							DoMethod( child, MM_Icon_Select, 0, 0);

							return (MUI_EventHandlerRC_Eat);
						}
					}
					NEXTCHILD;
				}

			}
			break;
		}
	}

	return (0);
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	stccpy(data->path, (STRPTR) getv(obj, MA_View_Path), sizeof(data->path));

	if (muiRenderInfo(obj) && _win(obj))
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
		data->ehnode.ehn_Object = NULL;
	}

	return (DOSUPER);
}


DEFMMETHOD(AskMinMax)
{
	GETDATA;

	DOSUPER;

	msg->MinMaxInfo->MinWidth += IV_MINWIDTH;
	msg->MinMaxInfo->MinHeight += IV_MINHEIGHT;

	/* XXX: we should only do that stuff on the first opening ! */

	if (data->win_xs && data->win_ys)
	{
		msg->MinMaxInfo->DefWidth = max(msg->MinMaxInfo->MinWidth, data->win_xs - (15 + 2)); /* XXX: ask jaca about the original window sizes */
		msg->MinMaxInfo->DefHeight = max(msg->MinMaxInfo->MinHeight, data->win_ys - (15 + 15)); /* XXX: hm.. that one can vary though */
	}

	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX;
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

	return (0);
}


DEFSET
{
	GETDATA;

	FORTAG(INITTAGS)
	{
		case MUIA_Pressed: /* we don't want that to be broadcasted to childs */
			tag->ti_Tag = TAG_IGNORE;
			break;

		case MUIA_Selected: /* ditto */
			tag->ti_Tag = TAG_IGNORE;
			break;

		case MUIA_Virtgroup_Left:
			if (data->isroot)
			{
				tag->ti_Tag = TAG_IGNORE; /* we don't want movements in the root */
			}
			else
			{
				if (_isvisible(obj))
				{
					/* we check the bounds ourself as we need to set the background offset *before* MUI's virtgroup rendering */
					tag->ti_Data = minmax(0, (LONG)tag->ti_Data, (LONG)getv(obj, MUIA_Virtgroup_Width) - _mwidth(obj));
				}

				if (muiRenderInfo(obj) && _win(obj))
				{
					set(_win(obj), MA_Window_LeftOffset, tag->ti_Data);
				}

				data->vx = tag->ti_Data;
			}
			break;

		case MUIA_Virtgroup_Top:
			if (data->isroot)
			{
				tag->ti_Tag = TAG_IGNORE; /* ditto */
			}
			else
			{
				if (_isvisible(obj))
				{
					/* ditto */
					tag->ti_Data = minmax(0, (LONG)tag->ti_Data, (LONG)getv(obj, MUIA_Virtgroup_Height) - _mheight(obj));
				}

				if (muiRenderInfo(obj) && _win(obj))
				{
					set(_win(obj), MA_Window_TopOffset, tag->ti_Data);
				}

				if (data->lasso == LASSO_DRAWING || data->lasso == LASSO_STARTING)
				{
					ULONG vt;

					vt = getv(obj, MUIA_Virtgroup_Top);

					if (vt != tag->ti_Data)
					{
						APTR handle;

						data->lasso_pending = 1;

						switch (data->lasso)
						{
							case LASSO_STARTING:
								if ( (data->lasso_ctx = lasso_create(_rp(obj), data->click_x, data->click_y, _left(obj), _top(obj), _right(obj), _bottom(obj), data->vx, data->vy, _screen(obj)->RastPort.BitMap, gfx_get_penspec_value(muiRenderInfo(obj), &_conf(lasso_pen)))) )
								{
									pointer_set(_window(obj), POINTER_CROSSAIR);
									data->lasso = LASSO_DRAWING;
								}
								break;

							case LASSO_DRAWING:
								handle = MUI_AddClipping(muiRenderInfo(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj));
								lasso_clear(data->lasso_ctx);
								MUI_RemoveClipping(muiRenderInfo(obj), handle);
								break;
						}
					}
				}

				data->vy = tag->ti_Data;
			}
			break;

		/*
		 * Actual mode change is done on
		 * abort.
		 */
		case MA_View_ModeIndex:
			{
				LONG mapping[] = {IVM_ICON,
								  IVM_SHOWALL,
								  #if USE_THUMBS
								  IVM_THUMBS
								  #endif
								 };
				ULONG nmodes = sizeof( mapping ) / sizeof( *mapping );

				if ( tag->ti_Data <= nmodes && data->viewmode != mapping[ tag->ti_Data ] )
				{
					data->viewmode_postpone = mapping[ tag->ti_Data ];
					threads_abort(obj, TA_MimeType_Scan, NULL);  /* This one is first to be aborted. Thumbs scanner will follow */
				}
			}
			break;

		case MA_Iconview_SortMode:
			data->sortmode = tag->ti_Data;
			break;

		case MA_View_DiskUsage:
			data->diskusage = *(UQUAD *)(tag->ti_Data);
			break;

		case MA_View_SelectedDiskUsage:
			data->selecteddiskusage = *(UQUAD *)(tag->ti_Data);
			break;

		case MA_Icon_SizeAdjustment:

			/* Should ALWAYS be set with MUIA_Group_Forward, FALSE set! */
			// tag->ti_Tag = TAG_IGNORE; // bitRocky: Why that? Notify on this attr doesn't work if this is done!
			if (data->iconsizeadjustment != (LONG)tag->ti_Data)
			{
				DoMethod(obj, MUIM_Group_InitChange);
				data->iconsizeadjustment = (LONG)tag->ti_Data;

				FORCHILD(obj, MUIA_Group_ChildList)
				{
					set(child, MA_Icon_SizeAdjustment, data->iconsizeadjustment);
				}
				NEXTCHILD

				DoMethod(obj, MM_Icon_UpdateBitMapBuffer); /* broadcast */

				layout_setattrs(data->lctx,
									LAYOUTTAG_VSpacing, _conf(icon_maxsize) + data->iconsizeadjustment,
									LAYOUTTAG_HSpacing, _conf(icon_maxsize) + data->iconsizeadjustment,
									TAG_DONE);

				DoMethod(obj, MM_Iconview_ClearLayout);
				DoMethod(obj, MUIM_Group_ExitChange);
			}
			break;

		case MA_Iconview_ShowMyMorphos:
			if (tag->ti_Data && data->mymorphosicon && _parent(data->mymorphosicon) == NULL)
			{
				/* we just try to addmymorphos icon again. */
				DoMethod(obj, MM_Iconview_AddIcon, data->mymorphosicon);
			}
			else if (!tag->ti_Data && data->mymorphosicon && _parent(data->mymorphosicon) != NULL)
			{
				/* remove mymorphos icon (without disposing it) */
				DoMethod(obj, MM_Iconview_DeleteIcon, data->mymorphosicon);
			}
			DoMethod(obj, MM_Iconview_DoLayout);

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
		case MA_Iconview_IconTextColor:
			*msg->opg_Storage = data->icon_text_color;
			return (TRUE);

		case MA_Iconview_IconTextBgColor:
			*msg->opg_Storage = data->icon_text_bgcolor;
			return (TRUE);

		case MA_Iconview_IconTextEffect:
			*msg->opg_Storage = data->isroot ? _conf(root_font_effect) : _conf(window_font_effect);
			return (TRUE);

		case MA_DragDrop_Path:
			*msg->opg_Storage = getv(_win(obj), MA_Window_Path);
			return (TRUE);

		case MA_DragDrop_Type:
			*msg->opg_Storage = data->isroot ? MV_DragDrop_Type_Root : MV_DragDrop_Type_Iconview;
			return (TRUE);

		case MA_Icon_FileType: /* for drag & drop, XXX: dodgy */
			*msg->opg_Storage = data ->isroot ? MV_Icon_FileType_Device : MV_Icon_FileType_File;
			return (TRUE);

		case MA_Iconview_Path:
			*msg->opg_Storage = getv(_win(obj), MA_Window_Path);
			return (TRUE);

		case MA_Iconview_NumIcons:
			*msg->opg_Storage = data->objcount;
			return (TRUE);

		case MA_Iconview_IsRoot:
			*msg->opg_Storage = data->isroot;
			return (TRUE);

		case MA_View_NeedsBackfill:
			*msg->opg_Storage = TRUE;
			return (TRUE);

		case MA_View_NewWin:
			*msg->opg_Storage = data->isroot ? TRUE : ( _conf(toolbar_browsermode) ? FALSE : TRUE );
			return (TRUE);

		case MA_View_HasBackground:
			*msg->opg_Storage = TRUE;
			return (TRUE);

		case MA_View_ModeIndex:
			switch (data->viewmode)
			{
				case IVM_ICON:
					*msg->opg_Storage = 0;
					break;

				case IVM_SHOWALL:
					*msg->opg_Storage = 1;
					break;

				#if USE_THUMBS
				case IVM_THUMBS:
					*msg->opg_Storage = 2;
					break;
				#endif

				#ifdef DEBUG
				default:
					PDB(("invalid index\n"));
					break;
				#endif
			}
			return (TRUE);

		case MA_View_ViewMode:	/* IVM_ICON, IVM_SHOWALL or IVM_THUMBS */
			*msg->opg_Storage = data->viewmode;
			return (TRUE);

		case MA_View_DiskUsage:
			*msg->opg_Storage = (ULONG)&(data->diskusage);
			return (TRUE);

		case MA_View_SelectedDiskUsage:
			*msg->opg_Storage = (ULONG)&(data->selecteddiskusage);
			return (TRUE);

		case MA_View_Type:
			*msg->opg_Storage = MV_View_Type_Icon;
			return (TRUE);

		case MA_Iconview_Qualifier:
			*msg->opg_Storage = data->qualifier;
			return (TRUE);

		case MA_View_ShowDevices:
			*msg->opg_Storage = data->show_devices && !data->isroot ? TRUE : FALSE;
			return (TRUE);

		case MA_View_SubObject:
			*msg->opg_Storage = (IPTR)data->selected_iconobj;
			return (TRUE);

		case MA_View_IsViewObject:
			*msg->opg_Storage = TRUE;
			return (TRUE);

		case MA_View_HandleIcons:
			*msg->opg_Storage = TRUE;
			return (TRUE);
		
		case MA_Icon_SizeAdjustment:
			*msg->opg_Storage = data->iconsizeadjustment;
			return (TRUE);
		
	}
	return (DOSUPER);
}


DEFMMETHOD(Backfill)
{
	GETDATA;

	if (!data->nobackfill)
	{
		APTR cliphandle = NULL;
		struct Region *clipregion = NULL;

		if ( data->noflicker && muiRenderInfo(obj) != NULL && data->isroot )
		{
			/*
			 * Iterate through icons and mask them from backfill.
			 * NOTE: For now only for root windows. generic case needs optimizations.
			 */

			clipregion = buildregionfromicons(obj);

			if ( clipregion != NULL )
			{
				struct Rectangle r;
				r.MinX = msg->left;
				r.MinY = msg->top;
				r.MaxX = msg->right;
				r.MaxY = msg->bottom;

				if ( XorRectRegion( clipregion, &r ) )
				{
					cliphandle = MUI_AddClipRegion(muiRenderInfo(obj), clipregion);
				}
				else
				{
					DisposeRegion( clipregion );
					clipregion = NULL;
				}
			}
		}

		/* draw */

		EraseRect(_rp(obj), msg->left, msg->top, msg->right, msg->bottom);

		if ( clipregion != NULL )
		{
			MUI_RemoveClipRegion(muiRenderInfo(obj), cliphandle); /* DisposeRegion() is done by MUI */
		}
	}
	return (0);
}

DEFMMETHOD(Draw)
{
	GETDATA;
	ULONG mleft, mtop, mwidth, mheight;
	ULONG rc = (ULONG)NULL;

	mleft = _mleft(obj);
	mtop = _mtop(obj);
	mwidth = _width(obj);
	mheight = _mheight(obj);

	if (data->lasso_ctx != NULL && data->lasso == LASSO_DRAWING && !data->lasso_pending)
	{
		APTR cliphandle;
		cliphandle = MUI_AddClipping(muiRenderInfo(obj), mleft, mtop, mwidth, mheight);
		lasso_render(data->lasso_ctx, data->click_xt, data->click_yt, data->vx, data->vy);
		MUI_RemoveClipping(muiRenderInfo(obj), cliphandle);
	}
	else
	{
		LONG vt, vl;

		vl = getv(obj, MUIA_Virtgroup_Left);
		vt = getv(obj, MUIA_Virtgroup_Top);

		/*
		 * Virtgroup sets those without notify so we
		 * have to do it ourself.
		 */
		if (muiRenderInfo(obj) && _win(obj) && !data->isroot)
		{
			SetAttrs(_win(obj), MA_Window_LeftOffset, vl,
								MA_Window_TopOffset, vt,
								TAG_DONE);
		}

		if (data->refresh & REFRESH_CLIP_ICON)
		{
			/*
			 * Optimization to avoid flickering
			 * when just one icon is added.
			 * XXX: not used atm, only for rootwin
			 */
			APTR cliphandle;

			cliphandle = MUI_AddClipping(muiRenderInfo(obj), _left(obj) + data->clip_x, _top(obj) + data->clip_y, data->clip_xs, data->clip_ys);

			rc = DOSUPER;

			MUI_RemoveClipping(muiRenderInfo(obj), cliphandle);
		}
		else if (bufferedlayers == FALSE && !(data->refresh & REFRESH_TRASHED) &&
				 muiRenderInfo(obj)->mri_Flags & MUIMRI_RESIZEREDRAW &&
				 data->prev_x == _window(obj)->LeftEdge && data->prev_y == _window(obj)->TopEdge && /* to prevent resize from left/top border to messup */
				 vl == data->prev_virt_x && vt == data->prev_virt_y)
		{
			/*
			 * Optimization to avoid flickering
			 * when the layout doesn't change.
			 */

			/*
			 * More than 1 MUI_AddClipping() call doesn't work so we
			 * use our own region instead. Go figure.. I already tried
			 * many times, it never works really. stuntzi says it's ok.
			 * Oh well.
			 */
			APTR cliphandle = NULL;
			struct Region *clipregion;

			if ( (clipregion = NewRegion()) )
			{
				struct Rectangle r;

				if (mheight > data->prev_ys)
				{
					r.MinX = mleft;
					r.MinY = mtop + data->prev_ys;
					r.MaxX = mleft + data->prev_xs - 1;
					r.MaxY = mtop + mheight - 1;

					OrRectRegion(clipregion, &r);
				}

				if (mwidth > data->prev_xs)
				{
					r.MinX = mleft + data->prev_xs;
					r.MinY = mtop;
					r.MaxX = mleft + mwidth - 1;
					r.MaxY = mtop + mheight - 1;

					OrRectRegion(clipregion, &r);
				}
				cliphandle = MUI_AddClipRegion(muiRenderInfo(obj), clipregion);

				rc = DOSUPER;

				MUI_RemoveClipRegion(muiRenderInfo(obj), cliphandle); /* DisposeRegion() is done by MUI */
			}
		}
		else
		{
			rc = DOSUPER;
		}

		data->prev_x = _window(obj)->LeftEdge;
		data->prev_y = _window(obj)->TopEdge;
		data->prev_xs = mwidth;
		data->prev_ys = mheight;
		data->prev_virt_x = vl;
		data->prev_virt_y = vt;
		data->prev_virt_xs = getv(obj, MUIA_Virtgroup_Width);
		data->prev_virt_ys = getv(obj, MUIA_Virtgroup_Height);

		data->refresh = 0;

		if (data->lasso_pending)
		{
			DoMethod(_app(obj), MUIM_Application_PushMethod, obj, 1, MM_Iconview_RefreshLasso);
		}

		return (rc);
	}

	return (DOSUPER);
}

#define MAXDRAG 8

struct oplace {
	APTR o;
	UBYTE x;
	UBYTE y;
};

#define _mw(o) getv(o, MA_Icon_ImageNormalWidth)
#define _mh(o) getv(o, MA_Icon_ImageNormalHeight)

#define DRAG_MAXSIZE_XS 256
#define DRAG_MAXSIZE_YS 256

static void xblitclipped(APTR src, ULONG src_x, ULONG src_y, ULONG src_xs, ULONG src_ys, APTR dst, ULONG minx, ULONG miny, ULONG maxx, ULONG maxy)
{
	if (!(src_x > maxx ||
		src_y > maxy ||
		src_x + src_xs - 1 < minx ||
		src_y + src_ys - 1 < miny))
	{
		ULONG bm_xs, bm_ys;

		/* XXX: hm.. tweak it for really big icons */

		if (src_x + src_xs - 1 > maxx)
		{
			bm_xs = src_xs - (src_x + src_xs - 1 - maxx);
		}
		else if (src_x < minx)
		{
			bm_xs = src_xs - (minx - src_x);
		}
		else
		{
			bm_xs = src_xs;
		}

		if (src_y + src_ys - 1 > maxy)
		{
			bm_ys = src_ys - (src_y + src_ys - 1 - maxy);
		}
		else if (src_y < miny)
		{
			bm_ys = src_ys - (miny - src_y);
		}
		else
		{
			bm_ys = src_ys;
		}

		gfx_blit(src, dst,
			BLITTAG_SrcX, (src_x < minx) ? minx - src_x : 0,
			BLITTAG_SrcY, (src_y < miny) ? miny - src_y : 0,
			BLITTAG_DstX, (src_x < minx) ? 0 : src_x - minx,
			BLITTAG_DstY, (src_y < miny) ? 0 : src_y - miny,
			BLITTAG_DstWidth, bm_xs,
			BLITTAG_DstHeight, bm_ys,
		TAG_DONE);
	}
}

DEFMMETHOD(CreateDragImage)
{
	struct MUI_AmbientDragImage *adi = malloc(sizeof(*adi));

	if (adi != NULL)
	{
		GETDATA;

		LONG minx, miny, maxx, maxy;
		ULONG icon_x, icon_y;
		ULONG bm_xs, bm_ys;
		ULONG bm_xs_unclipped, bm_ys_unclipped;
		APTR bm;

		memset(adi, 0, sizeof(*adi));

		adi->di.flags = MUIF_DRAGIMAGE_SOURCEALPHA | MUIF_DRAGIMAGE_NOSHADOWS;

		/*
		 * Try to figure out a proper
		 * size for the bitmap.
		 */
		minx = 0xffffffff;
		miny = 0xffffffff;
		maxx = 0;
		maxy = 0;

		FORCHILD(obj, MUIA_Group_ChildList)
		{
			if (getv(child, MA_Icon_Selected))
			{
				icon_x = ICON_GETIMG_LEFT(child);
				icon_y = ICON_GETIMG_TOP(child);

				minx = min(minx, icon_x);
				miny = min(miny, icon_y);

				maxx = max(maxx, icon_x + _mw(child));
				maxy = max(maxy, icon_y + _mh(child));
			}
		}
		NEXTCHILD

		bm_xs = bm_xs_unclipped = maxx - minx + 1;
		bm_ys = bm_ys_unclipped = maxy - miny + 1;

		//dprintf("original minx: %ld, maxx: %ld, miny: %ld, maxy: %ld\n", minx, maxx, miny, maxy);

		/* XXX: still lot of optimizations possible in the gfx_alpha() and there.. */
		{
			LONG tx = _window(obj)->MouseX + data->vx - _mleft(obj);
			LONG ty = _window(obj)->MouseY + data->vy - _mtop(obj);

			if (bm_xs > DRAG_MAXSIZE_XS)
			{
				LONG new_minx, new_maxx;

				bm_xs = DRAG_MAXSIZE_XS;

				new_minx = max(minx, tx - DRAG_MAXSIZE_XS / 2);
				new_maxx = new_minx + bm_xs - 1;

				if ( new_maxx > maxx )
				{
					LONG d = new_minx - minx;
					LONG maxd = new_maxx - maxx;

					if ( d > 0 )
					{
						new_minx -= min( d, maxd );
						new_maxx -= min( d, maxd );
					}
				}

				minx = new_minx;
				maxx = new_maxx;
			}

			if (bm_ys > DRAG_MAXSIZE_YS)
			{
				LONG new_miny, new_maxy;

				bm_ys = DRAG_MAXSIZE_YS;

				new_miny = max(miny, ty - DRAG_MAXSIZE_YS / 2);
				new_maxy = new_miny + bm_ys - 1;

				if ( new_maxy > maxy )
				{
					LONG d = new_miny - miny;
					LONG maxd = new_maxy - maxy;

					if ( d > 0 )
					{
						new_miny -= min( d, maxd );
						new_maxy -= min( d, maxd );
					}
				}

				miny = new_miny;
				maxy = new_maxy;

			}

		}

		/* XXX: hm.. which one ? well. is that needed by now ? */
		data->click_x = _window(obj)->MouseX;
		data->click_y = _window(obj)->MouseY;

		/*
		 * iconclass adds translation. no idea why but to not break it we compensate here.
		 */

		adi->di.touchx = _window(obj)->MouseX - minx + data->vx - _mleft(obj) - POSX_CORRECTION;
		adi->di.touchy = _window(obj)->MouseY - miny + data->vy - _mtop(obj) - POSY_CORRECTION;

		#if 0
		/* XXX: debug! check if touchx/y are outside! */
		{
			ULONG touchx2, touchy2;

			touchx2 = _window(obj)->MouseX + getv(obj, MUIA_Virtgroup_Left) - _mleft(obj);
			touchy2 = _window(obj)->MouseY + getv(obj, MUIA_Virtgroup_Top) - _mtop(obj);

			if (touchx2 < minx || touchx2 > maxx) dprintf("touchx2 is out of bounds %ld < %ld < %ld!\n", minx, touchx2, maxx);
			if (touchy2 < miny || touchy2 > maxy) dprintf("touchy2 is out of bounds %ld < %ld < %ld!\n", miny, touchy2, maxy);
		}
		#endif

		//DB(("min:%d,%d,%d,%d,%d,%d\n",minx,miny,getv(obj, MUIA_Virtgroup_Left),getv(obj, MUIA_Virtgroup_Top),_mleft(obj),_mtop(obj)));
		//DB(("compare: x: %ld/%ld, y: %ld/%ld\n", msg->touchx, data->click_x, msg->touchy, data->click_y)); /* XXX: hum.. which one do I have to take ? :) */

		bm = gfx_bitmap_create(bm_xs, bm_ys, 32, BITMAPTAG_Clear, TRUE, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE);

		if (bm != NULL)
		{
			adi->di.width = bm_xs;
			adi->di.height = bm_ys;

			FORCHILD(obj, MUIA_Group_ChildList)
			{
				if (getv(child, MA_Icon_Selected))
				{
					xblitclipped((APTR)getv(child, MA_Icon_ImageNormal), ICON_GETIMG_LEFT(child) - POSX_CORRECTION, ICON_GETIMG_TOP(child) - POSY_CORRECTION, _mw(child), _mh(child), bm, minx, miny, maxx, maxy);
				}
			}
			NEXTCHILD

			/* radius calculation. special cases where dragged ares is wide or thin */

			{
				ULONG radius;
				LONG rx = bm_xs, ry = bm_ys;
				LONG tx = adi->di.touchx, ty = adi->di.touchy;

				if (bm_ys_unclipped < DRAG_MAXSIZE_YS && bm_ys_unclipped < bm_xs_unclipped)
					radius = max(rx - tx, tx);
				else if (bm_xs_unclipped < DRAG_MAXSIZE_XS)
					radius = max(ry - ty, ty);
				else
				{
					rx = max(ry - ty, ty);
					ry = max(rx - tx, tx);
					radius = min(rx, ry);
				}

				//kprintf("%d,%d %d, t:%d,%d, r:%d,%d\n", bm_xs_unclipped, bm_ys_unclipped, radius, tx, ty, rx, ry);

				gfx_alpha_set_radial(bm, tx, ty, bm_xs, bm_ys, 0xff, radius);
			}
			adi->bitmap = bm;
			adi->di.bm = gfx_bitmap_bm(bm);
		}
		else
		{
			free(adi);
			adi = NULL;
		}
	}

	return ((ULONG)adi);
}

DEFMMETHOD(DeleteDragImage)
{
	struct MUI_AmbientDragImage *adi = (struct MUI_AmbientDragImage*)msg->di;
	ASSERT(adi);
	ASSERT(adi->bitmap);

	gfx_bitmap_delete(adi->bitmap);
	free(adi);

	return (0);
}


DEFMMETHOD(DragQuery)
{
	GETDATA;

	LONG ft = getv(msg->obj, MA_Icon_FileType);

	if (ft == MV_Icon_FileType_File || ft == MV_Icon_FileType_Directory || ft == MV_Icon_FileType_Device || (data->isroot && ft == MV_Icon_FileType_None) )
	{
		/* XXX: we need to detect if the object is ours */

		return (MUIV_DragQuery_Accept); /* XXX: not very advanced. refuse for Read only drives :) */
	}
	return (MUIV_DragQuery_Refuse);
}


DEFMMETHOD(DragBegin)
{
	/*
	 * MUI really sucks at redrawing the frame (flicker galore)
	 */

	/* XXX: implement own frame drawing */

	return (0);
}

DEFMMETHOD(DragDrop)
{
	GETDATA;
	LONG grid = data->qualifier & ( IEQUALIFIER_LALT | IEQUALIFIER_RALT ) ? FALSE : TRUE;

	if (getv(msg->obj, MA_View_NumSelected) > 1) /* multiple drop */
	{
		if (msg->obj == obj /*&& data->isroot == TRUE*/)
		{
			LONG oldx, oldy;
			LONG dx, dy;
			ULONG save_shortcuts = FALSE;
			APTR save_mycomputer_obj = NULL;

			/* XXX: there's a bug in there.. hm.. some offset thing.. yup. see below for "proper" handling.. bah. still wrong */
			dx = (LONG)msg->x - _window(obj)->LeftEdge - data->click_x;
			dy = (LONG)msg->y - _window(obj)->TopEdge - data->click_y;

			/* remove all selected icons from layouter so they won't cause conflicts when positioning new ones */

			FORCHILD(obj, MUIA_Group_ChildList)
			{
				if (getv(child, MA_Icon_Selected))
				{
					layout_icon_remove( data->lctx, child );
				}
			}
			NEXTCHILD

			FORCHILD(obj, MUIA_Group_ChildList)
			{
				if (getv(child, MA_Icon_Selected))
				{
					oldx = max((LONG)getv(child, MUIA_LeftEdge), _mleft(obj));
					oldy = max((LONG)getv(child, MUIA_TopEdge), _mtop(obj));

					DoMethod(obj, MUIM_Backfill, oldx, oldy, min(oldx + getv(child, MUIA_Width), _mright(obj)), min(oldy + getv(child, MUIA_Height), _mbottom(obj)), 0, 0);

					/* add icon to layout and reposition it */

					layout_icon_add(data->lctx, child);

					/* adjust coord to be real icon position and not object position */

					layout_icon_move(data->lctx, child,
						(LONG)getv(child, MA_Icon_Left) + dx + ICON_GETOBJ_WIDTH(child) / 2 - ICON_GETICON_WIDTH(child) / 2,
						(LONG)getv(child, MA_Icon_Top) + dy, grid);


					/* After moving an icon we mark it as one having defined position */

					set( child, MA_Icon_HasPos, TRUE );

					if (getv(child, MA_Icon_FileType) == MV_Icon_FileType_Device)
					{
						save_shortcuts = TRUE;
						hiddendrives_updatedevice(obj, (STRPTR)getv(child, MA_Icon_Path), TRUE, getv(child, MA_Icon_X), getv(child, MA_Icon_Y));
					}

					if (!save_shortcuts && getv(child, MA_Icon_IsShortcut))
					{
						save_shortcuts = TRUE;
					}

					if (getv(child, MA_Icon_Type) == MV_Icon_Type_MyComputer )
					{
						save_mycomputer_obj = child;
					}

					set(child, MA_Icon_ImmediateUpdate, TRUE);
				}
			}
			NEXTCHILD

			/*
			 * Force a redraw. We disable the backfill to not have
			 * everything cleared as MUI does that by default.
			 */
			data->nobackfill = TRUE;
			set(msg->obj, MA_Icon_ImmediateUpdate, TRUE);
			DoMethod(obj, MM_Iconview_Redraw);
			data->nobackfill = FALSE;

			FORCHILD(obj, MUIA_Group_ChildList)
			{
				if (getv(child, MA_Icon_Selected))
				{
					MUI_Redraw(child, MADF_DRAWUPDATE);
				}
			}
			NEXTCHILD

			if (_conf(icon_autosnapshot))
			{
				if (save_shortcuts)
				{
					DoMethod(obj, MM_Iconview_SaveShortcuts);
				}

				if (save_mycomputer_obj)
				{
					DoMethod(obj, MM_Iconview_SnapshotIcon, save_mycomputer_obj);
				}
			}
		}
		else if (msg->obj != obj)
		{
			#if USE_SHORTCUTS
			if (data->isroot)
			{
				struct MinList *ml;

				if( (ml = malloc(sizeof(*ml))) )
				{
					struct MinList ddnml;
					struct dragdropnode *ddn, *nextddn;
					struct shortcutnode *sn;

					NEWLIST(&ddnml);
					DoMethod(msg->obj, MM_View_GetSelectionList, &ddnml);

					NEWLIST(ml);

					/* convert dragdropnodes to shortcutnodes */
					ITERATELISTSAFE(ddn, nextddn, &ddnml)
					{
						if ( (sn = malloc(sizeof(*sn) + strlen(ddn->path) + 1)) )
						{
							sn->filetype = ddn->type;
							sn->x = ddn->x;
							sn->y = ddn->y;

							strcpy(sn->path, ddn->path);
							ADDTAIL(ml, sn);
						}
						/* XXX: careful here, if you break out you must still free the nodes */
						free(ddn);
					}

					if (!do_action(obj, TA_Shortcuts_Add,
						TT_Shortcuts_Add_List, ml,
						TAG_DONE))
					{
						struct shortcutnode *nextsn;

						ITERATELISTSAFE(sn, nextsn, ml)
						{
							free(sn);
						}
						free(ml);
					}
				}
				/* XXX */
			}
			else
			#endif
			{
				APTR dispatcher = (APTR)DoMethod(_app(obj), MM_Application_CreateActionDispatcher );

				if ( dispatcher )
				{
					struct dragdropnode *ddn, *nextddn;
					struct MinList ml;

					SetAttrs(dispatcher,
						MA_ActionDispatcher_IQualifier, getv(msg->obj, MA_Iconview_Qualifier),
						MA_ActionDispatcher_Event, ACTION_EVENT_DRAGNDROP,
						MA_ActionDispatcher_DstURI, (STRPTR)getv(_win(obj), MA_Window_Path),
						MA_ActionDispatcher_DstID, getv(_win(obj), MA_Window_ID),
						MA_ActionDispatcher_SrcURI, getv(_win(msg->obj), MA_Window_Path),
						MA_ActionDispatcher_RefWin, _win(msg->obj),
						TAG_DONE
					);

					NEWLIST(&ml);
					DoMethod(msg->obj, MM_View_GetSelectionList, &ml);

					ITERATELISTSAFE(ddn, nextddn, &ml)
					{
						DoMethod( dispatcher, MM_ActionDispatcher_AddURI, ddn->path, TRUE );
						free(ddn);
					}

					DoMethod( dispatcher, MM_ActionDispatcher_Execute );
				}
			}
		}
	}
	else
	{
		LONG dx, dy;

		dx = getv(msg->obj, MA_Icon_ClickPosX);
		dy = getv(msg->obj, MA_Icon_ClickPosY);

		/*
		 * Single moves.
		 */
		if (_parent(msg->obj) == obj)
		{
			/*
			 * Move in same iconview.
			 */
			ULONG oldx, oldy;

			oldx = max((LONG)getv(msg->obj, MUIA_LeftEdge), _mleft(obj));
			oldy = max((LONG)getv(msg->obj, MUIA_TopEdge), _mtop(obj));

			DoMethod(obj, MUIM_Backfill, oldx, oldy, min(oldx + getv(msg->obj, MUIA_Width), _mright(obj)), min(oldy + getv(msg->obj, MUIA_Height), _mbottom(obj)), 0, 0);

			/*
			 * We pass desired icon position but get object position, so have to shift x-coord to right position.
			 */

			layout_icon_move(data->lctx, msg->obj,
				msg->x - _window(obj)->LeftEdge - _left(obj) + getv(obj, MUIA_Virtgroup_Left) - dx + ICON_GETOBJ_WIDTH(msg->obj) / 2 - ICON_GETICON_WIDTH(msg->obj) / 2,
				msg->y - _window(obj)->TopEdge - _top(obj) + getv(obj, MUIA_Virtgroup_Top) - dy, grid);

			/*
			 * Force a redraw. We disable the backfill to not have
			 * everything cleared as MUI does that by default.
			 */
			data->nobackfill = TRUE;

			SetAttrs(msg->obj,
				MA_Icon_HasPos, TRUE,          /* After moving an icon we mark it as one having defined position */
				MA_Icon_ImmediateUpdate, TRUE,
				TAG_DONE
			);

			DoMethod(obj, MM_Iconview_Redraw);
			data->nobackfill = FALSE;

			MUI_Redraw(msg->obj, MADF_DRAWUPDATE);

			if (data->isroot && getv(msg->obj, MA_Icon_FileType) == MV_Icon_FileType_Device)
			{
				hiddendrives_updatedevice(obj, (STRPTR)getv(msg->obj, MA_Icon_Path), TRUE, getv(msg->obj, MA_Icon_X), getv(msg->obj, MA_Icon_Y));
			}

			if (_conf(icon_autosnapshot))
				DoMethod(obj, MM_Iconview_SnapshotIcon, msg->obj);
		}
		else
		{
			/*
			 * Move to iconview from another place.
			 */

			if (data->isroot)
			{
				do_action(obj, TA_Shortcuts_Add,
					TT_Shortcuts_Add_Path, (STRPTR)getv(msg->obj, MA_Icon_Path),
					TT_Shortcuts_Add_X, msg->x - _window(obj)->LeftEdge - _left(obj) - dx,
					TT_Shortcuts_Add_Y, msg->y - _window(obj)->TopEdge - _top(obj) - dy,
					TT_Shortcuts_Add_Type, getv(msg->obj, MA_Icon_FileType),
				TAG_DONE);
			}
			else
			{
				LONG filetype;

				filetype = getv(msg->obj, MA_Icon_FileType);
				if (filetype == MV_Icon_FileType_Directory || filetype == MV_Icon_FileType_File) /* XXX: and devices too, yep */
				{
					APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

					if ( dispatcher )
					{
						SetAttrs(dispatcher,
							MA_ActionDispatcher_IQualifier, getv(_parent(msg->obj), MA_Iconview_Qualifier),
							MA_ActionDispatcher_Event, ACTION_EVENT_DRAGNDROP,
							MA_ActionDispatcher_DstURI, (STRPTR)getv(_win(obj), MA_Window_Path),
							MA_ActionDispatcher_DstID, getv(_win(obj), MA_Window_ID),
							MA_ActionDispatcher_SrcURI, getv(_win(msg->obj), MA_Window_Path),
							MA_ActionDispatcher_RefWin, _win(msg->obj),
							TAG_DONE
						);

						DoMethod( dispatcher, MM_ActionDispatcher_AddURI, (STRPTR)getv(msg->obj, MA_Icon_Path), TRUE );
						DoMethod( dispatcher, MM_ActionDispatcher_Execute );
					}
				}
				#if 0
				// This looks wrong... -itix
				else if (filetype == MV_Icon_FileType_None && getv(msg->obj, MA_Icon_Type) != MV_Icon_Type_MyComputer)
				{
					do_action(NULL, TA_File_Move,
						TT_File_Move_SrcPath, (STRPTR)getv(msg->obj, MA_Icon_PathInfo),
						TT_File_Move_DstPath, (STRPTR)getv(_win(obj), MA_Window_Path),
						TT_File_Move_Refwin, _win(obj),
						(msg->mode == MV_DragDrop_Drop_Copy) ? TT_File_Move_Copy : TAG_IGNORE, TRUE,
						TT_File_Move_NoIcon, FALSE,
					TAG_DONE);
				}
				#endif
			}
		}
	}
	return (0);
}

DEFMMETHOD(DragFinish)
{
	/* XXX */

	return (0);
}

DEFMMETHOD(DragEvent)
{
	// objwindow is a temporary ptr, unless msg->obj is non-NULL
	// in our case, we just want to find out if the window is an appwindow
	// in case the msg->obj is NULL (meaning the win doesn't belong to us)
	// do not that we might get one of our app's windows here too
	if (msg->objwindow && !msg->obj)
	{
		struct ipc_appwindow *appwin;

		if ( (appwin = (APTR)AppWindowObtain(msg->objwindow)) )
		{
			// use a normal mouse ptr, means we can drop stuff
			msg->mouseptrtype = POINTERTYPE_NORMAL;
			// tell MUI we can drop stuff and that we've changed the mouse ptr
			msg->flags |= MUIF_DRAGEVENT_FOREIGNDROP | MUIF_DRAGEVENT_MOUSECHANGED;
		}

		AppWindowRelease();
	}

	return (0);
}


/*
 * Unselects every icon.
 */
DEFSMETHOD(View_Select)
{
	switch (msg->mode)
	{
		case MV_View_Select_Invert:
			{
				FORCHILD(obj, MUIA_Group_ChildList)
				{
					set(child, MA_Icon_Selected, !getv(child, MA_Icon_Selected));
				}
				NEXTCHILD
			}
			break;

		case MV_View_Select_All:
			set(obj, MA_Icon_Selected, TRUE);
			break;

		case MV_View_Select_None:
			set(obj, MA_Icon_Selected, FALSE);
			break;

        case MV_View_Select_Files:
        {
            FORCHILD(obj, MUIA_Group_ChildList)
            {
				set(child, MA_Icon_Selected, getv(child, MA_Icon_FileType) != MV_Icon_FileType_Directory);
            }
            NEXTCHILD
        }
        break;

        case MV_View_Select_Dirs:
        {
            FORCHILD(obj, MUIA_Group_ChildList)
            {
				set(child, MA_Icon_Selected, getv(child, MA_Icon_FileType) == MV_Icon_FileType_Directory);
            }
            NEXTCHILD
        }
        break;

		case MV_View_Select_Pattern:
		{
			STRPTR parsepat = NULL;
			ULONG patsize = strlen(msg->pattern) * 2 + 2;

			if ((parsepat = malloc(patsize)))
			{
				if ((ParsePatternNoCase(msg->pattern, parsepat, patsize) > -1))
				{
					FORCHILD(obj, MUIA_Group_ChildList)
					{
						if (MatchPatternNoCase(parsepat, (STRPTR) getv(child, MA_Icon_Name)))
						{
							set(child, MA_Icon_Selected, TRUE);
						}
						else
						{
							set(child, MA_Icon_Selected, FALSE);
						}
					}
					NEXTCHILD
				}

				free(parsepat);
			}
		}
		break;
	}
	return (0);
}


DEFSMETHOD(Iconview_SelectLasso)
{
	GETDATA;
	LONG b, t, l, r;

#if 0
	b = max(data->click_y, data->click_yt);
	t = min(data->click_y, data->click_yt);
	l = min(data->click_x, data->click_xt);
	r = max(data->click_x, data->click_xt);

	t = max(_top(obj), t);
	b = min(_bottom(obj), b);
	l = max(_left(obj), l);
	r = min(_right(obj), r);
#else
	b = max(data->click_vy, data->click_yt);
	t = min(data->click_vy, data->click_yt);
	l = min(data->click_vx, data->click_xt);
	r = max(data->click_vx, data->click_xt);
#endif

	if(data->click_xt == -1 || data->click_yt == -1)
		return (0);

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		#if 0
		// itix
		if (_isvisible(child))
		#endif
		{
			LONG itop = ICON_GETIMG_TOP(child) + _top(obj);
			LONG ibottom = ICON_GETOBJ_TOP(child) + ICON_GETOBJ_HEIGHT(child) + _top(obj);
			LONG ileft = ICON_GETOBJ_LEFT(child) + _left(obj);
			LONG iright = ileft + ICON_GETOBJ_WIDTH(child);

			if ((ibottom) < t ||
				(itop) > b ||
				(ileft) > r ||
				(iright) < l)
			{
				if (!msg->select)
				{
					/* fab. don't unselect icons here if shift is pressed */
					if (!(data->qualifier & (IEQUALIFIER_LSHIFT | IEQUALIFIER_RSHIFT)))
					{
						set(child, MA_Icon_Selected, FALSE);
					}
				}
			}
			else
			{
				if (msg->select)
				{
					set(child, MA_Icon_Selected, TRUE);
				}
			}
		}
	}
	NEXTCHILD

	return (0);
}


/*
 * Unselects every icon except the one supplied
 * as argument.
 */
DEFSMETHOD(Iconview_Unselect)
{

	if (msg->o && getv(obj, MA_View_NumSelected) > 1)
	{
		FORCHILD(obj, MUIA_Group_ChildList)
		{
			if (child != msg->o)
			{
				set(child, MA_Icon_Selected, FALSE);
			}
		}
		NEXTCHILD
	}
	return (0);
}


DEFTMETHOD(Iconview_RefreshLasso)
{
	GETDATA;

	data->lasso_pending = 0;

	data->click_xt = data->mousex + data->vx;
	data->click_yt = data->mousey + data->vy;

	DoMethod(obj, MM_Iconview_SelectLasso, TRUE);
	MUI_Redraw(obj, MADF_DRAWUPDATE);
	DoMethod(obj, MM_Iconview_SelectLasso, FALSE);

	return (0);
}


DEFTMETHOD(Iconview_Redraw)
{
	GETDATA;

	initializenoflicker( obj, data );

	DoMethod(obj, MUIM_Group_InitChange);
	DoMethod(obj, MUIM_Group_ExitChange2, TRUE);

	MUI_Redraw(obj, MADF_DRAWOBJECT);

	finalizenoflicker( obj, data );

	return (0);
}

DEFSMETHOD(View_RedrawEntry)
{
	APTR icon = msg->entry;


	/*
	 * XXX:Check later why the check is needed! It's NULL when quiting
	 * thumbnailer. _parent() check is needed as the object might be
	 * already detached from view.
	 */

	if (muiRenderInfo(obj) != NULL && _parent(icon) != NULL)
	{
		struct IconData *idata = (struct IconData *)muiUserData( icon );

		/* use this instead of _top as _top is 16bit only! */

		LONG iy = idata->y - getv( obj, MUIA_Virtgroup_Top ) + _top(obj);
		LONG iyb = iy + (LONG)_height(icon);

		LONG l = max( (LONG)_left( obj ), (LONG)_left( icon ) );
		LONG t = max( (LONG)_top( obj ), iy );
		LONG r = max( (LONG)_left( obj ), (LONG)_right( icon ) );
		LONG b = max( (LONG)_top( obj ), iyb );

		/* don't draw when not needed */

		if ( iyb < 0 )
			return 0;

		if ( iy > (LONG)_top( obj )  + (LONG)_height( obj ) - 1 )
			return 0;

		//DB(("%d vs %d\n", _top(icon), iy ));

		b = min( b, (LONG)_top( obj )  + (LONG)_height( obj ) - 1 );     /* clamp to bottom border */

		/* only draw background in place where icon won't draw it */

		if ( idata->addy )
		{
			//DoMethod(obj, MUIM_Backfill, l, t, r, b, 0, 0);
			DoMethod(obj, MUIM_Backfill, l, t, r, min( b, t + idata->addy ), 0, 0);
		}
		MUI_Redraw(icon, MADF_DRAWOBJECT);
	}

	return (0);
}


DEFTMETHOD(Iconview_AllocPens)
{
	GETDATA;

	if (data->isroot)
	{
		data->icon_text_color = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(root_pen1));
		data->icon_text_bgcolor = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(root_pen2));
	}
	else
	{
		data->icon_text_color = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(window_pen1));
		data->icon_text_bgcolor = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(window_pen2));
	}

	return (TRUE);
}


/*
 * Remove all the childs.
 */
DEFTMETHOD(Iconview_Empty)
{
	struct icon_node *in, *nextin;
	GETDATA;
	APTR *array = NULL;

	if (data->typescanner_methodid)
		DoMethod(obj, MUIM_Application_KillPushMethod, NULL, data->typescanner_methodid, 0);

	DoMethod(obj, MUIM_Group_InitChange);

	/*
	 * If more than 50 children then we dispose them from
	 * separate thread to improve responsiveness.
	 */

	if (data->objcount >= 50)
	{
		ULONG i = 0;
		array = (APTR*)malloc((data->objcount + 1) * sizeof(APTR));

		if (array != NULL)
		{
			FORCHILD(obj, MUIA_Group_ChildList)
			{
				/* remove reference from the icon, so it will be safe to dispose deficonpool */
				set(child, MA_Icon_Reference, NULL);

				DoMethod(obj, OM_REMMEMBER, child);
				array[ i++ ] = child;
			}
			NEXTCHILD

			array[ i ] = NULL;

			DoMethod(app, MM_Application_DisposeObjectArray, array, TRUE, TRUE);
		}
	}

	if (array == NULL)
	{
		FORCHILD(obj, MUIA_Group_ChildList)
		{
			DoMethod(obj, OM_REMMEMBER, child);
			MUI_DisposeObject(child);
		}
		NEXTCHILD
	}

	data->objectarray_dirty = TRUE;
	set(obj, MA_View_NumSelected, 0);

	/*
	 * Empty the 2 lists.
	 */

	ITERATELISTSAFE(in, nextin, &data->addlist)
	{
		MUI_DisposeObject(in->obj);
		icon_free(in);
	}
	NEWLIST(&data->addlist);

	ITERATELISTSAFE(in, nextin, &data->removelist)
	{
		/* ignore locked flag. no thread should be running at this point */
		MUI_DisposeObject(in->obj);
		icon_free(in);
	}
	NEWLIST(&data->removelist);

	DoMethod(obj, MUIM_Group_ExitChange);

	/*
	 * Because window should be empty here it might be good way to reset data->vy (for lasso) setup.
	 * Same for MyMorphos icon as it was deleted too.
	 */

	data->vy = 0;
	data->mymorphosicon = NULL;

	return (0);
}


DEFSMETHOD(Iconview_DeleteAppIcon)
{
	GETDATA;
	ULONG hits = 0;

	initializenoflicker( obj, data );

	//DB(("Remove app icon:0x%x\n", msg->address));

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		if (msg->address == (APTR)getv(child, MA_Icon_AppAddress))
		{
			DoMethod(obj, MUIM_Group_InitChange);
			DoMethod(obj, OM_REMMEMBER, child);
			MUI_DisposeObject(child);
			data->objectarray_dirty = TRUE;
			DoMethod(obj, MUIM_Group_ExitChange);
			hits++;

			/* XXX: Test for Chain|Q. Keep disabled for now. */
			//break;
		}
	}
	NEXTCHILD

	if ( hits>1 )
	{
		/* XXX: Test for Chain|Q. */
		DB(("***MULTIPLE HITS!(%d)\n", hits));
	}

	finalizenoflicker( obj, data );

	return (0);
}


/*
 * Handles both .info and without .info.
 * Beware, the input is the full path for rootwin
 * and the path relative to the window's basename otherwise.
 */
DEFSMETHOD(Iconview_RemoveByName)
{
	GETDATA;
	ULONG attr;

	initializenoflicker(obj, data);

	DoMethod(obj, MUIM_Group_InitChange);

	/* XXX: need to scan the 2 lists too ? */

	if (name_isinfo(msg->name))
	{
		if (data->isroot)
			attr = MA_Icon_PathInfo;
		else
			attr = MA_Icon_NameInfo;
	}
	else
	{
		if (data->isroot)
		{
			//attr = MA_Icon_Path;
			attr = MA_Icon_Name; /* XXX: hm.. well, it needs that because of MM_Iconview_RemoveByName.. rethink it ? */
		}
		else
		{
			attr = MA_Icon_Name;
		}
	}

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		if (!stricmp(msg->name, (STRPTR)getv(child, attr)))
		{
			struct IconData *idata = (struct IconData *)muiUserData(child);

			/* XXX: Disabled following code. It doesn't seem to have practical purpose anymore.
			 * Check if we could remove this. -itix
			 */

			#if 0
			/* Also in appclass.c/MM_Application_RemoveShortcut */
			if (data->isroot && getv(child, MA_Icon_IsShortcut) && getv(child, MA_Icon_Type) == MV_Icon_Type_Disk)
			{
				hiddendrives_updatedevice(obj, (STRPTR)getv(child, MA_Icon_Path), TRUE, getv(child, MA_Icon_X), getv(child, MA_Icon_Y));
			}
			#endif

			DoMethod(obj, OM_REMMEMBER, child);

			/* if object is locked, we just add it to remove list. it will be disposed at next layout */
			if (ATOMIC_FETCH(&idata->locked))
			{
				DoMethod(obj, MM_Iconview_DeleteIcon, child);
			}
			else
			{
				MUI_DisposeObject(child);
			}
			data->objectarray_dirty = TRUE;

			break;
		}
	}
	NEXTCHILD

	DoMethod(obj, MUIM_Group_ExitChange);
	finalizenoflicker(obj, data);
	SetWindowTitle(cl, obj, MF_View_SetStatus_Window);

	return (0);
}


DEFTMETHOD(Iconview_RemoveDefaults)
{
	GETDATA;
	
	initializenoflicker( obj, data );

	DoMethod(obj, MUIM_Group_InitChange);

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		if (getv(child, MA_Icon_IsDefault))
		{
			DoMethod(obj, OM_REMMEMBER, child);
			MUI_DisposeObject(child);
			data->objectarray_dirty = TRUE;

		}
	}
	NEXTCHILD

	DoMethod(obj, MUIM_Group_ExitChange);

	finalizenoflicker( obj, data );

	return (0);
}


DEFSMETHOD(Iconview_BuildSnapshotlist)
{
	struct MinList *il;
	struct snapshotnode *sn;
	/*
	 * Build up list so that the window can
	 * go away anytime. The list is freed
	 * from the process.
	 * (snapshotlist.c / tr_snapshot_list())
	 */

	if ( (il = malloc(sizeof(*il))) )
	{
		ULONG err = FALSE;

		NEWLIST(il);

		FORCHILD(obj, MUIA_Group_ChildList)
		{
			ULONG selection = msg->mode == MV_Iconview_BuildSnapshotlist_Snapshot_Selection || msg->mode == MV_Iconview_BuildSnapshotlist_Unsnapshot_Selection;
			STRPTR n = (STRPTR)getv(child, MA_Icon_PathInfo);
			ULONG selected = FALSE;

			ASSERT(n);

			if(selection)
			{
				selected = getv(child, MA_Icon_Selected);
			}

			if(!selection || selected)
			{
				if (!(sn = malloc(sizeof(*sn) + strlen(n) + 1)))
				{
					err = TRUE;
					PDB(("argh! not enough memory\n"));
					break;
				}

				strcpy(sn->name, n);
				if (msg->mode == MV_Iconview_BuildSnapshotlist_Snapshot || msg->mode == MV_Iconview_BuildSnapshotlist_Snapshot_Selection)
				{
					sn->x = getv(child, MA_Icon_X);
					sn->y = getv(child, MA_Icon_Y);
				}
				else
				{
					sn->x = NO_ICON_POSITION;
					sn->y = NO_ICON_POSITION;
				}

				ADDTAIL(il, sn);
			}
		}
		NEXTCHILD

		if (err)
		{
			struct snapshotnode *nextsn;

			ITERATELISTSAFE(sn, nextsn, il)
			{
				free(sn);
			}
			free(il);
			/* XXX */
		}
		else
		{
			do_action(obj, TA_Icon_Snapshot,
				TT_Icon_Snapshot_List, il,
			TAG_DONE);
		}
	}
	return (0);
}

DEFSMETHOD(Rexx_Snapshot)
{
	if (msg->icons)
	{
		DoMethod(obj, MM_Iconview_BuildSnapshotlist, msg->selection ? MV_Iconview_BuildSnapshotlist_Snapshot_Selection : MV_Iconview_BuildSnapshotlist_Snapshot);
	}

	return (DOSUPER);
}


DEFSMETHOD(Rexx_Unsnapshot)
{
	if (msg->icons)
	{
		DoMethod(obj, MM_Iconview_BuildSnapshotlist, msg->window ? MV_Iconview_BuildSnapshotlist_Unsnapshot_Selection : MV_Iconview_BuildSnapshotlist_Unsnapshot);
	}

	/* listviewclass.c calls MM_Application_ReloadIcons... should we too?
	*/
	return (DOSUPER);
}


DEFSMETHOD(Iconview_SnapshotIcon)
{
	GETDATA;

	if (data->isroot)
	{
		if (getv(msg->obj, MA_Icon_IsShortcut))
		{
			/* we now have a defined position - Piru */
			set( msg->obj, MA_Icon_HasPos, TRUE );

			/* this sucks but the Desktop.prefs stuff forces our hand. - Piru */
			if (getv(msg->obj, MA_Icon_FileType) == MV_Icon_FileType_Device)
			{
				hiddendrives_updatedevice(obj, (STRPTR)getv(msg->obj, MA_Icon_Path), TRUE, getv(msg->obj, MA_Icon_X), getv(msg->obj, MA_Icon_Y));
			}

			/* WTF, saving *all* shortcuts instead of just msg->obj? - fab */
			DoMethod(obj, MM_Iconview_SaveShortcuts);
		}
		else
		{
			ULONG type = getv(msg->obj, MA_Icon_Type);

			if (type == MV_Icon_Type_MyComputer)
			{
				dprefs_mymorphos_iconpos_set(getv(msg->obj, MA_Icon_X), getv(msg->obj, MA_Icon_Y));
				do_action(obj, TA_DesktopPrefs_Save, TAG_DONE);
			}
			else if (type == MV_Icon_Type_Device || getv(msg->obj, MA_Icon_Type) == MV_Icon_Type_Disk) /* XXX: sucks.. we don't take removables into account.. */
			{
				/* XXX: we could remove this code because all device and disk icons are shortcuts really... */

				DoMethod(msg->obj, MM_Rexx_Snapshot, TRUE, FALSE, FALSE); /* XXX: this stuff is missing from multiple drag & drop.. */
			}
		}
	}
	else
	{
		DoMethod(msg->obj, MM_Rexx_Snapshot, TRUE, FALSE, FALSE);
	}

	return (0);
}

DEFSMETHOD(Iconview_UnsnapshotIcon)
{
	GETDATA;

	if (data->isroot)
	{
		if (getv(msg->obj, MA_Icon_IsShortcut))
		{
			/* we now have no defined position - Piru */
			set( msg->obj, MA_Icon_HasPos, FALSE );

			/* this sucks but the Desktop.prefs stuff forces our hand. - Piru */
			if (getv(msg->obj, MA_Icon_FileType) == MV_Icon_FileType_Device)
			{
				hiddendrives_updatedevice(obj, (STRPTR)getv(msg->obj, MA_Icon_Path), TRUE, NO_ICON_POSITION, NO_ICON_POSITION);
			}

			/* WTF, saving *all* shortcuts instead of just msg->obj? - fab */
			DoMethod(obj, MM_Iconview_SaveShortcuts);
		}
		else
		{
			ULONG type = getv(msg->obj, MA_Icon_Type);

			if (type == MV_Icon_Type_MyComputer)
			{
				dprefs_mymorphos_iconpos_set(NO_ICON_POSITION, NO_ICON_POSITION);
				do_action(obj, TA_DesktopPrefs_Save, TAG_DONE);
			}
			else if (type == MV_Icon_Type_Device || getv(msg->obj, MA_Icon_Type) == MV_Icon_Type_Disk) /* XXX: sucks.. we don't take removables into account.. */
			{
				/* XXX: we could remove this code because all device and disk icons are shortcuts really... */

				DoMethod(msg->obj, MM_Rexx_Unsnapshot, TRUE, FALSE, FALSE); /* XXX: this stuff is missing from multiple drag & drop.. */
			}
		}
	}
	else
	{
		DoMethod(msg->obj, MM_Rexx_Unsnapshot, TRUE, FALSE, FALSE);
	}

	return (0);
}

DEFSMETHOD(Iconview_DoSort)
{
	GETDATA;

	layout_setattrs(data->lctx,
		LAYOUTTAG_SortMode, msg->mode,
		LAYOUTTAG_SortReversed, msg->sort,
	TAG_DONE);

	data->refresh |= REFRESH_TRASHED;

	DoMethod(obj, MM_Iconview_Redraw);

	return (0);
}


DEFTMETHOD(Iconview_SaveShortcuts)
{
	return (do_action(obj, TA_Shortcuts_SaveAll, TAG_DONE));
}


DEFSMETHOD(Iconview_AddShortcut)
{
	return (do_action(obj, TA_Shortcuts_Add, TT_Shortcuts_Add_Path, msg->path, TAG_DONE));
}


#if USE_SHORTCUTS
DEFSMETHOD(Iconview_SaveShortcuts2)
{
	ULONG i = 0;
	APTR pl, pi; /* list, item */
	#ifdef DEBUG
	GETDATA;

	ASSERT(data->isroot);
	#endif
	ASSERT(msg->pctx);

	if (!(pl = prefspool_item_get(msg->pctx, NULL, DSI_LISTPOOL_SHORTCUT, NULL, NULL)))
	{
		pl = prefspool_item_add(msg->pctx, NULL, DSI_LISTPOOL_SHORTCUT, NULL, (ULONG)NULL);
	}

	if (pl)
	{
		FORCHILD(obj, MUIA_Group_ChildList)
		{
			if (getv(child, MA_Icon_IsShortcut) && getv(child, MA_Icon_FileType) != MV_Icon_FileType_Device)
			{
				if (!(pi = prefspool_item_get(msg->pctx, pl, i | DSF_LISTPOOL, NULL, NULL)))
				{
					pi = prefspool_item_add(msg->pctx, pl, i | DSF_LISTPOOL, NULL, (ULONG)NULL);
				}

				if (pi)
				{
					TEXT device[ 64 ];
					STRPTR path = (STRPTR)getv(child, MA_Icon_Path);
					LONG haspos;

					/*
					 * While saving shortcuts, we use device name (DHx: thingie), so translate.
					 */

					D(SHORTCUT,bug("Shortcut to save:%s\n", path));

					if ( isdevicename( path ) )
					{
						struct dlcnode *dlcn;
						strcpy( device, path );
						device[ strlen( device ) - 1 ] = '\0'; /* ok since it's guaranteed to have at least 1 char */

						D(SHORTCUT,bug("Device detected <%s>\n", path));

						dlcn = doslistcache_find_dlcdevice_by_devicename(device);
						if ( !dlcn )
							dlcn = doslistcache_find_dlcdevice_by_volumename(device);

						if ( dlcn )
						{
							strcpy( device, dlcn->name );
							strcat( device, ":" );
							path = device;

							D(SHORTCUT,bug("Translated shortcut name:<%s>\n", path));

						}
					}

					D(SHORTCUT,bug("Saving shortcut <%s>\n", path));

					setprefsstr_lp(msg->pctx, pi, DSI_LISTPOOL_SHORTCUT_PATH, path);
					/* if position is unfined save NO_ICON_POSITION - Piru */
					haspos = getv(child, MA_Icon_HasPos);
					setprefslong_lp(msg->pctx, pi, DSI_LISTPOOL_SHORTCUT_X, haspos ? getv(child, MA_Icon_X) : NO_ICON_POSITION);
					setprefslong_lp(msg->pctx, pi, DSI_LISTPOOL_SHORTCUT_Y, haspos ? getv(child, MA_Icon_Y) : NO_ICON_POSITION);
					/* XXX: retvals.. */
				}
				/* XXX: hm.. */
				i++;
			}
		}
		NEXTCHILD
	}
	/* XXX */

	return (0);
}
#endif


DEFSMETHOD(Iconview_FindShortcut)
{
	#ifdef DEBUG
	GETDATA;
	ASSERT(data->isroot);
	#endif

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		if (getv(child, MA_Icon_IsShortcut) && !stricmp((STRPTR)getv(child, MA_Icon_PathInfo), msg->path)) /* XXX: hm. could there be some SYS: <-> realname: issues ? */
		{
			return ((ULONG)child);
		}
	}
	NEXTCHILD

	return (FALSE);
}

DEFSMETHOD(Thread_Finished)
{
	if (msg->status == MV_Thread_Finished_Abort)
	{
		ULONG callsuper = TRUE;
		GETDATA;

		switch (msg->action)
		{
			case TA_MimeType_Scan:

				data->typescanner_running = FALSE;

				threads_abort(obj, TA_Iconview_ShowPreview, NULL);
				callsuper = FALSE;
				break;

			case TA_Iconview_ShowPreview:

				data->videopreview_running = FALSE;
				data->videopreview_abort = FALSE;

				if(data->videopreview_restart) /* start a new videopreview thread case */
				{
					data->videopreview_restart = FALSE;
					DoMethod(obj, MM_Iconview_ShowPreview, TRUE);
					callsuper = TRUE;
				}
				else /* normal abort case */
				{
					if (data->isroot)
						threads_abort(obj, TA_Devices_UpdateInfo, NULL);
					else
						threads_abort(obj, TA_File_ScanDir, NULL);

					callsuper = FALSE;
				}

				break;

			case TA_Devices_UpdateInfo:
				threads_abort(obj, TA_Devices_Show, NULL);
				callsuper = FALSE;
				break;

			case TA_Devices_Show:
				threads_abort(obj, TA_File_ScanDir, NULL);
				callsuper = FALSE;

				break;

			case TA_File_ScanDir:

				/*
				 * We need to break 2 threads before starting new job. So we first break ScanDir one
				 * and it breaks thumbnails generation thread.
				 */

				threads_abort(obj, TA_Thumbnail_Create, NULL);

				/*
				 * We don't want to call superclass' Thread_Fished yet!!.
				 */
				callsuper = FALSE;

				break;

			case TA_Thumbnail_Create:

			#if 0
				threads_abort(obj, TA_MimeType_Scan, NULL);
				break;

			case TA_MimeType_Scan:
			#endif
				{
					GETDATA;

					if (data->devices_postpone)
					{
						data->devices_postpone = FALSE;
						DoMethod(obj, MM_Iconview_Empty);   // get rid of old icons if not root
						do_action(obj, TA_Devices_Show, TT_Devices_Show_Assigns, TRUE, TAG_DONE);
					}
					else if (data->viewmode_postpone != data->viewmode)
					{
						/*
						 * if data->viewmode_postpone == -1, then just use current viewmode.
						 */

						if (data->viewmode_postpone == -1)
							data->viewmode_postpone = data->viewmode;

						/*
						 * Here we have to perform mode change or initial dir reading.
						 * TODO: smarted mode switching.
						 */

						switch (data->viewmode_postpone)
						{
							case IVM_ICON:
								{
									DoMethod(obj, MM_Iconview_Empty); /* XXX: there should be a method to turn thumbs into defaults again.. maybe.. bah.. that sucks */

									do_action(obj, TA_File_ScanDir,
										TT_File_ScanDir_Path, getv(obj, MA_View_Path),
										TT_File_ScanDir_Mode, TV_File_ScanDir_Mode_Icons,
									TAG_DONE);

									data->viewmode = IVM_ICON;
								}
								break;

							case IVM_SHOWALL:
								{
									DoMethod(obj, MM_Iconview_Empty); /* XXX: there should be a method to turn thumbs into defaults again.. maybe.. bah.. that sucks */

									do_action(obj, TA_File_ScanDir,
										TT_File_ScanDir_Path, getv(obj, MA_View_Path),
										TT_File_ScanDir_Mode, TV_File_ScanDir_Mode_ShowAll,
									TAG_DONE);

									data->viewmode = IVM_SHOWALL;
								}
								break;

							#if USE_THUMBS
							case IVM_THUMBS:
								{
									DoMethod(obj, MM_Iconview_Empty);

									do_action(obj, TA_File_ScanDir,
										TT_File_ScanDir_Path, getv(obj, MA_View_Path),
										TT_File_ScanDir_Mode, TV_File_ScanDir_Mode_Thumbs,
									TAG_DONE);

									data->viewmode = IVM_THUMBS;
								}
								break;
							#endif
						}
					}
				}
				break;
		}

		if (callsuper)
			return (DOSUPER);
		else
			return 0;
	}
	else
	{
		switch (msg->action)
		{
			case TA_Iconview_ShowPreview:
				{
					GETDATA;
					data->videopreview_running = FALSE;
					data->videopreview_abort = FALSE;
				}
				break;

			case TA_Devices_Show:
				{
					#ifdef DEBUG
					GETDATA;
					ASSERT(data->show_devices);
					#endif

					/* XXX: check msg->status, damn! and check the scenario below.. */
					DoMethod(obj, MM_Iconview_DoLayout);

					if (_conf(misc_driveinfo))
					{
						DoMethod( obj, MM_Iconview_UpdateDriveInfo );
					}
				}
				break;

			case TA_Devices_RemoveAll:
				do_action(obj, TA_Devices_Show, TT_Devices_Show_Assigns, TRUE, TT_Devices_Fade, FALSE, TAG_DONE);
				break;

			case TA_Thumbnail_Create:
			case TA_MimeType_Scan:
					{
						GETDATA;

						SetWindowTitle(cl, obj, MF_View_SetStatus_Window | MF_View_SetStatus_Bar | MF_View_SetStatus_Busy);

						data->typescanner_running = FALSE;

						if ( data->numunscanned )
						{
							/* still something unscanned? */

							data->typescanner_methodid = DoMethod( app, MUIM_Application_PushMethod, obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE | MUIF_PUSHMETHOD_VERIFY, MM_Iconview_StartScanner);
						}
					}
				break;

			case TA_File_ScanDir:
				{
					GETDATA;
					/*
					 * Compute the title.
					 */

					ASSERT(!data->isroot);

					DoMethod(obj, MM_Iconview_DoLayout);	/* will spawn typescanner */

					SetWindowTitle(cl, obj, MF_View_SetStatus_Window);
				}
				break;
		}
		return (0);
	}
}


DEFTMETHOD(View_LoadURI)
{
	GETDATA;
	CONST_STRPTR type = "";
	STRPTR schemestr;
	STRPTR path = (STRPTR) getv(_view(obj), MA_View_Path);
	LONG mode;
	ULONG modeset;

	/* check if we are just reloading. (XXX: This will do IO ops, but it should be safe here) */
	if ( is_path_equal( path, data->path ) && data->initialselection == NULL )
	{
		struct Rect32 area;
		LONG i;

		DoMethod( obj, MM_View_QueryDisplayArea, &area );
		data->focuspos = area.MinY;
	
		data->initialselection = (STRPTR*)DoMethod( obj, MM_View_PickSelected );

		/* process array to contain names, not objects (objects will be disposed later). */

		for(i=0; data->initialselection != NULL && data->initialselection[ i ] != NULL; i++)
			data->initialselection[ i ] = name_build( FilePart( (STRPTR)getv( data->initialselection[ i ], MA_Icon_Path ) ) );
	}
	else
	{
		LONG i;
		data->focuspos = 0;

		/* initialselection might be still here */

		if (data->initialselection != NULL)
		{
			for(i=0; data->initialselection[ i ] != NULL ;i++)
				name_delete(data->initialselection[ i ]);

			FreeVecTaskPooled(data->initialselection);
			data->initialselection = NULL;
		}
	}

	stccpy( data->path, path, sizeof( data->path ) );

	/*
	 * Stop notification monitor before loading new URI.
	 */
	notify_unregister(data->nctx,
		NOTIFYTAG_Monitor_File, NOTIFYVAL_Monitor_File_ClearAll,
		NOTIFYTAG_Monitor_Device, NOTIFYVAL_Monitor_Device_ClearAll,
	TAG_DONE);

	/*
	 * If view had some mode selected previously, we will use it if no other is specified in the URI.
	 */

	if ( data->viewmode == IVM_NONE )
		data->viewmode = IVM_ICON;

	data->show_devices = 0;

	if (data->isroot || ((schemestr = (STRPTR)getv(_view(obj), MA_View_Scheme)) && !stricmp("devices", schemestr)))
	{
		data->show_devices = 1;

		UpdateHandlers(cl, obj, data, TRUE);

		notify_register(data->nctx,
			NOTIFYTAG_Monitor_Device, "*",
			NOTIFYTAG_Monitor_Device_UnMount, TRUE,
			NOTIFYTAG_Monitor_Device_Name, TRUE,
			NOTIFYTAG_Monitor_File_Name, TRUE,
			NOTIFYTAG_Inform_Object, obj,
		TAG_DONE);

		notify_register(data->nctx,
			NOTIFYTAG_Monitor_File, "*",
			NOTIFYTAG_Monitor_File_Name, TRUE,
			NOTIFYTAG_Inform_Object, obj,
		TAG_DONE);

		notify_register(data->nctx,
			NOTIFYTAG_Monitor_File, "*disk.info",
			NOTIFYTAG_Monitor_File_Create, TRUE,
			NOTIFYTAG_Inform_Object, obj,
		TAG_DONE);

		if (!data->isroot)
		{
			/*
			 * Here we can use custom dimensions.
			 */

			DoSuperMethod(cl, obj, MM_View_ParseWindowArgs, &type, &mode, &modeset, &data->win_xs, &data->win_ys, NULL, NULL, NULL);

			data->devices_postpone = TRUE;
			threads_abort(obj, TA_MimeType_Scan, NULL);
		}
		else if (!data->startup)
		{
			do_action(obj, TA_Devices_Show, TT_Devices_Show_IsRoot, TRUE, TAG_DONE);

			if (args.wbstartup)
			{
				do_action(obj, TA_WBStartup_Execute,
					TT_WBStartup_Execute_Path, getprefsstr(DSI_MISC_WBSTARTUP_PATH),
				TAG_DONE);
			}

			if (_conf(misc_remember_windows) || _conf(misc_remember_documents))
			{
				do_action(obj, TA_StartupPrefs_Load, TAG_DONE);
				sprefs_setup();
			}

			data->startup = TRUE;
		}

		return (0);
	}
	else
	{
		TEXT pat[PATH_SIZE];
		ULONG sortset, sortby, sortorder;

		data->show_devices = 0;

		iconview_enable_dosnotify((STRPTR) getv(obj, MA_View_Path), TRUE);

		UpdateHandlers(cl, obj, data, FALSE);

		name_quotepattern((STRPTR)getv(obj, MA_View_Path), pat, sizeof(pat)); /* XXX: that sucks.. and what about "" paths ? */
		AddPart(pat, "*", sizeof(pat));

		notify_register(data->nctx,
			NOTIFYTAG_Monitor_File, pat,
			NOTIFYTAG_Monitor_File_Name, TRUE,
			NOTIFYTAG_Monitor_File_Create, TRUE,
			NOTIFYTAG_Monitor_File_Delete, TRUE,
			NOTIFYTAG_Inform_Object, obj,
		TAG_DONE);

		if (DoSuperMethod(cl, obj, MM_View_ParseWindowArgs, &type, &mode, &modeset, &data->win_xs, &data->win_ys, &sortset, &sortby, &sortorder))
		{
			switch (mode)
			{
				case LVM_SHOWALL:
					data->viewmode = IVM_SHOWALL;
					break;

				case LVM_THUMBS:
					data->viewmode = IVM_THUMBS;
					break;

				case LVM_ICONS:
					data->viewmode = IVM_ICON;
					break;
			}
			
			if (sortset)
			{
				DoMethod( obj, MM_Iconview_DoSort, sortby, (sortorder==-1) ? IVSO_DECREMENTAL : IVSO_INCREMENTAL );
			}
		}
		else
		{
			/* XXX: use default */
		}

		if (!stricmp("vfs", schemestr))
		{
			APTR fscontext;

			if ( (fscontext = vfs_open(path, vfs_lookup(type))) )
			{
				STRPTR buffer = (STRPTR) malloc(strlen(vfs_device(fscontext))+2);

				if(buffer)
				{
					sprintf(buffer, "%s:", vfs_device(fscontext));

					SetAttrs(_view(obj),
						MA_View_Path, buffer,
						MA_View_PreviousPath, buffer,
						MA_View_Scheme, "file",
						TAG_DONE
					);

					/* we need to reload path after setting it with above function */

					path = (STRPTR) getv(_view(obj), MA_View_Path);

					free(buffer);
				}
			}
		}

		/*
		 * Load the icons.
		 */

		{
			TEXT wintitle[PATH_SIZE];
			TEXT resolved_path[PATH_SIZE];

			vfs_resolve_path(path, resolved_path, sizeof(resolved_path), FALSE);
			name_build_wintitle(wintitle, sizeof(wintitle), resolved_path);

			strncat(wintitle, GSI(MSG_ICONVIEW_SCANNING_LOCATION), PATH_SIZE);

			DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window | MF_View_SetStatus_Bar | MF_View_SetStatus_Busy, "%s", wintitle);
		}

		data->viewmode_postpone = -1;

		/*
		 * This one will first abort all running threads and then start
		 * new ones with new path.
		 */

		data->devices_postpone = FALSE;
		threads_abort(obj, TA_MimeType_Scan, NULL);
	}
	return (0);
}


DEFSMETHOD(View_Refresh)
{
	GETDATA;

	initializenoflicker( obj, data );

	if (msg->flags & (MF_View_Refresh_Background | MF_View_Refresh_Fonts | MF_View_Refresh_Size))
	{
		APTR lasso = data->lasso_ctx;
		if (lasso != NULL)
			lasso_invalidate(lasso);

		data->lasso_ctx = NULL;

		DoMethod(obj, MUIM_Group_InitChange);

		if (msg->flags & MF_View_Refresh_Fonts)
		{
			/*
				This is called on prefs change too, so we need to get new pens from prefs.
				Hope it's good place to call it.
			*/

			DoMethod(obj, MM_Iconview_AllocPens);
			SetAttrs(obj,
				MA_Icon_AFont, data->isroot ? _conf(root_font) : _conf(window_font), /* broadcast */
				MA_Icon_SmallFont, data->isroot ? _conf(root_font_small) : _conf(window_font), /* broadcast */
				MA_Icon_FontSpace, data->isroot ? _conf(root_spaceline) : _conf(window_spaceline), /* broadcast */
				MA_Icon_TextColor, data->icon_text_color,
				MA_Icon_TextBgColor, data->icon_text_bgcolor,
				TAG_DONE
			);
			DoMethod(obj, MM_Iconview_ClearLayout);
			DoMethod(obj, MM_Iconview_Redraw);
		}

		if (msg->flags & MF_View_Refresh_Background)
		{
			if (data->isroot && msg->dtp)
			{		
				// NOTE: do not use _screen(obj). it might not be valid at this point! -- kiero
				DoMethod(app, MM_Application_CenterBackground, getv(obj, MA_View_BgPen), get_screen()); /* XXX: there should be a getsuper() or so.. faster */
			}
		}

		if (msg->flags & MF_View_Refresh_Size)
		{
			/* relayout */

			FORCHILD( obj, MUIA_Group_ChildList )
			{
				DoMethod( child, MM_Icon_Generate, TRUE );
			}
			NEXTCHILD;

			DoMethod( obj, MM_Iconview_UpdateDriveInfo );
			DoMethod( obj, MM_Iconview_DoLayout );
		}

		DoMethod(obj, MM_Icon_UpdateBitMapBuffer); /* broadcast */
		DoMethod(obj, MUIM_Group_ExitChange);
		data->lasso_ctx = lasso;
	}

	finalizenoflicker( obj, data );

	return (0);
}

static int cmp_name( STRPTR *n1, STRPTR *n2)
{
	return StrnCmp(locale, FilePart(*n1), FilePart(*n2), -1, SC_ASCII);
}

DEFTMETHOD(Iconview_DoLayout)
{
	struct icon_node *in, *nextin;
	ULONG oldobjcount;
	ULONG newobjcount = 0;
	ULONG changed;
	GETDATA;

	oldobjcount = data->objcount;
	initializenoflicker(obj, data);
	changed = (ISLISTEMPTY(&data->addlist) == FALSE) || (ISLISTEMPTY(&data->removelist) == FALSE);

	DoMethod(obj, MUIM_Group_InitChange);

	ITERATELISTSAFE(in, nextin, &data->removelist)
	{
		/* icon might actualy be already removed from group here (when locked but not disposed yet) */
		if (_parent(in->obj) != NULL)
		{
			DoMethod(obj, OM_REMMEMBER, in->obj);
		}

		if (data->isroot)
		{
			/* Check if it's mymorphos icon. If it is then don't delete it just yet. */
			ULONG type = getv(in->obj, MA_Icon_Type);
			if (type != MV_Icon_Type_MyComputer)
				MUI_DisposeObject(in->obj);

			REMOVE(in);
			icon_free(in);
		}
		else
		{
			struct IconData *idata = (struct IconData *)muiUserData(in->obj);

			if (ATOMIC_FETCH(&idata->locked) == FALSE)
			{
				MUI_DisposeObject(in->obj);
				REMOVE(in);
				icon_free(in);
			}
		}

		oldobjcount--;
	}

	NEWLIST(&data->removelist);
	
	/*
	 * Add new objects, and move them from addlist to tobescanned list.
	 * Check if they are on initial selection list (dir reloading).
	 */

	if (data->initialselection != NULL)
	{
		ULONG i, n;

		/* sort array by name */

		for(n=0; data->initialselection[ n ] != NULL ;n++){}

		qsort(data->initialselection, n, sizeof( APTR ), (const void *)cmp_name);

		ITERATELISTSAFE(in, nextin, &data->addlist)
		{
			APTR select;
			STRPTR name;

			newobjcount++;

			set(in->obj, MA_Icon_SizeAdjustment, data->iconsizeadjustment);
			DoMethod(obj, OM_ADDMEMBER, in->obj);

			name = (STRPTR)getv( in->obj, MA_Icon_Path );
			select = bsearch(&name, data->initialselection, n, sizeof(APTR), (const void *)cmp_name);

			if (select)
				set(in->obj, MA_Icon_Selected, TRUE);

			icon_free(in);

		}

		for(i=0; data->initialselection[ i ] != NULL ;i++)
			name_delete(data->initialselection[ i ]);

		FreeVecTaskPooled(data->initialselection);
		data->initialselection = NULL;
	}
	else
	{
		ITERATELISTSAFE(in, nextin, &data->addlist)
		{
			newobjcount++;

			/*
			 * kiero: we set a font here because following situation may occur:
			 * icon is created without font. it's added to view but not added to mui group yet.
			 * in the meanwhile fonts are set and assigned to objects in group. objects on pending
			 * list will be missed. Or at least that's what i think is happening and is a reason for
			 * missing labels:)
			 */

			SetAttrs(in->obj, MA_Icon_SizeAdjustment, data->iconsizeadjustment,
				MA_Icon_AFont, data->isroot ? _conf(root_font) : _conf(window_font),
				MA_Icon_SmallFont, data->isroot ? _conf(root_font_small) : _conf(window_font),
				TAG_DONE);

			DoMethod(obj, OM_ADDMEMBER, in->obj);
			icon_free(in);
		}
	}
	
	NEWLIST(&data->addlist);

	data->objectarray_dirty = TRUE;

	/*
	 * Never autolayout root window
	 */
	if (!data->isroot)
	{
		ULONG autolayout = TRUE;

		if (!data->show_devices && !_aprefs(autosort))
		{
			autolayout = FALSE;

			layout_place_icons(data->lctx);
		}

		layout_setattrs(data->lctx,
			LAYOUTTAG_Auto, autolayout,
			LAYOUTTAG_WBMode, !autolayout,
			TAG_DONE);
	}

	/* XXX: here we should sort the objects. but only when needed (eg. not for snapshot layout) */
	if (layout_getattr(data->lctx, LAYOUTTAG_Auto))
	{
		layout_sort(data->lctx);
	}

	DoMethod(obj, MUIM_Group_ExitChange);

	if (data->focuspos != 0)
		set(obj, MUIA_Virtgroup_Top, data->focuspos);

	data->focuspos = 0;

	/*
	 * If new icons were added then we launch scanner thread.
	 * If view was empty before then we launch it immediately (new window contents probably).
	 * If there were some icons, then we defer it as new icons might apear soon (copy/move ops).
	 */

	data->numunscanned = newobjcount;

	if (data->numunscanned && !data->show_devices)
	{
		if (oldobjcount == 0)
		{
			DoMethod(obj, MM_Iconview_StartScanner);
		}
		else
		{
			data->typescanner_methodid = DoMethod(app, MUIM_Application_PushMethod, obj, 1 | MUIV_PushMethod_Delay(500) | MUIF_PUSHMETHOD_SINGLE | MUIF_PUSHMETHOD_VERIFY, MM_Iconview_StartScanner);
		}
	}

	finalizenoflicker(obj, data);
	if (changed)
		update_diskusage(cl, obj);
	return (0);
}


DEFTMETHOD(Iconview_ClearLayout)
{
	GETDATA;

	layout_clear(data->lctx);

	return (0);
}


DEFTMETHOD(Iconview_ClearBubble)
{
	GETDATA;

	if (data->bubbleobj)
	{
		DoMethod(obj, MUIM_DeleteBubble, data->bubbleobj);
	}
	data->bubbleobj = NULL;
	data->bubblecnt = 0;
	return (0);
}


DEFSMETHOD(Metadata_Info)
{
	GETDATA;

	if (data->bubblecnt && _window(obj)) /* check if the mouse didn't move outside by then */
	{
		LONG px, py;

		DoMethod(obj, MM_Iconview_ClearBubble); /* intuition sucks. make sure it's cleared */

#if 0
		if ( 0 && msg->o )
		{
			px = _left( msg->o );
			py = _top( msg->o ) + getv( msg->o, MA_Icon_ImageNormalHeight );
		}
		else
#endif
		{
			px = _window( obj )->MouseX + 10;
			py = _window( obj )->MouseY + 10;
		}

		data->bubbleobj = (APTR)DoMethod(obj, MUIM_CreateBubble,
			px, py,	msg->txt,0
			//MUIV_CreateBubble_DontHidePointer
		);
	}
	return (0);
}

DEFSMETHOD(Notify_Change)
{
	if (msg->ctx)
	{
		GETDATA;
		struct notifyact *na;
		ULONG save_shortcuts;

		save_shortcuts = FALSE;

		while ( (na = notify_action_get(msg->ctx)) )
		{
			switch (na->action)
			{
				case NOTIFYTAG_Monitor_Device_Name:
					{
						ULONG len = 0;
						TEXT uri[PATH_SIZE];

						struct notifyact_device_name *naf = (struct notifyact_device_name *)na;

						ASSERT(data->show_devices);

						stccpy(uri, naf->uri, sizeof(uri));
						len = strlen(uri);
						if (len && uri[len - 1] == ':')
							uri[len - 1] = '\0';

						FORCHILD(obj, MUIA_Group_ChildList)
						{
							if (!stricmp(uri, (STRPTR)getv(child, MA_Icon_Name)))
							{
								set(child, MA_Icon_Path, naf->newname);
								DoMethod(obj, MM_Iconview_DoLayout);
								break;
							}
						}
						NEXTCHILD
					}
					break;

				case NOTIFYTAG_Monitor_Device_UnMount:
					ASSERT(data->show_devices);
					DoMethod(obj, MM_Iconview_Redraw);
					break;

				case NOTIFYTAG_Monitor_File_Name:
					{
						STRPTR p;
						APTR truncation, truncation2;
						struct notifyact_file_name *naf = (struct notifyact_file_name *)na;

						//ASSERT(!data->isroot);

						p = FilePart(naf->uri);

						truncation  = name_truncateinfo(p);
						truncation2 = name_truncateinfo(naf->newname);

						FORCHILD(obj, MUIA_Group_ChildList)
						{
							/* Just check icon name part first, but then, make sure paths are corresponding for rootview to avoid renaming a shortcut
							 * that would have the same name as the renamed file in a child view.
							 * Problem : is_path_equal should be called from thread, and a simple stricmp on fullpaths may fail with assigns & co
							 * If you find a better solution, go ahead. (fab)
							 */
							if (!stricmp(p, (STRPTR)getv(child, MA_Icon_Name)) && (!(data->isroot || data->show_devices) || is_path_equal(naf->uri, (STRPTR) getv(child, MA_Icon_Path)) ) )
							{
								if (data->isroot && getv(child, MA_Icon_IsShortcut))
									save_shortcuts = TRUE;

								SetAttrs(child, MA_Icon_Path, naf->newname, MA_Icon_MimeType, NULL, TAG_DONE);
								DoMethod(obj, MM_Iconview_DoLayout);
								break;
							}
						}
						NEXTCHILD

						name_restoreinfo(p, truncation);
						name_restoreinfo(naf->newname, truncation2);
					}
					break;

				case NOTIFYTAG_Monitor_File_Create:
					{
						struct notifyact_file_create *naf = (struct notifyact_file_create *)na;

						if (data->isroot)
						{
							/*
							 * This is a case for disk.info file replacement (from information window), so only act when original is there.
							 */
							
							STRPTR p;

							p = naf->uri;

							FORCHILD(obj, MUIA_Group_ChildList)
							{
								if (!strnicmp(p, (STRPTR)getv(child, MA_Icon_Name), strlen((STRPTR)getv(child, MA_Icon_Name))))
								{
									do_action(obj, TA_Icon_Read,
										TT_Icon_Read_Path, naf->uri,
										TT_Icon_Read_NoInfo, TRUE,
									TAG_DONE);
							
									break;
								}
							}
							NEXTCHILD
						}
						else if (data->viewmode == IVM_SHOWALL || data->viewmode == IVM_THUMBS || name_isinfo(naf->uri))
						{
							/*
							 * Check if file is not disk.info. It happens when window is snapshotted.
							 */

							if ( stricmp( FilePart(naf->uri), "disk.info" ) )
							{
								do_action(obj, TA_Icon_Read,
									TT_Icon_Read_Path, naf->uri,
									TT_Icon_Read_NoInfo, TRUE,
								TAG_DONE);
							}
						}
					}
					break;

				case NOTIFYTAG_Monitor_File_Icon:
					{
						struct notifyact_file_create *naf = (struct notifyact_file_create *)na;

						ASSERT(!data->isroot);

						if (data->viewmode == IVM_SHOWALL || name_isinfo(naf->uri))
						{
							/*
							 * Check if file is not disk.info. It happens when window is snapshotted.
							 */

							if ( stricmp( FilePart(naf->uri), "disk.info" ) )
							{
								/* XXX: Try smarter way to handle it. Now it just readds icon to view */

								ASSERT(!data->isroot);
								DoMethod(obj, MM_Iconview_RemoveByName, FilePart(naf->uri));

								do_action(obj, TA_Icon_Read,
									TT_Icon_Read_Path, naf->uri,
									//TT_Icon_Read_NoInfo, TRUE,
								TAG_DONE);
							}
						}
					}
					break;

				case NOTIFYTAG_Monitor_File_Delete:
					{
						struct notifyact_file_delete *naf = (struct notifyact_file_delete *)na;

						ASSERT(!data->isroot);
						DoMethod(obj, MM_Iconview_RemoveByName, FilePart(naf->uri));
					}
					break;
			}
		}

		if (save_shortcuts)
		{
			DoMethod(obj, MM_Iconview_SaveShortcuts);
		}
	}
	return (0);
}


DEFTMETHOD(Iconview_Relayout)
{
	if (muiRenderInfo(obj))
	{
		GETDATA;
		initializenoflicker( obj, data );

		DoMethod(obj, MUIM_Group_InitChange);
		DoMethod(obj, MM_Iconview_ClearLayout);
		DoMethod(obj, MM_Iconview_DoLayout);
		DoMethod(obj, MUIM_Group_ExitChange2, TRUE);

		finalizenoflicker( obj, data );

	}
	return (0);
}

DEFTMETHOD(Iconview_UpdateDriveInfo)
{
	GETDATA;
	LONG initial = FALSE;

	/*
	 * This method is called both by driveinfo update thread and by iconview.
	 * Second case happens on initial display and when configuration changes.
	 * Here we can launch the thread or stop it. When configuration doesn't change
	 * it has to update driver info generated by thread.
	 */

	if (data->isroot == FALSE && data->show_devices == FALSE)
	{
		return 0;
	}

	initializenoflicker(obj, data);

	if (data->driveinfo != _conf(misc_driveinfo))
	{
		/* stop the thread if needed. only one is needed and root view is responsible for it */

		if (data->isroot)
		{
			if (_conf(misc_driveinfo) == 0)
                threads_abort(obj, TA_Devices_UpdateInfo, NULL);
			else if (data->driveinfo == 0 && _conf(misc_driveinfo))
				do_action(obj, TA_Devices_UpdateInfo, TT_Priority, -1, TAG_DONE);
		}

		data->driveinfo = _conf(misc_driveinfo);

		/*
		 * Remove existing text or gauge (all == 3).
		 */

		if (data->driveinfo != 3)
		{
			FORCHILD(obj, MUIA_Group_ChildList)
			{
				if (data->driveinfo == 0 || data->driveinfo == 1) /* text only */
					SetAttrs(child, MA_Icon_GaugePercent, -1, MA_Icon_AdditionalText, NULL, TAG_DONE);

				if (data->driveinfo == 0 || data->driveinfo == 2) /* gauge only */
				{
					SetAttrs(child, MA_Icon_AdditionalText, NULL, TAG_DONE);
					DoMethod(child, MM_Icon_UpdateBitMapBuffer);
					DoMethod(child, MM_Icon_Generate, TRUE);
					DoMethod(obj, MM_View_RedrawEntry, child);
				}
			}
			NEXTCHILD

			DoMethod(obj, MUIM_Group_InitChange);
			DoMethod(obj, MM_Iconview_ClearLayout);
			DoMethod(obj, MM_Iconview_DoLayout);
			DoMethod(obj, MUIM_Group_ExitChange2, TRUE);
		}
		else
		{
			initial = TRUE;
		}
	}

	if (data->driveinfo)
	{
		FORCHILD(obj, MUIA_Group_ChildList)
		{
			if (getv(child, MA_Icon_Type) == MV_Icon_Type_Disk)
			{
				STRPTR path = (STRPTR)getv(child, MA_Icon_Path);
				struct dlcnode *dlcn = doslistcache_find_dlcdevice_by_volumename(path);
				ULONG updated = FALSE;

				if (dlcn && dlcn->size > 0 && (data->driveinfo == 1 || data->driveinfo == 3)) /* text or text&gauge */
				{
					STRPTR oldname = (STRPTR)getv(child, MA_Icon_AdditionalText);

					TEXT size[ 16 ];
					TEXT free[ 16 ];
					TEXT name[ 64 ];

					capacity_format_size_compact(size, sizeof(size), dlcn->size);
					capacity_format_size_compact(free, sizeof(free), dlcn->free);
					snprintf(name, sizeof(name), GSI(MSG_DEVICES_FREE), size, free);

					if (!oldname)
					{
						updated = TRUE;
						initial = TRUE;
					}
					else if (strcmp(oldname, name) != 0)
					{
						updated = TRUE;
					}

					if (updated){
						nnset(child, MA_Icon_AdditionalText, name);
					}
				}
				if (dlcn && dlcn->size > 0 && (data->driveinfo == 2 || data->driveinfo == 3)) /* gauge or text&gauge */
				{
					LONG percent;
					LONG oldpercent = getv(child, MA_Icon_GaugePercent);
					UQUAD used = dlcn->size - dlcn->free;

					percent = ((float)used / dlcn->size) * 100;

					if (oldpercent == -1)
						updated = TRUE;
					else if (oldpercent != percent)
						updated = TRUE;

					if (updated)
						nnset(child, MA_Icon_GaugePercent, percent);
				}

				/* generate new image. should be done selectively to just update text */

				if (updated)
				{
					DoMethod( child, MM_Icon_Generate, TRUE );
					DoMethod( child, MM_Icon_UpdateBitMapBuffer );
					DoMethod( obj, MM_View_RedrawEntry, child );
				}
			}
		}
		NEXTCHILD

        if (initial)
		{
			DoMethod(obj, MM_Iconview_Relayout);
			initial = FALSE;
		}

	}
	finalizenoflicker( obj, data );
	return (0);
}

static LONG CheckVisible( struct Data *data UNUSED, APTR group, APTR obj )
{
	/* check constraints for object and group */

	int nTop = getv( obj, MUIA_TopEdge ) - getv( group, MUIA_TopEdge );
	int nBottom = nTop + getv( obj, MUIA_Height );
	int gBottom = getv( group, MUIA_Height ) - 5;    /* 5 is a margin */
	int gTop = 5;

	if  ( nBottom > gBottom )
	{
		/* bottom is invisible */
		return  nBottom - gBottom;
	}

	if  ( nTop < gTop )
	{
		/* top is invisible */
		return  nTop - gTop;
	}

	return  0;
}


DEFSMETHOD(Iconview_FocusIcon)
{
	GETDATA;

	APTR selected = NULL;

	if ( data->isroot )
		return (0);

	/* Check if anything was previously selected */

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		if ( getv( child, MA_Icon_Selected ) )
		{
			selected = child;
			break;
		}
	}
	NEXTCHILD;

	/* Make sure object is visible */

	if  ( !selected )
	{
		/* nothing was selected. check if new one is invisible */

		LONG diff = CheckVisible( data, obj, msg->obj );

		if  ( diff )
			set( obj, MUIA_Virtgroup_Top, getv( obj, MUIA_Virtgroup_Top ) + diff );

	}
	else
	{
		/* something was selected previously. check if we have to scroll the list to make new node visible */

		LONG diff = CheckVisible( data, obj, msg->obj );

		if  ( diff )
			set( obj, MUIA_Virtgroup_Top, getv( obj, MUIA_Virtgroup_Top ) + diff );
	}

	return (0);
}

DEFMMETHOD(GoActive)
{
	GETDATA;

	data->is_active = 1;
	set(_win(obj), MUIA_Window_DisableKeys, (1 << MUIKEY_GADGET_NEXT) | (1 << MUIKEY_GADGET_PREV));
	return (0);
}


DEFMMETHOD(GoInactive)
{
	GETDATA;
	DOSUPER;

	set(_win(obj), MUIA_Window_DisableKeys, 0);
	data->is_active = 0;
	return (0);
}

DEFSMETHOD(View_GetSelectionList)
{
	FORCHILD(obj, MUIA_Group_ChildList)
	{
		if (getv(child, MA_Icon_Selected))
		{
			struct dragdropnode *ddn;
			STRPTR p = (STRPTR)getv(child, MA_Icon_Path);

			if ( (ddn = malloc(sizeof(*ddn) + strlen(p) + 1)) )
			{
				strcpy(ddn->path, p);

				ddn->type = getv(child, MA_Icon_FileType);
				ddn->x = 0;
				ddn->y = 0;

				ADDTAIL(msg->list, ddn);
			}
		}
	}
	NEXTCHILD

	return (0);
}

struct name_node
{
	struct MinNode n;
	TEXT name[0];
};

DEFSMETHOD(View_GetSelectedNamesList)
{
	struct MinList *list = malloc( sizeof( *list ) );

	if ( list )
	{
		NEWLIST( list );

		FORCHILD(obj, MUIA_Group_ChildList)
		{
			if (getv(child, MA_Icon_Selected))
			{
				struct name_node *nn;
				STRPTR p = (STRPTR)getv(child, MA_Icon_Path);
				ULONG len = strlen( p ) + 1;

				if ( (nn = malloc(sizeof(*nn) + len)) )
				{
					memcpy(nn->name, p, len);

					ADDTAIL(list, nn);
				}
			}
		}
		NEXTCHILD
	}

	*msg->listptr = list;

	return list ? TRUE : FALSE;
}

DEFSMETHOD(View_GetEntry)
{
	GETDATA;
	ULONG cnt = 0;

	if ( msg->pos >= data->objcount )
		return (ULONG)NULL;

	if ( objectarray_check( obj, data ) )
	{
		return (ULONG)data->objectarray[ msg->pos ];
	}

	/* fallback mode */

	FORCHILD(obj, MUIA_Group_ChildList)
	{
		if ( cnt == msg->pos )
			return (ULONG)child;

		cnt++;
	}
	NEXTCHILD

	return (ULONG)NULL;
}

DEFSMETHOD(View_Focus)
{
	GETDATA;

	FORCHILD( obj, MUIA_Group_ChildList )
	{
		set(child, MA_Icon_Selected, FALSE);

		if ( strcmp( (STRPTR)getv( child, MA_Icon_Name ), msg->name ) == 0)
		{
			DoMethod( obj, MM_Iconview_FocusIcon, child );

			if ( data->lastobject )
				set(data->lastobject, MA_Icon_Highlighted, FALSE);

			data->lastobject = child;
			set(data->lastobject, MA_Icon_Selected, TRUE);
			set(data->lastobject, MA_Icon_Highlighted, TRUE);

			return 1;
		}
	}
	NEXTCHILD

	return 0;
}

DEFSMETHOD(View_CheckFocus)
{
	/* check constraints for object and group */

	APTR icon = msg->entry;

	struct IconData *idata = (struct IconData *)muiUserData( icon );

	/* use that as TopEdge is 16bit only! */
	LONG top = idata->y - (LONG)_addtop( obj ) - getv( obj, MUIA_Virtgroup_Top ) + _top( obj );
	LONG height = getv( icon, MUIA_Height );

	{

		/* check constraints for object and group */

		LONG nTop = top - _top( obj );
		LONG nBottom = nTop + height;
		LONG gBottom = _height( obj ) - 5;	  /* 5 is a margin */
		LONG gTop = 5;

		if	( nBottom > gBottom )
		{
			/* bottom is invisible */
			return	FALSE;
		}

		if	( nTop < gTop )
		{
			/* top is invisible */
			return	FALSE;
		}
	}

	return TRUE;
}

DEFSMETHOD(View_QueryDisplayArea)
{

	struct Rect32 *r = msg->rect;

	r->MinX = 0;
	r->MaxX = _width( obj );
	r->MinY = getv( obj, MUIA_Virtgroup_Top );
	r->MaxY = r->MinY + getv( obj, MUIA_Height );

	return (ULONG)r;

}

DEFTMETHOD(Iconview_StartScanner)
{
	GETDATA;

	if ( data->isroot )
		return 0;

	if ( data->typescanner_running )
	{
		/*
		 * We will try again after it finishes.
		 */

		return FALSE;
	}

	/*
	 * Clear unscanned entries flag and mark as running
	 */

	data->numunscanned = 0;

	/*
	 * We alloc temp list and just move nodes to it.
	 */

	{
		struct MinList *ml;
		ULONG gen_deficons = TRUE;
		ULONG gen_thumbs = data->viewmode == IVM_THUMBS;

		if ( (ml = malloc(sizeof(*ml))) )
		{
			STRPTR p;
			LONG items = 0;

			NEWLIST(ml);

			FORCHILD(obj, MUIA_Group_ChildList)
			{
				if ( ( gen_thumbs || getv(child, MA_Icon_Refine) ) && getv(child, MA_Icon_Type ) != MV_Icon_Type_Drawer )
				{
					struct typescannernode *tn;

					p = (STRPTR)getv(child, MA_Icon_Path);

					if( (tn = malloc(sizeof(*tn) + strlen(p) + 1)) )
					{
						struct IconData *idata = (struct IconData *)muiUserData(child);

						tn->obj = child;
						strcpy(tn->path, p);
						ATOMIC_STORE(&idata->locked, TRUE);
						ADDTAIL(ml, tn);
						items++;
					}
					/* XXX - harmless, but still */
				}
			}
			NEXTCHILD

			if ( items )
			{
				TEXT wintitle[PATH_SIZE];
				TEXT resolved_path[PATH_SIZE];
				
				vfs_resolve_path((STRPTR)getv(obj, MA_View_Path), resolved_path, sizeof(resolved_path), FALSE);
				name_build_wintitle(wintitle, sizeof(wintitle), resolved_path);

				// Localise me !
#define CREATETHUB_TEXT " - Generating Thumbnails..."

				if ( gen_thumbs )
					strncat(wintitle, CREATETHUB_TEXT, PATH_SIZE);

				DoSuperMethod(cl, obj, MM_View_SetStatus, MF_View_SetStatus_Window | MF_View_SetStatus_Bar | MF_View_SetStatus_Busy, "%s", wintitle);

				if (!do_action(obj, TA_MimeType_Scan,
					TT_MimeType_Scan_List, ml,
					TT_MimeType_Scan_GenerateThumbs, gen_thumbs,
					TT_MimeType_Scan_GenerateDefIcons, gen_deficons,
					TT_Priority, -2,
				TAG_DONE))
				{
					struct typescannernode *nexttn, *tn;

					ITERATELISTSAFE(tn, nexttn, ml)
					{
						struct IconData *idata = (struct IconData *)muiUserData(tn->obj);
						ATOMIC_STORE(&idata->locked, FALSE);
						free(tn);
					}
					free( ml );
				}
				else
				{
					data->typescanner_running = TRUE;
				}
			}
			else
			{
				free( ml );
			}
		}
		/* XXX */

	}

	return 0;

}

DEFSMETHOD(Iconview_ShowPreview)
{
#if USE_AVCODEC
	GETDATA;
	LONG percent = -1;

	if(!_aprefs(videopreview) || data->viewmode != IVM_THUMBS)
	{
		return 0;
	}

	if(msg->enable == FALSE)
	{
		data->videopreview_abort = TRUE;
	}
	else
	{
		if(data->videopreview_running)
		{
			data->videopreview_restart = TRUE;
			threads_abort(obj, TA_Iconview_ShowPreview, NULL);
		}
		else
		{
			data->videopreview_iconobj = data->lastobject;

			if(is_icon_object(data->videopreview_iconobj) &&
			   !getv(data->videopreview_iconobj , MA_Icon_Selected) &&
			   video_validate((STRPTR) getv(data->videopreview_iconobj , MA_Icon_Path), (APTR) getv(data->videopreview_iconobj, MA_Icon_MimeType), &percent))
			{
				/*
				 * Ensure that the iconobj cannot be disposed while
				 * the videopreview thread is working on it. - Piru
				 */
				//kprintf("%s: OM_RETAIN videopreview_iconobj %p\n", __func__, data->videopreview_iconobj);
				DoMethod(data->videopreview_iconobj, OM_RETAIN);

				if(do_action(obj, TA_Iconview_ShowPreview,
					TT_Iconview_ShowPreview_Object, data->videopreview_iconobj,
				TAG_DONE))
				{
					data->videopreview_running = TRUE;
				}
				else
				{
					DoMethod(data->videopreview_iconobj, OM_RELEASE);
				}
			}
		}
	}
#endif
	return 0;
}

ULONG tr_iconview_showpreview(APTR obj, APTR entry)
{
	ULONG rc = TRUE;

#if USE_AVCODEC
	
	ULONG abort = FALSE;
	STRPTR name = NULL;

	struct timerequest * timer;
	struct timeval tv;
	struct timeval tv_start;
	struct timeval tv_end;
	ULONG  elapsed = 0;
	double framerate = 24.0;
	APTR winsave = _win(obj);
	struct Data * data = INST_DATA(OCLASS(obj), obj);

	data->videopreview_abort = FALSE;

	THREAD;
	CHECKOBJECT(obj);

	timer = timer_create(UNIT_MICROHZ, 0);

	if(timer)
	{
		methodstack_push_sync(entry, 3, OM_GET, MA_Icon_Path, &name);

		if(name)
		{
			ULONG width, height;

			methodstack_push(entry, 3, OM_GET, MA_Icon_ImageNormalWidth, &width );
			methodstack_push_sync(entry, 3, OM_GET, MA_Icon_ImageNormalHeight, &height );

			if(ThumbnailsBase &&
			   ((ThumbnailsBase->lib_Version == 50 && ThumbnailsBase->lib_Revision >= 5) || ThumbnailsBase->lib_Version > 50) )
			{
				struct TagItem tags[] = {{THB_Width, width},
										 {THB_Height, height},
										 {THB_Format, THB_FORMAT_RGB888},
										 //{THB_ResizeFilter, THB_FILTER_QUALITY},
										 {THB_Animated, TRUE},
										 {TAG_END, 0}};

				APTR thumb = ThbGenerateThumbnailA(name, tags);

				if(thumb)
				{
					ULONG animated;
					ULONG width, height;
					UBYTE *bdata;
					struct TagItem init_attrs[] = {{THB_Width, (ULONG)&width},
												    {THB_Height, (ULONG)&height},
												    {THB_IsAnimated, (ULONG)&animated},
												    {THB_FramesPerSecond, (ULONG)&framerate},
												    {TAG_END, 0}};

					ThbGetAttrsA(thumb, init_attrs);

					if(animated)
					{
						/*SDB(("Playing %s, width = %ld height = %ld framerate = %f\n", name, width, height, framerate));*/

						while(!abort)
						{
							if(threads_check_abort() || data->videopreview_abort)
							{
								abort = TRUE;
								rc = ABORTED;
							}

							if(!abort)
							{
								struct TagItem frame_attrs[] = { {THB_Data, (ULONG)&bdata},
								                                 {THB_Width, (ULONG)&width},
													             {THB_Height, (ULONG)&height},
								                                 {TAG_END, 0} };
								ULONG win_opened;

								methodstack_push_sync(winsave, 3, OM_GET, MUIA_Window_Open, &win_opened);

								getlocaltime(&tv_start);

								if(win_opened)
								{
									ThbGetAttrsA(thumb, frame_attrs);

									if(bdata)
									{
										APTR tbm;

										if ( (tbm = gfx_bitmap_create(width, height, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
										{
											gfx_blit(bdata, tbm,
									            BLITTAG_SrcType, BLITVAL_SrcType_Array,
									            BLITTAG_SrcFormat, BLITVAL_SrcFormat_RGB,
											    BLITTAG_DstWidth, width,
											    BLITTAG_DstHeight, height,
												BLITTAG_Modulo, width * 3,
									            TAG_DONE );

											methodstack_push(entry, 4,
																MM_Icon_AddBitMap,
																tbm,
																MV_Icon_BitMap_Thumbicon,
																MV_Icon_BitMap_Normal
															);

											methodstack_push(entry, 1,
												MM_Icon_UpdateBitMapBuffer
											);

											methodstack_push(entry, 2,
												MM_Icon_Generate, TRUE
											);

											methodstack_push_sync(obj, 2, MM_View_RedrawEntry, entry);

											getlocaltime(&tv_end);

											elapsed = (tv_end.tv_secs - tv_start.tv_secs)*1000000 + (tv_end.tv_micro - tv_start.tv_micro);

											if(elapsed < 1000000/framerate)
											{
												tv.tv_secs  = 0;
												tv.tv_micro = 1000000/framerate - elapsed;
												timer_addreq_sync(timer, &tv);
											}
										}
									}
									ThbNextFrame(thumb);
								}
								else
								{
									tv.tv_secs  = 0;
									tv.tv_micro = 100000;
									timer_addreq_sync(timer, &tv);
								}
							}
						}
					}

					ThbDeleteThumbnail(thumb);
				}
			}
		}

		timer_delete(timer);
	}

	/*
	 * We no longer access the iconobj in question, so let it
	 * to be disposed. - Piru
	 */
	//kprintf("%s: OM_RELEASE entry %p\n", __func__, entry);
	methodstack_push(entry, 1, OM_RELEASE);

#endif
	return (rc);
}

BEGINMTABLE
DECNEW
DECDISP
DECSET
DECGET
DECADDMEMBER
DECREMMEMBER
DECMMETHOD(Hide)
DECMMETHOD(Show)
DECSMETHOD(Iconview_AddIcon)
DECSMETHOD(Iconview_DeleteIcon)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECTMETHOD(View_Setup)
DECTMETHOD(Iconview_AbortLasso)
DECMMETHOD(HandleEvent)
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
DECMMETHOD(CreateDragImage)
DECMMETHOD(DeleteDragImage)
DECMMETHOD(DragQuery)
DECMMETHOD(DragBegin)
DECMMETHOD(DragDrop)
DECMMETHOD(DragFinish)
DECMMETHOD(DragEvent)
DECSMETHOD(View_Select)
DECSMETHOD(Iconview_SelectLasso)
DECSMETHOD(Iconview_Unselect)
DECMMETHOD(Backfill)
DECTMETHOD(Iconview_RefreshLasso)
DECTMETHOD(Iconview_Redraw)
DECTMETHOD(View_RedrawEntry)
DECTMETHOD(Iconview_AllocPens)
DECTMETHOD(Iconview_Empty)
DECSMETHOD(Iconview_DeleteAppIcon)
DECSMETHOD(Iconview_RemoveByName)
DECTMETHOD(Iconview_RemoveDefaults)
DECSMETHOD(Iconview_BuildSnapshotlist)
DECSMETHOD(Rexx_Snapshot)
DECSMETHOD(Rexx_Unsnapshot)
DECSMETHOD(Iconview_SnapshotIcon)
DECSMETHOD(Iconview_UnsnapshotIcon)
DECSMETHOD(Iconview_DoSort)
DECTMETHOD(Iconview_SaveShortcuts)
DECTMETHOD(Iconview_UpdateDriveInfo)
DECSMETHOD(Iconview_FocusIcon)
DECSMETHOD(View_Focus)
#if USE_SHORTCUTS
DECSMETHOD(Iconview_SaveShortcuts2)
#endif
DECSMETHOD(Iconview_AddShortcut)
DECSMETHOD(Iconview_FindShortcut)
DECSMETHOD(Thread_Finished)
DECTMETHOD(View_LoadURI)
DECTMETHOD(Iconview_DoLayout)
DECTMETHOD(Iconview_ClearLayout)
#if USE_VIRTGROUP_CORRECT
DECMMETHOD(Virtgroup_Correct)
#endif
DECSMETHOD(View_Refresh)
DECTMETHOD(Iconview_ClearBubble)
DECSMETHOD(Metadata_Info)
DECSMETHOD(Notify_Change)
DECMMETHOD(GoActive)
DECMMETHOD(GoInactive)
DECSMETHOD(View_GetSelectionList)
DECSMETHOD(View_GetSelectedNamesList)
DECSMETHOD(View_GetEntry)
DECSMETHOD(View_CheckFocus)
DECSMETHOD(View_QueryDisplayArea)
DECTMETHOD(Iconview_StartScanner)
DECTMETHOD(Iconview_Relayout)
DECSMETHOD(Iconview_ShowPreview)


ENDMTABLE

DECSUBCLASSPTR_NC(viewclass, iconviewclass)
