/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2001-2005 by David Gerber <zapek@morphos.net>
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
 * $Id: iconzoomsliderclass.c,v 1.8 2020/03/25 13:44:47 bitrocky Exp $
 */

#include "ambient.h"

/* public */
#include <libraries/mui.h>
#include <graphics/rpattr.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h> // for ClipBlit()
#include <intuition/intuition.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "gfx_bitmap.h"
#include "legacy.h"

struct Data {
	LONG min, max, val;                 // copies of MUIA_Numeric_Min/Max/Value
	LONG oldVal;                        // the value before you hit the knob and MOUSEMOVE starts
	ULONG x, r;                         // the current x value where the knob is (relative to its _left() value!) and its currenty radius
	ULONG maxX;                         // the max x of the knob
	struct RastPort rpBack;             // rastport for the background
	struct bitmap_ctx *bmBack;          // bitmap for the background
	struct MUI_EventHandlerNode ehnode; // for the catching mousebutton
};


DEFNEW
{
	if ((obj = DoSuperNew(cl, obj,
		MUIA_Font, MUIV_Font_Tiny,
		//MUIA_Frame, MUIV_Frame_Text,
		//MUIA_Numeric_Horiz, TRUE,
		//MUIA_CustomBackfill, TRUE,
		MUIA_FillArea, FALSE,
		//MUIA_Background, "",
		TAG_MORE, INITTAGS)))
	{
		struct Data *data;

		data = INST_DATA(cl, obj);
		
		data->min = GetTagData(MUIA_Numeric_Min, 0, INITTAGS);
		data->max = GetTagData(MUIA_Numeric_Max, 0, INITTAGS);
		data->val = GetTagData(MUIA_Numeric_Value, 0, INITTAGS);
		data->oldVal = -1;
		DB(("data->min/max = %ld/%ld\n", data->min, data->max));
		return ((ULONG)obj);
	}

	return (0);
}//OM_NEW

DEFDISPOSE
{
	GETDATA;

	gfx_bitmap_delete(data->bmBack);

	return(DOSUPER);
}//OM_DISPOSE

DEFMMETHOD(AskMinMax)
{
	DOSUPER;

	msg->MinMaxInfo->MinWidth  += 4;
	msg->MinMaxInfo->MinHeight += 4;
	msg->MinMaxInfo->MaxWidth   = MUI_MAXMAX; /* XXX */
	msg->MinMaxInfo->MaxHeight += _font(obj)->tf_YSize + 4;
	msg->MinMaxInfo->DefWidth  += 10;
	msg->MinMaxInfo->DefHeight += _font(obj)->tf_YSize;

	return (0);
}//MUIM_AskMinMax

#ifndef TI_DATA
	#define TI_DATA tag->ti_Data
#endif
DEFSET
{
	GETDATA;
	
	FORTAG(INITTAGS)
	{ 
		case MUIA_Numeric_Min:   data->min = TI_DATA; break;
		case MUIA_Numeric_Max:   data->max = TI_DATA; break;
		case MUIA_Numeric_Value: data->val = TI_DATA; break;
		
		default:
		break;
	}
	NEXTTAG
		
	return DOSUPER;
}//OM_SET

DEFMMETHOD(Setup)
{
	GETDATA;

	if (!DOSUPER){ return (0); }

	if (muiRenderInfo(obj) && _win(obj))
	{
		data->ehnode.ehn_Object = obj;
		data->ehnode.ehn_Class = cl;
		data->ehnode.ehn_Events = IDCMP_MOUSEBUTTONS;
		data->ehnode.ehn_Priority = 1; /* XXX: don't really know, works also with 0 */
		data->ehnode.ehn_Flags = MUI_EHF_GUIMODE;
		DoMethod(_win(obj), MUIM_Window_AddEventHandler, (ULONG)&data->ehnode);
	}
	return (TRUE);
}

DEFMMETHOD(Cleanup)
{
	GETDATA;

	if (muiRenderInfo(obj) && _win(obj))
	{
		DoMethod(_win(obj), MUIM_Window_RemEventHandler, (ULONG)&data->ehnode);
	}
	return (DOSUPER);
}

// draw a filled circle with the center at cx/cy and radius r
// found the algorithm from https://stackoverflow.com/questions/1201200/fast-algorithm-for-drawing-filled-circles
static void DrawFilledCircle (struct RastPort *rp, LONG cx, LONG cy, LONG r, LONG col);

static void DrawFilledCircle (struct RastPort *rp, LONG cx, LONG cy, LONG r, LONG col)
{
	LONG r2 = r * r;
	LONG area = r2 << 2;
	LONG rr = r << 1;
	LONG i, tx, ty;

	if (col != -1) SetRPAttrs(rp, RPTAG_FgColor, col, TAG_DONE);
	for (i = 0; i < area; i++)
	{
		tx = (i % rr) - r; ty = (i / rr) - r;
		if (tx * tx + ty * ty <= r2)
		{
			WritePixel(rp, cx + tx, cy + ty);
		}
	}
}


