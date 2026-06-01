#include "ambient.h"

#if USE_INTERNAL_PANELS

/* ANSI C */
#include <stdlib.h>
//#include <string.h>
//#include <stdio.h>
//#include <ctype.h>
/* System */
#include <libraries/mui.h>
#include <dos/dos.h>
#include <graphics/gfxmacros.h>
#include <workbench/workbench.h>
#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>
#include <libraries/asl.h>

/* Prototypes */

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

#include <clib/datatypes_protos.h>
#include <clib/alib_protos.h>
#include <clib/debug_protos.h>


#include "classes.h"
#include "mui_internal.h"
#include "ambient.h"
#include "ambient_cat.h"
#include "mui_func.h"
#include "contextmenu.h"
#include "command.h"
#include "rexx.h"
#include "panelitem.h"
#include "smartreq.h"
#include "prefs.h"
#include "threads.h"
#include "gfx_dbuf.h"
#include "pointer.h"
#include "gfx_blit.h"
#include "playsound.h"
#include "paneltags.h"
#include "screen.h"
#include "name.h"

/************************************************************************/

#define CM_SAVE_PANEL    1001
#define CM_LOCKED        1002
#define CM_NEW_PANEL     1003
#define CM_DEL_PANEL     1004
#define CM_DEL_OBJECT    1005
#define CM_MOVE_OBJECT   1006
#define CM_SUBPANEL      1007
#define CM_DIRPANEL      1008
#define CM_DIGICLOCK     1009
#define CM_FLIP          1010


#define DIR_LEFTUP 0
#define DIR_RIGHTBOTTOM 1
                                            /**/
 
/************************************************************************/


struct MP_Panelgroup_FillZipBuffer
{
	ULONG MethodID;
	ULONG width,height;
	struct Screen *screen;
};

/************************************************************************/

struct Data
{
	struct Hook LayoutHook;
	ULONG backgroundcolor;
	ULONG backgroundmode;
	APTR active,mobj;
	APTR cmenu;
	ULONG MaxID;
	ULONG locked;
	ULONG moving;
	Object *highlighted;
	UWORD bg_width,bg_height;
	UWORD bg_stride;
	ULONG *bg_buffer;
	BOOL has_bg;
	STRPTR backname;
	ULONG size;
	ULONG zipped_size,unzipped_size,newpanel;
	ULONG horiz;
	ULONG dragmode;
	ULONG drag;
	ULONG max_w, max_h;
	APTR dragobj[2];
	ULONG hidedrag;
    	/* zip mode */
	ULONG allowzipping;
	ULONG zipdraw;
	ULONG zipped;
	ULONG zipping;
	ULONG zipdir; /* DIR_LEFTUP or DIR_RIGHTBOTTOM */
	ULONG zipsnap_xs; /* original size of the panel */
	ULONG zipsnap_ys;
	struct doublebuf *dbuf;
    	/* autozip */

	ULONG autozip, enable_autozip;
	struct MUI_InputHandlerNode ihnode;
	struct MUI_EventHandlerNode ehnode;

	/*parallel zip and hide*/
	BOOL hidden;

	APTR ext_pctx,ext_pi,ext_pl; /* only valid for an external objects saveconfig method*/
 };



/************************************************************************/

static ULONG LoadPic( APTR obj UNUSED, struct Data *data )
{
	ULONG retval = FALSE;
	struct BitMap *bitmap;
	//ULONG depth;
	Object *picture;
	struct BitMapHeader *bmh;
	struct RastPort mask;

	InitRastPort( &mask );
	if( ( picture = NewDTObject( (void *) data->backname,
												DTA_SourceType       , DTST_FILE,
												DTA_GroupID          , GID_PICTURE,
												PDTA_Remap           , TRUE,
												PDTA_DestMode        , PMODE_V43,
												PDTA_FreeSourceBitMap, TRUE,
												PDTA_PixelFormat     , PBPAFMT_ARGB ,
												OBP_Precision        , PRECISION_IMAGE,
												TAG_DONE ) ) )
	{
		if( DoMethod( picture, DTM_PROCLAYOUT, NULL, 1 ) )
		{
			GetDTAttrs( picture, PDTA_DestBitMap  , &bitmap, TAG_DONE );
			GetDTAttrs( picture, PDTA_BitMapHeader, &bmh   , TAG_DONE );
			if( bitmap )
			{
				mask.BitMap = bitmap;
				//depth = GetBitMapAttr( bitmap, BMA_DEPTH );
				data->bg_width  = bmh->bmh_Width;
				data->bg_height = bmh->bmh_Height;
				data->bg_stride = ( data->bg_width * 4 );
				if( ( data->bg_buffer = (ULONG*) AllocTaskPooled( data->bg_stride * data->bg_height ) ) )
				{
					/*ULONG r = */ReadPixelArray( data->bg_buffer, 0, 0, data->bg_stride, &mask, 0, 0, data->bg_width, data->bg_height, RECTFMT_ARGB );
					retval = TRUE;
				}
			}
		}
		DisposeDTObject( picture );
	}
	return( retval );
}

/************************************************************************/

#define DOPANELLAYOUT(co,x,y,xs,ys) \
	({if (lm->lm_Type == MUILM_MINMAX) \
	{ \
		max_x = max(max_x, x + xs); \
		max_y = max(max_y, y + ys); \
	} \
	else \
	{ \
		if ((x + xs > lm->lm_Layout.Width) || (y + ys > lm->lm_Layout.Height) || !MUI_Layout(co, x, y, xs, ys, 0)) \
		{ \
			PDB(("layout failed\n")); \
			return (FALSE);\
		} \
	}})

#define DOPLACE(c) \
	({if (data->horiz) \
	{ \
		l = dist; \
		t = 0; \
	} \
	else \
	{ \
		l = 0; \
		t = dist; \
	}})

/************************************************************************/

MUI_HOOK( layoutpanelgroup, APTR grp, struct MUI_LayoutMsg *lm )
{
	struct Data *data = INST_DATA( OCLASS( grp ), grp );

	switch( lm->lm_Type )
	{
		case MUILM_MINMAX:
			// fallthrough
		case MUILM_LAYOUT:
			{
				Object *cstate = (Object *) lm->lm_Children->mlh_Head;
				Object *child;
				ULONG t = 0, l = 0, w = 0, h = 0;
				ULONG max_x = 0, max_y = 0;
				ULONG childcount = 0;
				ULONG dist = 0;
				while ( ( child = NextObject( &cstate ) ) )
				{
					if( _flags( child ) & MADF_SHOWME )
					{
						w = _minwidth( child );  /* XXX: remember only _minwidth() is known at that point. we should check min/max and give the size value of our panel */
						h = _minheight( child );
						DOPLACE(child);
						DOPANELLAYOUT( child, l, t, w, h );
						childcount++;
						//if(type == 0) dist+=8;
						if( data->horiz )
						{
							dist += w;
						} else {
							dist += h;
						}
					}
				}
				
				if( lm->lm_Type == MUILM_MINMAX )
				{
					struct Screen *scr = _screen( grp );

					/*
					 * Make sure the object is big enough for
					 * the cases where it's empty or almost
					 * empty. Also add the bottom/right frame
					 * space.
					 */
					if( data->horiz )
					{
						max_x = max( max_x, 12 );
						max_y = max( max_y, data->size );
					} else {
						max_x = max( max_x, data->size );
						max_y = max( max_y, 12 );
					}

					/*
					 * If we don't fit, set our size to the screen's size,
					 * then we'll fail in layout and use MUIM_AdjustLayout
					 * until we fit.
					 */
					max_x = min( max_x, scr->Width  ); /* XXX: frame size taken into account? wtf is that mess anyway */
					max_y = min( max_y, scr->Height );

					lm->lm_MinMax.MinWidth  = max_x;
					lm->lm_MinMax.MinHeight = max_y;

					lm->lm_MinMax.DefWidth  = max_x;
					lm->lm_MinMax.DefHeight = max_y;

					if( data->zipping || data->zipped )
					{
						lm->lm_MinMax.MaxWidth  = MUI_MAXMAX;
						lm->lm_MinMax.MaxHeight = MUI_MAXMAX;
					} else {
						lm->lm_MinMax.MaxWidth  = max_x;
						lm->lm_MinMax.MaxHeight = max_y;
					}
					data->max_w = max_x;
					data->max_h = max_y;
				}

				#if 0
				if( lm->lm_Type == MUILM_MINMAX )
				{
					dprintf( "final childs: %ld\n", childcount );
					dprintf( "MinWidth: %ld, MinHeight: %ld\nDefWidth: %ld, DefHeight: %ld\nMaxWidth: %ld, MaxHeight: %ld\n",
						(LONG) lm->lm_MinMax.MinWidth, (LONG) lm->lm_MinMax.MinHeight,
						(LONG) lm->lm_MinMax.DefWidth, (LONG) lm->lm_MinMax.DefHeight,
						(LONG) lm->lm_MinMax.MaxWidth, (LONG) lm->lm_MinMax.MaxHeight
					);
				} else {
					dprintf("layouting..\n");
				}
				#endif

			}
			return( TRUE );
	}
	return ( MUILM_UNKNOWN );
}

