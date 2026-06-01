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
 * $Id: panelviewwatcherclass.c,v 1.9 2025/09/16 15:52:30 kronos Exp $
 */

#include "ambient.h"

#if USE_INTERNAL_PANELS

/* public */
#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "ambient_cat.h"
#include "rexx.h"
#include "command.h"
#include "contextmenu.h"
#include "name.h"
#include "prefs.h"
#include "methodstack.h"
#include "paneltags.h"

/************************************************************************/

struct Data {
	APTR   cmenu;
};

/************************************************************************/

DEFNEW
{
	if( ( obj = DoSuperNew( cl, obj,
		MA_PanelPopup_IconType , MV_Icon_Type_View,
		MA_PanelPopup_MenuTitle, GSI(MSG_CMENUTITLE_PANELPOPUPVIEWS),
		TAG_MORE               ,msg->ops_AttrList,
		TAG_DONE
	) ) ) {
		set( obj, MUIA_ShortHelp, GSI(MSG_PANELITEM_VIEWWATCHER_SHORTHELP) );
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFDISP
{
	return( DOSUPER );
}

/************************************************************************/

DEFTMETHOD(Panelbutton_Launch)
{
	APTR menustrip, menu, rc;
	ULONG type;

	menustrip = MenustripObject,
						Child, menu = MenuObject,
							MUIA_Menu_Title, GSI(MSG_CMENUTITLE_PANELPOPUPVIEWS),
						End,
                     End;

	FORCHILD( _app(obj), MUIA_Application_WindowList )
	{
		if( get(child, MA_Window_Type, &type ) )
		{
			if( type == MV_Window_Type_View )
			{
				STRPTR title = (STRPTR) getv( child, MUIA_Window_Title );

				DoMethod( menu, OM_ADDMEMBER, ( MenuitemObject, MUIA_Menuitem_Title, title, MUIA_UserData, child, End ) );
			}
		}
	}
	NEXTCHILD

	rc = (APTR) DoMethod( menustrip, MUIM_Menustrip_Popup, obj, 0 , _left(obj), _bottom(obj) - 1 );
	MUI_DisposeObject( menustrip );

	if( rc != NULL )
		set( rc, MUIA_Window_Open, TRUE );

	return( 0 );
}

/************************************************************************/

DEFGET
{
	ULONG result = TRUE;

	switch( msg->opg_AttrID )
	{
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_ViewWatcher;
			break;
		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_VIEWWATCHER);
			break;
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Revision:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Vladimir Alaev,\nAmbient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) GSI(MSG_PANELITEM_VIEWWATCHERDESC);
			break;
		default:
			result = DOSUPER;
			break;
	}
	return( result );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECDISP
DECGET
DECTMETHOD(Panelbutton_Launch)
ENDMTABLE

DECSUBCLASSPTR_NC(panelbasebuttonclass, panelviewwatcherclass)
#endif
