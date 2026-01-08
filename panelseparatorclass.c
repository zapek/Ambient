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
 * $Id: panelseparatorclass.c,v 1.9 2017/07/23 20:58:35 cyfm Exp $
 */

#include "ambient.h"

/* public */
#include <proto/cybergraphics.h>
#include <graphics/rpattr.h>
#include <proto/graphics.h>

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
#include "paneltags.h"
#include "gfx_analyze.h"

/************************************************************************/

struct Data {
	APTR   cmenu;
	ULONG  brightness_threshold;
	ULONG  alpha_threshold;
};

/************************************************************************/

#if 0
static void doset( APTR obj UNUSED, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
	}
	NEXTTAG 
}
#endif

/************************************************************************/

DEFNEW
{
	if( ( obj = DoSuperNew(cl, obj, TAG_DONE ) ) )
	{
//		  GETDATA;
	
//		  doset( obj, data, INITTAGS );
	}  
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(AskMinMax)
{
	ULONG size   = getv( _parent(obj), MA_Panelgroup_Size );
	ULONG horiz  = getv( _parent(obj), MA_Panelgroup_Horiz );
	
	ULONG defsize = 1;

	DOSUPER;

	if( horiz )
	{
		if( msg->MinMaxInfo->MinWidth < defsize )
		{
			msg->MinMaxInfo->MinWidth = defsize;
			msg->MinMaxInfo->MaxWidth = defsize;
		}
		msg->MinMaxInfo->MinHeight = size;
		msg->MinMaxInfo->MaxHeight = size;
	} else {
		if( msg->MinMaxInfo->MinHeight < defsize )
		{
			msg->MinMaxInfo->MinHeight = defsize;
			msg->MinMaxInfo->MaxHeight = defsize;
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
			*msg->opg_Storage = MV_Panel_Type_Separator;
			break;
		case MA_Panel_Properties:
            *msg->opg_Storage = TRUE;
			break;
		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_SEPARATOR);
			break;
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Revision:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Christian Rosentreter,\nAmbient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_SEPARATORDESC);
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
//	  GETDATA;

//	  doset( obj, data, INITTAGS );

	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(Draw)
{
	GETDATA;

	DOSUPER;
	if( msg->flags & ( MADF_DRAWOBJECT | MADF_DRAWUPDATE ) )
	{
		struct RastPort *rp = _rp(obj);

		ULONG mleft         = _mleft(obj);
		ULONG mtop          = _mtop(obj);
		ULONG mright        = _mright(obj);
		ULONG mbottom       = _mbottom(obj);
		ULONG xmid          = _mleft(obj) + _mwidth (obj) / 2;
		ULONG ymid          = _mtop(obj)  + _mheight(obj) / 2;
		ULONG x, y;
		ULONG colour = 0x00EFEFEF;

		ULONG horiz  = getv( _parent(obj), MA_Panelgroup_Horiz );
		ULONG hasalpha;
		
		GetAttr( SA_OpacitySupport, _screen( obj ), &hasalpha );
		DoMethod( _parent(obj), MM_Panelgroup_RefreshRect, mleft, mtop, _mwidth(obj), _mheight(obj), _rp( obj ) );
		if( ( hasalpha == SAOS_OpacitySupport_None )  || ( hasalpha == SAOS_OpacitySupport_OnOff ) )
		{
			data->brightness_threshold  = 255;
		}
		else
		{
			data->brightness_threshold = gfx_analyze_average_brightness( rp, mleft, mtop, _mwidth(obj), _mheight(obj) );
		}
		data->alpha_threshold = gfx_analyze_average_alpha( rp, mleft, mtop, _mwidth(obj), _mheight(obj) );

		if( data->brightness_threshold > 128 )
		{
			colour = 0x0000000;
		}
		if( data->alpha_threshold > 128 )
		{
			colour += 0xa0000000;
		} else {
			colour += 0xFF000000;
		}
		if( horiz )
		{   
			for( y = mtop + 1 ; y < mbottom ; y++ )
			{
				WriteRGBPixel( rp, xmid,  y, colour );
			}
		} else {
			for( x = mleft + 1 ; x < mright ; x++ )
			{
				WriteRGBPixel( rp, x, ymid, colour );
			}
		
		}
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panel_SaveConfig)
{
	//GETDATA;
	APTR pl, pi; /* list, item */
	ASSERT(msg->pctx);

	if( !( pl = prefspool_item_get( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, NULL ) ) )
	{
		pl = prefspool_item_add( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, (ULONG)NULL );
	}

	if( pl )
	{
		if( !( pi = prefspool_item_get(msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, NULL ) ) )
		{
			pi = prefspool_item_add( msg->pctx, pl, msg->index | DSF_LISTPOOL, NULL, (ULONG)NULL );
		}
//		  if( pi )
//		  {
//			  setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_something, data->something );
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
DECGET
DECSET
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
DECTMETHOD(Panel_Settings_Group)   
DECSMETHOD(Panel_SaveConfig)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, panelseparatorclass)