/************************************************************************/
DEFTMETHOD(Panelgroup_StartZipTimer)
{
	GETDATA;
	if( muiRenderInfo( obj ) == NULL || _app(obj) == NULL )
	{
		return 0;
	}

	if( data->ihnode.ihn_Object != NULL )
	{
		DoMethod( _app(obj), MUIM_Application_RemInputHandler, &data->ihnode );
		data->ihnode.ihn_Object = NULL;
	}
	if(!DoMethod(obj,MM_Panelgroup_CheckZipLock) && !data->mobj)
	{
		/* 5 seconds */

		memset( &data->ihnode, 0, sizeof( struct MUI_InputHandlerNode ) );
		data->ihnode.ihn_Millis = 5000;
		data->ihnode.ihn_Object = obj;
		data->ihnode.ihn_Flags  = MUIIHNF_TIMER;
		data->ihnode.ihn_Method = MM_Panelgroup_DelayedZip;
	
		DoMethod( _app(obj), MUIM_Application_AddInputHandler, &data->ihnode );
	}
	return 0;
}

/************************************************************************/

static VOID run_autozip_trigger( APTR obj, struct Data *data )
{
	/* XXX: run autozip timer. we should check if mouse is still above panel.. bah.. */
	/*PDB(("cz %x\n",DoMethod(obj,MM_Panelgroup_CheckZipLock)));*/
	if( !data->zipped && !data->zipping && data->enable_autozip )
	{
		DoMethod(obj,MM_Panelgroup_StartZipTimer);
		//start_autozip_timer( obj, data );
	}
}

/************************************************************************/

#define RFR_CHANGED  (1 << 0UL)
#define RFR_REFRESH  (1 << 1UL)
#define RFR_REFRESH2 (1 << 2UL)
#define RFR_NEWSIZE  (1 << 3UL)

/************************************************************************/

static void doset( APTR obj, struct Data *data, struct TagItem *tags, BOOL init )
{
	ULONG rfr = 0;
	FORTAG( tags )
	{
		                                                       
		case MA_Panelgroup_DragMode:
			if( data->dragmode != tag->ti_Data )
			{
				DoMethod( obj, MM_Panelgroup_ChangeDrag, tag->ti_Data );
				rfr |= ( RFR_CHANGED | RFR_REFRESH | RFR_NEWSIZE );
		   	}
			break;

		case MA_Panelgroup_Size:
			if(!data->zipped) data->unzipped_size = tag->ti_Data;
			if( data->size != tag->ti_Data )
			{
				data->size = tag->ti_Data;
				rfr |= ( RFR_CHANGED | RFR_NEWSIZE );
			}
			break;
	
		case MA_Panelgroup_HideDragBar:
			data->hidedrag = tag->ti_Data;
			rfr |= ( RFR_CHANGED | RFR_REFRESH );
			break;

		case MA_Panelgroup_BackMode:
			data->backgroundmode = tag->ti_Data;
			MUI_Redraw( obj, MADF_DRAWALL );
			break;

		case MA_Panelgroup_BackColor:
			data->backgroundcolor = tag->ti_Data;
			rfr |= ( RFR_CHANGED | RFR_REFRESH );
			break;

		case MA_Panelgroup_Locked:
			data->locked = tag->ti_Data;
			FORCHILD( obj, MUIA_Group_ChildList )
			{
				SetAttrs( child, MUIA_Dropable, data->locked, TAG_DONE );
			}
			NEXTCHILD
			break;

		case MA_Panelgroup_Backdrop:
			if( tag->ti_Data )
			{
				if (data->backname)
					name_delete( data->backname );
				data->backname = name_build_sysify( (STRPTR) tag->ti_Data );
				data->has_bg = LoadPic( obj, data );
			}
			rfr |= ( RFR_CHANGED | RFR_REFRESH );
			break;
	
		case MA_Panelgroup_Zipping:
			data->allowzipping   = tag->ti_Data;
			data->enable_autozip = data->allowzipping && data->autozip;
			rfr |= RFR_CHANGED;
			break;

		case MA_Panelgroup_AutoZip:
			data->autozip = tag->ti_Data;
			data->enable_autozip = data->allowzipping && data->autozip;
			if( !init )
			{
				run_autozip_trigger( obj, data );
			}
			rfr |= RFR_CHANGED;
			break;
		case MA_Panelgroup_Horiz:
			if( data->horiz != tag->ti_Data )
			{
				// invalidate dbuff image
				if (data->dbuf != NULL)
				{
					gfx_dbuf_free(data->dbuf);
					data->dbuf = NULL;
				}
				
				// we need valid <zipsnap> values so swapem
				swap(data->zipsnap_xs, data->zipsnap_ys);
				
				data->horiz = tag->ti_Data;
				rfr |= ( RFR_CHANGED | RFR_REFRESH | RFR_NEWSIZE );
			}
			break;
		case MA_Panelgroup_New:
			
			if(( !tag->ti_Data )&&( data->newpanel ) && ( NULL != muiGlobalInfo(obj) ))
			{
				SetAttrs( _win(obj), MUIA_Window_Width , MUIV_Window_Width_Default,
				                     MUIA_Window_Height, MUIV_Window_Height_Default,
				                     TAG_DONE );    /*force MUI to rethink sizes*/
			}
			data->newpanel = tag->ti_Data;
			break;
	}
	NEXTTAG
	
	if( rfr & ( RFR_REFRESH | RFR_REFRESH2 ) )
	{
		if( !(init) )
		{            		
			DoMethod(app, MUIM_Application_PushMethod, obj, 2, MM_Panelgroup_Refresh, rfr & RFR_REFRESH2 );
		}
		MUI_Redraw( obj, MADF_DRAWALL );
	}
	if( ( rfr & RFR_NEWSIZE ) && ( !( init ) ) )
	{
		APTR win;
		if((GetAttr(MUIA_WindowObject,obj,(ULONG*)&win)&&(win))&& (NULL != muiGlobalInfo(obj)))  /* sometimes calling _win(obj) crashes so make sure there is a valid window*/
		{
			SetAttrs( _win(obj), MUIA_Window_Width , MUIV_Window_Width_Default,
			                     MUIA_Window_Height, MUIV_Window_Height_Default,
			                     TAG_DONE );    /*force MUI to rethink sizes*/
		}
	}
}

