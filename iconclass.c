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
 * $Id: iconclass.c,v 1.60 2023/01/12 20:33:12 jacadcaps Exp $
 */

#include "ambient.h"

/* public */
#include <proto/exec.h>
#include <cybergraphx/cybergraphics.h>
#include <workbench/workbench.h>
#include <graphics/rpattr.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <stddef.h> /* for offsetof() */

#include <proto/vgraphics.h>
#include <clib/debug_protos.h>

/* private */
#include "dragdrop.h"
#include "deficon_getpath.h"
#include "mui_func.h"
#include "iconmem.h"
#include "time_func.h"
#include "fonts.h"
#include "wbstart.h"
#include "screen.h"
#include "path.h"
#include "contextmenu.h"
#include "command.h"
#include "rexx.h" /* XXX: this is wrong.. or not */
#include "tooltypelist.h"
#include "prefs.h"
#include "prefs_desktop.h"
#include "gfx_mask.h"
#include "gfx_alpha.h"
#include "gfx_pen.h"
#include "gfx_scale.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "threads.h"
#include "file_func.h"
#include "iconchunk.h"
#include "textbox.h"
#include "gfx_dbuf.h"
#include "wbarg.h"
#include "datatypes_picture.h"
#include "movelist.h"
#include "shortcut_logo.h"
#include "vars.h"
#include "pointer.h"
#include "icondata.h"
#include "name.h"
#include "pngio.h"
#include "viewapi.h"
#include "spacegauge.h"
#include "doslistcache.h"
#include "str.h"
#include "mimetype.h"
#include "actiondispatcherclass.h"
#include "action.h"
#include "methodstack.h"
#include "prefs_advanced.h"
#include "legacy.h"
#include "mimeuri.h"
#include "typescanner.h"
#include "appicon_logo.h"
#include "trashcan.h"
#include "networksfs.h"

#include "sxmlc.h"

/* XXX: those will be configurable */
#define MAXICONSIZE_X 64
#define MAXICONSIZE_Y 128

#define CONF_MAXSIZE (_conf(icon_maxsize) + data->sizeadjustment)
#define CONF_MINSIZE (_conf(icon_minsize) + data->sizeadjustment)

#define INFOWIN_MAXICONSIZE 128


enum {
	EDITMODE_NONE,    /* nothing */
	EDITMODE_STRING,  /* user is entering a new name */
	EDITMODE_IO,      /* a new name has been dispatched to the FS but is not done yet */
};

enum {
	RFR_NONE,
	RFR_UPDATE,
	RFR_RECALC,
};

enum {
	DIRTYFLAG_NAME = 1,
	DIRTYFLAG_LABEL = 2,
	DIRTYFLAG_IMAGE = 4,
};


/*
 *      Left    X
 * Top __|______|____________
 *      _|______|_____       |
 *     Y |      |     |      |
 *       |      |     |      |
 *       |      |Icon |      |
 *       |      |     |      |
 *       |      |     |      |
 *       |      |_____|      |
 *       |                   |
 *       |Blablablablablablaa|
 *       |___________________|
 *
 * X - Left = addx;
 * Y - Top = addy;
 *
 */

/* struct Data is in icondata.h */
#define Data IconData

static void delete_specialbm(APTR obj, struct IClass *cl, ULONG effect);

ULONG icon_flags_for_path(CONST_STRPTR path, ULONG type)
{
	if (is_networksfs(path))
	{
		return AS_NETWORKSFS;
	}
	else
	{
		switch (type)
		{
		case MV_Icon_Type_Disk:
		case MV_Icon_Type_Device:
			return;
		default:
			if (is_trashcan(path))
				return AS_INTRASHCAN;
			else if (trashcan_is_running())
				return AS_HASTRASHCAN;
			break;
		}
	}
	return 0;
}

/* function which returns shared dbuf bitmap to which icon can render. (XXX:Add semaphore protection? shouldn't be needed) */

static LONG icon_count = 0;
static struct doublebuf *icon_dbuf = NULL;

static struct doublebuf *icon_alloc_dbuf( APTR obj UNUSED, struct Data *data UNUSED, ULONG width, ULONG height )
{
	struct Screen *scr = get_screen();
	struct BitMap *fri = scr ? scr->RastPort.BitMap : NULL;
	#if USE_LEGACY
	ULONG flags = DBUF_CLIPPED;
	#else
	ULONG flags = DBUF_DISPLAYABLE | DBUF_CLIPPED;
	#endif

	//kprintf("alloc dbuf. oldone:0x%x:0x%x\n", icon_dbuf, fri);

	//if (icon_dbuf != NULL && fri != NULL && icon_dbuf->pixfmt == GetCyberMapAttr(fri, CYBRMATTR_PIXFMT)){
	//	  kprintf("foo1\n");
	//}

	//kprintf("bar\n");

	if ( width == 0 || height == 0 )
		return NULL;

	if (icon_dbuf != NULL && fri != NULL && icon_dbuf->pixfmt == GetCyberMapAttr(fri, CYBRMATTR_PIXFMT) && gfx_bitmap_width(icon_dbuf->bm) >= width && gfx_bitmap_height(icon_dbuf->bm) >= height && icon_dbuf->flags == flags)
	{
		//kprintf("return old one\n");
		return (icon_dbuf);
	}
	else
	{
		ULONG old_width = 0, old_height = 0;

		if (icon_dbuf)
		{
			//kprintf("return new one\n");

			D(ICONIO, bug("Realloc shared dbuffer (%d,%d->%d,%d)!\n", gfx_bitmap_width(icon_dbuf->bm),gfx_bitmap_height(icon_dbuf->bm),max( width, gfx_bitmap_width(icon_dbuf->bm) ), max( height, gfx_bitmap_height(icon_dbuf->bm ) )));

			old_width = gfx_bitmap_width(icon_dbuf->bm);
			old_height = gfx_bitmap_height(icon_dbuf->bm);

			gfx_dbuf_free(icon_dbuf);
		}

		if (fri == NULL)
			flags &= ~DBUF_DISPLAYABLE;

		//kprintf("alloc new one\n");
		icon_dbuf = gfx_dbuf_alloc( max( width, old_width ), max( height, old_height ), flags, fri);
		//kprintf("new one\n");
	}

	return icon_dbuf;
}

static void icon_free_dbuf( void )
{
	/* when we want displayable doublebuffer but it was allocated as not, then dispose it here no matter what */

	#if USE_LEGACY
	if ( icon_dbuf && icon_count < 10 )
	#else
	if ( (icon_dbuf && icon_count < 10) || (icon_dbuf && (icon_dbuf->flags & DBUF_DISPLAYABLE) == 0))
	#endif
	{
		/* no need to store it */
		//kprintf("free dbuf because iconcount:%d\n", icon_count);
		D(ICONIO, bug("Free shared dbuffer\n"));
		gfx_dbuf_free( icon_dbuf );
		icon_dbuf = NULL;
	}
}

/* XXX: This function should probably give a return value. */
static void setname_mymorphos(APTR obj UNUSED, struct Data *data)
{
	TEXT pathinfo[PATH_SIZE];
	CONST_STRPTR name;
	ULONG len;

	if (!deficon_getpath(pathinfo, sizeof(pathinfo), "def_mymorphos.info"))
	{
		/* would overflow */
		return;
	}

	name = dprefs_mymorphos_name_get();

	if (data->path)
		free(data->path);

	len = strlen(pathinfo) + 1;

	if ((data->path = malloc(len - 5))) /* without .info */
	{
		stccpy(data->path, pathinfo, len - 5);
	}

	data->pathinfo = name_replace( data->pathinfo, pathinfo );
	data->iconname = name_replace( data->iconname, name );
}

static void doset(APTR obj, struct Data *data, struct TagItem *tags, ULONG fromset, struct IClass *cl)
{
	ULONG rfr = RFR_NONE;
	ULONG dirtyflag = 0;
	ULONG forward = FALSE;

	FORTAG(tags)
	{
		case MA_Icon_PathInfo:
			data->pathinfo = name_replace( data->pathinfo, (STRPTR)tag->ti_Data );
			break;

		case MA_Icon_Path:
		case MA_Icon_Name:
			if ( tag->ti_Tag != MA_Icon_Name )
			{
				STRPTR filename;
				ULONG l;
				STRPTR newname;

				data->path = name_replace( data->path, (STRPTR)tag->ti_Data );
				rfr = RFR_RECALC;
				dirtyflag |= DIRTYFLAG_LABEL;

				filename = FilePart(data->path);
				l = strlen(filename);

				/* we might not have type set yet, so guess filename/device */

				if ( l )
				{
					newname = name_build( filename );
				}
				else
				{
					newname = name_build( data->path );
					l = newname ? strlen( newname ) : 0;
				}

				if ( newname && isdevicename( newname ) )
					newname[ l - 1 ] = 0;

				if ( newname )
				{
					if ( data->iconname )
						name_delete( data->iconname );

					data->iconname = newname;
				}
			}
			else
			{
				data->iconname = name_replace( data->iconname, (STRPTR)tag->ti_Data );
			}
			break;

		case MA_Icon_AdditionalText:
			if ( tag->ti_Data )
				data->additionaltext = name_replace( data->additionaltext, (STRPTR)tag->ti_Data );
			else
			{
				if ( data->additionaltext )
					name_delete( data->additionaltext );

				data->additionaltext = NULL;
			}

			data->needsupdate = TRUE;
			dirtyflag |= DIRTYFLAG_LABEL;

			break;

		case MA_Icon_DefaultTool:
			if (data->defaulttool)
			{
				icon_free(data->defaulttool);
			}

			if ( (data->defaulttool = icon_malloc(strlen((STRPTR)tag->ti_Data) + 1)) )
			{
				strcpy(data->defaulttool, (STRPTR)tag->ti_Data);
			}
			/* XXX */
			break;

		case MA_Icon_Type:
			data->type = tag->ti_Data;

			if (tag->ti_Data == MV_Icon_Type_MyComputer)
			{
				setname_mymorphos(obj, data);
				dprefs_mymorphos_notify_setobj(obj);
				rfr |= RFR_RECALC;
				data->needsupdate = TRUE;
				dirtyflag |= DIRTYFLAG_LABEL;
			}
			break;

		case MA_Icon_FileType:
			data->filetype = tag->ti_Data;
			break;

		case MA_Icon_StackSize:
			data->stacksize = (tag->ti_Data >= M68K_STACKSIZE) ? tag->ti_Data : M68K_STACKSIZE;
			break;

		case MA_Icon_DiskType:
			data->disktype = tag->ti_Data;
			break;

		case MA_Icon_Y:
			data->y = (LONG)tag->ti_Data + POSY_CORRECTION;
			break;

		case MA_Icon_X:
			data->x = (LONG)tag->ti_Data + POSX_CORRECTION;
			break;

		case MA_Icon_HasPos:
			data->has_pos = tag->ti_Data;
			break;

		case MA_Icon_WindowTop:
			data->win_y = tag->ti_Data;
			break;

		case MA_Icon_WindowLeft:
			data->win_x = tag->ti_Data;
			break;

		case MA_Icon_WindowHeight:
			data->win_ys = tag->ti_Data;
			break;

		case MA_Icon_WindowWidth:
			data->win_xs = tag->ti_Data;
			break;

		case MA_Icon_OffsetY:
			data->offs_y = tag->ti_Data;
			break;

		case MA_Icon_OffsetX:
			data->offs_x = tag->ti_Data;
			break;

		case MA_Icon_ViewMode:
			data->viewmode = tag->ti_Data;
			break;

		case MA_Icon_SortMode:
			data->sortmode = tag->ti_Data;
			break;

		case MA_Icon_Selected:
			if (data->selected != tag->ti_Data)
			{
				APTR parent = _parent(obj);
				UQUAD selecteddiskusage;

				data->selected = tag->ti_Data;

				delete_specialbm(obj, cl, BMEFFECT_ALL);

				ASSERT(parent);

				selecteddiskusage = *(UQUAD*)getv(parent, MA_View_SelectedDiskUsage) + (data->selected ? data->filesize : -data->filesize);
				SetAttrs(parent, MUIA_Group_Forward, FALSE,
								 MA_View_NumSelected, data->selected ? MV_View_NumSelected_Increase : MV_View_NumSelected_Decrease,
								 MA_View_SelectedDiskUsage, &selecteddiskusage,
								 TAG_DONE);

				DoMethod(parent, MM_View_IconSelect, data->selected ? MV_View_IconSelect_Select : MV_View_IconSelect_Unselect, data->type, data->is_shortcut);

				rfr = RFR_UPDATE;
			}
			break;

		case MA_Icon_Highlighted:
			if ((data->editmode != EDITMODE_STRING) && data->highlighted != tag->ti_Data)
			{
				data->highlighted = tag->ti_Data;

				delete_specialbm(obj, cl, BMEFFECT_ALL);

				#if USE_POINTER_HOVERING
				ASSERT(_window(obj));
				if (data->highlighted)
				{
					pointer_set(_window(obj), POINTER_TARGET);
				}
				else
				{
					pointer_clear(_window(obj));
				}
				#endif
				rfr = RFR_UPDATE;
			}
			break;

		case MA_Icon_Left:
		case MA_Icon_Top:
			DB(("MA_Icon_Left/Top are not settable!\n"));
			break;

		case MA_Icon_TextColor:
			if ( data->text_color != tag->ti_Data )
			{
				data->text_color = tag->ti_Data;
				data->text_hasbg = FALSE;
				//rfr = RFR_RECALC;
				dirtyflag |= DIRTYFLAG_LABEL;
			}

			// kiero: yes, this totaly blows, but cached states need to be invalidated. we hijack this attribute
			if (data->reference_icon)
			{
				delete_specialbm(data->reference_icon, cl, BMEFFECT_ALL);
			}
			else
			{
				delete_specialbm(obj, cl, BMEFFECT_SELECTED);
				delete_specialbm(obj, cl, BMEFFECT_MOUSEOVER);
			}

			break;

		case MA_Icon_TextBgColor:
			/*
			 * XXX: This is to force refresh when setting conf items which are not passed as attributes.
			 * Do something with it in nice way.
			 */

			//if ( data->text_bgcolor != tag->ti_Data )	
			{
				data->text_bgcolor = tag->ti_Data;
				data->text_hasbg = TRUE;
				//rfr = RFR_RECALC;
				dirtyflag |= DIRTYFLAG_LABEL;
			}
			break;

		case MA_Icon_AFont:
			if ( data->afont != (struct atextfont *)tag->ti_Data )
			{
				data->afont = (struct atextfont *)tag->ti_Data;

				//rfr = RFR_RECALC;

				dirtyflag |= DIRTYFLAG_LABEL;
				forward = TRUE;
			}
			break;

		case MA_Icon_SmallFont:
			if ( data->smallfont != (struct atextfont *)tag->ti_Data )
			{
				data->smallfont = (struct atextfont *)tag->ti_Data;

				/* kiero: because of some unknown for now bug this is safest place to invalidate old textbox! */
				if (data->tbctx2 != NULL)
					textbox_delete(data->tbctx2);
				data->tbctx2 = NULL;

				//rfr = RFR_RECALC;
				dirtyflag |= DIRTYFLAG_LABEL;
				forward = TRUE;

			}
			break;

		case MA_Icon_FontSpace:
			if ( data->fontspace != tag->ti_Data )
			{
				data->fontspace = tag->ti_Data;
				//rfr = RFR_RECALC;
				dirtyflag |= DIRTYFLAG_LABEL;
			}
			break;

		case MA_Icon_MsgPort:
			data->msgport = (struct MsgPort *)tag->ti_Data;
			break;

		case MA_Icon_AppAddress:
			data->appaddress = (APTR)tag->ti_Data;
			break;

		case MA_Icon_AppID:
			data->appid = (ULONG)tag->ti_Data;
			break;

		case MA_Icon_AppUserData:
			data->appuserdata = (ULONG)tag->ti_Data;
			break;

		case MA_Icon_HasDrawerData:
			data->has_drawerdata = (ULONG)tag->ti_Data;
			break;

		case MA_Icon_IsDefault:
			data->isdefault = (ULONG)tag->ti_Data;
			data->viewmode = MV_Icon_ViewMode_IconAll;
			data->sortmode = MV_Icon_SortMode_Name;

			/*
				Here alpha value for icon is calculated. It also depends on
				"ghosty" setting. We ignore it here and take care for at icon
				display.
			*/

			break;

		case MA_Icon_Refine:
			data->refine = (ULONG)tag->ti_Data;
			break;

		case MA_Icon_ImmediateUpdate:
			data->immediate_update = tag->ti_Data;
			break;

		case MA_Icon_IsShortcut:
			data->is_shortcut = tag->ti_Data;
			break;

		case MA_Icon_Infowin:
			data->infowin = tag->ti_Data;
			break;

		case MA_Icon_IsLink:
			data->is_link = tag->ti_Data;
			break;

		case MA_Icon_Fade:
			if (tag->ti_Data && _conf(icon_fade))
			{
				data->alphaval = 0;
			}
			break;

		#if USE_THUMBS
		case MA_Icon_ThumbIt:
			data->thumbit = tag->ti_Data;
			break;
		#endif

		case MA_Icon_FileSize:
			data->filesize = *((UQUAD *)tag->ti_Data);
			break;

		case MA_Icon_FileDate:
			data->filedate = tag->ti_Data;
			break;

		case MA_Icon_GaugePercent:
			data->gauge_percent = tag->ti_Data;
			data->needsupdate = TRUE;
			break;

		case MA_Icon_MimeType:
			data->mimetype = (APTR)tag->ti_Data;
			break;

		case MA_Icon_IconPath:
			data->iconpath = name_replace( data->iconpath, (STRPTR)tag->ti_Data );
			break;

		case MA_Icon_DeviceType:
			data->devicetype = tag->ti_Data;
			break;

		case MA_Icon_ClickPosX:
			data->click_pos_x = tag->ti_Data;
			break;

		case MA_Icon_ClickPosY:
			data->click_pos_y = tag->ti_Data;
			break;

		case MA_Icon_Reference:
			if (data->reference_icon != NULL)
				set(data->reference_icon, MA_Icon_ReferenceCount, MV_Icon_ReferenceCount_Decrease);

			data->reference_icon = (APTR)tag->ti_Data;

			if (data->reference_icon != NULL)
				SetAttrs( data->reference_icon,
					MA_Icon_ReferenceCount, MV_Icon_ReferenceCount_Increase,
					MA_Icon_SizeAdjustment, data->sizeadjustment,
					TAG_DONE);

			break;

		case MA_Icon_ReferenceCount:
			if ( tag->ti_Data == MV_Icon_ReferenceCount_Increase )
				data->reference_count++;
			else if ( tag->ti_Data == MV_Icon_ReferenceCount_Decrease )
				data->reference_count--;
			else
				data->reference_count = tag->ti_Data;

			break;

		case MA_Icon_ViewID:
			data->viewid = tag->ti_Data;
			break;

		case MA_Icon_SizeAdjustment:
			if (data->sizeadjustment != (LONG)tag->ti_Data)
			{
				data->sizeadjustment = (LONG)tag->ti_Data;
				//rfr = RFR_RECALC;
				dirtyflag |= DIRTYFLAG_LABEL | DIRTYFLAG_IMAGE;
				data->needsupdate = TRUE;
				forward = TRUE;
			}
			break;


		default:
			break; // bitRocky: GCC5 complains that "break" was missing

	}
	NEXTTAG

	if ( data->reference_icon && forward )
	{
		DoMethod( data->reference_icon, OM_SET, tags );
	}

	data->dirtyflags |= dirtyflag;

	if (fromset)
	{
		switch (rfr)
		{
			case RFR_NONE:
				break;

			case RFR_UPDATE:
				MUI_Redraw(obj, MADF_DRAWUPDATE);
				break;

			case RFR_RECALC:
				/* XXX: hum.. that sucks when applied on multiple icons at once .. especially MM_Iconview_Redraw */
				if (!data->infowin && _parent(obj))
				{
					DoMethod(_parent(obj), MM_Iconview_ClearLayout);
				}

				DoMethod(obj, MM_Icon_UpdateBitMapBuffer); /* needed when eg. renaming */

				if (!data->infowin && _parent(obj))
				{
					DoMethod(_parent(obj), MM_Iconview_Redraw);
				}
				break;

			#ifdef DEBUG
			default:
				PDB(("unknown rfr %ld\n", rfr));
				break;
			#endif
		}
	}
}


