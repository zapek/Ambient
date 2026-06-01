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
 * $Id: toolbutton_actionclass.c,v 1.6 2025/07/23 23:54:26 geit Exp $
 */

#define TOOLBUTTON(x) GSI(MSG_PREFSWIN_TOOLBAR_##x)

#include "ambient.h"

/* public */
#include <graphics/rpattr.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>

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
#include "dragdrop.h"
#include "mimetype.h"

struct Data {
	STRPTR command; /* command string */
	STRPTR type;	/* command type */
	STRPTR image;   /* name of image file */
	STRPTR flags;   /* command flags (numeric value)*/
	STRPTR views;   /* views for which this button is visible */
	APTR bm;
	ULONG imgwidth, imgheight;

	ULONG args[ 6 ];

	APTR action;
};


DEFNEW
{
	/* check if we have 4 args available (and only 4) */

	ULONG nargs = 0;
	ULONG *args = (ULONG*)GetTagData( MA_Toolbutton_Args, 0L, INITTAGS);

	while ( args && args[ nargs ] )
	{
		nargs++;
	}

	if ( nargs !=5 )
	{
		DB(("Wrong number of arguments (%d)\n", nargs));
		return( (ULONG) NULL );
	}

	obj = DoSuperNew(cl, obj,
		MUIA_InputMode,    MUIV_InputMode_RelVerify, /* no other modes supported, atm */
		TAG_MORE, INITTAGS
	);

	if (obj)
	{
		struct Data *data;

		data = INST_DATA(cl, obj);

		data->type    = name_build( (STRPTR)args[ 0 ] );
		data->command = name_build( (STRPTR)args[ 1 ] );
		data->image   = name_build( (STRPTR)args[ 2 ] );
		data->flags   = name_build( (STRPTR)args[ 3 ] );
		data->views   = name_build( (STRPTR)args[ 4 ] );

		data->bm = imagecache_getbitmap( data->image, 24 );

		DoMethod( obj, MUIM_Notify, MUIA_Pressed, FALSE, obj, 1, MM_Toolbutton_Execute );

		/* build action node */

		data->action = actionnode_createtemporary();	/* temporary flag will be inherited by duplicated one */
		if ( data->action )
		{
			if ( !stricmp( "internal", data->type ) )
				actionnode_addcommand( data->action, AC_INTERNAL, data->command );
			if ( !stricmp( "script", data->type ) )
				actionnode_addcommand( data->action, AC_SCRIPT, data->command );
			if ( !stricmp( "workbench", data->type ) )
				actionnode_addcommand( data->action, AC_WORKBENCH, data->command );
			if ( !stricmp( "amigados", data->type ) )
				actionnode_addcommand( data->action, AC_AMIGADOS, data->command );
			if ( !stricmp( "arexx", data->type ) )
				actionnode_addcommand( data->action, AC_AREXX, data->command );

			actionnode_setup( data->action );

			actionnode_setattrs(data->action,
								ACTIONNODETAG_FLAGS, ((ULONG) actionnode_getattr(data->action, ACTIONNODETAG_FLAGS )) |  atoi(data->flags),
								TAG_DONE);
		}
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

	if ( data->type )
		name_delete( data->type );

	if ( data->command )
		name_delete( data->command );

	if ( data->image )
		name_delete( data->image );

	if ( data->flags )
		name_delete( data->flags );

	if ( data->action )
		actionnode_delete( data->action );

	if ( data->views )
		name_delete( data->views );

	return DOSUPER;
}


DEFGET
{
	GETDATA;

	switch (msg->opg_AttrID)
	{
		case MA_Toolbutton_Args:
		{
			data->args[ 0 ] = (ULONG)data->type;
			data->args[ 1 ] = (ULONG)data->command;
			data->args[ 2 ] = (ULONG)data->image;
			data->args[ 3 ] = (ULONG)data->flags;
			data->args[ 4 ] = (ULONG)data->views;
			data->args[ 5 ] = (ULONG)NULL;;
			*msg->opg_Storage = (ULONG)data->args;
			return (TRUE);
		}

		case MA_Toolbutton_Viewflags:
		{
			ULONG views = atoi( data->views );
			*msg->opg_Storage = views;
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

	w = gfx_bitmap_width ( data->bm );
	h = gfx_bitmap_height( data->bm );

	data->imgwidth = w;
	data->imgheight = h;

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

		struct RastPort *rp;
		ULONG   mleft, mtop, mwidth; //, mheight;

		mleft   = _mleft(obj);
		mtop    = _mtop(obj);
		mwidth  = _mwidth(obj);
//		mheight = _mheight(obj);

		rp    = _rp(obj);

		gfx_blit( data->bm,  rp,
			BLITTAG_DstType, BLITVAL_DstType_RastPort,
			BLITTAG_DstX,    mleft + (mwidth - data->imgwidth) / 2,
			BLITTAG_DstY,    mtop + 1,
			BLITTAG_Alpha,   0xffffffff,
		TAG_DONE);

	}

	return 0;
}

DEFTMETHOD(Toolbutton_Execute)
{
	GETDATA;

	APTR wo = _win( obj );

	if ( getv( wo, MA_Window_Type ) == MV_Window_Type_View )
	{
		/* get view object */

		APTR vo = _view( obj );
		APTR action = actionnode_duplicate( data->action );

		if ( vo && action )
		{
			APTR dispatcher = (APTR)DoMethod( app, MM_Application_CreateActionDispatcher );

			if ( dispatcher )
			{
				struct MinList ml;
				ULONG needfiles = (ULONG)actionnode_getattr( data->action, ACTIONNODETAG_FLAGS ) & ACTION_FLAG_NEED_ENTRIES;

				NEWLIST(&ml);

				if ( needfiles )
				{
					DoMethod(vo, MM_View_GetSelectionList, &ml);
				}

				SetAttrs(dispatcher,
					MA_ActionDispatcher_SrcURI, (APTR)getv(vo, MA_View_Path),
					MA_ActionDispatcher_SrcID, (APTR)getv(vo, MA_Viewgroup_ID),
					MA_ActionDispatcher_RefWin, _win(obj),
					MA_ActionDispatcher_Action, action,
					TAG_DONE
				);

				if ( !ISLISTEMPTY( &ml ) )
				{
					struct dragdropnode *ddn, *nextddn;

					ITERATELISTSAFE(ddn, nextddn, &ml)
					{
						DoMethod( dispatcher, MM_ActionDispatcher_AddURI, ddn->path, TRUE );
						/* XXX: careful here, if you break out you must still free the nodes */
						free(ddn);
					}
				}
				else if ( !needfiles )
				{
					/*
					 * No files needed, but we need at least one, so give it dummy one.
					*/
					DoMethod( dispatcher, MM_ActionDispatcher_AddURI, "dummy", TRUE );
				}

				DoMethod( dispatcher, MM_ActionDispatcher_Execute );
			}
			else
			{
				actionnode_delete( action );
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
ENDMTABLE

DECSUBCLASSPTR_NC(toolbuttonclass, toolbutton_actionclass)