/************************************************************************/

DEFMMETHOD(AskMinMax)
{
	GETDATA;
	/*
	ULONG lbwidth = 0;
	ULONG lbheight = 0;
	ULONG rbwidth = 0;
	ULONG rbheight = 0;
	*/

	DoSuperMethodA( cl, obj, msg );
/*
	if( data->dragobj[ 0 ] )
	{
		lbwidth  = _width ( data->dragobj[ 0 ] );
		lbheight = _height( data->dragobj[ 0 ] );
	}
	if( data->dragobj[ 1 ] )
	{
		rbwidth  = _width ( data->dragobj[ 1 ] );
		rbheight = _height( data->dragobj[ 1 ] );
	}
*/
	if( !data->zipping && !data->zipped )
	{
		if( data->horiz )
		{
			msg->MinMaxInfo->MaxWidth  += 1500;
			msg->MinMaxInfo->MaxHeight += data->size;
		//	  if(msg->MinMaxInfo->MinWidth < data->size)  msg->MinMaxInfo->MinWidth += data->size;
		} else {
			msg->MinMaxInfo->MaxWidth  += data->size;
			msg->MinMaxInfo->MaxHeight += 1500;
		}
	} else {
		
		// kiero: this seems to work same way for zipped and unzipped. keep it this way for now
		if( data->horiz )
		{
			msg->MinMaxInfo->MaxWidth  += 1500;
			msg->MinMaxInfo->MaxHeight += data->size;
		//	  if(msg->MinMaxInfo->MinWidth < data->size)  msg->MinMaxInfo->MinWidth += data->size;
		} else {
			msg->MinMaxInfo->MaxWidth  += data->size;
			msg->MinMaxInfo->MaxHeight += 1500;
		}
	}
	if(data->newpanel)
	{
		//if(data->horiz) msg->MinMaxInfo->MinWidth += data->size; 
		//else msg->MinMaxInfo->MinHeight += data->size; 
	}
	DoMethod( _win(obj), MM_Panelwin_Placement, MV_Panelsubwin_Default, msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight, 0 );
	return( 0 );
}

/************************************************************************/

DEFMMETHOD(Draw)
{
	GETDATA;
	ULONG lbwidth = 0;
	ULONG lbheight = 0;
	ULONG rbwidth = 0;
	ULONG rbheight = 0;

	DoSuperMethodA( cl->cl_Super, obj, msg );  /* calling area-class method to init msg->flags, nasty but works .... */
	if(0)//data->hidden)
	{
		FillPixelArray( _rp(obj), _mleft(obj), _mtop(obj), _mwidth(obj), _mheight(obj), 0x01000000 );
		return 0;
	}
	if( ( msg->flags == MADF_DRAWUPDATE ) && ( data->drag ) )
	{
		struct tPoint box[ 4 ];
		WORD oldpattern;
		box[0].x = box[3].x	= _mleft(obj);
		box[0].y = box[1].y	= _mtop(obj);
		box[1].x = box[2].x = _mright(obj);
		box[2].y = box[3].y = _mbottom(obj);
		SetRPAttrs( _rp(obj), RPTAG_PenMode, FALSE,
								RPTAG_FgColor, 0xFFffffff,
								RPTAG_BgColor, 0xFF000000,
								TAG_DONE );
		oldpattern = _rp(obj)->LinePtrn;
		SetDrMd( _rp(obj), JAM2 );
		_rp(obj)->LinePtrn = 0x3333;
		Move( _rp(obj), box[3].x, box[3].y );
		PolyDraw( _rp(obj), 4, (WORD*) box );
		_rp(obj)->LinePtrn = oldpattern;
	}
	if( ( msg->flags & MADF_DRAWOBJECT ) )
	{
		if( data->dragobj[ 0 ] )
		{
			lbwidth  = _width ( data->dragobj[ 0 ] );
			lbheight = _height( data->dragobj[ 0 ] );
		}
		if( data->dragobj[ 1 ] )
		{
			rbwidth  = _width ( data->dragobj[ 1 ] );
			rbheight = _height( data->dragobj[ 1 ] );
		}
		if( data->zipping && data->dbuf != NULL)
		{
			switch ( data->zipdir )
			{
				case DIR_LEFTUP:
					if( data->horiz )
					{
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_DstType , BLITVAL_DstType_RastPort,
												BLITTAG_SrcX    , data->zipsnap_xs - _width(obj)+ lbwidth,
						 						BLITTAG_DstX    , _mleft(obj)  +lbwidth,
												BLITTAG_DstWidth, _width(obj)- lbwidth,
												TAG_DONE );
					} else {
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_DstType  , BLITVAL_DstType_RastPort,
												BLITTAG_SrcY     , data->zipsnap_ys - _height(obj) + lbheight ,
												BLITTAG_DstY     , _mtop(obj) + lbheight,
												BLITTAG_DstHeight, _height(obj) - lbheight,
												TAG_DONE );
					}
					break;
				case DIR_RIGHTBOTTOM:
					if (data->horiz)
					{
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_SrcX    , lbwidth,
												BLITTAG_DstX    , lbwidth,
												BLITTAG_DstType , BLITVAL_DstType_RastPort,
												BLITTAG_DstWidth,_width(obj) - (lbwidth + rbwidth) ,
												TAG_DONE );
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_SrcX    , data->zipsnap_xs - rbwidth ,
												BLITTAG_DstX    , _width(obj) - rbwidth,
												BLITTAG_DstType , BLITVAL_DstType_RastPort,
												BLITTAG_DstWidth, rbwidth ,
												TAG_DONE );
					} else {
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_SrcY     , lbheight,
												BLITTAG_DstY     , lbheight,
												BLITTAG_DstType  , BLITVAL_DstType_RastPort,
												BLITTAG_DstHeight, _height(obj) - (lbheight + rbheight),
												TAG_DONE );
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_SrcY     , data->zipsnap_ys - rbheight ,
												BLITTAG_DstY     , _height(obj) - rbheight,
												BLITTAG_DstType  , BLITVAL_DstType_RastPort,
												BLITTAG_DstHeight, rbheight ,
												TAG_DONE );
					}
					break;

			}
 
		}
		
		if (!data->zipping)
		{
			if(data->horiz)
			{
				DoMethod( obj, MM_Panelgroup_RefreshRect, _mleft(obj) + lbwidth , _mtop(obj) , _mwidth(obj), _mheight(obj), _rp(obj) );
			}
			else
			{
				DoMethod( obj, MM_Panelgroup_RefreshRect, _mleft(obj) , _mtop(obj) + lbheight , _mwidth(obj), _mheight(obj), _rp(obj) );
			}
			FORCHILD( obj, MUIA_Group_ChildList )
			{
				
				if ( _flags(child) & MADF_SHOWME )
				{
					_rp( child ) = _rp(obj);                         /* some trickery for*/
					_window( child ) = _window(obj);                 /*   zipping */
					MUI_Redraw( child, MADF_DRAWOBJECT );
				}
			}
			NEXTCHILD
		}
	}
	return( 0 );
}

/************************************************************************/
 