DEFNEW
{
	struct Data *data;

	obj = DoSuperNew(cl, obj,
		MUIA_Weight, 0,
		InnerSpacing(0, 0),
		MUIA_ContextMenu, 1,
		MUIA_Dropable, TRUE,
		MUIA_FillArea, FALSE,
		MUIA_InputMode, MUIV_InputMode_RelVerify,
		TAG_MORE, INITTAGS
	);

	if (!obj)
	{
		return (IPTR)(NULL);
	}

	data = INST_DATA(cl, obj);

	muiUserData(obj) = (ULONG)data; /* pointer to instance data to speed up the layout hook */

	/*
	 * Default values.
	 */
	//data->magic = ICONCLASS_MAGIC;
	data->stacksize = M68K_STACKSIZE;
	data->win_x = _conf(window_default_left);    /*50;*/
	data->win_y = _conf(window_default_top);     /*50;*/
	data->win_xs = _conf(window_default_width);  /*400;*/
	data->win_ys = _conf(window_default_height); /*200;*/ /* original is 100 there */

	data->alphaval = 0xffffffff;
	data->gauge_percent = -1;

	data->tbctx = NULL;
	data->tbctx2 = NULL;
	data->additionaltext = NULL;

	data->imagetype = MV_Icon_ImageType_PNGicon; /* Defaults to PNG... */

	data->appaddress = NULL;
	data->pathinfo = NULL;
	data->iconpath = NULL;
	data->path = NULL;
	data->reference_icon = NULL;
	data->reference_count = 0;
	data->is_link = FALSE;
	data->droptarget = FALSE;
	data->sizeadjustment = 0;

	data->x = POSX_CORRECTION;
	data->y = POSY_CORRECTION;
	/* XXX: put a minimum width/height ? */
	data->viewmode = MV_Icon_ViewMode_Icon;
	data->sortmode = MV_Icon_SortMode_Name;

	NEWLIST(&data->ttlist);
	/* XXX: following 2 not needed if no infomode.. fix ? */
	NEWLIST(&data->ni1_tt);
	NEWLIST(&data->ni2_tt);
	NEWLIST(&data->glow_chunk);
	NEWLIST(&data->png_chunk);

	data->bm1 = data->bm2 = NULL;
	data->bm1_real = data->bm2_real = NULL;
	data->dirtyflags = 0;//DIRTYFLAG_LABEL | DIRTYFLAG_IMAGE;
	data->needsupdate = TRUE;

	data->viewid = MV_ViewID_Unknown;

	doset(obj, data, INITTAGS, FALSE, cl);

	icon_count++;

	return ((ULONG)obj);
}


DEFSET
{
	GETDATA;

	doset(obj, data, INITTAGS, TRUE, cl);

	return (DOSUPER);
}


DEFGET
{
	GETDATA;
	struct IconData *idata;

	if ( data->reference_icon )
		idata = (struct IconData *)muiUserData(data->reference_icon);
	else
		idata = data;

	switch (msg->opg_AttrID)
	{
		case MA_Icon_PathInfo:
			*msg->opg_Storage = (ULONG)data->pathinfo;
			return (TRUE);

		case MA_Icon_NameInfo:
			*msg->opg_Storage = (ULONG)FilePart(data->pathinfo);
			return (TRUE);

		case MA_Icon_IconPath:
			*msg->opg_Storage = data->iconpath ? (ULONG)data->iconpath : (ULONG)data->pathinfo;
			return (TRUE);

		case MA_Icon_Path:
			*msg->opg_Storage = (ULONG)data->path;
			return (TRUE);

		case MA_Icon_Name:
			*msg->opg_Storage = (ULONG)data->iconname;
			return (TRUE);

		case MA_Icon_AdditionalText:
			*msg->opg_Storage = (ULONG)data->additionaltext;
			return (TRUE);

		case MA_Icon_Type:
			*msg->opg_Storage = data->type;
			return (TRUE);

		case MA_Icon_FileType:
			*msg->opg_Storage = data->filetype;
			return (TRUE);

		case MA_Icon_StackSize:
			*msg->opg_Storage = data->stacksize;
			return (TRUE);

		case MA_Icon_DiskType:
			*msg->opg_Storage = data->disktype;
			return (TRUE);

		case MA_Icon_DefaultTool:
			*msg->opg_Storage = (data->type == MV_Icon_Type_Project || data->type == MV_Icon_Type_Disk || data->type == MV_Icon_Type_Device) ? (ULONG)data->defaulttool : (ULONG)NULL;
			return (TRUE);

		case MA_Icon_Y:
			*msg->opg_Storage = data->y - POSY_CORRECTION;
			return (TRUE);

		case MA_Icon_X:
			*msg->opg_Storage = data->x - POSX_CORRECTION;
			return (TRUE);

		case MA_Icon_HasPos:
			*msg->opg_Storage = data->has_pos;
			return (TRUE);

		case MA_Icon_WindowTop:
			*msg->opg_Storage = data->win_y;
			return (TRUE);

		case MA_Icon_WindowLeft:
			*msg->opg_Storage = data->win_x;
			return (TRUE);

		case MA_Icon_WindowWidth:
			*msg->opg_Storage = data->win_xs;
			return (TRUE);

		case MA_Icon_WindowHeight:
			*msg->opg_Storage = data->win_ys;
			return (TRUE);

		case MA_Icon_OffsetY:
			*msg->opg_Storage = data->offs_y;
			return (TRUE);

		case MA_Icon_OffsetX:
			*msg->opg_Storage = data->offs_x;
			return (TRUE);

		case MA_Icon_ViewMode:
			*msg->opg_Storage = data->viewmode;
			return (TRUE);

		case MA_Icon_SortMode:
			*msg->opg_Storage = data->sortmode;
			return (TRUE);

		case MA_Icon_ImageType:
			*msg->opg_Storage = data->imagetype;
			return (TRUE);

		case MA_Icon_ImageNormal:
			*msg->opg_Storage = (ULONG)idata->bm1;
			return (TRUE);

		case MA_Icon_ImageSelected:
			*msg->opg_Storage = (ULONG)idata->bm2;
			return (TRUE);

		case MA_Icon_ImageNormalWidth:
			*msg->opg_Storage = gfx_bitmap_width(idata->bm1);
			return (TRUE);

		case MA_Icon_ImageNormalHeight:
			*msg->opg_Storage = gfx_bitmap_height(idata->bm1);
			return (TRUE);

		case MA_Icon_ImageSelectedWidth:
			*msg->opg_Storage = gfx_bitmap_width(idata->bm2);
			return (TRUE);

		case MA_Icon_ImageSelectedHeight:
			*msg->opg_Storage = gfx_bitmap_height(idata->bm2);
			return (TRUE);

		case MA_Icon_Selected:
			*msg->opg_Storage = (ULONG)data->selected;
			return (TRUE);

		case MA_Icon_Left:
			*msg->opg_Storage = ICON_GETOBJ_LEFT(obj);
			return (TRUE);

		case MA_Icon_Top:
			*msg->opg_Storage = ICON_GETOBJ_TOP(obj);
			return (TRUE);

		case MA_Icon_ToolTypeList:
			*msg->opg_Storage = (ULONG)&data->ttlist;
			return (TRUE);

		case MA_Icon_AppAddress:
			*msg->opg_Storage = (ULONG)data->appaddress;
			return (TRUE);

		case MA_DragDrop_Path:
			*msg->opg_Storage = (ULONG)data->path;
			return (TRUE);

		case MA_DragDrop_Type:
			*msg->opg_Storage = MV_DragDrop_Type_Icon;
			return (TRUE);

		case MA_View_NumSelected: /* for drag & drop */
			*msg->opg_Storage = 0;
			return (TRUE);

		case MA_Icon_HasDrawerData:
			*msg->opg_Storage = data->has_drawerdata;
			return (TRUE);

		case MA_Icon_IsDefault:
			*msg->opg_Storage = data->isdefault;
			return (TRUE);

		case MA_Icon_Refine:
			*msg->opg_Storage = data->refine;
			return (TRUE);

		case MA_Icon_IsShortcut:
			*msg->opg_Storage = data->is_shortcut;
			return (TRUE);

		case MA_Icon_IsLink:
			*msg->opg_Storage = data->is_link;
			return (TRUE);

		case MA_Icon_DoubleClick:
			*msg->opg_Storage = data->doubleclick;
			return (TRUE);

		#if USE_THUMBS
		case MA_Icon_ThumbIt:
			*msg->opg_Storage = data->thumbit;
			return (TRUE);
		#endif

		case MA_Icon_GaugePercent:
			*msg->opg_Storage = data->gauge_percent;
			return (TRUE);

		case MA_Icon_MimeType:
			*msg->opg_Storage = (ULONG) data->mimetype;
			return (TRUE);

		case MA_Icon_ClickPosX:
			*msg->opg_Storage = data->click_pos_x;
			return (TRUE);

		case MA_Icon_ClickPosY:
			*msg->opg_Storage = data->click_pos_y;
			return (TRUE);

		case MA_Icon_ReferenceCount:
			*msg->opg_Storage = data->reference_count;
			return (TRUE);

		case MA_View_IsViewObject:
			*msg->opg_Storage = FALSE;
			return (FALSE);

		case MA_View_HandleIcons:
			*msg->opg_Storage = TRUE;
			return (TRUE);

		case MA_Icon_FileDate:
			*msg->opg_Storage = data->filedate;
			return (TRUE);

		case MA_Icon_FileSize:
			*msg->opg_Storage = (ULONG) &data->filesize;
			return (TRUE);

		case MA_Icon_ViewID:
			*msg->opg_Storage = data->viewid;
			return (TRUE);
	}
	return (DOSUPER);
}


