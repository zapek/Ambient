/* MUI */
#ifdef MYDEBUG
#include "mui.h"
#else
#include <libraries/mui.h>
#endif


/* ANSI C */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* System */
#include <dos/dos.h>
#include <graphics/gfxmacros.h>
#include <graphics/rpattr.h>
#include <workbench/workbench.h>


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
#include <intuition/extensions.h>



#include "ambient.h"

#if USE_INTERNAL_PANELS

/* public */
#include <cybergraphx/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <clib/datatypes_protos.h>



/* private */
#include "rexx.h"
#include "command.h"
#include "contextmenu.h"
#include "iconio.h"
#include "panelitem.h"
#include "name.h"
#include "gfx_scale.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_pen.h"
#include "prefs.h"
#include "wbstart.h"
#include "threads.h"
#include "file_func.h"
#include "dragdrop.h"
#include "legacy.h"

/* private */
#include "mui_func.h"
#include "legacy.h"
#include "gfx_analyze.h"
#include "paneltags.h"

/************************************************************************/

#define USE_INLINE_STDARG

/************************************************************************/

struct Data
{
	APTR draggad;
	BOOL visible;
	BOOL sel;
	ULONG horiz;
	struct MUI_EventHandlerNode ehn;
	ULONG selected;
	ULONG type;
	UBYTE *tbuf;
	ULONG oldbufsize;
	ULONG oldx;
	ULONG oldy;
};

#define MINEMPTY 12
#define BIG_DRAG 12;
#define MEDIUM_DRAG 8;
#define SMALL_DRAG 6;

/************************************************************************/

static void doset( APTR obj UNUSED, struct Data *data, struct TagItem *tags )
{
	FORTAG( tags )
	{
		case MA_Paneldrag_Type:
			data->type = tag->ti_Data;
			break;
		case MA_Panelwin_Position:
			if( ( tag->ti_Data != MV_Panelwin_Position_Fixed ) && ( !data->draggad))
			{	
				DoMethod( obj, MM_Paneldrag_AddDragGad );
			}
			else if( ( tag->ti_Data == MV_Panelwin_Position_Fixed ) && ( data->draggad))
			{	
				DoMethod( obj, MM_Paneldrag_RemoveDragGad );
			}
			break;
	}
	NEXTTAG
}

/************************************************************************/

DEFMMETHOD(HandleInput)
{
	GETDATA;
	ULONG  wx, wy; 
	APTR par;
	BOOL redraw = FALSE;
	GetAttr( MUIA_Parent, obj, (ULONG*) &par );
	switch( msg->imsg->Class )
	{
		case IDCMP_MOUSEBUTTONS:
			if( data->visible )
			{
				if( msg->imsg->Code == SELECTDOWN )
				{
					GetAttr( MUIA_Window_LeftEdge,_win( obj ), &data->oldx );
					GetAttr( MUIA_Window_TopEdge ,_win( obj ), &data->oldy );
					data->sel = TRUE;
				}
				else if( ( msg->imsg->Code == SELECTUP ) && ( data->sel ) )
				{
					data->sel = FALSE;
					GetAttr( MUIA_Window_LeftEdge, _win( obj ), &wx );
					GetAttr( MUIA_Window_TopEdge , _win( obj ), &wy );
					if( ( data->oldx == wx ) && ( data->oldy == wy ) )
					{
						DoMethod( _parent( obj ), MM_Panelgroup_ToggleZip, data->type );
					}
					else
					{
						DoMethod( _win( obj ), MM_Panelwin_AttachToBorder, TRUE );
					}
				}
			}
			break;
		case IDCMP_MOUSEMOVE:
			if((data->sel))
			{
				GetAttr( MUIA_Window_LeftEdge, _win( obj ), &wx );
				GetAttr( MUIA_Window_TopEdge , _win( obj ), &wy );
				set( _parent( obj ), MA_Panel_HasMoved, TRUE );
			} else {
				if( ( msg->imsg->MouseX > _mleft( obj ) ) && ( msg->imsg->MouseX < _mright ( obj ) )
				 && ( msg->imsg->MouseY > _mtop ( obj ) ) && ( msg->imsg->MouseY < _mbottom( obj ) ) )
				{
					if(!(data->visible)) redraw = TRUE;
					data->visible = TRUE;
				} else {
					if(data->visible) redraw = TRUE;
					data->visible = FALSE;
				}
				if( ( redraw ) && ( par ) )  MUI_Redraw( obj, MADF_DRAWOBJECT );
			}
			break;
	}
	return( 0 );
}

/************************************************************************/