DEFSMETHOD(Panelgroup_RefreshRect)
{
	GETDATA;
	ULONG xoffset,yoffset; /*offsets in sourcebitmap during 1st pass */
	ULONG backgroundmode = data->backgroundmode;

	if( data->zipping )
	{
		return( 0 );
	}
	if(data->hidden)
	{
		FillPixelArray( msg->rp, msg->left, msg->top, msg->width, msg->height, 0x01000000 );
		return 0;
	}
	else 
	{
		if( backgroundmode == MV_Panelgroup_BackMode_Clone )
		{
			APTR rootwin,rootgroup;
			rootwin = (APTR) DoMethod( _win(obj), MM_Panelwin_FindRoot );
			if( ( rootgroup = (APTR) getv( rootwin, MA_Panelwin_Group ) ) )
			{
				backgroundmode = getv( rootgroup, MA_Panelgroup_BackMode );
				data->backgroundcolor = getv( rootgroup, MA_Panelgroup_BackColor );
				if( backgroundmode == MV_Panelgroup_BackMode_Picture )
				{
					STRPTR name;
					name = (STRPTR) getv( rootgroup, MA_Panelgroup_Backdrop );
					if( strcmp( data->backname, name ) )
					{
						set( obj, MA_Panelgroup_Backdrop, name );
					}
				}
			}
		}

		if( ( backgroundmode == MV_Panelgroup_BackMode_Picture) && ( data->has_bg ) )
		{
			BOOL x_cont = TRUE;
			BOOL y_cont = TRUE;
			ULONG blt_width, blt_height;
			ULONG x_pos,y_pos = msg->top;
			ULONG x_start = 0;
			ULONG y_start = 0;
			if( data->dragobj[ 0 ] ) 
			{
				if( data->horiz )
				{
					x_start +=  _width( data->dragobj[ 0 ] );
				}
				else
				{
					y_start +=  _height( data->dragobj[ 0 ] );
				}
			}
			FillPixelArray( msg->rp, msg->left, msg->top, msg->width, msg->height, 0x000000 );
	
			yoffset = (msg->top - y_start) % data->bg_height;
	
			for( y_pos = msg->top ; y_cont ; )
			{
				if( y_pos + ( data->bg_height - yoffset ) < msg->top + msg->height )
				{
					blt_height = data->bg_height -yoffset;
				} else {
					blt_height = msg->height - ( y_pos - msg->top );
					y_cont = FALSE;
				}
				x_cont = TRUE;
				xoffset = (msg->left - x_start ) % data->bg_width;
				for( x_pos = msg->left ; x_cont ; )
				{
					if( x_pos + ( data->bg_width - xoffset ) < msg->left + msg->width )
					{
						blt_width = data->bg_width - xoffset;
					} else {
						blt_width = msg->width - ( x_pos - msg->left );
						x_cont = FALSE;
					}
					WritePixelArrayAlpha( data->bg_buffer, xoffset , yoffset  , data->bg_stride, msg->rp, x_pos , y_pos, blt_width, blt_height, data->backgroundcolor );
					x_pos += ( data->bg_width - xoffset );
					xoffset = 0;
				}
				y_pos += ( data->bg_height - yoffset );
				yoffset = 0;
			}
		} else {
			FillPixelArray( msg->rp, msg->left, msg->top, msg->width, msg->height, data->backgroundcolor );
		}
	}
	return( 0 );
}

/************************************************************************/

DEFNEW
{
	if( ( obj = DoSuperNew( cl, obj,
					MUIA_Group_LayoutHook, (ULONG) &layoutpanelgroup_hook,
					MUIA_FillArea        , FALSE,
					MUIA_ContextMenu     , 1,
					TAG_DONE ) ) )
	{
			struct Data *data = (struct Data*) INST_DATA( cl, obj );
			data->newpanel    = FALSE;
			data->size        = 48;
			data->unzipped_size = 48;
			data->zipped_size = 16;
			data->mobj        = 0;
			data->active      = 0;
			data->MaxID       = 0;
			data->highlighted = 0;
			data->newpanel 	  = 0;
			data->cmenu       = NULL;
			data->moving      = 0;
			data->has_bg      = data->zipdraw = FALSE;
			data->backgroundcolor = 0xefefefef;
			data->hidden = FALSE;
			SetAttrs( obj, MUIA_Dropable, TRUE, MUIA_FillArea, FALSE, MUIA_Background, "", TAG_DONE );

			doset( obj, data, INITTAGS, TRUE );
	}
	return( (ULONG) obj );
}

/************************************************************************/

DEFDISP
{
	GETDATA;

	if( data->backname ) {
		name_delete( data->backname );
		data->backname = NULL;
	}
	return( DOSUPER );
}
/************************************************************************/

DEFADDMEMBER
{
	if( getv( msg->opam_Object, MA_Panel_Type) != MV_Panel_Type_Drag )
	{
		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );
		DoMethod(msg->opam_Object,MUIM_Notify,MA_PanelZipLock,FALSE,obj,1,MM_Panelgroup_StartZipTimer);
	}
	
	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(Group_AddHead)
{
//	GETDATA;
PDB(("\n"));
	if(  getv( msg->obj, MA_Panel_Type ) != MV_Panel_Type_Drag ) 
	{
		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );
		DoMethod(obj,MUIM_Notify,MA_PanelZipLock,FALSE,obj,1,MM_Panelgroup_StartZipTimer);
	}

	return( DOSUPER );
}
/************************************************************************/

DEFMMETHOD(Group_AddTail)
{
//	GETDATA;

	if(  getv( msg->obj, MA_Panel_Type ) != MV_Panel_Type_Drag ) 
	{
		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );
		DoMethod(obj,MUIM_Notify,MA_PanelZipLock,FALSE,obj,1,MM_Panelgroup_StartZipTimer);
	}

	return( DOSUPER );
}

/************************************************************************/
    
DEFMMETHOD(Setup)
{
	ULONG rc;
	GETDATA;

	if( ( rc = DOSUPER ) && _win(obj) )
	{
		data->ehnode.ehn_Object   = obj;
		data->ehnode.ehn_Class    = cl;
		data->ehnode.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_MOUSEOBJECT;
		data->ehnode.ehn_Priority = 1; /* XXX: ok ? */
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;
		DoMethod( _win(obj), MUIM_Window_AddEventHandler, (ULONG) &data->ehnode );

		run_autozip_trigger( obj, data );
	}
	return (rc);
}

/************************************************************************/