#define UPDATE_BMHELPERS \
	({data->bm_max_xs = gfx_bitmap_width(idata->bm1); \
	data->bm_max_ys = gfx_bitmap_height(idata->bm1); \
	if (idata->bm2_real) \
	{ \
		data->bm_max_xs = max(gfx_bitmap_width(idata->bm1), gfx_bitmap_width(idata->bm2)); \
		data->bm_max_ys = max(gfx_bitmap_height(idata->bm1), gfx_bitmap_height(idata->bm2)); \
	}})


DEFSMETHOD(Icon_Scale)
{
	GETDATA;
	struct IconData *idata = data;

	ULONG xs, ys;
	ULONG oxs, oys;

	if(!(data->dirtyflags & DIRTYFLAG_IMAGE))
		if(!data->infowin)
			return 0;

	/*
	 * If reference icon is present then we forward scaling to it.
	 */

	if(data->reference_icon)
	{
		ULONG rc;
		idata = (struct IconData *)muiUserData(data->reference_icon);

		rc = DoMethodA(data->reference_icon, msg);
		UPDATE_BMHELPERS;
		data->scaled = idata->scaled;
		return rc;
	}

	switch(msg->mode)
	{
	case MV_Icon_Scale_Magnify:
		xs = ys = CONF_MINSIZE;
		break;

	case MV_Icon_Scale_Reduce:
		xs = ys = CONF_MAXSIZE;
		break;

	case MV_Icon_Scale_Normalize:
		xs = MAXICONSIZE_X + data->sizeadjustment;
		ys = MAXICONSIZE_Y + data->sizeadjustment;
		break;

	case MV_Icon_Scale_Infowin:
		xs = ys = INFOWIN_MAXICONSIZE;
		break;

#ifdef DEBUG
	default:
		PDB(("hm, no mode\n"));
		break;
#endif
	}

	oxs = xs;
	oys = ys;

	gfx_scale_calc_aspect_constraints(gfx_bitmap_width(data->bm1_real), gfx_bitmap_height(data->bm1_real), &xs, &ys);

	/* Don't resize if not needed. */

	if(data->scaled && data->bm1 && gfx_bitmap_width(data->bm1) == xs && gfx_bitmap_height(data->bm1) == ys)
	{
		UPDATE_BMHELPERS;
		return 0;
	}

	/* Note: When scaled is set, bm1_real != bm1 */

	if(data->scaled)
	{
		gfx_bitmap_delete(data->bm1);
		data->bm1 = data->bm1_real;

		if(data->bm2)
		{
			gfx_bitmap_delete(data->bm2);
			data->bm2 = data->bm2_real;
		}
		data->scaled = FALSE;
	}

	if(data->vgobj)
	{
		#ifdef BUILD_ICONLIB
		struct Library *VGraphicsBase = OpenLibrary("vgraphics.library", 0);
		#endif
		APTR svg = data->vgobj;

		if(VGraphicsBase)
		{
			struct RastPort rp;
			FLOAT vec_width;
			FLOAT vec_height;
			FLOAT scalex;
			FLOAT scaley;
			FLOAT movementx = 0;
			FLOAT movementy = 0;

			VG_GetAttr(svg, VG_BBWidth, (ULONG*)&vec_width);
			VG_GetAttr(svg, VG_BBHeight, (ULONG*)&vec_height);

			scalex = ((float)xs / vec_width);
			scaley = ((float)ys / vec_height);

			VG_SetAttrs(svg, VG_ScaleX, (ULONG)&scalex, TAG_END);
			VG_SetAttrs(svg, VG_ScaleY, (ULONG)&scaley, TAG_END);
			VG_SetAttrs(svg, VG_MovementX, (ULONG)&movementx, TAG_END);
			VG_SetAttrs(svg, VG_MovementY, (ULONG)&movementy, TAG_END);

			/* create a dummy rastport */
			InitRastPort(&rp);

			if((rp.BitMap = AllocBitMap(xs, ys, 32, BMF_CLEAR | BMF_SPECIALFMT | SHIFT_PIXFMT(PIXFMT_ARGB32), NULL)) != NULL)
			{
				struct bitmap_ctx *ct;

				if((ct = malloc(sizeof(*ct))))
				{
					ct->isnative = FALSE;
					ct->usecount = 1;
					ct->ref = NULL;
					ct->width  = xs;
					ct->height = ys;
					ct->depth  = 32;
					ct->bm = rp.BitMap;
					ct->bpr = GetCyberMapAttr(ct->bm, CYBRMATTR_XMOD);
					ct->modulo = ct->bpr / GetCyberMapAttr(ct->bm, CYBRMATTR_BPPIX);

					/* render vectors */
					VG_Render(svg, VGR_DestWidth, xs, VGR_DestHeight, ys, VGR_DestDepth, 32, TAG_DONE);
					/* and blit that */
					VG_Blit(&rp, svg, VGR_RawAlpha, TRUE, TAG_DONE);

					/* delete previous scaled bitmap, if any */
					if (data->bm1 != data->bm1_real)
						gfx_bitmap_delete(data->bm1);
					data->bm1 = ct;
				}
				else
				{
					FreeBitMap(rp.BitMap);
					D(ICONIO, bug("out of memory\n", __FUNCTION__));
				}
			}
			else
			{
				D(ICONIO, bug("%s out of memory\n", __FUNCTION__));
			}

			#ifdef BUILD_ICONLIB
			CloseLibrary(VGraphicsBase);
			#endif
		}
	}

	else
	{
		struct bitmap_ctx *newbm1 = NULL;
		struct bitmap_ctx *newbm2 = NULL;
		BOOL success = FALSE;

		if((newbm1 = gfx_bitmap_create(xs, ys, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
		{
			if(gfx_scale(data->bm1_real, newbm1, xs, ys,
			             //SCALETAG_Scale2x, TRUE,
			             SCALETAG_Bilinear, TRUE,
			             SCALETAG_Average, TRUE,
			             TAG_DONE))
			{
				if(data->bm2_real)
				{
					//xs = ys = msg->mode == MV_Icon_Scale_Magnify ? _conf(icon_minsize) : _conf(icon_maxsize);
					//xs = MAXICONSIZE_X;
					//ys = MAXICONSIZE_Y; /* XXX: set like above.. or make a method for that.. */
					xs = oxs;
					ys = oys;

					gfx_scale_calc_aspect_constraints(gfx_bitmap_width(data->bm2_real), gfx_bitmap_height(data->bm2_real), &xs, &ys);

					if((newbm2 = gfx_bitmap_create(xs, ys, 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
					{
						if(gfx_scale(data->bm2_real, newbm2, xs, ys,
						             SCALETAG_Scale2x, TRUE,
						             SCALETAG_Bilinear, TRUE,
						             SCALETAG_Average, TRUE,
						             TAG_DONE))
						{
							success = TRUE;
						}
					}
				}
				else
					success = TRUE;
			}
		}

		if (success)
		{
			data->bm1 = newbm1;
			data->bm2 = newbm2;
			data->scaled = TRUE;
		}
		else
		{
			gfx_bitmap_delete(newbm1);
			gfx_bitmap_delete(newbm2);
		}
	}

	UPDATE_BMHELPERS;

	return (0);
}

#define FADE_FPS 20

DEFMMETHOD(Setup)
{
	if (DOSUPER)
	{
		GETDATA;

		if (data->alphaval == 0)
		{
			data->ihnode.ihn_Object = obj;
			data->ihnode.ihn_Flags = MUIIHNF_TIMER;
			data->ihnode.ihn_Millis = 1000 /FADE_FPS;
			data->ihnode.ihn_Method = MM_Icon_FadeIn;

			//kprintf("grp size: %ld (x: %ld)\n", _width(_parent(obj)), data->x);
			if (data->y > data->x)
			{
				data->prefadeticks = FADE_FPS * data->y / _height(_parent(obj));
			}
			else
			{
				data->prefadeticks = FADE_FPS * data->x / _width(_parent(obj));
			}

			DoMethod(app, MUIM_Application_AddInputHandler, &data->ihnode);
		}
		return (TRUE);
	}
	return (FALSE);
}

DEFSMETHOD(Icon_Generate)
{
	GETDATA;
	struct IconData *idata = data;
	ULONG noscale = TRUE;
	LONG old_text_width = data->text_width;
	APTR old_tbctx2 = data->tbctx2;

	if (data->reference_icon != NULL)
	{
		idata = (struct IconData *)muiUserData(data->reference_icon);

		DoMethodA(data->reference_icon, msg);
	}

	/*
	 * This method creates icon imaginery. Should be invoked when icon size changes,
	 * when icon title changes.
	 */

	if (msg->force)
		data->dirtyflags |= DIRTYFLAG_IMAGE;

	if (data->dirtyflags & DIRTYFLAG_LABEL)
	{
		if (data->tbctx)
		{
			textbox_delete(data->tbctx);
			data->tbctx = NULL;
		}

		if (data->tbctx2)
		{
			textbox_delete(data->tbctx2);
			data->tbctx2 = NULL;
		}

		data->text_color = getv(_parent(obj), MA_Iconview_IconTextColor);
		data->text_bgcolor = getv(_parent(obj), MA_Iconview_IconTextBgColor);

		data->text_width = 0;
		data->text_height = 0;
		data->dirtyflags &= ~DIRTYFLAG_LABEL;
	}

	if (!data->infowin)
	{
		if (data->dirtyflags & DIRTYFLAG_IMAGE)
		{
			/*
			 * Check if we have to scale the icon.
			 */

			if (data->bm_max_xs_real < CONF_MINSIZE && data->bm_max_ys_real < CONF_MINSIZE)
			{
				noscale = FALSE;

				if (msg->force || data->minsize != CONF_MINSIZE)
				{
					DoMethod(obj, MM_Icon_Scale, MV_Icon_Scale_Magnify);
				}
			}

			if (noscale)
			{
				if (data->bm_max_xs_real > CONF_MAXSIZE || data->bm_max_ys_real > CONF_MAXSIZE)
				{
					noscale = FALSE;
					if (msg->force || data->maxsize != CONF_MAXSIZE)
					{
						DoMethod(obj, MM_Icon_Scale, MV_Icon_Scale_Reduce);
					}
				}
			}

			data->minsize = CONF_MINSIZE;
			data->maxsize = CONF_MAXSIZE;

			if (noscale && data->scaled)
			{
				/*
				 * Restore the size.
				 */

				gfx_bitmap_delete(data->bm1);
				data->bm1 = data->bm1_real;

				if (data->bm2)
				{
					gfx_bitmap_delete(data->bm2);
					data->bm2 = data->bm2_real;
				}

				data->scaled = FALSE;

				UPDATE_BMHELPERS;
			}
			data->dirtyflags &= ~DIRTYFLAG_IMAGE;
		}
	}
	else
	{
		/*
		 * Infowin mode. Try to not take too much size (XXX: use the data->infowin instead)
		 */

		if (data->bm_max_xs_real > INFOWIN_MAXICONSIZE || data->bm_max_ys_real > INFOWIN_MAXICONSIZE)
		{
			DoMethod(obj, MM_Icon_Scale, MV_Icon_Scale_Infowin);
		}
	}

	/*
	 * Calculate the text size (XXX: there should be a method to recalculate that for name updating)
	 */

	if (data->tbctx == NULL && data->afont && data->iconname)
	{
		LONG tbwidth = data->filetype == MV_Icon_FileType_Device ? 130 : 90; /* XXX: hack for now.. make it adapt to the gridsize */

		/* give tbox more space if image is larger (4 pixels margin)*/

		if (CONF_MAXSIZE - 8 > tbwidth)
			tbwidth = CONF_MAXSIZE - 8;

		data->tbctx = textbox_create(data->afont,
			TEXTBOXTAG_RenderMode, getv(_parent(obj), MA_Iconview_IconTextEffect),
			TEXTBOXTAG_Text, data->iconname,
			TEXTBOXTAG_Color1, data->text_color,
			TEXTBOXTAG_Color2, data->text_bgcolor,
			//TEXTBOXTAG_Width, data->bm_max_xs * 2, /* XXX: tune up ? */
			TEXTBOXTAG_Width, tbwidth,
			TEXTBOXTAG_Italic, data->is_link,
		TAG_DONE);

		if (!data->tbctx)
		{
			return (FALSE); /* XXX */
		}
		data->text_width = textbox_getwidth(data->tbctx);
		data->text_height = textbox_getheight(data->tbctx) + data->fontspace;
	}

	/*
	 * Additional text displayed under icon label.
	 */

	if (data->tbctx2 == NULL && data->smallfont && data->additionaltext)
	{
		data->tbctx2 = textbox_create(data->smallfont,
			TEXTBOXTAG_RenderMode, getv(_parent(obj), MA_Iconview_IconTextEffect),
			TEXTBOXTAG_Text, data->additionaltext,
			TEXTBOXTAG_Color1, data->text_color,
			TEXTBOXTAG_Color2, data->text_bgcolor,
			//TEXTBOXTAG_Width, data->bm_max_xs * 2, /* XXX: tune up ? */
			TEXTBOXTAG_Width, 0, /* XXX: hacky. Prevents breaking of lines. Should be fixed but for now only used for desktop anyway */
		TAG_DONE);

		if (!data->tbctx2)
		{
			PDB(("textbox creation failed\n"));
			return (FALSE); /* XXX */
		}

		data->text_width = max( textbox_getwidth(data->tbctx2) + data->fontspace, data->text_width );
		data->text_height += textbox_getheight(data->tbctx2) + data->fontspace;

		if (old_tbctx2 != NULL && data->text_width > old_text_width)
		{
			/* we need more space, relayout */

			DoMethod(_parent(obj), MM_Iconview_ClearLayout);
			DoMethod(obj, MM_Icon_UpdateBitMapBuffer);
			if (!data->infowin && _parent(obj))
				DoMethod(_parent(obj), MM_Iconview_Redraw);
		}
	}

	UPDATE_BMHELPERS;

	if (!data->infowin)
	{
		/*
		 * Find out the real X position.
		 */

		if (data->text_width > CONF_MAXSIZE)
		{
			data->addx = (data->text_width - data->bm_max_xs) / 2; /* XXX: that won't work if both images have a different size */
		}
		else
		{
			data->addx = (CONF_MAXSIZE - data->bm_max_xs) / 2;
		}

		/*
		 * Same for Y. Our icon image always has same height so calculate offset.
		 */

		data->addy = CONF_MAXSIZE - data->bm_max_ys;
	}
	else
	{
		data->addx = 0;
		data->addy = 0;
	}

	data->dirtyflags &= ~DIRTYFLAG_LABEL;
	data->dirtyflags &= ~DIRTYFLAG_IMAGE;

	return TRUE;
}

DEFMMETHOD(AskMinMax)
{
	GETDATA;
	LONG dirtyflags_image = data->dirtyflags & DIRTYFLAG_IMAGE;

	DOSUPER;

	/* We only need icon width for now, so skip image generation */

	data->dirtyflags &= ~DIRTYFLAG_IMAGE;

	if (data->reference_icon != NULL)
	{
		DoMethod(data->reference_icon, MM_Icon_Generate, FALSE);
	}

	DoMethod(obj, MM_Icon_Generate, FALSE);

	if (!data->infowin)
	{
		/*
		 * We need independant (from bitmap size) icon size, because icon image might be replaced.
		 */
	
		msg->MinMaxInfo->MinWidth += max(CONF_MAXSIZE, data->text_width);
		msg->MinMaxInfo->MinHeight += CONF_MAXSIZE + data->text_height;
	}
	else
	{
		msg->MinMaxInfo->MinWidth += data->bm_max_xs;
		msg->MinMaxInfo->MinHeight += data->bm_max_ys;
	}

	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX; /* XXX */
	msg->MinMaxInfo->MaxHeight = MUI_MAXMAX;

	data->dirtyflags |= dirtyflags_image;

	return (0);
}


#define _isinicon(_o, _parentobj, _x, _y) ({ \
	ULONG vx = getv(_parentobj, MUIA_Virtgroup_Left); \
	ULONG vy = getv(_parentobj, MUIA_Virtgroup_Top); \
	(_between(ICON_GETIMG_LEFT(_o),((_x)-_left(_parentobj)+vx),ICON_GETIMG_LEFT(_o) + ICON_GETIMG_WIDTH(_o)) && _between(ICON_GETIMG_TOP(_o),((_y)-_top(_parentobj)+vy),ICON_GETIMG_TOP(_o) + ICON_GETIMG_HEIGHT(_o))); \
   })
#define _isinicontext(_parentobj,_x,_y) ({ \
	LONG xs = (_mwidth(obj) - textbox_getwidth(data->tbctx)) / 2; \
	ULONG ys = textbox_getheight(data->tbctx); \
	ULONG vx = getv((_parentobj), MUIA_Virtgroup_Left); \
	ULONG vy = getv((_parentobj), MUIA_Virtgroup_Top); \
	if (xs < 0) xs = 0; \
	(_between(_mleft(obj)+xs,((_x)-_left((_parentobj))+vx),_mright(obj)-xs)) && (_between(data->y+data->bm_max_ys+data->fontspace,((_y)-_top((_parentobj))+vy),data->y+data->bm_max_ys+data->fontspace+ys)); \
   })


static ULONG create_drag_image(struct MUI_AmbientDragImage *adi, APTR bm)
{
	APTR tbm = gfx_bitmap_create(gfx_bitmap_width(bm), gfx_bitmap_height(bm), 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE);

	if (tbm != NULL)
	{
		gfx_blit(bm, tbm, TAG_DONE);
		adi->di.bm = gfx_bitmap_bm(tbm);
		adi->bitmap = tbm;

		adi->di.width = gfx_bitmap_width(bm);
		adi->di.height = gfx_bitmap_height(bm);
	}

	return tbm != NULL;
}


static void delete_specialbm(APTR obj, struct IClass *cl, ULONG effect)
{
	GETDATA;

	if ( effect == BMEFFECT_ALL )
	{
		LONG i;

		for(i=0; i<BMEFFECT_TOTAL; i++)
		{
			gfx_bitmap_delete(data->bm_effect[ i ]);
			data->bm_effect[ i ] = NULL;
		}
	}
	else
	{
		gfx_bitmap_delete(data->bm_effect[ effect ]);
		data->bm_effect[ effect ] = NULL;
	}
}


static APTR create_specialbm(APTR obj, struct IClass *cl, ULONG effect, APTR srcbm)
{
	GETDATA;
	struct IconData *idata;

	if ( data->reference_icon )
		idata = (struct IconData *)muiUserData(data->reference_icon);
	else
		idata = data;

	/* check if we changed dimensions */

	if ( idata->bm_effect[ effect ] )
	{
		if ( gfx_bitmap_width( idata->bm_effect[ effect ] ) != gfx_bitmap_width( srcbm ) ||
				gfx_bitmap_height( idata->bm_effect[ effect ] ) != gfx_bitmap_height( srcbm ) )
		{
			delete_specialbm(data->reference_icon ? data->reference_icon : obj, cl, effect);
		}
	}

	/* generate new special bitmap */

	if (!idata->bm_effect[effect] && (idata->bm_effect[effect] = gfx_bitmap_create(gfx_bitmap_width(srcbm), gfx_bitmap_height(srcbm), 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)))
	{
		struct RastPort rp;

		InitRastPort(&rp);
		/* XXX: put some ppa interface as gfx_something */
		rp.BitMap = gfx_bitmap_bm(idata->bm_effect[effect]);

		gfx_blit(srcbm, idata->bm_effect[effect], TAG_DONE);

		switch (effect)
		{
			case BMEFFECT_SELECTED:
			case BMEFFECT_SELECTED2:
				#if USE_DROP_EFFECT_PREFS
				switch (_conf(icon_selecteffect))
				#else
				switch (DE_TINT)
				#endif
				{
					case DE_TINT:
						ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_TINT, gfx_get_penspec_value(muiRenderInfo(obj), &_conf(icon_tintval)), NULL);
						break;

					#if USE_DROP_EFFECT_PREFS
					case DE_BLUR:
						ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_BLUR, 0, NULL);
						break;

					case DE_BRIGHTEN:
						ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_BRIGHTEN, _conf(icon_brightenval), NULL);
						break;

					case DE_DARKEN:
						ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_DARKEN, _conf(icon_darkenval), NULL);
						break;

					case DE_GREY:
						ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_COLOR2GREY, 0, NULL);
						break;

					case DE_NEGATIVE:
						ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_NEGATIVE, 0, NULL);
						break;

					case DE_NEGFADE:
						ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_NEGFADE, 0, NULL);
						break;

					case DE_TINTFADE:
						ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_TINTFADE, gfx_get_penspec_value(muiRenderInfo(obj), &_conf(icon_tintfadeval)), NULL);
						break;
					#endif
				}
				break;

			case BMEFFECT_MOUSEOVER:
			case BMEFFECT_MOUSEOVER2:
			case BMEFFECT_SELECTEDMOUSEOVER:	/* we are given selected bitmap as source */
			case BMEFFECT_SELECTEDMOUSEOVER2:	 /* we are given selected bitmap as source */
				ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(idata->bm_effect[effect]), gfx_bitmap_height(idata->bm_effect[effect]), POP_BRIGHTEN, 0x20, NULL);
				break;
		}
	}
	return (APTR)idata->bm_effect[effect];
}

