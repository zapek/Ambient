/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
 * Copyright 2005-2015 Ambient Open Source Team
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
 * $Id: statusbarclass.c,v 1.21 2018/08/12 20:53:15 itix Exp $
 */

#include "ambient.h"

/* public */
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <proto/dos.h>

/* private */
#include "mui_func.h"
#include "capacity.h"
#include "prefs.h"
#include "background.h"
#include "common_picture.h"
#include "gfx_bitmap.h"
#include "gfx_pen.h" // bitRocky
#include "prefs.h"
#include "ambient_cat.h"
#include "legacy.h"
#include "prefs_advanced.h"


#define SB_TOTALFILES        "%tf%"
#define SB_TOTALDIRS         "%td%"
#define SB_ICONS             "%i%"
#define SB_LINKS             "%l%"
#define SB_DISKUSAGE         "%du%"
#define SB_NUMSELECTED       "%ns%"
#define SB_SELECTEDDISKUSAGE "%sdu%"

struct Data {
	STRPTR text;
	LONG top;
	ULONG totalfiles;
	ULONG totaldirs;
	ULONG viewobjects;
	ULONG links;
	ULONG icons;
	ULONG numselected;
	ULONG fontcol;
	BOOL set;
	UQUAD diskusage;
	UQUAD selecteddiskusage;
	struct bitmap_ctx *bm[2];
	LONG mode;
	LONG dirty;
	ULONG pushid;
	
	APTR sliIconZoom;
};


static void setstatus(struct RastPort *rp, APTR obj, struct Data *data, LONG ox, LONG oy, LONG width, LONG height)
{
	ULONG textlen, numchars;
	LONG textheight;
	struct TextExtent te;

	SetRPAttrs(rp,
		RPTAG_Font, _font(obj),
		RPTAG_PenMode, FALSE,
		RPTAG_FgColor, data->fontcol,
		RPTAG_DrMd, JAM1,
	TAG_DONE);

	textlen = strlen(data->text);

	TextExtent(rp, data->text, textlen, &te);
	textheight = te.te_Extent.MaxY - te.te_Extent.MinY + 1;

	Move(rp, ox, oy + (height - textheight) / 2 +  _font(obj)->tf_Baseline);

	numchars = TextFit(rp, data->text,
		textlen, &te, NULL, 1,
		width - 4, height
	);

	Text(rp, data->text, numchars < textlen && numchars > 3 ?
		numchars - 3 : numchars
	);

	if (numchars < textlen)
		Text(rp, "...", 3);
}

static void setbg(struct RastPort *rp, APTR obj, int x, int y, int w, int h)
{
	APTR dtp = (APTR) getv(_win(obj), MA_Window_DTP);
	APTR bm = dtp ? picture_getattr(dtp, PICTURE_BITMAP) : 0;
	ULONG bgpen = getv(_win(obj), MA_Window_BgPen);
	ULONG bgmode = getv(_win(obj), MA_Window_BGMode);

	int so_x = x % (bm ? gfx_bitmap_width(bm) : 0);
	int so_y = y % (bm ? gfx_bitmap_height(bm) : 0);

	background_blit(bgmode, bm, bgpen, so_x, so_y, rp, 0, 0, w, h);
}