DEFMMETHOD(Cleanup)
{
	GETDATA;

	DoMethod( _win(obj), MUIM_Window_RemEventHandler, (ULONG) &data->ehnode );

	if( data->ihnode.ihn_Object )
	{
		DoMethod( _app(obj), MUIM_Application_RemInputHandler, (ULONG) &data->ihnode );
		data->ihnode.ihn_Object = NULL;
	}
	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(ContextMenuAdd)
{
	ULONG flags = 0;
	APTR mouse_object;
	GETDATA;
	if( !data->locked )
	{
		flags |= AS_PANEL_UNLOCKED;
	}
	
	switch( getv( _win(obj), MA_Panelwin_Type ) )
	{
		case  MV_Panelwin_Type_Root:
			if( ( mouse_object = (APTR) getv( _win(obj), MUIA_Window_MouseObject ) ) )
			{
				if( getv( mouse_object, MA_Panel_Type ) == MV_Panel_Type_Drag )
				{
					data->cmenu	= contextmenu_build_simple( CM_PANELGROUP_DRAG, flags );
				} else {
					data->cmenu	= contextmenu_build_simple( CM_PANELGROUP, flags );
				}
			}
			if( data->cmenu )
			{
				contextmenu_item_check( data->cmenu, "Panel ToggleLock", data->locked );
			}
			break;       	
		 case  MV_Panelwin_Type_SubPanel:
			data->cmenu	= contextmenu_build_simple( CM_PANELGROUP_SUBPANEL, flags );

			if( data->cmenu )
			{
				contextmenu_item_check( data->cmenu, "Panel ToggleLock", data->locked );
				contextmenu_item_check( data->cmenu, "Panel Stayopen"  , getv( _win(obj), MA_Panelsubwin_StayOpen ) );
			}
			break;
		case  MV_Panelwin_Type_DirPanel:
			data->cmenu	= contextmenu_build_simple( CM_PANELGROUP_DIRPANEL, flags );
			if( data->cmenu )
			{
				contextmenu_item_check( data->cmenu, "Panel Stayopen", getv( _win(obj), MA_Panelsubwin_StayOpen ) );
			}
			break;
	};

	if( data->cmenu )
	{
		DoMethod( msg->menustrip, OM_ADDMEMBER, data->cmenu );
	}
	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(ContextMenuChoice)
{
	GETDATA;
	ULONG rc = 0;
	struct command_menu *cm;

	if( data->cmenu && DoMethod( data->cmenu, MUIM_FindObject, msg->item ) )
	{
		if ( ( ( cm = (struct command_menu *) getv( msg->item, MA_Menuitem_Command ) ) ) )
		{
			if( cm->name && *cm->name )
				execute_command( _parent(obj), cm->type, cm->args, NULL );
		}
	} else {
		rc = DOSUPER;
	}
	data->cmenu = NULL; /* ??? */

	return( rc );
}

/************************************************************************/
  
DEFMMETHOD(HandleEvent)
{
	GETDATA;
	APTR mobj;
	struct List *button_list;
	Object *button;
	APTR object_state;

	if( ( data->zipping ) || ( data->zipped ) )
	{ 
		return( 0 );
	}
	if(data->hidden)
	{
		PDB(("%x\n",msg->imsg->Class));
		data->hidden = FALSE;
		run_autozip_trigger( obj, data );
		MUI_Redraw( obj, MADF_DRAWALL );
	}
	else
	{
		switch( msg->imsg->Class )
		{
			case IDCMP_MOUSEBUTTONS:
				if( (msg->imsg->Code == SELECTUP) || ( msg->imsg->Code == MIDDLEUP ) )
				{
					if( data->moving )
					{
						FORCHILD( obj, MUIA_Group_ChildList )
						{
							SetAttrs( child, MUIA_InputMode, MUIV_InputMode_RelVerify, TAG_DONE );
						}
						NEXTCHILD
						if( getprefslong( DSI_PANEL_AUTOSAVE_MOVE ) )
						{
							DoMethod( _win(obj), MM_Panelwin_SaveConfig );
						}
						pointer_clear( _window(obj) );
						data->moving = FALSE;
					}
				}
				else if( msg->imsg->Code == MIDDLEDOWN )
				{
					GetAttr( MUIA_Window_MouseObject, _win(obj), (ULONG*) &data->active );
					if( data->active )
					{
						data->moving = TRUE;
						FORCHILD( obj, MUIA_Group_ChildList )
						{
							SetAttrs( child, MUIA_InputMode, MUIV_InputMode_None, TAG_DONE );
						}
						NEXTCHILD
						pointer_set( _window(obj), POINTER_MOVE );
					}
				}
			break;
		
			case IDCMP_MOUSEOBJECT:
				GetAttr( MUIA_Window_MouseObject, _win(obj), (ULONG*) &mobj );
				if( data->moving )
				{
					ULONG pos = 0;
					PDB(("\n"));
					if( (mobj) && ( (ULONG) mobj != (ULONG) data->active ) )
					{
						GetAttr( MUIA_Group_ChildList, obj, (ULONG*) &button_list );
						object_state = button_list->lh_Head;
						if( data->horiz )
						{
							while( ( button = (Object*) NextObject( &object_state ) ) && ( _mright(button) < msg->imsg->MouseX ) )
							{
								pos++;
							}
						} else {
							while( ( button = (Object*) NextObject( &object_state ) ) && ( _mbottom(button) < msg->imsg->MouseY ) )
							{
							pos++;
							}
						}
						DoMethod( obj,MUIM_Group_InitChange );
						DoMethod( obj,MUIM_Group_MoveMember, data->active, pos );
						DoMethod( obj,MUIM_Group_ExitChange );
						set(obj, MA_Panelgroup_HasChanged, TRUE );
						MUI_Redraw( mobj, MADF_DRAWOBJECT );
						MUI_Redraw( data->active, MADF_DRAWOBJECT );
					}
				} else {
					if( data->mobj )
					{
						set( data->mobj, MA_Panel_Highlighted, FALSE );
					}
					data->mobj = mobj;
					if( data->mobj )
					{
						data->active = data->mobj;
					}
					if(data->mobj)
					{
						if(getv( data->mobj, MA_Panel_Type) == MV_Panel_Type_Drag) data->mobj = 0;
						else set( data->mobj, MA_Panel_Highlighted, TRUE );
					}
					if(!data->mobj) DoMethod(obj,MM_Panelgroup_StartZipTimer); 
				}
				break;
		}
	}
	return( 0 );
}

/************************************************************************/

DEFMMETHOD(DragDrop)
{
	STRPTR path;
	GETDATA;
	ASSERT(msg->obj);

	if( get(msg->obj, MA_Icon_PathInfo, &path ) )
	{
		int pos = 0;
		ULONG rx,ry,x,y;
		Object *nbutton;
		APTR object_state;
		struct List *button_list;
		Object *button,*cbutton = 0;

		GetAttr( MUIA_Window_LeftEdge, _win(obj), &x );
		GetAttr( MUIA_Window_TopEdge , _win(obj), &y );
		rx = msg->x - x;
		ry = msg->y - y;
		GetAttr( MUIA_Window_MouseObject, _win(obj), (ULONG*) &cbutton );
		GetAttr( MUIA_Group_ChildList   , obj      , (ULONG*) &button_list );
		object_state = button_list->lh_Head;
		if( data->horiz )
		{
			while( ( button = (Object*) NextObject( &object_state ) ) && ( ( _mright( button ) + _mleft( button ) ) / 2 < rx ) )
			{
				pos++;
			}
		} else {
			while( ( button = (Object*) NextObject( &object_state ) ) && ( ( _mbottom( button ) + _mtop( button ) ) / 2 < ry ) )
			{
				pos++;
			}
		}

		if( ( data->locked ) )
		{
			pos = -1;    /* must have been dropped over draggadget insert at end */
		}
		if( ( nbutton = (Object*) NewObject( getpanelcommandbuttonclass(), NULL,
																MA_Panel_URI      , getv( msg->obj, MA_Icon_Path ),
																MA_Panel_Imagepath, path,
																TAG_DONE ) ) )
		{
			DoMethod( obj, MUIM_Group_InitChange);
			DoMethod( obj, OM_ADDMEMBER, nbutton );
			DoMethod( obj, MUIM_Group_MoveMember, nbutton, pos );
			DoMethod( obj, MUIM_Group_ExitChange );
			SetAttrs( _win(obj), MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );
			if( getprefslong( DSI_PANEL_AUTOSAVE_DROP ) )
			{
				DoMethod( _win(obj), MM_Panelwin_SaveConfig );
			}
			set( obj, MA_Panelgroup_HasChanged, TRUE );
		}
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelgroup_SaveConfig)
{
	GETDATA;
	if( !data->zipped ) /* XXX: actually we could save a zipped state as well but it might be tricky */
	{
		APTR pl, pi; /* list, item */
		ULONG index = 0;
		ULONG type = 0;

		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );  /*we ain't new anymore get rid of the dodo*/
		prefspool_item_remove( msg->pctx, NULL, DSI_LISTPOOL_PANEL ); /* purge the whole prefs so that it doesn't mess when items are removed */

		pl = prefspool_item_add( msg->pctx, NULL, DSI_LISTPOOL_PANEL, NULL, (ULONG)NULL );
		setprefslong_ctx( msg->pctx, DSI_PANELGROUP_BACKMODE, data->backgroundmode );
		setprefslong_ctx( msg->pctx, DSI_PANELGROUP_BACKCOLOR, data->backgroundcolor );
		if( data->backgroundmode == MV_Panelgroup_BackMode_Picture )
		{
			setprefsstr_ctx( msg->pctx, DSI_PANELGROUP_BACKDROP, data->backname );
		}
		if( pl )
		{
			FORCHILD( obj, MUIA_Group_ChildList )
			{
				if( !( pi = prefspool_item_get( msg->pctx, pl, index | DSF_LISTPOOL, NULL, NULL ) ) )
				{
					pi = prefspool_item_add( msg->pctx, pl, index | DSF_LISTPOOL, NULL, (ULONG)NULL );
				}

				if( pi )
				{
					setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_SUBPANEL, 0 );

					type = getv( child, MA_Panel_Type );
					if( ( type != MV_Panel_Type_External ) )
					{
						setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_TYPE, type );
						index += DoMethod( child, MM_Panel_SaveConfig, msg->pctx, index );  /* subpanels will add items here*/
					} else {
						STRPTR classname;
						APTR supobj;
						data->ext_pctx = msg->pctx;
						data->ext_pi   = pi;
						data->ext_pl   = pl;

						setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_TYPE, MV_Panel_Type_External );
						get( child, MA_Panel_Extern_ClassName, (ULONG*) &classname );
						setprefsstr_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_EXT_NAME, classname ? classname : (STRPTR)"" );

						if( ( get( child, MA_Panelextern_SupportObject, (ULONG*) &supobj ) ) && ( supobj ) )
						{
							DoMethod( supobj, MM_Panelsupport_Saveconfig, msg->pctx, pi );
						}
						else
						{
							index += DoMethod( child, MM_Panel_SaveConfig, msg->pctx, pi ); 
						}
						data->ext_pctx = data->ext_pi = data->ext_pl = 0;
					}

                   
				}
				/* XXX */
				index++;
			}
			NEXTCHILD
		}
	}
	return (0);
}