DEFMMETHOD(CreateDragImage)
{
	GETDATA;
	struct MUI_AmbientDragImage *adi;

	/* delegate image creation to container.*/

    if (getv(_parent(obj), MA_View_NumSelected) > 1)
		return DoMethodA(_parent(obj), msg);

	if ((adi = malloc(sizeof(*adi))))
	{
		struct IconData *idata;

		if (data->reference_icon != NULL)
			idata = (struct IconData *)muiUserData(data->reference_icon);
		else
			idata = data;

		memset(adi, 0, sizeof(*adi)); /* XXX: a bit of a waste.. */

		adi->di.flags = MUIF_DRAGIMAGE_SOURCEALPHA | MUIF_DRAGIMAGE_NOSHADOWS;

		if (idata->bm2)
			create_drag_image(adi, idata->bm2);
		else if (idata->bm1)
			create_drag_image(adi, idata->bm1);

		/* XXX: what if there's no image ? */

		/*
		 * We give it a position within container, and not within window.
		 */

		adi->di.touchx = msg->touchx - _left(_parent(obj)) - data->addx;
		adi->di.touchy = msg->touchy - _top(_parent(obj)) - data->addy;

	}

	return ((ULONG)adi);
}

DEFMMETHOD(DeleteDragImage)
{
	/* warning: this one will be called also on iconview DeleteDragImage! */

	struct MUI_AmbientDragImage *adi = (struct MUI_AmbientDragImage*)msg->di;

	gfx_bitmap_delete(adi->bitmap);
	free(adi);

	return (0);
}

DEFMMETHOD(DragQuery)
{
	GETDATA;

	if (msg->obj != obj)
	{
		if (!data->infowin)
		{
			if (data->filetype == MV_Icon_FileType_Directory || data->filetype == MV_Icon_FileType_Device || data->type == MV_Icon_Type_AppIcon)
			{
				switch (getv(msg->obj, MA_DragDrop_Type))
				{
					case MV_DragDrop_Type_Root:
						/* XXX: that's not really great */
						break;

					case MV_DragDrop_Type_Iconview:
						ASSERT(getv(msg->obj, MA_View_NumSelected) > 1);

						FORCHILD(msg->obj, MUIA_Group_ChildList)
						{
							if (getv(child, MA_Icon_Selected))
							{
								if (child == obj)
								{
									return (MUIV_DragQuery_Refuse);
								}
							}
						}
						NEXTCHILD

						return (MUIV_DragQuery_Accept);

					case MV_DragDrop_Type_Icon:
						{
							LONG ft = getv(msg->obj, MA_Icon_FileType);

							if (ft == MV_Icon_FileType_File || ft == MV_Icon_FileType_Directory)
							{
								return (MUIV_DragQuery_Accept);
							}
						}
						break;
				}
			}
		}
	}

	return (MUIV_DragQuery_Refuse);
}


/*
 * Prevents areaclass from drawing a stupid frame around.
 */
DEFMMETHOD(DragBegin)
{
	GETDATA;
	data->droptarget = FALSE;
	return (0);
}

DEFMMETHOD(DragReport)
{
	GETDATA;
	APTR bm;
	struct IconData *idata;

	if (data->reference_icon)
		idata = (struct IconData *)muiUserData(data->reference_icon);
	else
		idata = data;

	if (msg->update)
	{
		MUI_Redraw(obj, MADF_DRAWUPDATE);
		return (MUIV_DragReport_Continue);
	}

	if (msg->obj != obj)
	{
		if (_isinobject(msg->x, msg->y))
		{
			if (_isinicon(obj, _parent(obj), msg->x, msg->y)) /* XXX: won't work in iconinfo */
			{
				if ( data->droptarget )	 /* We already have needed bitmap */
				{
					return (MUIV_DragReport_Continue);
				}
				else
				{
					if (data->selected && data->numimages == 2)
					{
						bm = idata->bm2;
					}
					else
					{
						bm = idata->bm1;
					}

					if ( (idata->bm_effect[BMEFFECT_DRAGDROP] = gfx_bitmap_create(gfx_bitmap_width(bm), gfx_bitmap_height(bm), 32, BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
					{
						struct RastPort rp;

						InitRastPort(&rp);
						rp.BitMap = gfx_bitmap_bm(idata->bm_effect[BMEFFECT_DRAGDROP]);

						gfx_blit(bm, idata->bm_effect[BMEFFECT_DRAGDROP], TAG_DONE);

						#if USE_DROP_EFFECT_PREFS
						switch (_conf(dragdrop_dropeffect))
						#else
						switch (DE_TINT)
						#endif
						{
							case DE_TINT:
								ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(bm), gfx_bitmap_height(bm), POP_TINT, gfx_get_penspec_value(muiRenderInfo(obj), &_conf(dragdrop_tintval)), NULL);
								break;

							#if USE_DROP_EFFECT_PREFS
							case DE_BLUR:
								ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(bm), gfx_bitmap_height(bm), POP_BLUR, 0, NULL);
								break;

							case DE_BRIGHTEN:
								ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(bm), gfx_bitmap_height(bm), POP_BRIGHTEN, _conf(dragdrop_brightenval), NULL);
								break;

							case DE_DARKEN:
								ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(bm), gfx_bitmap_height(bm), POP_DARKEN, _conf(dragdrop_darkenval), NULL);
								break;

							case DE_GREY:
								ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(bm), gfx_bitmap_height(bm), POP_COLOR2GREY, 0, NULL);
								break;

							case DE_NEGATIVE:
								ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(bm), gfx_bitmap_height(bm), POP_NEGATIVE, 0, NULL);
								break;

							case DE_NEGFADE:
								ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(bm), gfx_bitmap_height(bm), POP_NEGFADE, 0, NULL);
								break;

							case DE_TINTFADE:
								ProcessPixelArray(&rp, 0, 0, gfx_bitmap_width(bm), gfx_bitmap_height(bm), POP_TINTFADE, gfx_get_penspec_value(muiRenderInfo(obj), &_conf(dragdrop_tintfadeval)), NULL);
								break;
							#endif
						}

						data->droptarget = TRUE;
						return (MUIV_DragReport_Refresh);
					}
				}
			}
			else
			{
				if (idata->bm_effect[BMEFFECT_DRAGDROP])
				{
					delete_specialbm(obj, cl, BMEFFECT_DRAGDROP);
					if (data->reference_icon != NULL)
						delete_specialbm(data->reference_icon, cl, BMEFFECT_DRAGDROP);
					data->droptarget = FALSE;
					return (MUIV_DragReport_Refresh);
				}
				else
				{
					return (MUIV_DragReport_Continue);
				}
			}
		}
	}
	return (MUIV_DragReport_Continue);
}