#if 0 // bitRocky: not needed anymore, there is now a prefs item for the color
static ULONG getfontcol(struct RastPort *rp, int x, int y, int w, int h)
{
	int ix, iy, count = 0;
	ULONG colsum_r = 0, colsum_g = 0, colsum_b = 0;
	ULONG pixels[ 128 ];

	ASSERT( w<=128 )

	if ( w > 128 )
		w = 128;

	for (iy = y; iy < h + y; iy++)
	{
		ReadPixelArray( pixels, 0, 0, w * 4, rp, x, iy, w, 1, RECTFMT_RGBA );

		for (ix = 0; ix < w; ix++)
		{
			ULONG pixel = pixels[ ix ];

			colsum_r += (pixel >> 24) & 0xff;
			colsum_g += (pixel >> 16) & 0xff;
			colsum_b += (pixel >> 8 ) & 0xff;
		}
	}

	count = w * h;

	/* XXX: Dunno if theese values are the right ones to choose... but... well ... :) */
	if ((colsum_r / count) > 90 || (colsum_g / count) > 90 || (colsum_b / count) > 90)
		return 0x00000000;
	else
		return 0x00eeeeee;
}
#endif
DEFNEW
{
	APTR sliIconZoom;

	obj = DoSuperNew(cl, obj,
		GroupSpacing(0),
		InnerSpacing(0,0),
		MUIA_Font, MUIV_Font_Tiny,
		MUIA_Frame, MUIV_Frame_None,
		MUIA_FillArea, FALSE,
		MUIA_Group_Horiz, TRUE,
		MUIA_CustomBackfill, TRUE,
		Child, HVSpace,
		//Child, BalanceObject, End,
		Child, sliIconZoom = NewObject(geticonzoomsliderclass(), NULL,
			MUIA_Weight, 20,
			MUIA_ShowMe, FALSE,
			MUIA_Numeric_Min, -20,
			MUIA_Numeric_Max, 128,
			MUIA_Numeric_Value, 0,
		End,
		Child, HSpace(2),
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data;

		data = INST_DATA(cl, obj);

		if (!(data->text = malloc(STATUSBARTEXTSIZE + 1)))
		{
			CoerceMethod(cl, obj, OM_DISPOSE);
			return (ULONG)NULL;
		}

		data->text[0] = 0;
		data->bm[0] = data->bm[1] = NULL;
		data->set = FALSE;
		data->fontcol = 0x00000000;
		data->mode = MV_Statusbar_SetMode_Icon; /* default mode */
		data->sliIconZoom = sliIconZoom;

		DoMethod(data->sliIconZoom, MUIM_Notify, MUIA_Numeric_Value, MUIV_EveryTime, MUIV_Notify_Window, 2, MM_Window_ZoomIcons, MUIV_TriggerValue);
	}

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	if ( data->pushid )
		DoMethod( app, MUIM_Application_KillPushMethod, obj, data->pushid);

	if (data->text)
		free(data->text);

	gfx_bitmap_delete(data->bm[0]);
	gfx_bitmap_delete(data->bm[1]);

	return(DOSUPER);
}


DEFMMETHOD(Setup)
{
	ULONG rc = DOSUPER;
	GETDATA;

	data->set = TRUE;

	if (rc)
	{
		DoMethod(obj, MM_Statusbar_UpdateTriggers);

		if (data->mode == MV_Statusbar_SetMode_Icon && _aprefs(iconzoomslider))
		{
			APTR viewobj = (APTR)getv(_win(obj), MA_Window_Viewobj);
			APTR currview = (APTR)getv(viewobj, MA_Viewgroup_CurrentView);
			
			DoMethod(currview, MUIM_KillNotifyObj, MA_Icon_SizeAdjustment, data->sliIconZoom);
			
			set(data->sliIconZoom, MUIA_Numeric_Value, getv(currview, MA_Icon_SizeAdjustment));
			DoMethod(currview, MUIM_Notify, MA_Icon_SizeAdjustment, MUIV_EveryTime, data->sliIconZoom, 3,
				MUIM_Set, MUIA_Numeric_Value, MUIV_TriggerValue);
		}
	}

	return rc;
}

static void buildstatusstring(APTR obj UNUSED, struct Data *data)
{
	CONST_STRPTR seek = _conf(window_statusbarformat);
	UBYTE repl[16];
	UBYTE *ptr;
	UBYTE c;
	ULONG len;

	ptr = data->text;
	len = 1;   /* account for terminating '\0' */

	while ((c = *seek))
	{
		repl[0] = '\0';

		if (c == '%')
		{
			if (strncmp(seek, SB_TOTALFILES, strlen(SB_TOTALFILES)) == 0)
			{
				sprintf(repl, "%lu", data->totalfiles);
				seek += strlen(SB_TOTALFILES);
			}
			else if (strncmp(seek, SB_TOTALDIRS, strlen(SB_TOTALDIRS)) == 0)
			{
				sprintf(repl, "%lu", data->totaldirs);
				seek += strlen(SB_TOTALDIRS);
			} 
			else if (strncmp(seek, SB_ICONS, strlen(SB_ICONS)) == 0)
			{
				sprintf(repl, "%lu", data->icons);
				seek += strlen(SB_ICONS);
			}
			else if (strncmp(seek, SB_LINKS, strlen(SB_LINKS)) == 0)
			{
				sprintf(repl, "%lu", data->links);
				seek += strlen(SB_LINKS);
			}
			else if (strncmp(seek, SB_DISKUSAGE, strlen(SB_DISKUSAGE)) == 0)
			{
				capacity_format_size(repl, sizeof(repl), data->diskusage);
				seek += strlen(SB_DISKUSAGE);
			}
			else if (strncmp(seek, SB_NUMSELECTED, strlen(SB_NUMSELECTED)) == 0)
			{
				sprintf(repl, "%lu", data->numselected);
				seek += strlen(SB_NUMSELECTED);
			}
			else if (strncmp(seek, SB_SELECTEDDISKUSAGE, strlen(SB_SELECTEDDISKUSAGE)) == 0)
			{
				capacity_format_size(repl, sizeof(repl), data->selecteddiskusage);
				seek += strlen(SB_SELECTEDDISKUSAGE);
			}
			else
				seek++;
		}
		else
			seek++;

		if (repl[0])
		{
			ULONG sublen;

			sublen = strlen(repl);
			len += sublen;
			if (len > STATUSBARTEXTSIZE)
			{
				break;
			}

			memcpy(ptr, repl, sublen);
			ptr += sublen;
		}
		else
		{
			len++;
			if (len > STATUSBARTEXTSIZE)
			{
				break;
			}

			*ptr++ = c;
		}
	}
	*ptr = '\0';
}


DEFSET
{
	GETDATA;
	LONG rebuild = FALSE;

	FORTAG(INITTAGS)
	{
		case MA_Statusbar_TotalFiles:
			data->totalfiles = tag->ti_Data;
			rebuild = TRUE;
			break;

		case MA_Statusbar_TotalDirs:
			data->totaldirs = tag->ti_Data;
			rebuild = TRUE;
			break;

		case MA_Statusbar_Links:
			data->links = tag->ti_Data;
			rebuild = TRUE;
			break;

		case MA_Statusbar_IconFiles:
			data->icons = tag->ti_Data;
			rebuild = TRUE;
			break;

		case MA_Statusbar_DiskUsage:
			data->diskusage = *(UQUAD *)(tag->ti_Data);
			rebuild = TRUE;
			break;

		case MA_Statusbar_SelectedDiskUsage:
			data->selecteddiskusage = *(UQUAD *)(tag->ti_Data);
			rebuild = TRUE;
			break;

		case MA_Statusbar_NumSelected:
			switch (tag->ti_Data)
			{
				case MV_View_NumSelected_Increase:
					data->numselected++;
					break;

				case MV_View_NumSelected_Decrease:
					data->numselected--;
					break;

				default:
					data->numselected = tag->ti_Data;
					break;
			}
			rebuild = TRUE;
			break;

	}
	NEXTTAG;

	data->dirty |= rebuild;

	if ( data->dirty && data->pushid == 0 )
	{
		data->dirty = 0;
		data->pushid = DoMethod( app, MUIM_Application_PushMethod, obj, 1 | MUIV_PushMethod_Delay(50), MM_Statusbar_Refresh );
	}

	return DOSUPER;
}


DEFMMETHOD(Draw)
{
	struct RastPort rp;
	//static struct RastPort trp;
	ULONG s, wid, hei, widIconZoomSlider=0;
	LONG viewbg;
	GETDATA;

	s = DOSUPER;

	if (!(data->mode == MV_Statusbar_SetMode_List && !getprefslong(DSI_FASTLIST_WINDOW_BG)))
	{
		viewbg = TRUE;
	}
	else
	{
		DoMethod( obj, MUIM_DrawBackground, _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), 0, 0, 0 );
		viewbg = FALSE;
	}

	wid = _width(obj); hei = _height(obj);
	if (msg->flags & MADF_DRAWOBJECT)
	{
		LONG i;
		LONG reallocate;

		if ( data->bm[ 0 ] && gfx_bitmap_width( data->bm[ 0 ] ) == wid + 1 && gfx_bitmap_height( data->bm[ 0 ] ) == hei + 1 )
			reallocate = FALSE;
		else
			reallocate = TRUE;


		for (i = 0; i < 2; i++)
		{
			struct bitmap_ctx *bm = data->bm[ i ];

			if ( reallocate )
			{
				bm = gfx_bitmap_create(wid + 1,
					hei + 1, 32,
					BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
					TAG_DONE);

				gfx_bitmap_delete(data->bm[i]);
				data->bm[i] = bm;
			}

			if ( bm == NULL )
			{
				PDB(("Bitmap creation failed.\n"));
				return 0;
			}
		}

		InitRastPort(&rp);
		rp.BitMap = gfx_bitmap_bm(data->bm[0]);

		if ( viewbg )
		{
			setbg(&rp, obj, _left(obj), _top(obj), wid, hei);

			if ( _top(obj) != data->top )
			{
				data->top = _top(obj);

				data->fontcol = gfx_get_penspec_value(muiRenderInfo(obj), &_conf(window_statusbarcolor)); // bitRocky // orig: getfontcol(&rp, 0, 0, min(wid, 128), hei);
			}

			ProcessPixelArray(&rp, 0, 0, wid, hei, POP_BRIGHTEN, 30, NULL);  // brighten bg
			//ProcessPixelArray(&rp, 0, 2, wid, hei - 2, POP_BRIGHTEN, 30, NULL);  // brighten bg
			/*
			SetAPen(&rp, BLOCKPEN); Move(&rp, 0, 0); Draw(&rp, wid, 0);
			SetAPen(&rp, TEXTPEN); Move(&rp, 0, 1); Draw(&rp, wid, 1);
			*/
			
			/*
			ProcessPixelArray(&rp, 0, 1, wid, 1,       POP_BRIGHTEN, 90, NULL);  // white line
			ProcessPixelArray(&rp, 0, 0, wid, 1,       POP_DARKEN,   90, NULL);  // black line
			*/
			
		}
	}

	/* ...else (and) if MADF_DRAWUPDATE */

	InitRastPort(&rp);
	rp.BitMap = gfx_bitmap_bm(data->bm[1]);

	if (data->mode == MV_Statusbar_SetMode_Icon && _aprefs(iconzoomslider))
	{
		widIconZoomSlider = _width(data->sliIconZoom);
	}

	if ( viewbg )
	{
		BltBitMapRastPort(gfx_bitmap_bm(data->bm[0]), 0, 0, &rp, 0, 0, wid, hei, 0xc0);

		setstatus(&rp, obj, data, 4, 2, wid-widIconZoomSlider, hei - 2);
		ClipBlit(&rp, 0, 0, _rp(obj), _left(obj), _top(obj), wid, hei, 0xc0);
	}
	else
	{
		setstatus(_rp(obj), obj, data, _left(obj) + 4, _top(obj), wid-widIconZoomSlider, hei);
	}
	if (widIconZoomSlider > 0) // if (data->mode == MV_Statusbar_SetMode_Icon && _aprefs(iconzoomslider))
	{
		MUI_Redraw(data->sliIconZoom, MADF_DRAWOBJECT);
	}
	return s;
}