/************************************************************************/

DEFSMETHOD(Panelgroup_ToggleZip)
{

	GETDATA;
	APTR rootobj;
	if( data->zipping )
	{
		return( 0 );
	}
	PDB(("%x\n",data->zipped));
	GetAttr( MUIA_Window_RootObject, _win(obj), (ULONG*) &rootobj );
	switch( msg->type )
	{
		case MV_Paneldrag_Type_LeftUp:
			data->zipdir = DIR_LEFTUP;
			break;

		case MV_Paneldrag_Type_RightBottom:
			data->zipdir = DIR_RIGHTBOTTOM;
			break;

		#ifdef DEBUG
		default:
			PDB(("argh, wrong zipdir\n"));
			break;
		#endif
	}

	if( data->zipped )
	{
		ULONG zipspeed = _conf( panel_zipspeed );
		
	/*	data->zipped = FALSE;
		set(obj,MA_Panelgroup_Size,data->unzipped_size);
*/
		if(data->dbuf == NULL)
		{
			// no dbuff as it is most likely invalid
			zipspeed = MV_Panel_ZipSpeed_Instant;
		}

		data->zipping = TRUE;

		set( obj, MA_Paneldrag_Zipping, TRUE ); /* broadcast */
		PLAYSFX( panelunzip );
		
		do_action( obj, TA_Panels_Zip,
								TT_Panels_Zip_Direction, FALSE,
								TT_Panels_Zip_Window   , _window(obj),
								TT_Panels_Zip_XS       , data->zipsnap_xs,
								TT_Panels_Zip_YS       , data->zipsnap_ys,
								TT_Panels_Zip_Reversed , ( data->zipdir == DIR_RIGHTBOTTOM ) ? TRUE : FALSE,
								TT_Panels_Zip_ZipSpeed , zipspeed,
								TAG_DONE );

	} else {
	/*data->zipped = TRUE;
	set(obj,MA_Panelgroup_Size,data->zipped_size);
	*/
		/*force all subpanels to close*/
		FORCHILD( obj, MUIA_Group_ChildList )
		{
			DoMethod(child,MM_Panelbutton_Close);
		}
		NEXTCHILD
		
		if(data->allowzipping )
		{
			data->zipsnap_xs = _width ( rootobj );
			data->zipsnap_ys = _height( rootobj );

			ASSERT( _window( obj ) );

			if( ( data->dbuf = gfx_dbuf_alloc_reuse( data->dbuf, data->zipsnap_xs, data->zipsnap_ys, DBUF_DISPLAYABLE, _screen( obj )->RastPort.BitMap ) ) )
			{
				struct RastPort *oldrp;
				struct Window *oldwin;
				
				oldrp        = _rp(obj);
				oldwin       = _window(obj);
				_rp(obj)     = data->dbuf->rp;
				_window(obj) = NULL;

				MUI_Redraw( obj, MADF_DRAWALL );

				_rp(obj)      = oldrp;
				_window(obj)  = oldwin;
				
				data->zipping = TRUE;
			
				/*
				 * Set all the objects (except paneldrags) to
				 * hidden.
				 */
				DoMethod( obj, MUIM_Group_InitChange );
				{
					FORCHILD( obj, MUIA_Group_ChildList )
					{
						if( getv( child, MA_Panel_Type ) == MV_Panel_Type_Drag )
						{
							continue;
						}
						_flags(child) &= ~MADF_SHOWME;
					}
					NEXTCHILD
				}
				DoMethod( obj, MUIM_Group_ExitChange );

				set( obj, MA_Paneldrag_Zipping, TRUE ); /* broadcast */

				PLAYSFX( panelzip );
                data->zipping = TRUE;
				do_action( obj, TA_Panels_Zip,
										TT_Panels_Zip_Direction, TRUE,
										TT_Panels_Zip_Window   , _window(obj),
										TT_Panels_Zip_XS       , _minwidth(obj),
										TT_Panels_Zip_YS       , _minheight(obj),
										TT_Panels_Zip_Reversed , ( data->zipdir == DIR_RIGHTBOTTOM ) ? TRUE : FALSE,
										TT_Panels_Zip_ZipSpeed , _conf( panel_zipspeed ),
										TAG_DONE );
			}
			/* XXX */
		}
	}
	MUI_Redraw( obj, MADF_DRAWALL );

	return( 0 );
}
#if 0
/************************************************************************/

DEFSMETHOD(Panelgroup_ToggleResize)
{
	return( 0 );
}