DEFMMETHOD(DragDrop)
{
	GETDATA;

	/* XXX: we should check the dragdrop_type like with 'accept' above */

	if (msg->obj != obj)
	{
		if (data->type == MV_Icon_Type_AppIcon)
		{
			struct wbargs *wba;

			if ( (wba = wba_create(msg->obj)) )
			{
				do_action(NULL, TA_AppMsg_Send,/* we might go away */
						  TT_AppMsg_Send_Type, AMTYPE_APPICON,
						  TT_AppMsg_Send_MsgPort, data->msgport,
						  TT_AppMsg_Send_Path, wba->basepath,
						  TT_AppMsg_Send_ID, data->appid,
						  TT_AppMsg_Send_Userdata, data->appuserdata,
						  TT_AppMsg_Send_NumArgs, wba->count,
						  TT_AppMsg_Send_WBArgList, wba->wba,
						  TT_AppMsg_Send_MouseX, 0, /* XXX */
						  TT_AppMsg_Send_MouseY, 0, /* XXX */
						  TAG_DONE
				);
				wba_delete(wba);
			}
		}
		else
		{
			if (getv(msg->obj, MA_View_NumSelected) > 1)
			{
				APTR dispatcher = (APTR)DoMethod(_app(obj), MM_Application_CreateActionDispatcher);

				if (dispatcher)
				{
					struct MinList ml;
					struct dragdropnode *ddn, *dds;
					NEWLIST(&ml);

					SetAttrs(dispatcher,
						MA_ActionDispatcher_IQualifier, (APTR)getv(_parent( msg->obj ), MA_Iconview_Qualifier),
						MA_ActionDispatcher_Event, ACTION_EVENT_DRAGNDROP,
						MA_ActionDispatcher_DstURI, data->path,
						MA_ActionDispatcher_DstID, (APTR)getv(_win( obj ), MA_Window_ID),
						MA_ActionDispatcher_SrcURI, (APTR)getv(msg->obj, MA_View_Path),
						MA_ActionDispatcher_RefWin, getv(_view(msg->obj), MA_View_IsRoot) ? NULL : _win(msg->obj),
						TAG_DONE
					);

					NEWLIST(&ml);
					DoMethod(msg->obj, MM_View_GetSelectionList, &ml);

					ITERATELISTSAFE(ddn, dds, &ml)
					{
						DoMethod( dispatcher, MM_ActionDispatcher_AddURI, ddn->path, TRUE );
						free(ddn);
					}

					DoMethod( dispatcher, MM_ActionDispatcher_Execute );
				}
			}
			else
			{
				LONG filetype = getv(msg->obj, MA_Icon_FileType);

				if (filetype == MV_Icon_FileType_Directory || filetype == MV_Icon_FileType_File) /* XXX: and devices too, yep */
				{
					APTR dispatcher = (APTR)DoMethod(_app(obj), MM_Application_CreateActionDispatcher);

					if (dispatcher)
					{
						SetAttrs(dispatcher,
							MA_ActionDispatcher_IQualifier, getv(_parent( msg->obj ), MA_Iconview_Qualifier),
							MA_ActionDispatcher_Event, ACTION_EVENT_DRAGNDROP,
							MA_ActionDispatcher_DstURI, data->path,
							MA_ActionDispatcher_DstID, getv(_win( obj ) , MA_Window_ID),
							MA_ActionDispatcher_SrcURI, getv(msg->obj, MA_Icon_Path),
							MA_ActionDispatcher_RefWin, getv(_view(msg->obj), MA_View_IsRoot) ? NULL : _win(msg->obj),
							TAG_DONE
						);

						DoMethod( dispatcher, MM_ActionDispatcher_AddURI, (STRPTR)getv(msg->obj, MA_Icon_Path), TRUE );
						DoMethod( dispatcher, MM_ActionDispatcher_Execute );
					}
				}
			}
		}
	}
	return (0);
}

DEFMMETHOD(DragEvent)
{
	/* forward call to container object. will be ignored for anything else than iconview. */

	return DoMethodA(_parent(obj), msg);
}

DEFMMETHOD(DragFinish)
{
	if (msg->obj != obj)
	{
		GETDATA;
		delete_specialbm(obj, cl, BMEFFECT_DRAGDROP);
		data->droptarget = FALSE;
		MUI_Redraw(obj, MADF_DRAWUPDATE);
	}
	return (0);
}


DEFTMETHOD(Icon_FadeIn)
{
	GETDATA;

	if (data->prefadeticks)
	{
		data->prefadeticks--;
		return (0);
	}

	data->alphaval += 0x11111111;

	DoMethod(obj, MM_Icon_UpdateBitMapBuffer);
	MUI_Redraw(obj, MADF_DRAWOBJECT);

	if (data->alphaval == 0xffffffff)
	{
		DoMethod(app, MUIM_Application_RemInputHandler, &data->ihnode);
		data->ihnode.ihn_Object = NULL;
	}
	return (0);
}


DEFMMETHOD(Cleanup)
{
	GETDATA;

	if (data->ihnode.ihn_Object)
	{
		DoMethod(app, MUIM_Application_RemInputHandler, &data->ihnode);
		data->ihnode.ihn_Object = NULL;
	}
	return (DOSUPER);
}


DEFMMETHOD(Draw)
{
	ULONG rc;
	GETDATA;

	rc = DOSUPER; /* XXX: I *think* this belongs there.. or maybe afterwards in "We really draw here" below ? */

	/*
	 * If icon is in dirty state we need to fix that first.
	 */

	if (data->dirtyflags)
	{
		if (data->reference_icon != NULL)
		{
			DoMethod(data->reference_icon, MM_Icon_Generate, FALSE);
		}

		/* Generate icon imaginery */

		DoMethod(obj, MM_Icon_Generate, FALSE);
	}

	if (data->immediate_update)
	{
		/*
		 * For drag & drop
		 */
		msg->flags |= MADF_DRAWUPDATE;
		data->immediate_update = FALSE;
	}

	if (msg->flags & MADF_DRAWUPDATE)
	{
		/*
		 * Update the buffer and redisplay.
		 */
		DoMethod(obj, MM_Icon_UpdateBitMapBuffer);
		msg->flags |= MADF_DRAWOBJECT;
	}
	else
	{
		/*
		 * Check if we moved.
		 */
		if (data->x != data->prev_x ||
			data->y != data->prev_y)
		{
			DoMethod(obj, MM_Icon_UpdateBitMapBuffer);
		}
	}

	if (msg->flags & MADF_DRAWOBJECT)
	{
		if (!data->dbuf || data->needsupdate)
		{
			if (DoMethod(obj, MM_Icon_UpdateBitMapBuffer2))
			{
				data->needsupdate = FALSE;
			}
			else
			{
				PDB(("argh! couldn't build icon buffer<%s>!\n", data->path));
				/* XXX: fail! */
			}
		}

		if (data->dbuf)
		{

			#if 0
			gfx_blit(data->dbuf->bm, _rp(obj),
				BLITTAG_DstType, BLITVAL_DstType_RastPort,
				BLITTAG_DstX, _mleft(obj),
				BLITTAG_DstY, _mtop(obj),
				BLITTAG_SrcY, 0,
				BLITTAG_DstWidth, _mwidth(obj),
				BLITTAG_DstHeight, _mheight(obj),
			TAG_DONE);
			#else
			gfx_blit(data->dbuf->bm, _rp(obj),
				BLITTAG_DstType, BLITVAL_DstType_RastPort,
				BLITTAG_DstX, _mleft(obj),
				BLITTAG_DstY, _mtop(obj) + data->addy,
				BLITTAG_SrcY, data->addy,
				BLITTAG_DstWidth, _mwidth(obj),
				BLITTAG_DstHeight, _mheight(obj) - data->addy,
			TAG_DONE);
			#endif

			icon_free_dbuf();
			data->dbuf = NULL;
		}
	}
	return (rc);
}


DEFTMETHOD(Icon_UpdateBitMapBuffer)
{
	GETDATA;

	data->needsupdate = TRUE;

	return (0);
}

DEFTMETHOD(Icon_UpdateBitMapBuffer2)
{
	GETDATA;

	if ( (data->dbuf = icon_alloc_dbuf( obj, data, _mwidth(obj), _mheight(obj))) )
	{
		APTR bm = NULL;
		struct IconData *idata;

		if ( data->reference_icon )
			idata = (struct IconData *)muiUserData(data->reference_icon);
		else
			idata = data;

		/*
		 * We really draw here (no need to compute the virtgroup, correct
		 * offset is already stored in the window backfill hook)
		 */
		if (!data->infowin)
		{
			ASSERT(_win(obj));
			#if 0
			SetRast(data->dbuf->rp,0);	/* Test mode to see the area icon occupies */
			#else
			DoMethod(_win(obj), /* XXX: is it ok to use _win() here? I think so.. */
				MM_Window_FillBackground,
				data->dbuf->rp,
				_mleft(obj),
				_mtop(obj),
				_mwidth(obj),
				_mheight(obj)
			);
			#endif
			data->prev_x = data->x;
			data->prev_y = data->y;
		}
		else
		{
			/* infowin mode */
			SetRPAttrs(data->dbuf->rp,
				RPTAG_PenMode, TRUE,
				RPTAG_APen, _pens(obj)[MPEN_BACKGROUND],
			TAG_DONE);
			RectFill(data->dbuf->rp,
				0,
				0,
				_mwidth(obj) - 1,
				_mheight(obj) - 1
			);
		}

		if ( data->droptarget && idata->bm_effect[BMEFFECT_DRAGDROP])
		{
			/*
			 * Dropping selection effect.
			 */
			bm = idata->bm_effect[BMEFFECT_DRAGDROP];
		}
		else
		{
			/*
			 * Normal drawing.
			 */

			if (data->selected)
			{
				if (data->highlighted)
				{
					if (data->numimages == 2)
					{
						bm = create_specialbm(obj, cl, BMEFFECT_SELECTEDMOUSEOVER2, idata->bm2);
						/* XXX */
					}
					else
					{
						if ( create_specialbm(obj, cl, BMEFFECT_SELECTED, idata->bm1) )
						{
							bm = create_specialbm(obj, cl, BMEFFECT_SELECTEDMOUSEOVER, idata->bm_effect[BMEFFECT_SELECTED]);
						}
						/* XXX: clear the image once we're done ? and handle the prefs ? */
					}
				}
				else
				{
					if (data->numimages == 2)
					{
						bm = idata->bm2;
					}
					else
					{
						bm = create_specialbm(obj, cl, BMEFFECT_SELECTED, idata->bm1);
						/* XXX: clear the image once we're done ? and handle the prefs ? */
					}
				}
			}
			else if (data->highlighted)
			{
				bm = create_specialbm(obj, cl, BMEFFECT_MOUSEOVER, idata->bm1);
			}
			else
			{
				bm = idata->bm1;
			}

		}

		if (bm)
		{
			/*
			 * Rendering of icon. Take modify icon's alpha if ghosty is
			 * enabled in prefs. Text alpha is not modified.
			 */

			ULONG alphaval = data->alphaval;

			if (alphaval > 0x88888888)
			{
				if (data->filetype != MV_Icon_FileType_Device)
				{
					if (!data->infowin && data->isdefault && _conf(icon_defghosted))
						alphaval = 0x88888888;
				}
				#if USE_THUMBS
				if (data->imagetype == MV_Icon_ImageType_Thumbicon)
				{
					/*
						Thumnbails are deficons too, but we give them full visibility.
					*/

					alphaval = 0xffffffff;
				}
				#endif
			}

			gfx_blit(bm, data->dbuf->rp,
				BLITTAG_DstType, BLITVAL_DstType_RastPort,
				BLITTAG_DstX, _mwidth(obj) / 2 - gfx_bitmap_width(bm) / 2,
				BLITTAG_DstY, data->addy,
				BLITTAG_Alpha, alphaval,
			TAG_DONE);
		}

		/*
		 * Draw text
		 */
		if (data->afont && data->iconname && data->tbctx)
		{
			textbox_setattrs(data->tbctx, TEXTBOXTAG_Alpha, data->alphaval, TAG_DONE);
			textbox_render(data->tbctx,
				data->dbuf->rp,
				/*data->addx ? 0 :*/ (_mwidth(obj) / 2 - textbox_getwidth( data->tbctx ) / 2),
				CONF_MAXSIZE + data->fontspace
			);
		}

		/*
		 * draw shortcut arrow.
		 */

		if ( data->is_shortcut && data->type != MV_Icon_Type_Disk && _conf(icon_shortcutsidentifier))
		{
			if (SHORTCUT_WIDTH < gfx_bitmap_width(idata->bm1) && SHORTCUT_HEIGHT < gfx_bitmap_height(idata->bm1))


			gfx_blit(shortcut, data->dbuf->rp,
					BLITTAG_DstType, BLITVAL_DstType_RastPort,
					BLITTAG_SrcType, BLITVAL_SrcType_Array,
					BLITTAG_Modulo, SHORTCUT_WIDTH * SHORTCUT_DEPTH / 8, /* modulo is different from the target bitmap so we specify */
					BLITTAG_Alpha, data->alphaval,
					BLITTAG_DstX, data->addx + (gfx_bitmap_width(idata->bm1) - SHORTCUT_WIDTH) / 4,
					BLITTAG_DstY, data->addy + gfx_bitmap_height(idata->bm1) - SHORTCUT_HEIGHT,
					BLITTAG_DstWidth, SHORTCUT_WIDTH,
					BLITTAG_DstHeight, SHORTCUT_HEIGHT,
				TAG_DONE);
		}

		/*
		 * draw spacegauge
		 */
		if (data->type == MV_Icon_Type_Disk || data->type == MV_Icon_Type_Device)
		{
			if ( data->gauge_percent != -1 )
				spacegauge_draw( data->dbuf->rp,
					data->bm_max_xs + data->addx,
					data->bm_max_xs,
					data->bm_max_ys + data->addy,
					data->gauge_percent,
					( ( data->alphaval >> 24 ) * 0xBB ) << 16
				);
		}

		/*
		 * Draw additional text
		 */
		if (data->smallfont && data->additionaltext && data->tbctx2)
		{
			textbox_setattrs(data->tbctx2, TEXTBOXTAG_Alpha, data->alphaval, TAG_DONE);
			textbox_render(data->tbctx2,
				data->dbuf->rp,
				/*data->addx ? 0 :*/ (_mwidth(obj) / 2 - textbox_getwidth( data->tbctx2 ) / 2),
				CONF_MAXSIZE + data->fontspace + ( data->tbctx ? textbox_getheight( data->tbctx ) : 0 )
			);
		}

		/* Draw Appicon identifier */
		if ( data->type == MV_Icon_Type_AppIcon && _conf(icon_appiconsidentifier))
		{
			if (APPICON_WIDTH < gfx_bitmap_width(idata->bm1) && APPICON_HEIGHT < gfx_bitmap_height(idata->bm1))


			gfx_blit(appicon, data->dbuf->rp,
					BLITTAG_DstType, BLITVAL_DstType_RastPort,
					BLITTAG_SrcType, BLITVAL_SrcType_Array,
					BLITTAG_Modulo, APPICON_WIDTH * APPICON_DEPTH / 8, /* modulo is different from the target bitmap so we specify */
					BLITTAG_Alpha, data->alphaval,
					BLITTAG_DstX, data->addx + (gfx_bitmap_width(idata->bm1) - APPICON_WIDTH) / 4,
					BLITTAG_DstY, data->addy + gfx_bitmap_height(idata->bm1) - APPICON_HEIGHT,
					BLITTAG_DstWidth, APPICON_WIDTH,
					BLITTAG_DstHeight, APPICON_HEIGHT,
				TAG_DONE);
		}
	}
	else
	{
		//PDB(("dbuf alloc failed! width: %ld, height: %ld\n", (ULONG)_mwidth(obj), (ULONG)_mheight(obj)));
	}
	return ((ULONG)data->dbuf);
}


DEFSMETHOD(Icon_InsertToolType)
{
	GETDATA;
	struct ttnode *tn;

	if ( (tn = icon_malloc(sizeof(struct ttnode) + msg->size)) )
	{
		stccpy(tn->tt, msg->tt, msg->size);
		ADDTAIL(&data->ttlist, tn);
		return (TRUE);
	}
	return (FALSE);
}


DEFTMETHOD(Icon_ClearToolTypes)
{
	GETDATA;
	struct ttnode *tn, *nexttn;

	ITERATELISTSAFE(tn, nexttn, &data->ttlist)
	{
		icon_free(tn);
	}
	NEWLIST(&data->ttlist);
	return (0);
}