DEFMMETHOD(AskMinMax)
{
	DOSUPER;

	msg->MinMaxInfo->MaxWidth = MUI_MAXMAX; /* XXX */
	msg->MinMaxInfo->MaxHeight = _font(obj)->tf_YSize + 4;
	msg->MinMaxInfo->MinHeight = _font(obj)->tf_YSize + 4;

	return (0);
}


DEFTMETHOD(Statusbar_Refresh)
{
	GETDATA;
	ULONG dirty = data->dirty;

	data->pushid = 0;
	data->dirty = 0;

	if (!data->set)
		return 0;

	buildstatusstring(obj, data);

	set(data->sliIconZoom, MUIA_ShowMe, data->mode == MV_Statusbar_SetMode_Icon && _aprefs(iconzoomslider));

	MUI_Redraw(obj, MADF_DRAWOBJECT);

	/* if state is dirty again, then re-push */

	if (dirty)
		data->pushid = DoMethod(app, MUIM_Application_PushMethod, obj, 1 | MUIV_PushMethod_Delay(50), MM_Statusbar_Refresh);

	return 0;
}


DEFTMETHOD(Statusbar_UpdateBackground)
{
	GETDATA;

	if (data->mode == MV_Statusbar_SetMode_Icon ||
	    (data->mode == MV_Statusbar_SetMode_List && getprefslong(DSI_FASTLIST_WINDOW_BG)))
	{
		SetAttrs(obj,
			MUIA_CustomBackfill, TRUE,
			MUIA_Background, MUII_BACKGROUND,
			MUIA_Frame, MUIV_Frame_None,
			TAG_DONE
		);
	}
	else if (data->mode == MV_Statusbar_SetMode_List && !getprefslong(DSI_FASTLIST_WINDOW_BG))
	{
		SetAttrs(obj,
			MUIA_CustomBackfill, FALSE,
			MUIA_Background, MUII_TextBack,
			MUIA_Frame, MUIV_Frame_Text,
			TAG_DONE
		);
	}

	/*
	 * This will cause a full redraw and recalculation of the font
	 * colour.
	 */
	data->top = -1;
	MUI_Redraw(obj, MADF_DRAWOBJECT);

	return 0;
}