DEFMMETHOD(AskMinMax)
{
	ULONG size = getv( _parent( obj ), MA_Panelgroup_Size );
	ULONG drag_size = BIG_DRAG;
	DOSUPER;
	ASSERT( size );
	if( size < 48 ) drag_size = MEDIUM_DRAG;
	if( size < 24 ) drag_size = SMALL_DRAG;
	if( getv( _parent( obj ), MA_Panelgroup_Horiz ) )
	{
		msg->MinMaxInfo->MinWidth  += drag_size;
		msg->MinMaxInfo->MinHeight += size;
		msg->MinMaxInfo->MaxWidth  += drag_size;
		msg->MinMaxInfo->MaxHeight += size;
	} else {
		msg->MinMaxInfo->MinWidth  += size;
		msg->MinMaxInfo->MinHeight += drag_size;
		msg->MinMaxInfo->MaxWidth  += size;
		msg->MinMaxInfo->MaxHeight += drag_size;
	}
	return( 0 );
}

/************************************************************************/

DEFNEW
{
	if( ( obj = (Object*) DoSuperMethodA( cl, obj, msg ) ) )
	{
		struct Data *data = (struct Data*) INST_DATA( cl, obj );

		data->visible = data->sel = FALSE;

		doset( obj, data, INITTAGS );
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFMMETHOD(Draw)
{
	#define MAXVECTORS 5
	GETDATA;
	struct AreaInfo ai;
	struct AreaInfo *oldai;
	struct TmpRas tr;
	struct TmpRas *oldtr;
	ULONG x, y;
	ULONG size;
	UBYTE buf[ MAXVECTORS * 5 ];
	ULONG horiz,zipped;
	ULONG rc;
	/*
	 * The following should probably go to
	 * a 'vector' class and be double
	 * buffered also.
	 */
	rc = DOSUPER;
			
	if( msg->flags & ( MADF_DRAWOBJECT | MADF_DRAWUPDATE ) )
	{
		ULONG hasalpha;
		GetAttr( SA_OpacitySupport, _screen( obj ), &hasalpha );
		GetAttr( MA_Panelgroup_Zipped, _parent( obj ),&zipped );
		if( ( data->visible )
				|| ( zipped )
				|| ( !( getv( _parent( obj ),MA_Panelgroup_HideDragBar ) ) )
				|| ( hasalpha == SAOS_OpacitySupport_None )
				|| ( hasalpha == SAOS_OpacitySupport_OnOff ) )
		{
			ULONG bgbrightness;
			FillPixelArray( _rp( obj ), _left( obj ), _top( obj ), _width( obj ), _height( obj ), 0xFFe0e0e0 );
			if( ( hasalpha == SAOS_OpacitySupport_None )  || ( hasalpha == SAOS_OpacitySupport_OnOff ) )
			{
				bgbrightness = 255;
			}
			else
			{
				bgbrightness = gfx_analyze_average_brightness( _rp( obj ), _left( obj ), _top( obj ), _width( obj ) , _height( obj ) );
			}
			/*
			* Adjust background width brightening/darkening and draw simple frame
			*/
			

			ProcessPixelArray(_rp( obj ), _left( obj ) + 1                , _top( obj ) + 1                 , _width( obj ) - 1, _height( obj ) - 1, bgbrightness > 128 ? POP_DARKEN : POP_BRIGHTEN, 45, NULL );
			ProcessPixelArray(_rp( obj ), _left( obj )                    , _top( obj )                     , _width( obj )    , 1                 ,  data->selected    ? POP_DARKEN : POP_BRIGHTEN, 70, NULL );
			ProcessPixelArray(_rp( obj ), _left( obj ) + _width( obj ) - 1, _top( obj )                     , 1                , _height( obj )  -1, !data->selected    ? POP_DARKEN : POP_BRIGHTEN, 70, NULL );
			ProcessPixelArray(_rp( obj ), _left( obj )                    , _top( obj ) + 1                 , 1                , _height( obj )  -1,  data->selected    ? POP_DARKEN : POP_BRIGHTEN, 70, NULL );
			ProcessPixelArray(_rp( obj ), _left( obj )                    , _top( obj ) + _height( obj ) - 1, _width( obj )    , 1                 , !data->selected    ? POP_DARKEN : POP_BRIGHTEN, 70, NULL );

			horiz = getv( _parent( obj ), MA_Panelgroup_Horiz );
			if(horiz)
			{
				ULONG arrow_off = 2;
				if(_height( obj ) < 24 ) arrow_off = 1;
				if( data->type == MV_Paneldrag_Type_LeftUp )
				{
					x = _left( obj ) + ( _right( obj ) - _left( obj ) ) / 2 - arrow_off;
				}
				else
				{
					x = _left( obj ) + (_right( obj ) - _left( obj ) ) / 2 + arrow_off;
				}
				y = _top( obj ) + (_bottom( obj ) - _top( obj ) ) / 2;
			}
			else
			{
				ULONG arrow_off = 2;
				if(_width( obj ) < 24) arrow_off = 1;
				x = _left( obj ) + (_right( obj ) - _left( obj ) ) / 2;

				if(data->type == MV_Paneldrag_Type_LeftUp)
				{
					y = _top( obj ) + (_bottom( obj ) - _top( obj ) ) / 2 - arrow_off;
				}
				else
				{
					y = _top( obj ) + (_bottom( obj ) - _top( obj ) ) / 2 + arrow_off;
				}
			}

			if( data->selected ) /* XEN selected effect :) */
			{
				x++;
				y++;
			}

			size = RASSIZE( _width( obj ), _height( obj ) );

			if( !data->tbuf || data->oldbufsize != size )
			{
				if( data->tbuf )
				{
					free( data->tbuf );
				}
				data->tbuf = malloc( size );
				data->oldbufsize = size;
			}

			if( data->tbuf ) {
			
				SetRPAttrs( _rp( obj ), RPTAG_PenMode, FALSE, TAG_DONE );
				InitArea( &ai, buf, MAXVECTORS );
				oldai = _rp( obj )->AreaInfo;
				_rp( obj )->AreaInfo = &ai;
				InitTmpRas( &tr, data->tbuf, size );
				oldtr = _rp( obj )->TmpRas;
				_rp( obj )->TmpRas = &tr;

				if( bgbrightness > 128 ) {
				  SetRPAttrs( _rp( obj ), RPTAG_FgColor, 0xff404040, TAG_DONE );
				} else {
				  SetRPAttrs( _rp( obj ), RPTAG_FgColor, 0xffc0c0c0, TAG_DONE );
				}

				AreaMove( _rp( obj ), x, y );

				if( horiz )
				{
					ULONG arrow_size = 4;
					if( _height( obj ) < 48 ) arrow_size = 3;
					if( _height( obj ) < 24 ) arrow_size = 2;
					if( data->type == MV_Paneldrag_Type_LeftUp )
					{
						AreaDraw( _rp( obj ), x + arrow_size, y - arrow_size );
						AreaDraw( _rp( obj ), x + arrow_size, y + arrow_size );
					}
					else
					{
						AreaDraw( _rp( obj ), x - arrow_size, y - arrow_size );
						AreaDraw( _rp( obj ), x - arrow_size, y + arrow_size );
					}
				}
				else
				{
					ULONG arrow_size = 4;
					if( _width( obj ) < 48 ) arrow_size = 3;
					if( _width( obj ) < 24 ) arrow_size = 2;
					if( data->type == MV_Paneldrag_Type_LeftUp )
					{
						AreaDraw( _rp( obj ), x + arrow_size, y + arrow_size );
						AreaDraw( _rp( obj ), x - arrow_size, y + arrow_size );
					}
					else
					{
						AreaDraw( _rp( obj ), x + arrow_size, y - arrow_size );
						AreaDraw( _rp( obj ), x - arrow_size, y - arrow_size );
					}
				}
				AreaDraw( _rp( obj ), x, y );
				AreaEnd( _rp( obj ) );

				_rp( obj )->AreaInfo = oldai;
				_rp( obj )->TmpRas = oldtr;
			}
		}
		else
		{
			FillPixelArray( _rp( obj ), _left( obj ), _top( obj ), _width( obj ), _height( obj ), 0x01000000 );
		}
	}
	return( rc );
}

/************************************************************************/

DEFMMETHOD(DragDrop)
{
	STRPTR path;
	ULONG dummy;
	int pos = 0;
	APTR tbar, nbutton;

	ASSERT( msg->obj );
	GetAttr( MA_Panelwin_Group, _win( obj ), (ULONG*) &tbar );
	if( tbar )
	{
		if( get( msg->obj, MA_Icon_PathInfo, &path ) )
		{
			if( ( nbutton = (Object*) NewObject( getpanelcommandbuttonclass(),NULL,
															MA_Panel_URI, getv( msg->obj, MA_Icon_Path ) ,
															MA_Panel_Imagepath, path,
															TAG_DONE ) ) )
			{
				DoMethod( tbar, MUIM_Group_InitChange );
				DoMethod( tbar, OM_ADDMEMBER,nbutton );
				DoMethod( tbar, MUIM_Group_ExitChange );
				SetAttrs( _win( obj ), MUIA_Window_Width, MUIV_Window_Width_MinMax(0), TAG_DONE );
				DoMethod( _win( obj ), MM_Panelwin_SaveConfig );
			}
		}
		else if( get( msg->obj, MA_Panelitem_list_IsList, &dummy ) )
		{
			LONG id = MUIV_List_NextSelected_Start;
			struct PanelItem *pi;
			ULONG i;

			for( i = 0 ; ; i++ )
			{
				APTR o = NULL;
				DoMethod( msg->obj, MUIM_List_NextSelected, &id );
				if( id == MUIV_List_NextSelected_End ) {
					break;
				}
				DoMethod( msg->obj, MUIM_List_GetEntry, id, &pi );

				if( pi->pi_Type ) // <<<<< this looks and is wrong. Name the type! Currently this means != PT_BUTTON (geit)
				{
					o = NewObject( pi->pi_Class, NULL, TAG_DONE );
				}
				if( o )
				{
					DoMethod( tbar, MUIM_Group_InitChange );
					DoMethod( tbar, OM_ADDMEMBER, o );
					DoMethod( tbar, MUIM_Group_MoveMember, o, pos );
					DoMethod( tbar, MUIM_Group_ExitChange );
					set( tbar, MA_Panelgroup_HasChanged, TRUE );
				}
			}
		}
	}
	return( DoSuperMethodA( cl, obj, msg ) );
}

/************************************************************************/

DEFMMETHOD( DragQuery )
{
	return( MUIV_DragQuery_Refuse );
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
	GETDATA;

	switch( msg->opg_AttrID )
	{
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_Drag;
			return( TRUE );
		case MA_Paneldrag_Type:
			*msg->opg_Storage = data->type;
			return( TRUE );
	}
	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(Setup)
{
	GETDATA;
	Object *parent;

	if( !( DoSuperMethodA( cl, obj, msg ) ) ) {
		return( FALSE );
	}
	MUI_RequestIDCMP( obj, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS );

	GetAttr( MUIA_Parent, obj, (ULONG*) &parent );
	if( parent ) {
		GetAttr( MUIA_Group_Horiz, parent, &data->horiz );
	}
	DoMethod(_win(obj),MUIM_Notify,MA_Panelwin_Position,MUIV_EveryTime,obj,3,MUIM_Set,MA_Panelwin_Position,MUIV_TriggerValue);
	return( TRUE );
}

/************************************************************************/

DEFMMETHOD(Cleanup)
{
	MUI_RejectIDCMP( obj, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS );
	return( DoSuperMethodA( cl, obj, msg ) );
}

/************************************************************************/

DEFTMETHOD(Paneldrag_AddDragGad)
{
	GETDATA;

	if( !data->draggad )
	{
		if( ( data->draggad = NewObject( 0, BUTTONGCLASS,
			GA_Left,      _left( obj ),
			GA_Top,       _top( obj ),
			GA_Width,     _width( obj ),
			GA_Height,    _height( obj ),
			GA_SysGType,  GTYP_WDRAGGING2,
			GA_Immediate, TRUE,
			TAG_DONE ) ) )
		{
			AddGList(_window( obj ), data->draggad, -1, -1, NULL );
		}
	}
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Paneldrag_RemoveDragGad)
{
	GETDATA;

	if( data->draggad )
	{
		RemoveGadget( _window( obj ), data->draggad );
		DisposeObject( data->draggad );
		data->draggad = NULL;
	}
	return (0);
}

/************************************************************************/

DEFMMETHOD(Show)
{
	ULONG rc,position;

	if( ( rc = DOSUPER ) )
	{
		//GETDATA;

		/*
		 * This is the cleanest way. Other options are:
		 * - EventHandler + MoveWindow(): doesn't work
		 *   because intuition delays events for the next
		 *   intuitick. It becomes jerky then.
		 *
		 * - PeekQualifier() and check if LMB is pressed
		 *   50 times per seconds. This works OK but is not
		 *   very friendly.
		 *
		 * - Current way: system friendly but intuition had
		 *   to get some support for it :)
		 */
		GetAttr(MA_Panelwin_Position,_win(obj),&position);
		if( position != MV_Panelwin_Position_Fixed )
		{
			DoMethod( obj, MM_Paneldrag_AddDragGad );
		}
	}
	return( rc );
}

/************************************************************************/

DEFMMETHOD(Hide)
{
	DoMethod( obj, MM_Paneldrag_RemoveDragGad );

	return( DOSUPER );
}

/************************************************************************/

BEGINMTABLE
DECNEW
DECGET
DECSET
DECMMETHOD(Cleanup)
DECMMETHOD(Setup)
DECMMETHOD(Show)
DECMMETHOD(Hide)
DECMMETHOD(AskMinMax)
DECMMETHOD(HandleInput)
DECMMETHOD(DragQuery)
DECMMETHOD(DragDrop)
DECTMETHOD(Paneldrag_AddDragGad)
DECTMETHOD(Paneldrag_RemoveDragGad)
DECMMETHOD(Draw)
ENDMTABLE

DECSUBCLASS_NC(MUIC_Area, paneldragclass)
#endif