DEFSMETHOD(Icon_GetToolTypes)
{
	GETDATA;
	struct ttnode *tn;
	STRPTR *tt = msg->tt;
	ULONG i = 0;

	while (*tt)
	{
		*msg->var[i] = 0;

		ITERATELIST(tn, &data->ttlist)
		{
			STRPTR p;
			ULONG len;

			len = strlen(*tt);
			p = tn->tt + len;

			#if 1
			if (!strnicmp(tn->tt, *tt, len) && (*p == ' ' || *p == '=' || *p == '\0'))
			#else
			if(((STRPTR) strstr(tn->tt, *tt) == tn->tt) && (*p == ' ' || *p == '=' || *p == '\0'))
			#endif
			{
				/*
				 * Ah well.. we find out if it's BLA=<val>
				 * and return the numeric of val, otherwise
				 * we return 0 as 0 is the default for the
				 * callers anyway.
				 */
				while (*p == ' ') p++; /* skip spaces */
				if (*p == '=')
				{
					p++;
					while (*p == ' ') p++; /* strip fucking spaces */
					*msg->var[i] = atol(p);
					D(WBSTARTUP, bug("found tooltype %s giving %ld\n", *tt, *msg->var[i]));
				}
				else
				{
					D(WBSTARTUP, bug("found tooltype %s (plain)\n", *tt));
					*msg->var[i] = 0xffff; /* ok.. this is to signal we found something */
				}
				break;
			}
		}

		tt++;
		i++;
	}
	return (0);
}


/* Note: when used must update data->bm1 afterwards! */
#define FREE_ICON_BITMAPS ({gfx_bitmap_delete(data->bm1); gfx_bitmap_delete(data->bm2); data->bm2 = NULL; })

DEFSMETHOD(Icon_AddBitMap)
{
	GETDATA;

	if ( data->reference_icon )
	{
		set( data->reference_icon, MA_Icon_ReferenceCount, MV_Icon_ReferenceCount_Decrease );
		data->reference_icon = NULL;
	}

	switch (msg->type)
	{
		case MV_Icon_BitMap_Standard:
			switch (msg->state)
			{
				case MV_Icon_BitMap_Normal:
					// why no FREE_ICON_BITMAPS; here ?
					data->imagetype = MV_Icon_ImageType_Standard;
					data->bm1 = msg->bm;
					break;

				case MV_Icon_BitMap_Selected:
					ASSERT(data->bm1);
					data->bm2 = msg->bm;
					break;
			}
			break;

		case MV_Icon_BitMap_Newicon:
			switch (msg->state)
			{
				case MV_Icon_BitMap_Normal:
					FREE_ICON_BITMAPS;
					data->imagetype = MV_Icon_ImageType_Newicon;
					data->bm1 = msg->bm;
					break;

				case MV_Icon_BitMap_Selected:
					ASSERT(data->bm1);
					data->bm2 = msg->bm;
					break;
			}
			break;

		case MV_Icon_BitMap_Glowicon:
			switch (msg->state)
			{
				case MV_Icon_BitMap_Normal:
					FREE_ICON_BITMAPS;
					data->imagetype = MV_Icon_ImageType_Glowicon;
					data->bm1 = msg->bm;
					break;

				case MV_Icon_BitMap_Selected:
					ASSERT(data->bm1);
					data->bm2 = msg->bm;
					break;
			}
			break;

		#if USE_PNGICONS
		case MV_Icon_BitMap_PNGicon:
			if (msg->state == MV_Icon_BitMap_Normal)
			{
				data->imagetype = MV_Icon_ImageType_PNGicon;

				gfx_bitmap_delete(data->bm1);
				gfx_bitmap_delete(data->bm2);
				if ( data->bm1_real && data->bm1_real != data->bm1 )
				{
					gfx_bitmap_delete( data->bm1_real );
					data->bm1_real = NULL;
				}

				data->bm1 = msg->bm;
				data->bm2 = NULL;
			}
			else
			{
				data->bm2 = msg->bm;
			}

			break;
		#endif

		#if USE_THUMBS
		case MV_Icon_BitMap_Thumbicon:

			data->imagetype = MV_Icon_ImageType_Thumbicon;

			gfx_bitmap_delete(data->bm1);
			gfx_bitmap_delete(data->bm2);
			if ( data->bm1_real && data->bm1_real != data->bm1 )
			{
				gfx_bitmap_delete( data->bm1_real );
				data->bm1_real = NULL;
			}

			data->bm1 = msg->bm;
			data->bm2 = NULL;

			DoMethod(obj, MM_Icon_SetIcon);
			break;
		#endif

		#if USE_DTICONS
		case MV_Icon_BitMap_DTicon:
			data->imagetype = MV_Icon_ImageType_DTicon;
			gfx_bitmap_delete(data->bm1);
			gfx_bitmap_delete(data->bm2);
			if ( data->bm1_real && data->bm1_real != data->bm1 )
			{
				gfx_bitmap_delete( data->bm1_real );
				data->bm1_real = NULL;
			}
			data->bm1 = msg->bm;
			data->bm2 = NULL;
			break;
		#endif

		#if USE_SVGICONS
		case MV_Icon_BitMap_SVGicon:
			data->imagetype = MV_Icon_ImageType_SVGicon;
			gfx_bitmap_delete(data->bm1);
			gfx_bitmap_delete(data->bm2);
			if ( data->bm1_real && data->bm1_real != data->bm1 )
			{
				gfx_bitmap_delete( data->bm1_real );
				data->bm1_real = NULL;
			}
			data->bm1 = msg->bm;
			data->bm2 = NULL;
			break;
		#endif

		#ifdef DEBUG
		default:
			PDB(("out of bound bitmap: val: %ld\n", msg->type));
			break;
		#endif
	}

	/* invalidate special images */

	delete_specialbm(obj, cl, BMEFFECT_ALL);
	data->dirtyflags |= DIRTYFLAG_IMAGE;

	return (0);
}


/*
 * Duplicates a bitmap and returns that.
 */
DEFSMETHOD(Icon_CreateBitMap)
{
	APTR bm = bm; /* shut up gcc */
	APTR tbm;
	GETDATA;

	switch (msg->type)
	{
		case MV_Icon_BitMap_Standard:
		case MV_Icon_BitMap_Newicon:
		case MV_Icon_BitMap_Glowicon:
			switch (msg->state)
			{
				case MV_Icon_BitMap_Normal:
					bm = data->bm1_real;
					break;

				case MV_Icon_BitMap_Selected:
					if (data->bm2_real)
					{
						bm = data->bm2_real;
					}
					else
					{
						return ((ULONG)NULL);
					}
					break;

				#ifdef DEBUG
				default:
					PDB(("eek\n"));
					break;
				#endif
			}
			break;

		case MV_Icon_BitMap_SVGicon:
		case MV_Icon_BitMap_PNGicon:
		case MV_Icon_BitMap_DTicon:
			bm = data->bm1_real;
			break;

		#ifdef DEBUG
		default:
			PDB(("blah\n"));
			break;
		#endif
	}

	if ( (tbm = gfx_bitmap_create(gfx_bitmap_width(bm), gfx_bitmap_height(bm), gfx_bitmap_depth(bm), BITMAPTAG_Format, BITMAPVAL_Format_ARGB32, TAG_DONE)) )
	{
		gfx_blit(bm, tbm, TAG_DONE);
		return ((ULONG)tbm);
	}
	return ((ULONG)NULL);
}


/*
 * Sets the right bitmap pointer/size that will be used for
 * primary and secondary imagery. This simplifies the code.
 */
DEFTMETHOD(Icon_SetIcon)
{
	GETDATA;
	struct IconData *idata;

	ULONG xs1, ys1;
	ULONG xs2, ys2;

	if ( data->reference_icon )
		idata = (struct IconData *)muiUserData(data->reference_icon);
	else
		idata = data;

	xs1 = gfx_bitmap_width(idata->bm1);
	ys1 = gfx_bitmap_height(idata->bm1);
	data->numimages = 1;

	if (idata->bm2)
	{
		xs2 = gfx_bitmap_width(idata->bm2);
		ys2 = gfx_bitmap_height(idata->bm2);
		data->numimages++;
	}
	else
	{
		xs2 = 0;
		ys2 = 0;
	}

	data->scaled = FALSE;

	data->bm1_real = data->bm1;
	data->bm2_real = data->bm2;

	/*
	 * Update helpers
	 */
	data->bm_max_xs_real = data->bm_max_xs = max(xs1, xs2);
	data->bm_max_ys_real = data->bm_max_ys = max(ys1, ys2);

	data->dirtyflags |= DIRTYFLAG_IMAGE;

	return (0);
}


DEFTMETHOD(Icon_End)
{
	DoMethod(obj, MM_Icon_SetIcon);

	MUI_Redraw(obj, MADF_DRAWOBJECT); /* XXX: hey.. is that needed ?? perhaps not! */

	return (0);
}


DEFSMETHOD(Icon_Select)
{
	GETDATA;

	data->seconds = 0;
	data->micros = 0;

	if (data->infowin)
	{
		return (0);
	}

	if (data->type == MV_Icon_Type_AppIcon)
	{
		do_action(NULL, /* we might go away */
			TA_AppMsg_Send,
			TT_AppMsg_Send_Type, AMTYPE_APPICON,
			TT_AppMsg_Send_MsgPort, data->msgport,
			TT_AppMsg_Send_ID, data->appid,
			TT_AppMsg_Send_Userdata, data->appuserdata,
			TT_AppMsg_Send_MouseX, msg->x,
			TT_AppMsg_Send_MouseY, msg->y,
		TAG_DONE);

		return (0);
	}

	/* Build URI to be executed */

	if (data->type == MV_Icon_Type_MyComputer)
	{
		DoMethod(_app(obj), MM_Application_OpenDevicesWindow, 0);
	}
	else if(data->type == MV_Icon_Type_View)
	{
		if (getv( _parent( obj ), MA_Iconview_IsRoot ) )
		{
			APTR wo = NULL;

			wo = (APTR)DoMethod(app, MM_Application_FindWindowByID, data->appid); /* abusing AppID */

			if ( wo )
			{
				return (set(wo, MUIA_Window_Open, TRUE));
			}
		}	 
	}
	else
	{
		TEXT buf[PATH_SIZE + 256]; /* should be enough (tm) */
		CONST_STRPTR mode = "";
		CONST_STRPTR view = "ICON";
		LONG viewmode;
		ULONG newwin = FALSE;

		/* check if we clicked deficon */

		if ( !data->isdefault )
		{
			/* for nondefault use stored viewmode */

			viewmode = data->viewmode;

			if ( viewmode == MV_Icon_ViewMode_IconAll )
				mode = "ALL";
			else if ( viewmode == MV_Icon_ViewMode_Thumbs )
				mode = "THUMBS";
			else if ( viewmode == MV_Icon_ViewMode_Lister )
				mode = "LIST";
			else
				mode = "ICON";

			/* XXX: sigh */
			if(viewmode == MV_Icon_ViewMode_Lister)
			{
				view = "LIST";
				mode = "ALL";
			}
		}
		else
		{
			if ( getv( _parent( obj ), MA_View_IsRoot ) || !_conf( window_inherit_viewmode ) )
			{
				switch(_conf(window_default_view))
				{
					default:
					case 0:
						view = "ICON";
						break;
					case 1:
						view = "LIST";
						break;
				}

				switch(_conf(window_default_viewmode))
				{
					default:
					case 0:
						mode = "ICONS";
						break;
					case 1:
						mode = "ALL";
						break;
					case 2:
						mode = "THUMBS";
				}
			}
			else
			{
				/* for default icons, we inherit viewmode of iconviewclass */
				int viewindex = getv( _parent( obj ), MA_View_ModeIndex );

				/* get viewname for index */

				struct viewnode *vn = viewapi_findbymime( (STRPTR)getv( _parent( obj ), MA_View_MIME ) );
				mode = viewapi_getmodename( vn, viewindex );
			}
		}

		newwin = !_conf(toolbar_browsermode);

		if ( newwin || getv( _parent( obj ), MA_Iconview_IsRoot ) )
		{
			/*
			 * Check if window is already opened. If it is then pop it to front.
			 */
			APTR wo = NULL;

			if ( path_expand( data->path, buf, sizeof( buf ) ) )
			{
				wo = (APTR)DoMethod(app, MM_Application_FindWindowByName, MV_Window_Type_View, buf);
			}
			else
			{
				wo = (APTR)DoMethod(app, MM_Application_FindWindowByName, MV_Window_Type_View, data->path);
			}

			if ( wo )
			{
				return (set(wo, MUIA_Window_Open, TRUE));
			}
		}

		/*
		 * Execute. If no defaulttool is set then let mimetypes handle it. Otherwise we run it.
		 */

		if ( !data->defaulttool || !*(data->defaulttool) || data->filetype == MV_Icon_FileType_Device || data->filetype == MV_Icon_FileType_Directory )
		{
			APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

			if ( dispatcher )
			{
				SetAttrs(dispatcher,
					MA_ActionDispatcher_IQualifier, getv(_parent( obj ), MA_Iconview_Qualifier),
					MA_ActionDispatcher_Event, ACTION_EVENT_DOUBLECLICK,
					MA_ActionDispatcher_SrcURI, getv(_parent( obj ), MA_View_Path),
					MA_ActionDispatcher_SrcID, getv(_win( obj ), MA_Window_ID),
					MA_ActionDispatcher_RefWin, _win(obj),
					TAG_DONE
				);

				{
					UBYTE encpath[mimeuri_encodepath(NULL, 0, data->path)];

					mimeuri_encodepath(encpath, sizeof(encpath), data->path);

					snprintf(buf, sizeof(buf), "file:///%s?left=%ld&top=%ld&width=%ld&height=%ld&mode=%s&view=%s",
						encpath,
						data->win_x,
						data->win_y,
						data->win_xs,
						data->win_ys,
						mode,
						view);
				}
				PDB(("!!!!!!!!!!!!!!!!!%s\n",buf));
				DoMethod( dispatcher, MM_ActionDispatcher_AddURI, buf, TRUE );
					PDB(("!!!!!!!!!!!!!!!!!%s\n",buf));
				DoMethod( dispatcher, MM_ActionDispatcher_Execute );
			}
		}
		else
		{
			/*  XXX: what if the path contains quotes itself?
			 */

			snprintf(buf, sizeof(buf), "Run \"%s\"", data->path);
			execute_command(obj, AC_INTERNAL , buf, NULL);
		}
	}

	/* XXX: in the arexx, check if it's internal and if it is, get the id */

	return (0);
}


DEFSMETHOD(Icon_ErrorString)
{
	GETDATA;

	if (data->errorstr)
	{
		icon_free(data->errorstr);
	}

	if ( (data->errorstr = icon_malloc(strlen(msg->error) + 1)) )
	{
		strcpy(data->errorstr, msg->error);
		D(ICONIO,bug("ErrorString: <%s>\n", data->errorstr));
	}

	return (0);
}