DEFTMETHOD(Statusbar_UpdateTriggers)
{
	APTR viewobj = NULL;
	GETDATA;

	/* If this happens it means MUIM_Setup did not yet happen... */
	if (!data->set)
		return 0;

	if (_win(obj))
		viewobj = (APTR)getv(_win(obj), MA_Window_Viewobj);

	if (viewobj)
	{
		DoMethod(viewobj, MUIM_Notify, MA_View_TotalFiles,        MUIV_EveryTime, obj, 3, MUIM_Set, MA_Statusbar_TotalFiles,          MUIV_TriggerValue);
		DoMethod(viewobj, MUIM_Notify, MA_View_TotalDirs,         MUIV_EveryTime, obj, 3, MUIM_Set, MA_Statusbar_TotalDirs,           MUIV_TriggerValue);
		DoMethod(viewobj, MUIM_Notify, MA_View_Links,             MUIV_EveryTime, obj, 3, MUIM_Set, MA_Statusbar_Links,               MUIV_TriggerValue);
		DoMethod(viewobj, MUIM_Notify, MA_View_IconFiles,         MUIV_EveryTime, obj, 3, MUIM_Set, MA_Statusbar_IconFiles,           MUIV_TriggerValue);
		DoMethod(viewobj, MUIM_Notify, MA_View_DiskUsage,         MUIV_EveryTime, obj, 3, MUIM_Set, MA_Statusbar_DiskUsage,           MUIV_TriggerValue);
		DoMethod(viewobj, MUIM_Notify, MA_View_NumSelected,       MUIV_EveryTime, obj, 3, MUIM_Set, MA_Statusbar_NumSelected,         MUIV_TriggerValue);
		DoMethod(viewobj, MUIM_Notify, MA_View_SelectedDiskUsage, MUIV_EveryTime, obj, 3, MUIM_Set, MA_Statusbar_SelectedDiskUsage,   MUIV_TriggerValue);
	}
	else
	{
		PDB(("Failed to update triggers!\n"));
	}

	return 0;
}

DEFSMETHOD(Statusbar_SetMode)
{
	GETDATA;

	/* This trigger gets preserved between viewtypes so we need to clear it... */
	set(obj, MA_Statusbar_NumSelected, 0);

	if (msg->mode == MV_Statusbar_SetMode_Icon ||
	    (msg->mode == MV_Statusbar_SetMode_List && getprefslong(DSI_FASTLIST_WINDOW_BG)))
		data->mode = msg->mode;

	else if (msg->mode == MV_Statusbar_SetMode_List && !getprefslong(DSI_FASTLIST_WINDOW_BG))
		data->mode = msg->mode;

	DoMethod(obj, MM_Statusbar_UpdateBackground);
	return 0;
}

BEGINMTABLE
DECSET
DECNEW
DECDISP
DECMMETHOD(Setup)
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
DECTMETHOD(Statusbar_UpdateBackground)
DECTMETHOD(Statusbar_UpdateTriggers)
DECSMETHOD(Statusbar_SetMode)
DECSMETHOD(Statusbar_Refresh)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, statusbarclass)
