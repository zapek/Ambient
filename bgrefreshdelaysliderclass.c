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
 * $Id: bgrefreshdelaysliderclass.c,v 1.6 2013/08/22 19:18:03 jacadcaps Exp $
 */

/************************************************************************/

#include "ambient.h"
#include "ambient_cat.h"

/* public */

/* private */
#include "mui_func.h"

/************************************************************************/

struct Data {
	TEXT buffer[ 128 ];
};

/************************************************************************/

DEFNEW 
{
	if( ( obj = DoSuperNew( cl, obj, MUIA_CycleChain, 1, MUIA_Numeric_Min, 0, MUIA_Numeric_Max, 40, TAG_MORE, INITTAGS ) ) )
	{
		FORTAG(INITTAGS)
		{
			case MUIA_Slider_Level:
			{
				LONG val = tag->ti_Data;
				LONG level = 0;

				if( val <= 5 ) /* 1-5 minutes */
					level = val;
				else if( val >= 10      && val <=  5 * 10 ) /* 10-50 minutes */
					level = val / 10 + 5;
				else if( val >= 60      && val <= 23 * 60 ) /* 1-23 hours */
					level = val / 60 + 10;
				else if( val >= 60 * 24 && val <=  7 * 60 * 24 ) /* 1-7 days */
					level = val / ( 60 * 24 ) + 33;

				DoSuperMethod( cl, obj, MUIM_Set, MUIA_Slider_Level, level );
				break;
			}
		}
		NEXTTAG
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(Numeric_Stringify)
{
	GETDATA;
	LONG val = msg->value;
	LONG count = 0;
	STRPTR format = "";

	switch( val )
	{
		case 0:
			return( (ULONG) GSI(MSG_BGREFRESHSLIDERCLASS_NEVER) );

		default:
			if( val <= 5 ) /* 1-5 minutes */
			{
				count = val;
				format = GSI(MSG_BGREFRESHSLIDERCLASS_MINUTES);
			}
			else if( val >  5 && val <= 10 ) /* 10-50 minutes */
			{
				count = ( val - 5 ) * 10;
				format = GSI(MSG_BGREFRESHSLIDERCLASS_MINUTES);
			}
			else if( val > 10 && val <= 33 ) /* 1-23 hours */
			{
				count = val - 10;
				format = GSI(MSG_BGREFRESHSLIDERCLASS_HOURS);
			}
			else if( val > 33 && val <= 40 ) /* 1-7 days */
			{
				count = val - 33;
				format = GSI(MSG_BGREFRESHSLIDERCLASS_DAYS);
			}
			snprintf( data->buffer, sizeof( data->buffer ), format, count );
			break;
	}
	return( (ULONG) data->buffer );
}

/************************************************************************/

DEFGET
{
	switch( msg->opg_AttrID )
	{
		case MUIA_Numeric_Value:
		{
			LONG val;
			DoSuperMethod( cl, obj, OM_GET, msg->opg_AttrID, &val );

			if( val <= 5 ) /* 1-5 minutes */
				*msg->opg_Storage = val;
			else if( val >  5 && val <= 10 ) /* 10-50 minutes */
				*msg->opg_Storage = ( val -  5 ) * 10;
			else if( val > 10 && val <= 33 ) /* 1-23 hours */
				*msg->opg_Storage = ( val - 10 ) * 60;
			else if( val > 33 && val <= 41 ) /* 1-7 days */
				*msg->opg_Storage = ( val - 33 ) * 60 * 24;

			return( TRUE );
		}
	}

	return( DOSUPER );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECGET
DECMMETHOD(Numeric_Stringify)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Slider, bgrefreshdelaysliderclass)
