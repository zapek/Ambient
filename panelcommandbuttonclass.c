
/* ANSI C */
#include <stdlib.h>
#include <string.h>

/* System */
#include <dos/dos.h>
#include <graphics/gfxmacros.h>
#include <workbench/workbench.h>
#include <libraries/mui.h>

#include <proto/alib.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/icon.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/utility.h>
#include <proto/asl.h>
#include <proto/muimaster.h>
#include <proto/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <cybergraphx/cybergraphics.h>
#include <clib/datatypes_protos.h>

#include "ambient.h"
#include "classes.h"

#include "ambient_cat.h"
#include "rexx.h"
#include "command.h"
#include "contextmenu.h"
#include "iconio.h"
#include "name.h"
#include "gfx_scale.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_pen.h"
#include "prefs.h"
#include "datatypes_picture.h"
#include "wbstart.h"
#include "threads.h"
#include "file_func.h"
#include "dragdrop.h"
#include "legacy.h"
#include "paneltags.h"

/************************************************************************/

struct Data
{
	STRPTR help;
};
 

/************************************************************************/

static void doset( APTR obj, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_Panel_URI:
			if( data->help )
			{
				name_delete( data->help );
			}
			data->help = name_build( (STRPTR) tag->ti_Data );
			if( data->help )
			{
				set( obj, MUIA_ShortHelp, data->help );
			}
			break;
	
	}
	NEXTTAG
}
/************************************************************************/

DEFNEW
{
	struct Data *data;
	if( ( obj = (Object*) DoSuperNew( cl, obj,  TAG_MORE, INITTAGS ) ) ) {
		data = (struct Data*) INST_DATA( cl, obj );
		data->help = 0;
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


DEFMMETHOD(DragDrop)
{
	STRPTR path, uripath;

	ASSERT(msg->obj);

	if( get( msg->obj, MA_Icon_PathInfo, &path ) )
	{
		if( get( obj, MA_Panel_URI, &uripath ) )
		{
			struct MinList *l = malloc( sizeof( struct MinList ) );
			if( l )
			{
				int num = 0;

				NEWLIST( l );

				/*
				 * This list is deallocated (together with nodes) when thread finished.
				 */

				/* i wonder if numsel == 0 still happens */
				if( getv( msg->obj, MA_View_NumSelected ) == 0 )
				{
					STRPTR path = (STRPTR) getv( msg->obj, MA_Icon_Path );
					if( path )
					{
						struct wbsnode *n = malloc( sizeof( struct wbsnode ) + strlen( path ) + 1 );
						if( n )
						{
							strcpy( n->arg, path );
							ADDTAIL( l, n );
							num++;
						}
					}
				}
				else if( getv( msg->obj, MA_View_NumSelected ) >= 1 )
				{
					struct MinList ml;
					struct dragdropnode *ddn, *nextddn;

					NEWLIST( &ml );

					DoMethod( _view( msg->obj ), MM_View_GetSelectionList, &ml );

					ITERATELISTSAFE( ddn, nextddn, &ml )
					{
						struct wbsnode *n = malloc( sizeof( struct wbsnode ) + strlen( ddn->path ) + 1 );
						if( n )
						{
							strcpy( n->arg, ddn->path );
							ADDTAIL( l, n );
							num++;
						}
						/* XXX: careful here, if you break out you must still free the nodes */
						free( ddn );
					}
				}

				if( num )
				{
					/*
					 * apparently TA_WBStart does free the argument list.
					 * check wbstart.c/wbstart(). - piru
					 */

					do_action( obj, TA_WBStart,
											TT_WBStart_Path         , uripath,
											TT_WBStart_Argument_List, l,
											TAG_DONE );
				}
			}
		}

	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelbutton_Launch)
{
	//GETDATA;
	STRPTR uripath;
	TEXT buf[ PATH_SIZE + 256 ]; /* should be enough (tm) */
	ULONG win_x,win_y,win_xs,win_ys,viewmode;
	CONST_STRPTR mode = "";
	CONST_STRPTR view = "ICON";
		
	if( !DOSUPER )
	{
		if( get( obj, MA_Panel_URI, &uripath ) )
		{
			if( ( get( obj, MA_Icon_WindowLeft, &win_x) )
			 && ( get( obj, MA_Icon_WindowTop, &win_y) )
			 && ( get( obj, MA_Icon_WindowWidth, &win_xs) ) 
			 && ( get( obj, MA_Icon_WindowHeight, &win_ys) ) 
			 && ( get( obj, MA_Icon_ViewMode, &viewmode) )
			 && (viewmode != MV_Icon_ViewMode_None  ) )
			{
				if ( viewmode == MV_Icon_ViewMode_IconAll )
					mode = "ALL";
				else if ( viewmode == MV_Icon_ViewMode_Thumbs )
					mode = "THUMBS";
				else if ( viewmode == MV_Icon_ViewMode_Lister )
				{	
					view = "LIST";
					mode = "ALL";
				}
				else
				mode = "ICON";

		
				
				snprintf(buf, sizeof(buf), "LoadURI file:///%s?left=%ld&top=%ld&width=%ld&height=%ld&mode=%s&view=%s TOFRONT",
						uripath,
						win_x,
						win_y,
						win_xs,
						win_ys,
						mode,
						view);
			}
			else
			{
				snprintf( buf, sizeof( buf ), "LoadURI \"%s\" TOFRONT", uripath );
			}		
				
			
			execute_command( obj, AC_INTERNAL, buf, NULL );
		}
	}
	return( 0 );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS );

	return( DOSUPER );
}

/************************************************************************/


DEFGET
{
	ULONG result = TRUE;
	STRPTR uripath;
	static TEXT buf[ PATH_SIZE + 256 ]; /* should be enough (tm) */
	
	switch( msg->opg_AttrID )
	{
		case MA_Panelbutton_DefaultImagePath:
			if( ( get( obj, MA_Panel_URI, &uripath ) ) && ( uripath ) && ( strlen(uripath) ) )
			{
				snprintf( buf, sizeof( buf ), "%s.info", uripath );
				*msg->opg_Storage = (ULONG) buf;
			}
			else
			{
				*msg->opg_Storage = 0;
			}
			break;
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_Button;
			break;
		case MA_Panel_Extern_DisplayName:
			*msg->opg_Storage = (ULONG) "CommandButton";
			break;
		case MA_Panel_Extern_Revision:
		case MA_Panel_Extern_Version:
			*msg->opg_Storage = 1;
			break;
		case MA_Panel_Extern_Author:
			*msg->opg_Storage = (ULONG) "Ambient Open Source Team";
			break;
		case MA_Panel_Extern_Description:
			*msg->opg_Storage = (ULONG) "None";
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
DECSET
DECGET
DECDISPOSE
DECMMETHOD(DragDrop)
DECTMETHOD(Panelbutton_Launch)
ENDMTABLE
 
DECSUBCLASSPTR_NC( panelbasebuttonclass, panelcommandbuttonclass )

