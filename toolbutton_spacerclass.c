/*
 * Ambient - the ultimate desktop
 * ------------------------------
 * Copyright 2006 by Michal Wozniak
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
 * $Id: toolbutton_spacerclass.c,v 1.6 2006/12/20 13:34:15 fab Exp $
 */

#include "ambient.h"

/* public */
#include <cybergraphx/cybergraphics.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "prefs.h"
#include "name.h"
#include "legacy.h"
#include "gfx_analyze.h"



struct Data {
	ULONG  mode;
	ULONG  location;

	ULONG  brightness_threshold;  
	BOOL   brightness_checked;

	STRPTR mode_str;
	ULONG  args[ 2 ];
};

/*   modes
 */
#define TOOLBARSPACER_FLEXIBLE   (0)
#define TOOLBARSPACER_FIXED      (1)
#define TOOLBARSPACER_SEPARATOR  (2)


DEFNEW
{
	ULONG location = MV_Toolbutton_Location_Toolbar;
	ULONG *args    = NULL; 

	FORTAG( INITTAGS )
	{
		case MA_Toolbutton_Location:
			location = tag->ti_Data;
			break;

		case MA_Toolbutton_Args:
			args = (ULONG *)tag->ti_Data;
			break;

		/* filter background tags 
		 */
		case MUIA_Background:
			tag->ti_Tag = TAG_IGNORE;
			break;

		case MUIA_Frame:
			tag->ti_Tag = TAG_IGNORE;
			break;
	}
	NEXTTAG


	obj = DoSuperNew(cl, obj,
		MUIA_CycleChain,   FALSE,
		InnerSpacing(0,0),
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		GETDATA;

		data->mode_str = name_build( args ? (STRPTR)args[ 0 ] : (STRPTR)"1" );  /* fixed is default */
		data->mode     = atoi(data->mode_str);

		data->location = location;

		ASSERT(data->mode <= TOOLBARSPACER_SEPARATOR);

		data->brightness_checked   = FALSE;
		data->brightness_threshold = 255; /* default to bright bgs */
	}

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	if ( data->mode_str )
	{
		name_delete( data->mode_str );
	}

	return DOSUPER;
}

DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Toolbutton_Label:
		{
			*msg->opg_Storage = (ULONG)"";
			return (TRUE);
		}

		case MA_Toolbutton_Args:
		{
			data->args[ 0 ]   = (ULONG)data->mode_str;
			data->args[ 1 ]   = NULL;
			*msg->opg_Storage = (ULONG)data->args;
			return (TRUE);
		}
	}

	return (DOSUPER);
}

#define BUTTON_WIDTH 32  /* XXX: ? */
#define SPACE_WIDTH  10  /* XXX: should maybe depend on MUIs groupspacing settings */

DEFMMETHOD(AskMinMax)
{
	ULONG maxwidth, defwidth, minwidth;
	ULONG location, mode;

	GETDATA;

	location = data->location;

	DOSUPER;

	/*  XXX: width should depend on actual toolbar image sizes. 
	 */
	maxwidth = SPACE_WIDTH;
	defwidth = SPACE_WIDTH;
	minwidth = SPACE_WIDTH;

	mode = data->mode;

	if (location == MV_Toolbutton_Location_Storage)
	{
		if (mode != TOOLBARSPACER_SEPARATOR)
		{
			minwidth = BUTTON_WIDTH;
			defwidth = BUTTON_WIDTH;
			maxwidth = BUTTON_WIDTH;
		}
	}
	else
	{
		if (mode == TOOLBARSPACER_FLEXIBLE)
		{
			minwidth = 1;
			defwidth = 1;
			maxwidth = 16384;
		}
	}

	msg->MinMaxInfo->MinWidth  += minwidth;
	msg->MinMaxInfo->DefWidth  += defwidth;
	msg->MinMaxInfo->MaxWidth  += maxwidth;

    msg->MinMaxInfo->MinHeight += 24; 
	msg->MinMaxInfo->MaxHeight += 48;
	msg->MinMaxInfo->DefHeight += 32;

	return 0;
}

DEFMMETHOD(Draw)
{
	DOSUPER;

	if (msg->flags & MADF_DRAWOBJECT)
	{
		GETDATA;

		struct RastPort *rp = _rp(obj);

		ULONG mleft         = _mleft(obj);
		ULONG mtop          = _mtop(obj);
		ULONG mwidth        = _mwidth(obj);
		ULONG mheight       = _mheight(obj);
		ULONG mright        = _mright(obj);
		ULONG mbottom       = _mbottom(obj);

		if (data->mode == TOOLBARSPACER_SEPARATOR) /* separator */
		{
			ULONG vertoffs   = mleft + (mwidth / 2);

			ProcessPixelArray( rp, vertoffs,     mtop + 1, 1, mheight - 2, POP_BRIGHTEN, 60, NULL);
			ProcessPixelArray( rp, vertoffs - 1, mtop + 1, 1, mheight - 2, POP_DARKEN,   60, NULL);
		}
		else if (data->location != MV_Toolbutton_Location_Toolbar)
		{
			ULONG x, y;
			BOOL ff;
			ULONG colour = 0xFFFFFFFF;

			/*   check if we have to draw white or black lines; cache the value 
		 	 */
			if (data->brightness_checked == FALSE)
			{
				data->brightness_checked   = TRUE;
				data->brightness_threshold = gfx_analyze_average_brightness(rp, mleft, mtop, mwidth, mheight);
			}

			if (data->brightness_threshold > 128)
			{
				colour = 0xFF000000;
			}

			/*  draw placeholder lines (dotted)
			 */
			for (x = mleft + 1, ff = TRUE; x < mright; x++)
			{
				if (ff) 
				{ 
					WriteRGBPixel(rp, x, mtop,    colour); 
					WriteRGBPixel(rp, x, mbottom, colour);
				}

				ff = !ff;
			}

			for (y = mtop + 1, ff = TRUE; y < mbottom; y++)
			{
				if (ff) 
				{ 
					WriteRGBPixel(rp, mleft,  y, colour); 
					WriteRGBPixel(rp, mright, y, colour);
				}

				ff = !ff;
			}


			/*  XXX: add some "<--->" look to flexible space inside storage (tokai)
			 */
		}
	}

	return 0;
}

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
ENDMTABLE

DECSUBCLASSPTR_NC(toolbuttonclass, toolbutton_spacerclass)

