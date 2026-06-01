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
 * $Id: panelspacerclass.c,v 1.8 2025/09/16 15:52:30 kronos Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_PANELS

/* public */
#include <proto/cybergraphics.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "rexx.h"
#include "command.h"
#include "contextmenu.h"
#include "name.h"
#include "prefs.h"
#include "threads.h"
#include "file_func.h"
#include "legacy.h"
#include "prefs_advanced.h"
#include "gfx_analyze.h"
#include "paneltags.h"

/************************************************************************/

struct Data {
	ULONG  size;
	APTR   cmenu;
	STRPTR help;

	ULONG  brightness_threshold;  
	BOOL   brightness_checked;    
};

/************************************************************************/

static void doset( APTR obj, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_Panelgroup_Locked:
			MUI_Redraw( obj, MADF_DRAWOBJECT );
			break;

		case MA_Panelgroup_ImageSpec:  
			data->brightness_checked = FALSE; /* invalidate cache, see MUIM_Draw */
			MUI_Redraw( obj, MADF_DRAWOBJECT );
			break;
	}
	NEXTTAG 
}

/************************************************************************/

DEFNEW
{
	if( ( obj = DoSuperNew( cl, obj,
									MUIA_Frame      , MUIV_Frame_None,
									MUIA_InnerLeft  , 1,
									MUIA_InnerRight , 1,
									MUIA_InnerTop   , 1,
									MUIA_InnerBottom, 1,
									TAG_DONE ) ) )
	{
		struct Data *data = INST_DATA( cl, obj );

		data->help  = NULL;
		data->size  = _aprefs( panelspacersize );
		
		data->brightness_checked   = FALSE;
		data->brightness_threshold = 255; /* default to bright bgs */

		doset( obj, data, INITTAGS );

	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFDISP
{
	GETDATA;

	if( data->help )
	{
		name_delete( data->help );
	}

	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(AskMinMax)
{
    GETDATA;
	ULONG size;
	ULONG horiz;

	DOSUPER;

	size  = getv( _parent(obj), MA_Panelgroup_Size );
	horiz = getv( _parent(obj), MA_Panelgroup_Horiz );

	if( horiz )
	{
		if( msg->MinMaxInfo->MinWidth < data->size )
		{
			msg->MinMaxInfo->MinWidth = data->size;
			msg->MinMaxInfo->MaxWidth = data->size;
		}
		msg->MinMaxInfo->MinHeight = size;
		msg->MinMaxInfo->MaxHeight = size;
	} else {
		if( msg->MinMaxInfo->MinHeight < data->size )
		{
			msg->MinMaxInfo->MinHeight = data->size;
			msg->MinMaxInfo->MaxHeight = data->size;
		}
		msg->MinMaxInfo->MinWidth = size;
		msg->MinMaxInfo->MaxWidth = size;
	}
	return( 0 );
}

/************************************************************************/

DEFGET
{
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_Spacer;
			break;
		case MA_Panel_Properties:
            *msg->opg_Storage = TRUE;
			break;
		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_SPACER);
			break;
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Revision:
			*msg->opg_Storage = 2;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Vladimir Alaev,\nAmbient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_SPACERDESC);
			break;
		default:
			result = DOSUPER;
			break;
	}
	return( result );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS );

	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(Draw)
{
	return( 0 );
#if 0
	GETDATA;
	DOSUPER;

	if (msg->flags & MADF_DRAWOBJECT)// &&
//		  (!getv(_parent(obj), MA_Panelgroup_Locked)))
	{
		struct RastPort *rp = _rp(obj);

        ULONG mleft         = _mleft(obj);
		ULONG mtop          = _mtop(obj);
		ULONG mright        = _mright(obj);
		ULONG mbottom       = _mbottom(obj);

		ULONG x, y;
		ULONG colour = 0x00FFFFFF;
		BOOL ff;

		/*   check if we have to draw white or black lines; cache the value 
		 */
		if( data->brightness_checked == FALSE )
		{
			data->brightness_checked   = TRUE;
			data->brightness_threshold = gfx_analyze_average_brightness( rp, mleft, mtop, _mwidth(obj), _mheight(obj) );
		}

		if( data->brightness_threshold > 128 )
		{
			colour = 0xFF000000;
		}

		for( x = mleft + 1, ff = TRUE ; x < mright ; x++ )
		{
			if (ff) 
			{ 
				WriteRGBPixel( rp, x, mtop,    colour );
				WriteRGBPixel( rp, x, mbottom, colour );
			}

			ff = !ff;
		}

		for( y = mtop + 1, ff = TRUE ; y < mbottom ; y++ )
		{
			if (ff) 
			{ 
				WriteRGBPixel( rp, mleft,  y, colour );
				WriteRGBPixel( rp, mright, y, colour );
			}

			ff = !ff;
		}
	}
#endif
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panel_SaveConfig)
{
	//GETDATA;
	APTR pl, pi; /* list, item */
	ASSERT( msg->pctx );

	if( !( pl = prefspool_item_get( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, NULL ) ) )
	{
		pl = prefspool_item_add( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, (ULONG)NULL );
	}

	if (pl)
	{
		if ( !( pi = prefspool_item_get( msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, NULL ) ) )
		{
			pi = prefspool_item_add( msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, (ULONG)NULL );
		}
//		  if( pi )
//		  {
//			  setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_Something, data->something );
//		  }
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panel_Settings_Group)
{
	return( 0 );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECSET
DECMMETHOD(AskMinMax)
DECTMETHOD(Panel_Settings_Group)
DECMMETHOD(Draw)
DECSMETHOD(Panel_SaveConfig)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, panelspacerclass)

#endif