DEFMMETHOD(Draw)
{
	struct RastPort *rp = _rp(obj);
	ULONG s, wid, hei, minR, maxR;
	ULONG mousePressed;

	GETDATA;

	s = DOSUPER;

	wid = _width(obj); hei = _height(obj);
	minR = 3; maxR = hei/2-2;

	if (msg->flags & MADF_DRAWOBJECT)
	{
		if ( !data->bmBack || (gfx_bitmap_width(data->bmBack) != wid + 1) || (gfx_bitmap_height(data->bmBack) != hei + 1) )
		{
			if (data->bmBack) gfx_bitmap_delete(data->bmBack);
			data->bmBack = gfx_bitmap_create(wid + 1, hei + 1, 32,
				BITMAPTAG_Format, BITMAPVAL_Format_ARGB32,
				TAG_DONE);
		}

		if (!data->bmBack)
		{
			PDB(("Bitmap creation failed.\n"));
			return 0;
		}

		InitRastPort(&data->rpBack);
		data->rpBack.BitMap = gfx_bitmap_bm(data->bmBack);

		// copy the background from the parent...
		ClipBlit(rp, _left(obj), _top(obj), &data->rpBack, 0, 0, wid, hei, 0xc0);
		// and darken the background and draw 3 lines (lent from statusbarclass;)
		//ProcessPixelArray(&data->rpBack, 0,    2,       wid,           hei-2, POP_DARKEN,   30, NULL);  // brighten bg
		ProcessPixelArray(&data->rpBack, minR+1, hei/2-1, wid-maxR-minR-2, 1,     POP_BRIGHTEN, 30, NULL);  // black line
		ProcessPixelArray(&data->rpBack, minR,   hei/2,   wid-maxR-minR,   1,     POP_DARKEN,   90, NULL);  // white line
		ProcessPixelArray(&data->rpBack, minR+1, hei/2+1, wid-maxR-minR-2, 1,     POP_BRIGHTEN, 30, NULL);  // black line
	}

	mousePressed = (data->ehnode.ehn_Events & IDCMP_MOUSEMOVE);

	// draw the background
	ClipBlit(&data->rpBack, 0, 0, rp, _left(obj), _top(obj), wid, hei, 0xc0);

	// draw the new position
	data->r = DoMethod(obj, MUIM_Numeric_ValueToScale, minR, maxR);
	data->maxX = wid-data->r-1;
	data->x = DoMethod(obj, MUIM_Numeric_ValueToScale, minR, data->maxX);

	// same attrs for all next drawing operations
	SetRPAttrs(rp, RPTAG_PenMode,FALSE, RPTAG_DrMd,JAM1, RPTAG_AlphaMode,TRUE, TAG_DONE);

	// draw the middle point
	SetRPAttrs(rp, RPTAG_FgColor, 0xFF000000, TAG_DONE);
	WritePixel(rp, _left(obj)+data->x, _top(obj)+hei/2);
	//Move(rp, _left(obj)+data->x, _top(obj)+hei/2); Draw(rp, _left(obj)+data->x, _top(obj)+hei/2);

	// draw the filled circle
	SetRPAttrs(rp, RPTAG_FgColor, mousePressed ? 0xAA000000 : 0xAAffffff, TAG_DONE);
	//DrawEllipse(rp, _left(obj)+data->x, _top(obj)+hei/2, data->r-1, data->r-1);
	DrawFilledCircle(rp, _left(obj)+data->x, _top(obj)+hei/2, data->r, -1); //mousePressed ? 0xFF000000 : 0xFFffffff);

	// draw the outline 
	SetRPAttrs(rp, RPTAG_FgColor, mousePressed ? 0xAAffffff : 0xAA000000, TAG_DONE);
	DrawEllipse(rp, _left(obj)+data->x, _top(obj)+hei/2, data->r, data->r);

	SetRPAttrs(rp, RPTAG_AlphaMode,FALSE, TAG_DONE); // I need to set it to FALSE else the icons have artefacts!

	//DB(("x/y = %ld/%ld, wid/hei = %ld/%ld\n", _left(obj), _top(obj), wid, hei));
	//DB(("min/max = %ld/%ld, val = %ld\n", data->min, data->max, data->val));
	return s;
}//MUIM_Draw


DEFMMETHOD(HandleEvent)
{
	GETDATA;

	if (msg->imsg)
	{
		switch (msg->imsg->Class)
		{
			case IDCMP_MOUSEBUTTONS:
			{
				if (msg->imsg->Code == SELECTDOWN)
				{
					ULONG x = msg->imsg->MouseX - _left(obj);
					
					if (_isinobject(msg->imsg->MouseX, msg->imsg->MouseY))
					{
						if (x >= data->x-data->r && x <= data->x+data->r)
						{
							// Whoo, you hit the knob!
							data->oldVal = data->val;
							data->ehnode.ehn_Events |= IDCMP_MOUSEMOVE;
							MUI_Redraw(obj, MADF_DRAWUPDATE);
						}
						else if (x < data->x-data->r)
						{
							DoMethod(obj, MUIM_Numeric_Decrease, 5);
						}	
						else if (x > data->x+data->r)
						{
							DoMethod(obj, MUIM_Numeric_Increase, 5);
						}	
						return (MUI_EventHandlerRC_Eat);
					}
				}
				if (msg->imsg->Code == SELECTUP && data->oldVal != -1)
				{
					data->ehnode.ehn_Events &= ~IDCMP_MOUSEMOVE;
					data->oldVal = -1;
					MUI_Redraw(obj, MADF_DRAWUPDATE);
					return (MUI_EventHandlerRC_Eat);
				}
				// abort MOUSEMOVE with click on right mouse button and jump to its previous position
				if (msg->imsg->Code == MENUDOWN && data->oldVal != -1)
				{
					data->ehnode.ehn_Events &= ~IDCMP_MOUSEMOVE;
					set(obj, MUIA_Numeric_Value, data->oldVal);
					return (MUI_EventHandlerRC_Eat);
				}
			}
			break;

			case IDCMP_MOUSEMOVE:
			{
				set(obj, MUIA_Numeric_Value, DoMethod(obj, MUIM_Numeric_ScaleToValue, 2, data->maxX, msg->imsg->MouseX - _left(obj)));			
			}
			break;
		}
	}

	return 0;
}//MUIM_HandleEvent


BEGINMTABLE
DECNEW
DECDISPOSE
DECSET
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(HandleEvent)
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Numeric, iconzoomsliderclass)