/************************************************************************/
#endif
DEFTMETHOD(Panelgroup_DelayedZip)
{
	GETDATA;
	PDB(("%x\n",&data->ihnode));
	if( data->ihnode.ihn_Object != NULL )
	{
		DoMethod( _app(obj), MUIM_Application_RemInputHandler, &data->ihnode );
		data->ihnode.ihn_Object = NULL;
	}
//	data->hidden = TRUE;
	if(!data->zipped && data->autozip && data->allowzipping  && !DoMethod(obj,MM_Panelgroup_CheckZipLock) && !data->mobj) /* prefs may have changed when delaying... */
	{
		DoMethod( obj, MM_Panelgroup_ToggleZip, data->zipdir );
	}
	MUI_Redraw( obj, MADF_DRAWALL );

	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Thread_Finished)
{
	GETDATA;
	ASSERT(msg->action == TA_Panels_Zip);
	
	data->zipping = FALSE;
	if( msg->taglist && GetTagData( TT_Panels_Zip_Direction, FALSE, msg->taglist ) )
	{
		data->zipped = TRUE;
	} else {
		data->zipped = FALSE;
		/*
		 * Put back all objects as visible.
		 */
		DoMethod( obj, MUIM_Group_InitChange );
		{
			FORCHILD( obj, MUIA_Group_ChildList )
			{
				if( getv( child, MA_Panel_Type ) == MV_Panel_Type_Drag )
				{
					continue;
				}
				_flags(child) |= MADF_SHOWME;
			}
			NEXTCHILD
		}
		DoMethod( obj, MUIM_Group_ExitChange ); /* XXX: sigh.. it flickers */

		// no need for dbuff anymore
		if (data->dbuf != NULL)
		{
			gfx_dbuf_free(data->dbuf);
			data->dbuf = NULL;
		}
		
		/* XXX: start autozip timer. crap. imagine something better. */

		run_autozip_trigger( obj, data );
	}
	set( obj, MA_Paneldrag_Zipping, FALSE ); /* broadcast */
	if(data->dragobj[ 0 ] ) MUI_Redraw( data->dragobj[ 0 ], MADF_DRAWALL ); /* make sure hidden dragbars are shown while zipped*/
	if(data->dragobj[ 1 ] ) MUI_Redraw( data->dragobj[ 1 ], MADF_DRAWALL ); /* and hidden again when unzipped*/
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelgroup_CheckZipLock)
{
//	  GETDATA;
	ULONG type, ziplock;
	APTR subwin,subwintbar;
	FORCHILD( obj, MUIA_Group_ChildList )
	{
		ziplock = 0;
		get( child,	MA_PanelZipLock,&ziplock);
		if(ziplock) return ( TRUE );
		get( child, MA_Panel_Type,&type );
		if( type == MV_Panel_Type_SubPanel )
		{
			get( child , MA_Panelbutton_AttachedObject, (ULONG*) &subwin );
			get( subwin, MA_Panelwin_Group, (ULONG*) &subwintbar );
			if(DoMethod(subwintbar,MM_Panelgroup_CheckZipLock)) return (TRUE);
		}
	}
	NEXTCHILD

	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelgroup_FindSubPanel)
{
	ULONG type, ID;
	APTR subwin,subwintbar;
	PDB(("subsearch %d\n",msg->ID));
	FORCHILD( obj, MUIA_Group_ChildList )
	{
		get( child, MA_Panel_Type,&type );
		if( type == MV_Panel_Type_SubPanel )
		{
			get( child , MA_SubPanel_ID, &ID );
			get( child , MA_Panelbutton_AttachedObject, (ULONG*) &subwin );
			get( subwin, MA_Panelwin_Group, (ULONG*) &subwintbar );
			PDB(("msg->ID %x ID %x\n",msg->ID,ID));
			if( ID == msg->ID )
			{
				PDB(("subfound\n",subwintbar));
				return( (ULONG)subwintbar );
			}
			else
			{
				ULONG subsub = 0;
				subsub = DoMethod(subwintbar,MM_Panelgroup_FindSubPanel,msg->ID);
				PDB(("subsub %x\n",subsub));
				if(subsub != 0) return subsub;
			}
		}
	}
	NEXTCHILD
	return( 0 );
}
 
/************************************************************************/

DEFGET
{
	GETDATA;
	ULONG result = TRUE;
	switch( msg->opg_AttrID )
	{ 
		case MA_Panelgroup_Size:
			*msg->opg_Storage = data->size;
			break;
		case MA_Panelgroup_Horiz:
			*msg->opg_Storage = data->horiz;
			break; 
		case MA_Panelgroup_Backdrop:
			*msg->opg_Storage  = (ULONG)data->backname;
			break;
		case MA_Panelgroup_DragMode:
			*msg->opg_Storage = data->dragmode;
			break;
		case MA_Panelgroup_BackMode:
			*msg->opg_Storage  = data->backgroundmode;
			break;
		case MA_Panelgroup_BackColor:
			*msg->opg_Storage  = data->backgroundcolor;
			break;
		case MA_Panelgroup_HasChanged:
			/* Always TRUE just to trigger the notify. */
			*msg->opg_Storage = TRUE;
			break;
		case MA_Panel_HasMoved:
			/* Always TRUE just to trigger the notify. */
			*msg->opg_Storage = TRUE;
			break;
		case  MA_Panelgroup_Zipped:
			*msg->opg_Storage  = data->zipped;
			result = DOSUPER;
			break;
		case MA_Panelgroup_Locked:
			*msg->opg_Storage = data->locked;
			break;
		case MA_Panelgroup_Zipping:
			*msg->opg_Storage = data->allowzipping;
			break;
		case MA_Panelgroup_AutoZip:
			*msg->opg_Storage = data->autozip;
			break;
		case MA_Panelgroup_DragBar1:
			*msg->opg_Storage = (ULONG)data->dragobj[0];
			break;
		case MA_Panelgroup_DragBar2:
			*msg->opg_Storage = (ULONG)data->dragobj[1];
			break;
		case MA_Panelgroup_HideDragBar:
			*msg->opg_Storage = data->hidedrag;
			break;
		default:
			result = DOSUPER;
	}
	return( result );
}

/************************************************************************/

DEFSET
{
	GETDATA;

	doset( obj, data, INITTAGS,FALSE );

	return( DOSUPER );
}

/************************************************************************/

DEFMMETHOD(DragQuery)
{
   
	return( MUIV_DragQuery_Accept );
}

/************************************************************************/

DEFMMETHOD(DragFinish)
{
	ULONG rc;
	GETDATA;
	rc = DOSUPER;
	data->drag = FALSE;
	MUI_Redraw( obj, MADF_DRAWOBJECT );
	
	return (rc);
}

/************************************************************************/

DEFMMETHOD(DragBegin)
{
	ULONG rc;
	GETDATA;

	rc = DOSUPER;
	data->drag = TRUE;

	MUI_Redraw( obj, MADF_DRAWUPDATE );
	return( rc );
}

/************************************************************************/

DEFMMETHOD(Group_ExitChange)
{
	GETDATA;

	/*
	 * Reposition the dragbars properly
	 * if they exists.
	 */
	if ( data->dragobj[ 0 ] )
	{
		DoMethod( obj, MUIM_Group_MoveMember, data->dragobj[ 0 ], 0 );
	}

	if( data->dragobj[ 1 ] )
	{
		DoMethod( obj, MUIM_Group_MoveMember, data->dragobj[ 1 ], -1 );
	}
	return( DOSUPER );
}

/************************************************************************/

#define ADDDRAGOBJ(x) ({if (data->dragobj[x]) \
	{ \
		DoMethod( obj, OM_REMMEMBER, data->dragobj[ x ] ); \
		MUI_DisposeObject( data->dragobj[ x ] ); \
	} \
	if ( (data->dragobj[ x ] = NewObject( getpaneldragclass(), NULL, MA_Paneldrag_Type, x, TAG_DONE ) ) ) \
	{ \
		DoMethod( obj, OM_ADDMEMBER, data->dragobj[ x ] ); \
		DoMethod( obj, MUIM_Group_MoveMember, data->dragobj[ x ], x ? -1 : 0 ); \
	}})