static ULONG get_device_properties( struct Data *data )
{
	struct dlcnode *dlc; /* depending on the device we hide "Eject" and "Format" */
	ULONG flags = 0;
	
	flags |= AS_DEVICE;
	
	doslistcache_lock();
	if( (dlc = doslistcache_find_dlcdevice_by_volumename( data->path )) )
	{
		flags |= AS_FORMATABLE;
		if( (dlc->flags & DLF_REMOVABLE) )
		{
			flags |= AS_REMOVABLE;
		}
		else if( (dlc->flags & DLF_UNMOUNTABLE) )
		{
			flags |= AS_UNMOUNTABLE;
		}
	}

	doslistcache_unlock();

	if (is_trashcan(data->path))
		flags |= AS_ISTRASHCAN;
	
	return flags;
}

static int bitcount(int val)
{
	int s = 31, bits = 0;

	do
	{
		if ( val & ( 1 << s ) )
			bits++;
	} while(s--);

	return bits;
}

DEFMMETHOD(ContextMenuBuild)
{
	GETDATA;

	data->parentcm = FALSE;

	if (!data->infowin)
	{
		if (_isinobject2(_parent(obj), msg->mx, msg->my)) /* XXX: won't work in iconinfo */
		{
			ULONG flags;

			if (data->cmenu)
			{
				MUI_DisposeObject(data->cmenu);
				data->cmenu = NULL;
			}

			flags  = getv(_parent(obj), MA_View_ShowDevices) ? AS_DEVICEVIEW : 0;
			flags |= AS_ICONVIEW;

			if (data->type == MV_Icon_Type_AppIcon)
			{
                flags |= AS_APPICON;

				data->cmenu = contextmenu_build(CM_APPICON, flags);
				return ((ULONG)data->cmenu);
			}

			if (data->selected && getv(_parent(obj), MA_View_NumSelected) > 1)
			{
				LONG isshortcut = TRUE;

				/*
				 * Grouped mode.
				 */

				FORCHILD(_parent(obj), MUIA_Group_ChildList)
				{
					if (getv(child, MA_Icon_Selected))
					{
						ULONG type = getv(child, MA_Icon_Type);
						isshortcut &= getv(child, MA_Icon_IsShortcut);

						switch (type)
						{

							case MV_Icon_Type_Disk:
							case MV_Icon_Type_Device:
								flags |= get_device_properties( INST_DATA( cl, child ) );
								flags |= icon_flags_for_path(data->path, type);
								break;

							case MV_Icon_Type_Drawer:
								flags |= AS_DRAWER;
								flags |= icon_flags_for_path(data->path, type);
								break;

							case MV_Icon_Type_Tool:
								flags |= AS_TOOL;
								flags |= icon_flags_for_path(data->path, type);
								break;

							case MV_Icon_Type_Project:
								flags |= AS_PROJECT;
								flags |= icon_flags_for_path(data->path, type);
								break;

							case MV_Icon_Type_AppIcon:
								flags |= AS_APPICON;
								break;

							case MV_Icon_Type_MyComputer:
								flags |= AS_MYCOMPUTER;
								break;

							case MV_Icon_Type_View:
								flags |= AS_ICONIFIED_VIEW;
								break;

							default:
								PDB(("huh.. no type..\n"));
								flags |= AS_TOOL; /* XXX: workaround.. */
								break;
						}
					}
				}
				NEXTCHILD

				if (isshortcut)
					flags |= AS_SHORTCUT;

				/*
				 * build mimtype-dependant part of menu if all entries share same type.
				 * for root view we do things bit different. only allow groupped menu if all selected
				 * entries are device/drawer/tool/project.
				 */

				if (getv(_view(obj), MA_View_IsRoot))
				{
					LONG strippedflags = flags & ~(AS_DEVICEVIEW | AS_ICONVIEW | AS_FORMATABLE | AS_REMOVABLE | AS_UNMOUNTABLE | AS_SHORTCUT);
					if ((flags & (AS_ICONIFIED_VIEW | AS_MYCOMPUTER | AS_APPICON)) || bitcount(strippedflags) > 1)
						return 0;
				}

				data->cmenu = contextmenu_build(CM_ICONGROUP, flags);

				if (data->cmenu != NULL)
				{
					struct MinList *l;
					struct typescannernode *tn;
					LONG cnt = 0;
					ULONG globaltype;

					data->cmgrouped = TRUE;

					/* check if we allow type-specific actions */

					l = malloc( sizeof( struct MinList ) );
					if( l )
					{
						if( data->path && (tn = malloc(sizeof(*tn) + strlen(data->path) + 1)) )
						{
							Object *mimeTypeObject = DoMethod(NewObject(getmimetypeclass(), NULL, TAG_DONE), OM_RETAIN);

							NEWLIST(l);

							/* list with names to check. first is a reference one */

							tn->obj = obj;
							strcpy(tn->path, data->path);
							ADDTAIL(l, tn);

							FORCHILD(_parent(obj), MUIA_Group_ChildList)
							{
								if ( child != obj && getv(child, MA_Icon_Selected ) )
								{
									STRPTR p = (STRPTR)getv(child, MA_Icon_Path);

									if( (tn = malloc(sizeof(*tn) + strlen(p) + 1)) )
									{
										tn->obj = child;
										strcpy(tn->path, p);
										ADDTAIL(l, tn);
									}
								}
							}
							NEXTCHILD

							if (do_action(obj, TA_MimeType_Scan,
										TT_MimeType_Scan_List, l,
										TT_MimeType_Scan_MatchFirst, TRUE,
										TT_MimeType_Scan_MimetypeObject, mimeTypeObject,
										TAG_DONE))
							{
								l = NULL; /* thread will free nodes and the list */

								/*
								 * We give it max 2s to check the conditions.
								 */

								while( cnt < 50 && !xget(mimeTypeObject, MA_Mimetype_TypeResolved) )
								{
									Delay( 2 );
									cnt++;
									methodstack_check( FALSE );
								}
							}
							else
							{
								struct typescannernode *nexttn;

								ITERATELISTSAFE(tn, nexttn, l)
								{
									free(tn);
								}
							}

							/* */

							/* Add global menu */
							if((flags & AS_DRAWER) && ((flags & (AS_TOOL | AS_PROJECT)) == 0))
							{
								globaltype = MENU_GLOBALACTION_DIRECTORY;
							}
							else
							{
								globaltype = MENU_GLOBALACTION_FILE;
							}

							contextmenu_add_global( data->cmenu, NULL, globaltype );

							/* Add mimetype menu */
							if ( xget(mimeTypeObject, MA_Mimetype_Type) != NULL )
							{
								D(MIMETYPE,bug("Common mimetype:%s\n", ((struct internal_mimetype_node *)xget(mimeTypeObject, MA_Mimetype_Type))->mimetype ));
								contextmenu_add_mime( data->cmenu, NULL, xget(mimeTypeObject, MA_Mimetype_Type) );
							}

							DoMethod(mimeTypeObject, OM_RELEASE);
						}

						free(l);
					}
				}

				if (data->cmenu != NULL)
					return ((ULONG)data->cmenu);
			}
			else
			{
				if (data->is_shortcut)
				{
					flags |= AS_SHORTCUT;

					if ( data->type == MV_Icon_Type_Disk || data->type == MV_Icon_Type_Device )
						flags |= get_device_properties(data);
				}
				else
				{
					switch (data->type) /* XXX: type or filetype? tricky question.. */
					{
						case MV_Icon_Type_Disk:
						case MV_Icon_Type_Device:
							flags |= get_device_properties( data );
							flags |= icon_flags_for_path(data->path, data->type);
							break;

						case MV_Icon_Type_Drawer:
							flags |= AS_DRAWER;
							flags |= icon_flags_for_path(data->path, data->type);
							break;

						case MV_Icon_Type_Tool:
							flags |= AS_TOOL;
							flags |= icon_flags_for_path(data->path, data->type);
							break;

						case MV_Icon_Type_Project:
							flags |= AS_PROJECT;
							flags |= icon_flags_for_path(data->path, data->type);
							break;

						case MV_Icon_Type_AppIcon:
							flags |= AS_APPICON;
							break;

						case MV_Icon_Type_MyComputer:
							flags |= AS_MYCOMPUTER;
							break;

						case MV_Icon_Type_View:
							flags |= AS_ICONIFIED_VIEW;
							break;

						default:
							PDB(("huh.. no type..\n"));
							flags |= AS_TOOL; /* XXX: workaround.. */
							break;
					}
				}

				if ( (data->cmenu = contextmenu_build(CM_ICON, flags)) )
				{
					if (data->type == MV_Icon_Type_MyComputer)
					{
						/* XXX: anything more to add ? */	 
					}
					else if(data->type == MV_Icon_Type_View)
					{
						/* XXX: anything more to add ? */					 
					}
					else
					{
						TEXT buf[ 1024 ];
						CONST_STRPTR mode = "";
						CONST_STRPTR view = "ICON";
						LONG viewmode;
						APTR mimetype = data->mimetype;
						ULONG globaltype;

						/*
						 * Add type dependant actions to context menu.
						 */

						if ( !data->isdefault )
						{
							/* for nondefault use stored viewmode */

							viewmode = data->viewmode;

							if ( viewmode == MV_Icon_ViewMode_IconAll )
								mode = "ALL";
							else if ( viewmode == MV_Icon_ViewMode_Thumbs )
								mode = "THUMBS";
							else if ( viewmode == MV_Icon_ViewMode_Lister )
								mode = "LIST";
							else
								mode = "ICON";

							/* XXX: sigh */
							if(viewmode == MV_Icon_ViewMode_Lister)
							{
								view = "LIST";
								mode = "ALL";
							}
						}
						else
						{
							if ( getv( _parent( obj ), MA_View_IsRoot )  || !_conf( window_inherit_viewmode ) )
							{
								switch(_conf(window_default_view))
								{
									default:
									case 0:
										view = "ICON";
										break;
									case 1:
										view = "LIST";
										break;
								}

								switch(_conf(window_default_viewmode))
								{
									default:
									case 0:
										mode = "ICONS";
										break;
									case 1:
										mode = "ALL";
										break;
									case 2:
										mode = "THUMBS";
								}
							}
							else
							{
								/* for default icons, we inherit viewmode of iconviewclass */
								int viewindex = getv( _parent( obj ), MA_View_ModeIndex );

								/* get viewname for index */

								struct viewnode *vn = viewapi_findbymime( (STRPTR)getv( _parent( obj ), MA_View_MIME ) );
								mode = viewapi_getmodename( vn, viewindex );
							}
						}

						if ( !mimetype )
						{
							/*
							 * Needs to be found. This is a bit problematic as it should be done on
							 * a thread but for now we do this workaround.
							 */

							LONG cnt = 0;
							Object *mimeTypeObject = DoMethod(NewObject(getmimetypeclass(), NULL, TAG_DONE), OM_RETAIN);

							if (do_action(obj, TA_MimeType_Scan,
										TT_MimeType_Scan_Path, data->path,
										TT_MimeType_Scan_MimetypeObject, mimeTypeObject,
										TAG_DONE))
							{
								/*
								 * We give it max 1s to find the type.
								 */

								while( cnt < 25 && !xget(mimeTypeObject, MA_Mimetype_TypeResolved) )
								{
									Delay( 2 );
									cnt++;
									methodstack_check( FALSE );
								}
							}

							mimetype = xget(mimeTypeObject, MA_Mimetype_Type);
							DoMethod(mimeTypeObject, OM_RELEASE);
						}

						if ( mimetype )
						{
							{
								UBYTE encpath[mimeuri_encodepath(NULL, 0, data->path)];

								mimeuri_encodepath(encpath, sizeof(encpath), data->path);

								snprintf(buf, sizeof(buf), "file:///%s?left=%ld&top=%ld&width=%ld&height=%ld&mode=%s&view=%s",
									encpath,
									data->win_x,
									data->win_y,
									data->win_xs,
									data->win_ys,
									mode,
									view );
							}

							if(flags & AS_DRAWER)
							{
								globaltype = MENU_GLOBALACTION_DIRECTORY;
							}
							else if(flags & AS_DEVICE)
							{
								globaltype = MENU_GLOBALACTION_DEVICE;
							}
							else
							{
								globaltype = MENU_GLOBALACTION_FILE;
							}

							contextmenu_add_global( data->cmenu, buf, globaltype );

							contextmenu_add_mime( data->cmenu, buf, mimetype );
						}
					}

					data->cmgrouped = FALSE;
					return ((ULONG)data->cmenu);
				}
			}
			errormsg(ERR_NOMEM);
		}
		else
		{
			/*
			 * Defer to iconview.
			 */
			data->parentcm = TRUE;
			return (DoMethod(_parent(obj), MUIM_ContextMenuBuild, msg->mx, msg->my));
		}
	}

	return (0);
}


DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;
	APTR parent = _parent(obj);

	if (data->parentcm)
	{
		return (DoMethod(parent, MUIM_ContextMenuChoice, msg->item));
	}
	else
	{
		contextmenu_execute(obj, parent, msg->item, data->cmgrouped);
	}
	return (0);
}


