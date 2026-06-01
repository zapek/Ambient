 //#include "demo.h"

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



/* public */
#include <cybergraphx/cybergraphics.h>
#include <datatypes/pictureclass.h>
#include <proto/cybergraphics.h>
#include <proto/graphics.h>
#include <clib/datatypes_protos.h>



/* private */

#include "name.h"
#include "gfx_scale.h"
#include "gfx_bitmap.h"
#include "gfx_blit.h"


/* private */
#include "../mui_func.h"

#include "paneltags.h"


#include "muifuncs.h"
#include "MUIClasses.h"
#include "debug.h"

struct MUI_CustomClass *DragButtonClass_Class(void);

/************************************************************************/

#define USE_INLINE_STDARG

/************************************************************************/

struct Data
{
	APTR draggad;
	BOOL visible;
	BOOL sel;
	ULONG horiz;
	struct MUI_EventHandlerNode ehnode;
	ULONG selected;
	ULONG zipping;
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

static void doset( APTR obj , struct Data *data, struct TagItem *tags,BOOL _init )
{
	FORTAG( tags )
	{
		case MA_Paneldrag_Type:
			data->type = tag->ti_Data;
			break;
		case MA_Panelwin_Position:
		//	PDB(("\n"));
			if( ( tag->ti_Data != MV_Panelwin_Position_Fixed ) && ( !data->draggad))
			{	
				DoMethod( obj, MM_Paneldrag_AddDragGad );
			}
			else if( ( tag->ti_Data == MV_Panelwin_Position_Fixed ) && ( data->draggad))
			{	
				DoMethod( obj, MM_Paneldrag_RemoveDragGad );
			}
			break;
		case MA_Paneldrag_Zipping:
			PDB(("zipping %d\n",tag->ti_Data));
			data->zipping = tag->ti_Data;
		/*	if(tag->ti_Data)
			{
				MUI_RejectIDCMP( obj, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS );
			}
			else
			{
				MUI_RequestIDCMP( obj, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS );
			}*/
			break;
			
	}
	NEXTTAG
}





static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	if( ( obj = (Object*) DoSuperMethodA( cl, obj, msg ) ) )
	{
		struct Data *data = (struct Data*) INST_DATA( cl, obj );

		data->visible = data->sel = data->zipping = FALSE;

		doset( obj, data, INITTAGS, TRUE );
	}
	return( (ULONG) obj );
}

/************************************************************************/

static ULONG mHandleInput(struct IClass *cl,Object *obj,struct MUIP_HandleInput *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG  wx, wy; 
	APTR par;
	BOOL redraw = FALSE;
	if(data->zipping) return 0;
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
						if(!data->zipping)	DoMethod( _parent( obj ), MM_Panelgroup_ToggleZip, data->type );
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
static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct TagItem *tstate, *tag;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	tstate = msg->ops_AttrList;
	doset( obj, data,msg->ops_AttrList,FALSE); 
	return DoSuperMethodA(cl,obj,msg);
}

static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	switch( msg->opg_AttrID )
	{
		case MA_Panel_Type:
			*msg->opg_Storage = MV_Panel_Type_Drag;
			return( TRUE );
		case MA_Paneldrag_Type:
			*msg->opg_Storage = data->type;
			return( TRUE );
	}
	return DoSuperMethodA(cl,obj,msg);
}

/************************************************************************/
static ULONG mAskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax*msg)
{

	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	
	ULONG size = getv( _parent( obj ), MA_Panelgroup_Size );
	ULONG drag_size = BIG_DRAG;
	DoSuperMethodA(cl,obj,msg);
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





static ULONG mSetup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg)
{
	Object *parent;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	if( !( DoSuperMethodA( cl, obj, msg ) ) ) {
		
		return( FALSE );
	}
	MUI_RequestIDCMP( obj, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS );
//	PDB(("\n"));
	GetAttr( MUIA_Parent, obj, (ULONG*) &parent );
	if( parent ) {
		GetAttr( MUIA_Group_Horiz, parent, &data->horiz );
	}
	DoMethod(_win(obj),MUIM_Notify,MA_Panelwin_Position,MUIV_EveryTime,obj,3,MUIM_Set,MA_Panelwin_Position,MUIV_TriggerValue);
#warning
//	DoMethod( obj, MM_Paneldrag_AddDragGad );
#warning
	return( TRUE );
}

/************************************************************************/

static ULONG mCleanup(struct IClass *cl,Object *obj,struct MUIP_Cleanup *msg)
{
	MUI_RejectIDCMP( obj, IDCMP_MOUSEMOVE | IDCMP_MOUSEBUTTONS );
	return( DoSuperMethodA( cl, obj, msg ) );
}

/************************************************************************/

static ULONG mDraw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
	#define MAXVECTORS 5
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
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
	rc = DoSuperMethodA(cl,obj,msg);
			
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
#warning	
			//if( ( hasalpha == SAOS_OpacitySupport_None )  || ( hasalpha == SAOS_OpacitySupport_OnOff ) )
			{
				bgbrightness = 255;
			}
			//else
			{
				//bgbrightness = gfx_analyze_average_brightness( _rp( obj ), _left( obj ), _top( obj ), _width( obj ) , _height( obj ) );
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

static ULONG mAddDragGad(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

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
		//	PDB(("\n"));
		}
	}
	return( 0 );
}

/************************************************************************/

static ULONG mRemoveDragGad(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	if( data->draggad )
	{
		RemoveGadget( _window( obj ), data->draggad );
		DisposeObject( data->draggad );
		data->draggad = NULL;
	//	PDB(("\n"));
	}
	return (0);
}

/************************************************************************/

static ULONG mShow(struct IClass *cl,Object *obj,Msg msg)
{
	ULONG rc,position;
	if( ( rc = DoSuperMethodA(cl,obj,msg) ) )
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


static ULONG mHide(struct IClass *cl,Object *obj,Msg msg)
{
	DoMethod( obj, MM_Paneldrag_RemoveDragGad );
//PDB(("\n"));
	return( DoSuperMethodA(cl,obj,msg) );
}

/************************************************************************/


DISPATCHER(DragButtonClass)
{
	switch (msg->MethodID)
	{
		case OM_NEW             			: return(mNew			(cl,obj,(struct opSet*)msg));
		case OM_SET        	    			: return(mSet			(cl,obj,(struct opSet*)msg));
		case OM_GET			    			: return(mGet			(cl,obj,(struct opGet *)msg));
		case MUIM_Setup     				: return(mSetup    		(cl,obj,(struct MUIP_Setup *)msg));
		case MUIM_Cleanup    				: return(mCleanup  		(cl,obj,(struct MUIP_Cleanup *)msg));
		case MUIM_HandleInput 				: return(mHandleInput	(cl,obj,(struct MUIP_HandleInput *)msg));
		case MUIM_AskMinMax		   			: return(mAskMinMax		(cl,obj,(struct MUIP_AskMinMax *)msg));
		case MUIM_Draw     					: return(mDraw     		(cl,obj,(struct MUIP_Draw *)msg));
		case MM_Paneldrag_RemoveDragGad		: return(mRemoveDragGad	(cl,obj,msg));
		case MM_Paneldrag_AddDragGad		: return(mAddDragGad	(cl,obj,msg));
		case MUIM_Show						: return(mShow			(cl,obj,msg));
		case MUIM_Hide						: return(mHide			(cl,obj,msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *DragButtonClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Area,NULL,sizeof(struct Data),DISPATCHER_REF(DragButtonClass));
}