#define REMDRAGOBJ(x) ({if (data->dragobj[x]) \
	{ \
		DoMethod( obj, OM_REMMEMBER, data->dragobj[ x ] ); \
		MUI_DisposeObject( data->dragobj[ x ] ); \
		data->dragobj[ x ] = NULL; \
	}})

DEFSMETHOD(Panelgroup_ChangeDrag)
{
	GETDATA;

	DoMethod( obj, MUIM_Group_InitChange );

	data->dragmode = msg->dragmode;

	switch( data->dragmode )
	{
		case MV_Panelgroup_DragMode_None:
			REMDRAGOBJ( 0 );
			REMDRAGOBJ( 1 );
			break;

		case MV_Panelgroup_DragMode_LeftUp:
			ADDDRAGOBJ( 0 );
			REMDRAGOBJ( 1 );
			data->zipdir = DIR_LEFTUP;
			break;

		case MV_Panelgroup_DragMode_RightBottom:
			REMDRAGOBJ( 0 );
			ADDDRAGOBJ( 1 );
			data->zipdir = DIR_RIGHTBOTTOM;
			break;

		case MV_Panelgroup_DragMode_Both:
			ADDDRAGOBJ( 0 );
			ADDDRAGOBJ( 1 );
			break;

		#ifdef DEBUG
		default:
			PDB(("unimplemented dragmode: %ld\n", data->dragmode));
			break;
		#endif
	}

	set( obj, MA_Paneldrag_LockMode, data->locked ); /* broadcast */

	DoMethod( obj, MUIM_Group_ExitChange );
	
	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelgroup_Clear)
{
	GETDATA;

	DoMethod( obj, MUIM_Group_InitChange );
	FORCHILD( obj, MUIA_Group_ChildList )
	{
		if( ( child != data->dragobj[ 0 ] ) && ( child != data->dragobj[ 1 ] ) )
		{
			DoMethod( obj, MUIM_Group_Remove, child );
		}
	}
	NEXTCHILD
	DoMethod( obj, MUIM_Group_ExitChange );

	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelgroup_RemoveItem)
{
	GETDATA;
	APTR o;
	if( msg->o == MV_Panelgroup_RemoveItem_Current )
	{
		o = data->active;
		ASSERT(o);
	} else {
		o = msg->o;
	}

	DoMethod( obj, MUIM_Group_InitChange );
	DoMethod( obj, OM_REMMEMBER, o );
	DoMethod( obj, MUIM_Group_ExitChange );

	SetAttrs( _win( obj ), MUIA_Window_Width , MUIV_Window_Width_Default , TAG_DONE );
	SetAttrs( _win( obj ), MUIA_Window_Height, MUIV_Window_Height_Default, TAG_DONE );

	data->highlighted = data->active = data->mobj = 0;

	set( obj, MA_Panelgroup_HasChanged, TRUE );

	MUI_DisposeObject( o );

	if( getprefslong( DSI_PANEL_AUTOSAVE_DELETE ) )
	{
		DoMethod( _win(obj), MM_Panelwin_SaveConfig );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelgroup_MoveMode_Start)
{
	GETDATA;
	GetAttr( MUIA_Window_MouseObject, _win(obj), (ULONG*) &data->active );
	if( data->active )
	{
		data->moving = TRUE;
			PDB(("\n"));
		FORCHILD( obj, MUIA_Group_ChildList )
		{
			SetAttrs( child, MUIA_InputMode, MUIV_InputMode_None, TAG_DONE );
		}
		NEXTCHILD
		pointer_set( _window(obj), POINTER_MOVE );
	}
	return( 0 );
}

/************************************************************************/

DEFSMETHOD(Panelgroup_Lock)
{
	SetAttrs( obj,
				MA_Paneldrag_LockMode, msg->lock, /* broadcast */
				MA_Panelgroup_Locked, msg->lock,
				TAG_DONE );

	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelgroup_ToggleLock)
{
	GETDATA;

	data->locked ^= 1;

	SetAttrs( obj,
				MA_Paneldrag_LockMode, data->locked, /* broadcast */
				MA_Panelgroup_Locked, data->locked,
				TAG_DONE );

	return( 0 );
}

/************************************************************************/

DEFTMETHOD(Panelgroup_Delete)
{
	if( muiRenderInfo(obj) && _win(obj) )
	{
		smartreq_request( _win(obj), _win(obj), GSI( MSG_PANELGROUPCLASS_TITLE), MM_Panelwin_Delete_Ok, 0, GSI( MSG_PANELGROUPCLASS_DELETECANCEL), MV_Notification_Warning, GSI( MSG_PANELGROUPCLASS_DELETEMSG ), NULL ); /* XXX: errmsg or so.. the displaybeep stuff */
	}
	/* XXX */
	return( 0 );
}

DEFTMETHOD(Panelgroup_OpenParent) // bitRocky
{
	GETDATA;
	APTR pi = data->active; // active panelitem
	
	if( pi )
	{
		STRPTR uripath;

		if( get( pi, MA_Panel_URI, &uripath ) && uripath && uripath[0] )
		{
			TEXT buf[ PATH_SIZE + 256 ]; /* should be enough (tm) */
			STRPTR pp;
			char ch;
			
			if( (pp = PathPart(uripath)) ) { ch = pp[0]; pp[0] = 0; }
			
			snprintf( buf, sizeof( buf ), "LoadURI \"%s\" TOFRONT", uripath );
			execute_command( obj, AC_INTERNAL, buf, NULL );

			if (pp) pp[0] = ch;
		}
	}
	
	return( 0 );
}
/************************************************************************/

BEGINMTABLE
DECNEW
DECDISPOSE
DECADDMEMBER
DECMMETHOD(Group_AddHead)
DECMMETHOD(Group_AddTail)
DECMMETHOD(Setup)
DECMMETHOD(Cleanup)
DECMMETHOD(AskMinMax)
DECMMETHOD(Draw)
DECMMETHOD(DragDrop)
DECMMETHOD(HandleEvent)
DECMMETHOD(ContextMenuAdd)
DECMMETHOD(ContextMenuChoice)
DECTMETHOD(Panelgroup_StartZipTimer)
DECSMETHOD(Panelgroup_MoveMode_Start)
DECTMETHOD(Panelgroup_Delete)
DECSMETHOD(Panelgroup_ToggleZip)
DECTMETHOD(Panelgroup_DelayedZip)
DECTMETHOD(Panelgroup_CheckZipLock)
//DECSMETHOD(Panelgroup_ToggleResize)
DECSMETHOD(Panelgroup_Lock)
DECTMETHOD(Panelgroup_ToggleLock)
DECSMETHOD(Thread_Finished)
DECSMETHOD(Panelgroup_SaveConfig)
DECSMETHOD(Panelgroup_FindSubPanel)
DECSMETHOD(Panelgroup_RefreshRect)
DECMMETHOD(DragQuery)
DECMMETHOD(DragFinish)
DECMMETHOD(DragBegin)
DECSMETHOD(Panelgroup_ChangeDrag)
DECMMETHOD(Group_ExitChange)
DECTMETHOD(Panelgroup_Clear)
DECSMETHOD(Panelgroup_RemoveItem)
DECSMETHOD(Panelgroup_OpenParent)
DECGET
DECSET

ENDMTABLE

DECSUBCLASS_NC(MUIC_Group, panelgroupclass)

#endif

