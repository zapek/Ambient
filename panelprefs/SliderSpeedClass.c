
#define CATCOMP_NUMBERS
extern const char * const __stringtable[];
#ifdef __SASC
#define GSI(x) __stringtable[x]
#else
#define GSI(x) ( STRPTR )__stringtable[x]
#endif

#include "muifuncs.h"

#include "locale.h"
#include "panelprefs_cat.h"
//#include "debug.h"

struct Data {
	int dummy;
};

static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	 obj = DoSuperNew( cl, obj,
									MUIA_Numeric_Min, 0,
									MUIA_Numeric_Max, 3,
									TAG_MORE, 	msg->ops_AttrList,
									TAG_DONE);
	{
			return( (ULONG) obj );
	}
	return( (ULONG) obj );
}


static ULONG mStringify(struct IClass *cl,Object *obj,struct MUIP_Numeric_Stringify *msg)
{
	switch( msg->value )
	{
		case 0:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_SLOW) );

		case 1:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_MEDIUM) );

		case 2:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_FAST) );

		case 3:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_INSTANT) );

		default:
		//	PDB(("wrong value\n"));
			break;
	}
	return( 0 );
}

DISPATCHER(SliderSpeedClass)
{
 	switch (msg->MethodID)
	{
		case OM_NEW               				: return(mNew    					(cl,obj,(struct opSet *)msg));
		case MUIM_Numeric_Stringify				: return(mStringify					(cl,obj,(struct MUIP_Numeric_Stringify *)msg));	
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END


struct MUI_CustomClass *SliderSpeed_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Slider,NULL,sizeof(struct Data),DISPATCHER_REF(SliderSpeedClass));
}

#if 0
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
 * $Id: SliderSpeedClass.c,v 1.2 2026/02/18 17:54:55 kronos Exp $
 */

#include "ambient.h"
#include "ambient_cat.h"


/* public */

/* private */
#include "mui_func.h"

/************************************************************************/

struct Data {
	int dummy;
};

/************************************************************************/

DEFNEW
{
	if( ( obj = DoSuperNew( cl, obj,
									MUIA_Numeric_Min, 0,
									MUIA_Numeric_Max, 3,
									TAG_MORE, INITTAGS
									) ) )
	{

	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(Numeric_Stringify)
{
	switch( msg->value )
	{
		case 0:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_SLOW) );

		case 1:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_MEDIUM) );

		case 2:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_FAST) );

		case 3:
			return( (ULONG) GSI(MSG_PREFSWIN_PANEL_SPEED_INSTANT) );

		default:
			PDB(("wrong value\n"));
			break;
	}
	return( 0 );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECMMETHOD(Numeric_Stringify)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Slider, panelsliderspeedclass)
#endif
