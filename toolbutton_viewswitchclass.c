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
 * $Id: toolbutton_viewswitchclass.c,v 1.8 2013/10/29 22:14:59 geit Exp $
 */

#include "ambient.h"

/* public */
#include <graphics/rpattr.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <libraries/gadtools.h>

/* private */
#include "ambient_cat.h"
#include "mui_func.h"
#include "gfx_blit.h"
#include "gfx_bitmap.h"
#include "prefs.h"
#include "imagecache.h"
#include "name.h"
#include "command.h"
#include "actiondispatcherclass.h"
#include "action.h"
#include "viewapi.h"
#include "rexx.h"
#include "command.h"
#include "tags.h"
#include "contextmenu.h"


#define MAX_VIEWS 10
#define MAX_MODES 5

struct Data {
	APTR bm[ MAX_VIEWS ][ MAX_MODES ];
	STRPTR name[ MAX_VIEWS ][ MAX_MODES ];
	ULONG args[ 1 ];
	APTR viewobj; /* cache it */

};

static APTR get_view( APTR obj, struct Data *data )
{
	if ( !data->viewobj )
	{
		APTR vo = _view( obj );
	
		if ( vo )
		{
			data->viewobj = vo;

			/* setup notify while we are at it */

			DoMethod( vo, MUIM_Notify, MA_Viewgroup_ViewChanged, TRUE,
				obj, 1, MM_Toolbutton_Update );
		}
	}

	return data->viewobj;
}


DEFNEW
{
	obj = DoSuperNew(cl, obj,
		MUIA_InputMode,    MUIV_InputMode_RelVerify, /* no other modes supported, atm */
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data;

		data = INST_DATA(cl, obj);

		memset( data, 0, sizeof( struct Data ) );

		/* load images for each view */

		{
			TEXT name[ 64 ];
			struct viewnode *vn;

			LONG vi = 1;

			while ( ( vn = viewapi_findbyid( vi ) ) )
			{
				/* view exists, get modes */

				LONG mi = 0;

				FORTAG( vn->querytagarray )
				{
					case AVIEW_Query_Viewmode_RexxName:
					{
						snprintf( name, sizeof( name ), "toolbar/view_%s", vn->name );
						snprintf( name, sizeof( name ), "%s_%s.png", name, (STRPTR)tag->ti_Data );
						data->bm[ vi ][ mi ] = imagecache_getbitmap( name, 24 );

						/* label name for contextmenu */

						{
							//static const STRPTR template = "\033I[4:PROGDIR:images/%s] %s - %s";
							static const STRPTR template = "%s - %s";
							TEXT buf[ 128 ];

							//snprintf( buf, sizeof( buf ), template, name, vn->label, (STRPTR)tags_nth_tagdata(AVIEW_Query_Viewmode_Name, tag->ti_Data, vn->querytagarray, mi + 1 ) );
							snprintf( buf, sizeof( buf ), template, vn->label, (STRPTR)tags_nth_tagdata(AVIEW_Query_Viewmode_Name, tag->ti_Data, vn->querytagarray, mi + 1 ) );

							data->name[ vi ][ mi ] = name_build( buf );
						}
						mi++;
					}
				}
				NEXTTAG

				/* some views don't have modes */

				if ( mi == 0 )
				{
					snprintf( name, sizeof( name ), "toolbar/view_%s.png", vn->name );
					data->bm[ vi ][ 0 ] = imagecache_getbitmap( name, 24 );
				}

				vi++;
			}
		}

		DoMethod( obj, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_Toolbutton_Execute );
	}
	else
	{
		DB(("Failed to create toolbutton\n"));
	}

	return ((ULONG)obj);
}

DEFDISP
{
	GETDATA;

	/* delete labels */

	LONG vi, mi;

	for( vi = 0; vi<MAX_VIEWS; vi++)
	{
		for( mi = 0; mi<MAX_MODES; mi++)
		{
			if ( data->name[ vi ][ mi ] )
				name_delete( data->name[ vi ][ mi ] );
		}
	}

	/* no need to delete bitmaps. they are just references */

	return DOSUPER;
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Toolbutton_Args:
		{
			data->args[ 0 ] = NULL;
			*msg->opg_Storage = (ULONG)data->args;
			return (TRUE);
		}
	}
	return (DOSUPER);
}


DEFMMETHOD(AskMinMax)
{
	ULONG h, w;
	GETDATA;

	DOSUPER;

	w = gfx_bitmap_width ( data->bm[ 1 ][ 0 ] );
	h = gfx_bitmap_height( data->bm[ 1 ][ 0 ] );

	msg->MinMaxInfo->MinWidth  += w + 2;
	msg->MinMaxInfo->MinHeight += h + 2;
	msg->MinMaxInfo->MaxWidth  += w + 2;
	msg->MinMaxInfo->MaxHeight += h + 2;
	msg->MinMaxInfo->DefWidth  += w + 2;
	msg->MinMaxInfo->DefHeight += h + 2;

	return 0;
}