DEFSMETHOD(Icon_AddAncillary)
{
	GETDATA;

	switch (msg->type)
	{
		case MV_Icon_Ancillary_Gadget:
			ASSERT(!data->gad);
			if ( (data->gad = icon_malloc(sizeof(*data->gad))) )
			{
				memcpy(data->gad, msg->data, sizeof(*data->gad));
				return (TRUE);
			}
			break;

		case MV_Icon_Ancillary_ImageNormal:
			ASSERT(!data->im1.size);
			memcpy(&data->im1.img, msg->data, sizeof(struct Image));
			if ( (data->im1.img.ImageData = icon_malloc(msg->size)) )
			{
				data->im1.size = msg->size;
				CopyMem((UBYTE *)*(ULONG *)((UBYTE *)msg->data + offsetof(struct Image, ImageData)), data->im1.img.ImageData, msg->size);
				return (TRUE);
			}
			break;

		case MV_Icon_Ancillary_ImageSelected:
			ASSERT(!data->im2.size);
			memcpy(&data->im2.img, msg->data, sizeof(struct Image));
			if ( (data->im2.img.ImageData = icon_malloc(msg->size)) )
			{
				data->im2.size = msg->size;
				CopyMem((UBYTE *)*(ULONG *)((UBYTE *)msg->data + offsetof(struct Image, ImageData)), data->im2.img.ImageData, msg->size);
				return (TRUE);
			}
			break;

		case MV_Icon_Ancillary_NewiconNormalTT:
			{
				struct iconchunk *chunk;
				ULONG size = msg->size;

				if (!size)
				{
					size = strlen(msg->data) + 1;
				}

				if ( (chunk = icon_malloc(sizeof(*chunk) + size)) )
				{
					chunk->size = size;
					CopyMem(msg->data, chunk->data, size);
					ADDTAIL(&data->ni1_tt, chunk);
					return (TRUE);
				}
			}
			break;

		case MV_Icon_Ancillary_NewiconSelectedTT:
			{
				struct iconchunk *chunk;
				ULONG size = msg->size;

				if (!size)
				{
					size = strlen(msg->data) + 1;
				}

				if ( (chunk = icon_malloc(sizeof(*chunk) + size)) )
				{
					chunk->size = size;
					CopyMem(msg->data, chunk->data, size);
					ADDTAIL(&data->ni2_tt, chunk);
					return (TRUE);
				}
			}
			break;

		case MV_Icon_Ancillary_Glowicon_Chunk:
			{
				struct iconchunk *chunk;

				if ( (chunk = icon_malloc(sizeof(*chunk) + msg->size)) )
				{
					chunk->size = msg->size;
					CopyMem(msg->data, chunk->data, msg->size);
					ADDTAIL(&data->glow_chunk, chunk);
					return (TRUE);
				}
			}
			break;

		case MV_Icon_Ancillary_PNGicon_Chunk:
			{
				struct iconchunk *chunk;

				if ( (chunk = icon_malloc(sizeof(*chunk) + msg->size)) )
				{
					chunk->size = msg->size;
					CopyMem(msg->data, chunk->data, msg->size);
					ADDTAIL(&data->png_chunk, chunk);
					return (TRUE);
				}
			}
			break;

		case MV_Icon_Ancillary_PNGicon_Ctx:
			{
				ASSERT(!data->png_ctx);

				data->png_ctx = msg->data;
				return (TRUE);
			}
			break;
			
		case MV_Icon_Ancillary_SVGDoc:
			{
				ASSERT(!data->svgdoc);

				data->svgdoc = msg->data;
				return (TRUE);
			}
			break;
			
		case MV_Icon_Ancillary_VGObj:
			{
				ASSERT(!data->vgobj);

				data->vgobj = msg->data;
				return (TRUE);
			}
			break;

		#ifdef DEBUG
		default:
			PDB(("hum, no ancillary handling for that\n"));
			break;
		#endif
	}
	return (FALSE); /* XXX */
}


DEFSMETHOD(Icon_GetAncillary)
{
	GETDATA;

	switch (msg->type)
	{
		case MV_Icon_Ancillary_Gadget:
			if (data->gad)
			{
				if (msg->ptr)
				{
					memcpy(msg->ptr, data->gad, sizeof(*data->gad)); /* XXX: I know this is lame but.. */
				}
				if (msg->size)
				{
					*msg->size = sizeof(*data->gad);
				}
				return (TRUE);
			}
			break;

		case MV_Icon_Ancillary_ImageNormal:
			if (data->im1.img.ImageData)
			{
				if (msg->ptr)
				{
					*msg->ptr = (UBYTE *)&data->im1.img;
				}
				if (msg->size)
				{
					*msg->size = data->im1.size;
				}
				return (TRUE);
			}
			else
			{
				if (msg->ptr)
				{
					*msg->ptr = 0;
				}
				if (msg->size)
				{
					*msg->size = 0;
				}
			}
			break;

		case MV_Icon_Ancillary_ImageSelected:
			if (data->im2.img.ImageData)
			{
				if (msg->ptr)
				{
					*msg->ptr = (UBYTE *)&data->im2.img;
				}
				if (msg->size)
				{
					*msg->size = data->im2.size;
				}
				return (TRUE);
			}
			else
			{
				if (msg->ptr)
				{
					*msg->ptr = 0;
				}
				if (msg->size)
				{
					*msg->size = 0;
				}
			}
			break;

		case MV_Icon_Ancillary_NewiconNormalTT:
			if (msg->ptr)
			{
				*msg->ptr = (UBYTE *)&data->ni1_tt;
			}
			if (msg->size)
			{
				*msg->size = sizeof(data->ni1_tt);
			}
			return (TRUE);

		case MV_Icon_Ancillary_NewiconSelectedTT:
			if (msg->ptr)
			{
				*msg->ptr = (UBYTE *)&data->ni2_tt;
			}
			if (msg->size)
			{
				*msg->size = sizeof(data->ni2_tt);
			}
			return (TRUE);

		case MV_Icon_Ancillary_Glowicon_Chunk:
			if (msg->ptr)
			{
				*msg->ptr = (UBYTE *)&data->glow_chunk;
			}
			if (msg->size)
			{
				*msg->size = sizeof(data->glow_chunk);
			}
			return (TRUE);

		case MV_Icon_Ancillary_PNGicon_Chunk:
			if (msg->ptr)
			{
				*msg->ptr = (UBYTE *)&data->png_chunk;
			}
			if (msg->size)
			{
				*msg->size = sizeof(data->png_chunk);
			}
			return (TRUE);

		case MV_Icon_Ancillary_PNGicon_Ctx:
			if (msg->ptr)
			{
				*msg->ptr = data->png_ctx;
			}
			return (TRUE);
			
		case MV_Icon_Ancillary_SVGDoc:
			if (msg->ptr)
			{
				*msg->ptr = data->svgdoc;
			}
			return (TRUE);
			
		case MV_Icon_Ancillary_VGObj:
			if (msg->ptr)
			{
				*msg->ptr = data->vgobj;
			}
			return (TRUE);
	}

	return (FALSE);
}


DEFSMETHOD(Rexx_Snapshot)
{
	GETDATA;

	if (data->isdefault)
	{
		return (0); /* XXX: we don't snapshot those for now.. maybe this will change later (create an icon?) */
	}

	ASSERT(!data->is_shortcut);

	do_action(obj, TA_Icon_Snapshot,
		TT_Icon_Snapshot_Path, data->pathinfo,
		TT_Icon_Snapshot_X, data->x - POSX_CORRECTION,
		TT_Icon_Snapshot_Y, data->y - POSY_CORRECTION,
	TAG_DONE); /* XXX: we should get the retval to update the bubble help */

	return (0);
}


DEFSMETHOD(Rexx_Unsnapshot)
{
	GETDATA;

	if (data->isdefault)
	{
		return (0); /* XXX */
	}

	ASSERT(!data->is_shortcut);

	do_action(obj, TA_Icon_Unsnapshot,
		TT_Icon_Snapshot_Path, data->pathinfo,
	TAG_DONE); /* XXX: we should get the retval to update the bubble help */

	return (0);
}


DEFTMETHOD(Rexx_Rename)
{
	GETDATA;

#warning "Inline rename disabled until it's fixed"
	return(FALSE);

	if (data->editobj)
	{
		if (_parent(data->editobj))
		{
			DoMethod(obj, MM_EditString_EditStop, NULL);
		}

		MUI_DisposeObject(data->editobj);
		data->editobj = NULL;
	}

	if ( (data->editobj = NewObject(geteditstringclass(), NULL,
		MUIA_Font, _font(obj),
		MUIA_LeftEdge, getv(obj, MUIA_LeftEdge),
		MUIA_TopEdge, getv(obj, MUIA_TopEdge) + data->bm_max_ys + data->fontspace + 1,
		MUIA_Width, getv(obj, MUIA_Width),
		MUIA_String_Contents, data->iconname,
		MA_EditString_ReportObj, obj,
		MUIA_String_Reject, "/:",
		MUIA_String_MaxLen, NAME_SIZE,
		TAG_DONE)
	) )
	{
		DoMethod(_parent(obj), MUIM_Group_InitChange);
		DoMethod(_parent(obj), OM_ADDMEMBER, data->editobj);
		DoMethod(_parent(obj), MUIM_Group_ExitChange);

		set(_win(obj), MUIA_Window_ActiveObject, data->editobj);

		data->editmode = EDITMODE_STRING;

		return (TRUE);
	}
	return (FALSE);
}


DEFSMETHOD(EditString_EditStop)
{
	GETDATA;
	APTR p = _parent(obj);

	ASSERT(data->editobj);

	if (msg->s && *msg->s)
	{
		ASSERT(data->path);

		if ( (data->editname = name_build(msg->s)) )
		{
			if (do_action(obj, TA_File_Rename,
				TT_File_Rename_Path, data->path,
				TT_File_Rename_Name, msg->s,
			TAG_DONE))
			{
				data->editmode = EDITMODE_IO;
			}
		}
		/* XXX: etc.. */
	}

	DoMethod(p, MUIM_Group_InitChange);
	DoMethod(p, OM_REMMEMBER, data->editobj);
	DoMethod(p, MUIM_Group_ExitChange);

	if (data->editmode == EDITMODE_STRING)
	{
		data->editmode = EDITMODE_NONE;
	}

	set(obj, MA_Icon_Highlighted, FALSE);

	return (0);
}


DEFSMETHOD(Thread_Finished)
{
	GETDATA;

	switch (msg->action)
	{
		case TA_File_Rename:
		{
			ASSERT(data->editname);
			name_delete(data->editname);
		}
		break;

		case TA_Icon_Unsnapshot:
		{
			/* XXX: A little ugly, and reloads all icons... Should be reworked later. */
			DoMethod(app, MM_Application_ReloadIcons, TRUE);
		}
		break;
	}
	return (0);
}


DEFDISPOSE
{
	struct ttnode *tn, *nexttn;
	struct iconchunk *chunk, *nextchunk;
	GETDATA;
	ASSERT(!data->bm_effect[BMEFFECT_DRAGDROP]);

	if (data->type == MV_Icon_Type_MyComputer)
	{
		dprefs_mymorphos_notify_setobj(NULL);
	}

	if (data->tbctx)
	{
		textbox_delete(data->tbctx);
		data->tbctx = NULL;
	}

	if (data->tbctx2)
	{
		textbox_delete(data->tbctx2);
		data->tbctx2 = NULL;
	}

	if (data->png_ctx)
	{
		pngio_delete(data->png_ctx);
	}
	
	if (data->svgdoc)
	{
		XMLDoc_free(data->svgdoc);
		FreeMem(data->svgdoc, sizeof(XMLDoc));
	}
	
	if (data->vgobj)
	{
		//struct Library *VGraphicsBase = OpenLibrary("vgraphics.library", 0);

		if(VGraphicsBase)
		{
			VG_DisposeVGObject(data->vgobj);
		}
	}

	if (data->editobj && !_parent(data->editobj))
	{
		MUI_DisposeObject(data->editobj);
	}

	if (data->cmenu)
	{
		MUI_DisposeObject(data->cmenu);
	}

	if (data->errorstr)
	{
		icon_free(data->errorstr);
	}

	if (data->path)
	{
		name_delete(data->path);
	}

	if (data->pathinfo)
	{
		name_delete(data->pathinfo);
	}

	if (data->iconpath)
	{
		name_delete(data->iconpath);
	}

	if (data->iconname)
	{
		name_delete(data->iconname);
	}

	if (data->additionaltext)
	{
		name_delete(data->additionaltext);
	}

	if (data->defaulttool)
	{
		icon_free(data->defaulttool);
	}

	ITERATELISTSAFE(tn, nexttn, &data->ttlist)
	{
		icon_free(tn);
	}

	if ( data->reference_icon )
	{
		set( data->reference_icon, MA_Icon_ReferenceCount, MV_Icon_ReferenceCount_Decrease );
	}

	if ( !data->reference_icon )
	{
		delete_specialbm(obj, cl, BMEFFECT_ALL);
		delete_specialbm(obj, cl, BMEFFECT_DRAGDROP); /* unlikely but well.. */
	}

	if ( !data->reference_icon )
	{
		gfx_bitmap_delete(data->bm1_real);

		if (data->scaled)
		{
			ASSERT(data->bm1);
			gfx_bitmap_delete(data->bm1);
			gfx_bitmap_delete(data->bm2);
		}

		gfx_bitmap_delete(data->bm2_real);
	}

	/* when the icon is in infomode.. XXX: set that mode somewhere ? */
	if (data->gad)
	{
		icon_free(data->gad);
	}

	if (data->im1.size)
	{
		icon_free(data->im1.img.ImageData);
	}

	if (data->im2.size)
	{
		icon_free(data->im2.img.ImageData);
	}

	#if USE_NEWICONS
	ITERATELISTSAFE(chunk, nextchunk, &data->ni1_tt)
	{
		icon_free(chunk);
	}

	ITERATELISTSAFE(chunk, nextchunk, &data->ni2_tt)
	{
		icon_free(chunk);
	}
	#endif

	#if USE_GLOWICONS
	ITERATELISTSAFE(chunk, nextchunk, &data->glow_chunk)
	{
		icon_free(chunk);
	}
	#endif

	#if USE_PNGICONS
	ITERATELISTSAFE(chunk, nextchunk, &data->png_chunk)
	{
		icon_free(chunk);
	}
	#endif

	icon_count--;
	icon_free_dbuf();

	return DOSUPER;
}


DEFSMETHOD(Icon_Click)
{
	GETDATA;

	if (DoubleClick(data->seconds, data->micros, msg->seconds, msg->micros))
	{
		data->doubleclick = TRUE;
		data->seconds = 0;
		data->micros = 0;
	}
	else
	{
		data->doubleclick = FALSE;
		data->seconds = msg->seconds;
		data->micros = msg->micros;
	}

	return (0);
}


BEGINMTABLE
DECNEW
DECSET
DECGET
DECMMETHOD(AskMinMax)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(Draw)
DECTMETHOD(Icon_UpdateBitMapBuffer)
DECTMETHOD(Icon_UpdateBitMapBuffer2)
DECTMETHOD(Icon_FadeIn)
DECSMETHOD(Icon_Generate)
DECMMETHOD(CreateDragImage)
DECMMETHOD(DeleteDragImage)
DECMMETHOD(DragQuery)
DECMMETHOD(DragBegin)
DECMMETHOD(DragFinish)
DECMMETHOD(DragReport)
DECMMETHOD(DragDrop)
DECMMETHOD(DragEvent)
DECMMETHOD(ContextMenuBuild)
DECMMETHOD(ContextMenuChoice)
DECSMETHOD(Icon_AddAncillary)
DECSMETHOD(Icon_GetAncillary)
DECSMETHOD(Icon_InsertToolType)
DECTMETHOD(Icon_ClearToolTypes)
DECSMETHOD(Icon_AddBitMap)
DECSMETHOD(Icon_CreateBitMap)
DECSMETHOD(Icon_ErrorString)
DECSMETHOD(Icon_GetToolTypes)
DECTMETHOD(Icon_End)
DECSMETHOD(Icon_Select)
DECTMETHOD(Icon_SetIcon)
DECSMETHOD(Icon_Scale)
DECSMETHOD(Rexx_Snapshot)
DECSMETHOD(Rexx_Unsnapshot)
DECTMETHOD(Rexx_Rename)
DECSMETHOD(EditString_EditStop)
DECSMETHOD(Icon_Click)
DECSMETHOD(Thread_Finished)
DECDISP
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, iconclass)