/* since this is like an icon we could abuse iconclass.c here? */
DEFMMETHOD(Draw)
{
	DOSUPER;

	if (msg->flags & MADF_DRAWOBJECT)
	{
		GETDATA;

		ULONG vind = 1;
		ULONG mind = 0;

		/* check which view we are using */

		APTR wo = _win( obj );

		if ( getv( wo, MA_Window_Type ) == MV_Window_Type_View )
		{
			APTR vo = get_view( obj, data );
			vind = getv( vo, MA_Viewgroup_ViewIndex );
			mind = getv( vo, MA_Viewgroup_ViewModeIndex );
		}

		if ( data->bm[ vind ][ mind ] )
		{
			struct RastPort *rp;
			ULONG mleft, mtop, mwidth, mheight;

			ULONG imgwidth = gfx_bitmap_width ( data->bm[ vind ][ mind ] );
			//ULONG imgheight = gfx_bitmap_height( data->bm[ vind ][ mind ] );

			mleft   = _mleft(obj);
			mtop    = _mtop(obj);
			mwidth  = _mwidth(obj);
			mheight = _mheight(obj);

			rp    = _rp(obj);

			gfx_blit( data->bm[ vind ][ mind ],  rp,
				BLITTAG_DstType, BLITVAL_DstType_RastPort,
				BLITTAG_DstX,    mleft + (mwidth - imgwidth) / 2,
				BLITTAG_DstY,    mtop + 1,
				BLITTAG_Alpha,   0xffffffff,
			TAG_DONE);
		}
	}

	return 0;
}

DEFTMETHOD(Toolbutton_Update)
{
	MUI_Redraw( obj, MADF_DRAWOBJECT );

	return 0;
}

DEFTMETHOD(Toolbutton_Execute)
{
	GETDATA;

	APTR wo = _win( obj );

	if ( getv( wo, MA_Window_Type ) == MV_Window_Type_View )
	{
		/* get view object */

		APTR vo = get_view( obj, data );

		if ( vo )
		{
			/* build menu strip with available view modes */

			ULONG map[ 10 ][ 2 ];
			APTR menustrip = contextmenu_build(CM_VIEW,0);

			if ( menustrip != NULL )
            {
				ULONG index = index; /* shut up gcc */
				APTR menu = menu; /* shut up gcc */

				/* menustrip -> menu (sigh) */
				FORCHILD(menustrip, MUIA_Family_List)
				{
					menu = child;
					break;
				}
				NEXTCHILD;

				ASSERT(menu);

				if (!get(vo, MA_View_ModeIndex, &index))
				{
					index = 0;
				}
				contextmenu_addviews(vo, menu);
				contextmenu_addmodes(vo, menu, index); /* XXX (and retcode too) */

				/* now, we move first submenu contents into main menu. yay */

				FORCHILD(menu, MUIA_Family_List)
				{
					DoMethod(menu, OM_REMMEMBER, child);
					DoMethod(menustrip, OM_REMMEMBER, menu);
					DoMethod(menustrip, OM_ADDMEMBER, child);
					MUI_DisposeObject(menu);
					menu = child;
					break;
				}
				NEXTCHILD;

				/* hackery + 10, assign userdatas and map commands to them. blame MUIM_Menustrip_Popup and it's return value. */

				index = 1;
				FORCHILD(menu, MUIA_Family_List)
				{
					map[ index ][ 0 ] = getv( child, MA_Menuitem_SubType );
					map[ index ][ 1 ] = getv( child, MA_Menuitem_Command );
					set( child, MUIA_UserData, index );
					index++;
				}
				NEXTCHILD;
			}

			/* show menu and get result */

			{
				ULONG poprc = DoMethod( menustrip , MUIM_Menustrip_Popup, obj, 0 , _left(obj),_bottom(obj)+1);

				if ( poprc > 0 )
				{
					TEXT viewcmd[ 128 ];

					LONG type = map[ poprc ][ 0 ];
					struct command_menu *cm = (struct command_menu*)map[ poprc ][ 1 ];
					struct viewnode *vn = viewapi_findbyid( getv( vo, MA_Viewgroup_ViewIndex ) );

					switch (type)
					{
						case MV_Menuitem_SubType_Mode:
							snprintf(viewcmd, sizeof(viewcmd), "Viewmode mode=\"%s %s\"", vn->name, cm->args);
							break;

						case MV_Menuitem_SubType_View:
							snprintf(viewcmd, sizeof(viewcmd), "Viewmode mode=\"%s\"", cm->args);
							break;
					}

					execute_command( vo, cm->type, viewcmd, NULL);
				}

				MUI_DisposeObject( menustrip );

			}

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
DECTMETHOD(Toolbutton_Execute)
DECTMETHOD(Toolbutton_Update)
ENDMTABLE

DECSUBCLASSPTR_NC(toolbuttonclass, toolbutton_viewswitchclass)

