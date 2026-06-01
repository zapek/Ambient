#include <libraries/mui.h>

#include <cybergraphx/cybergraphics.h>
#include <graphics/rpattr.h>
#include <libraries/mui.h>


#include "muifuncs.h"
#include "MUIClasses.h"

#include "debug.h"

#include <proto/panel.h>
#include <libraries/panel.h>


#include "../prefs.h"
#include "../name.h"


#include "gfx_bitmap.h"
#include "gfx_blit.h"
#include "gfx_dbuf.h"

#include "pointer.h"

#include "paneltags.h"

#include "panellib.h"

#include <proto/cybergraphics.h>
#include <proto/datatypes.h>
#include <proto/ambient.h>

#include <AppWindow.h>

#warning put in some include

struct MUI_CustomClass *DefaultGroupClass_Class(void);
extern ULONG panel_modus;
/************************************************************************/

#define CM_SAVE_PANEL	1001
#define CM_PREFS		1002
#define CM_LOCKED		1003
#define CM_NEW_PANEL	1004
#define CM_DEL_PANEL	1005
#define CM_DEL_OBJECT	1006
#define CM_MOVE_OBJECT	1007
#define CM_QUIT			1008
/*#define CM_SUBPANEL      1009
#define CM_DIRPANEL      1010
#define CM_DIGICLOCK     1011
#define CM_FLIP          1012*/

             
#define RFR_CHANGED  (1 << 0UL)
#define RFR_REFRESH  (1 << 1UL)
#define RFR_REFRESH2 (1 << 2UL)
#define RFR_NEWSIZE  (1 << 3UL)

/************************************************************************/
/*
 * min()/max()/abs() without macro side effects.
 */
#define max(a,b) \
	({typeof(a) _a = (a); \
	typeof(b) _b = (b);	\
	_a > _b ? _a : _b;})

#define min(a,b) \
	({typeof(a) _a = (a); \
	typeof(b) _b = (b); \
	_a > _b ? _b : _a;})

#define abs(a) \
	({typeof(a) _a = (a); \
	_a < 0 ? -_a : _a;})

#define pos(x) \
	({typeof(x) _x = (x); \
	_x > 0 ? _x : 0;})

#define neg(x) \
	({typeof(x) _x = (x); \
	_x < 0 ? _x : 0;})

#define minmax(a,x,b) (max((a),min((x),(b))))

#define swap(a,b) \
	({typeof(a) _swp = a; \
	a = b; b = _swp;})



struct Data
{
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
	ULONG horiz,grid;
	ULONG dragmode;
	ULONG drag;
	ULONG max_w, max_h;
	APTR dragobj[2];
	APTR droptarget;
	ULONG hidedrag;
	ULONG dockmode;
    	/* zip mode */
	ULONG allowzipping;
	ULONG modus;
	ULONG zipdraw;
	ULONG zipped;
	ULONG zipping;
	ULONG zipdir; /* DIR_LEFTUP or DIR_RIGHTBOTTOM */
	ULONG zipsnap_xs; /* original size of the panel */
	ULONG zipsnap_ys;
	struct doublebuf *dbuf;
    	/* autozip */

	ULONG autozip, enable_autozip;
	struct MUI_EventHandlerNode ehnode;
	struct MUI_InputHandlerNode ihnode;
	BOOL m_hover;

	APTR ext_pctx,ext_pi,ext_pl; /* only valid for an external objects saveconfig method*/
};
static VOID run_autozip_trigger( APTR obj, struct Data *data );
/*   */

static struct Hook layout_panelgroup_hook;


static ULONG LoadPic( APTR obj , struct Data *data )
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
					ReadPixelArray( data->bg_buffer, 0, 0, data->bg_stride, &mask, 0, 0, data->bg_width, data->bg_height, RECTFMT_ARGB );
					retval = TRUE;
				}
			}
		}
		DisposeDTObject( picture );
	}
	return( retval );
}


static void doset( APTR obj, struct Data *data,struct TagItem *tags,BOOL init )
{
	struct TagItem *tstate = tags, *tag;
	ULONG rfr = 0;
	while ((tag = (struct TagItem *) NextTagItem(&tstate)))
	{
		if (init)
		{
			switch (tag->ti_Tag)
			{
			}
		}
		
		switch (tag->ti_Tag)
		{
			case MA_Panelgroup_FixedMode:
				if(data->zipped)DoMethod( obj , MM_Panelgroup_ToggleZip,MV_Paneldrag_Type_LeftUp );
				data->modus = tag->ti_Data;;
				break;
			case MA_Panelgroup_DockMode:
				data->dockmode = tag->ti_Data;
				if(init)
				{
					if ( data->dockmode ) data->dragmode = MV_Panelgroup_DragMode_None;
				}
	
			
				rfr |= ( RFR_CHANGED | RFR_REFRESH | RFR_NEWSIZE );
				break;
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
					rfr = ( RFR_CHANGED | RFR_NEWSIZE );
				}
				break;
#warning
				
			case MA_Panelgroup_HideDragBar:
				data->hidedrag = tag->ti_Data;
				rfr |= ( RFR_CHANGED | RFR_REFRESH );
				break;

			case MA_Panelgroup_BackMode:
				data->backgroundmode = tag->ti_Data;
				MUI_Redraw( obj, MADF_DRAWALL );
				break;
			case MA_Panelgroup_Alpha:
				{
				ULONG alpha = tag->ti_Data << 24;
				data->backgroundcolor = ( data->backgroundcolor & 0x00FFFFFF ) + alpha;
				rfr |= ( RFR_CHANGED | RFR_REFRESH );
				}
				break;
			case MA_Panelgroup_BackRGB:
				data->backgroundcolor = ( data->backgroundcolor & 0xFF000000 ) + tag->ti_Data;
				rfr |= ( RFR_CHANGED | RFR_REFRESH );
				break;
			case MA_Panelgroup_BackColor:
				data->backgroundcolor = tag->ti_Data;
				rfr |= ( RFR_CHANGED | RFR_REFRESH );
				break;
	
			case MA_Panelgroup_Locked:
				data->locked = tag->ti_Data;
			/*	FORCHILD( obj, MUIA_Group_ChildList )
				{
					SetAttrs( child, MUIA_Dropable, data->locked, TAG_DONE );
				}
				NEXTCHILD*/
			break;

			case MA_Panelgroup_Backdrop:
				if( tag->ti_Data )
				{
					if (data->backname)
						name_delete( data->backname );
					if((data->backname = name_build_sysify( (STRPTR) tag->ti_Data )))
					{
						STRPTR backdrop = (STRPTR)tag->ti_Data;
					
#warning
						if((data->has_bg = LoadPic( obj, data )))
							data->backgroundmode = MV_Panelgroup_BackMode_Picture;
					}
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
#warning
			//						gfx_dbuf_free(data->dbuf);
						data->dbuf = NULL;
					}
					
				// we need valid <zipsnap> values so swapem
						swap(data->zipsnap_xs, data->zipsnap_ys);
				
						data->horiz = tag->ti_Data;
					rfr |= ( RFR_CHANGED | RFR_REFRESH | RFR_NEWSIZE );
			}
					break;
			case MA_Panelgroup_GridMode:
				data->grid = tag->ti_Data;
				rfr |= ( RFR_CHANGED | RFR_REFRESH | RFR_NEWSIZE );
				
				break;
			case MA_Panelgroup_New:
				data->newpanel = tag->ti_Data;
				break;
			case MA_Panelgroup_HasChanged:
			
				
				break;
		}
	}
	if( rfr & ( RFR_REFRESH | RFR_REFRESH2 ) )
	{
		if( !(init) )
		{            
			DoMethod(obj, MM_Panelgroup_Refresh, rfr & RFR_REFRESH2 );
		}
	//PDB(("redraw\n"));
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
			DoMethod(_win(obj),MM_Panelwin_AttachToBorder,FALSE);
		}
	}
}


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
			/*PDB(("layout failed\n"));*/ \
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





static LONG layoutpanelgroup_func(void)
{
	struct Hook *hook = (struct Hook*)REG_A0;
	APTR *grp = (APTR *)REG_A2;
	struct MUI_LayoutMsg *lm = (struct MUI_LayoutMsg *)REG_A1;

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
				//	dprintf("layouting..\n");
				}
				#endif

			}
			return( TRUE );
	}
	return ( MUILM_UNKNOWN );
	
	
}

struct EmulLibEntry layout_panelgroup_func = {
		 TRAP_LIB, 0, (void (*)(void))layoutpanelgroup_func
     };


static ULONG mNew(struct IClass *cl,Object *obj,struct opSet *msg)
{
	layout_panelgroup_hook.h_Entry = (ULONG(*)())&layout_panelgroup_func;
	if( ( obj = DoSuperNew( cl, obj,
					MUIA_Group_LayoutHook, (ULONG) &layout_panelgroup_hook,
							
					MUIA_FillArea        , FALSE,

					MUIA_ContextMenu     , 1,
					TAG_MORE			,	msg->ops_AttrList,
					TAG_DONE ) ) )
	{
			struct Data *data = (struct Data*) INST_DATA( cl, obj );
			data->newpanel    = FALSE;
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
			data->backgroundmode = MV_Panelgroup_BackMode_Color;
			data->droptarget = 0;
			data->dockmode = 0;
			data->modus = 0;
			data->dragobj[0] = data->dragobj[1] = 0;
			SetAttrs( obj, MUIA_FillArea, FALSE, MUIA_Background, "", TAG_DONE );
			doset( obj, data, msg->ops_AttrList, TRUE );
		
	}
	return( (ULONG) obj );
}

static ULONG mSet(struct IClass *cl,Object *obj,struct opSet *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	doset( obj, data,msg->ops_AttrList,FALSE); 
	return DoSuperMethodA(cl,obj,msg);
}


   
static ULONG mSetup(struct IClass *cl,Object *obj,struct MUIP_Setup *msg)
{
	ULONG rc;
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	if( ( rc = DoSuperMethodA(cl,obj,msg) ) && _win(obj) )
	{
		data->ehnode.ehn_Object   = obj;
		data->ehnode.ehn_Class    = cl;
		data->ehnode.ehn_Events   = IDCMP_MOUSEBUTTONS | IDCMP_MOUSEOBJECT | IDCMP_MOUSEHOVER ;
		data->ehnode.ehn_Priority = 1; /* XXX: ok ? */
		data->ehnode.ehn_Flags    = MUI_EHF_GUIMODE;

		DoMethod( _win(obj), MUIM_Window_AddEventHandler, (ULONG) &data->ehnode );
		run_autozip_trigger( obj, data );
	}
	return (rc);
}

/************************************************************************/

static ULONG mCleanup(struct IClass *cl,Object *obj,struct MUIP_Cleanup *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);	
	if( data->ihnode.ihn_Object != NULL )
	{
		DoMethod( _app(obj), MUIM_Application_RemInputHandler, &data->ihnode );
		data->ihnode.ihn_Object = NULL;
	}
	if(data->ehnode.ehn_Object)
	{
		DoMethod( _win(obj), MUIM_Window_RemEventHandler, (ULONG) &data->ehnode );
		data->ehnode.ehn_Object = 0;
	}
	return( DoSuperMethodA(cl,obj,msg) );
}

/************************************************************************/



static ULONG mGet(struct IClass *cl,Object *obj,struct opGet *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG result = TRUE;
	switch( msg->opg_AttrID )
	{ 
		case MA_Panelgroup_Size:
			*msg->opg_Storage = data->size;
			break;
		case MA_Panelgroup_Horiz:
			*msg->opg_Storage = data->horiz;
			break; 
		case MA_Panelgroup_GridMode:
			*msg->opg_Storage = data->grid;
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
		case MA_Panelgroup_Zipped:
			*msg->opg_Storage  = data->zipped;
			break;
		case MA_Panelgroup_Locked:
			*msg->opg_Storage = data->locked;
			break;
		case MA_Panelgroup_DockMode:
			*msg->opg_Storage = data->dockmode;
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
			result = DoSuperMethodA(cl, obj, (Msg)msg);

	}
	return( result );
}



static ULONG mAskMinMax(struct IClass *cl,Object *obj,struct MUIP_AskMinMax*msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	ULONG lbwidth = 0;
	ULONG lbheight = 0;
	ULONG rbwidth = 0;
	ULONG rbheight = 0;
	DoSuperMethodA( cl, obj, msg );

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
		msg->MinMaxInfo->MinHeight = 2;	
		msg->MinMaxInfo->MinWidth = 2;
		if( data->horiz )
		{
			msg->MinMaxInfo->MaxWidth  += 1500;
			msg->MinMaxInfo->MaxHeight += data->size;
			msg->MinMaxInfo->MinHeight = 2;
		//	  if(msg->MinMaxInfo->MinWidth < data->size)  msg->MinMaxInfo->MinWidth += data->size;
		} else {
			msg->MinMaxInfo->MaxWidth  += data->size;
			msg->MinMaxInfo->MaxHeight += 1500;
		}
	
	}
	
	if(data->newpanel)
	{
		if(data->horiz) msg->MinMaxInfo->MinWidth += data->size; 
		else msg->MinMaxInfo->MinHeight += data->size; 
	}
#warning affect subpanel position??
	//	msg->MinMaxInfo->MinHeight = 2;
	DoMethod( _win(obj), MM_Panelwin_Placement, MV_Panelsubwin_Default, msg->MinMaxInfo->MinWidth, msg->MinMaxInfo->MinHeight, 0 );

	return( 0 );
}

/************************************************************************/

static ULONG mDraw(struct IClass *cl,Object *obj,struct MUIP_Draw *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	ULONG lbwidth = 0;
	ULONG lbheight = 0;
	ULONG rbwidth = 0;
	ULONG rbheight = 0;

	DoSuperMethodA( cl->cl_Super, obj, msg );  /* calling area-class method to init msg->flags, nasty but works .... */
	if((data->dockmode)&& (data->zipped) && (!data->zipping))
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
			struct Window *win = _window(obj);
			switch ( data->zipdir )
			{
				case DIR_LEFTUP:
					if( data->horiz )
					{
						#warning
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_DstType , BLITVAL_DstType_RastPort,
											//	BLITTAG_SrcX    , data->zipsnap_xs - _width(obj)+ lbwidth,
						 						BLITTAG_SrcX    , data->zipsnap_xs - win->Width + lbwidth,
						 						BLITTAG_DstX    , _mleft(obj) + lbwidth,
												BLITTAG_DstWidth, win->Width - lbwidth,
												TAG_DONE );
					} else {
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_DstType  , BLITVAL_DstType_RastPort,
												BLITTAG_SrcY     , data->zipsnap_ys - win->Height + lbheight ,
												BLITTAG_DstY     , _mtop(obj) + lbheight,
												BLITTAG_DstHeight, win->Height - lbheight,
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
		#warning
						gfx_blit( data->dbuf->bm, _rp(obj),
												BLITTAG_SrcX    , data->zipsnap_xs - rbwidth ,
												BLITTAG_DstX    , win->Width - rbwidth,
										//		BLITTAG_DstX    , _width(obj) - rbwidth,
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
												BLITTAG_DstY     , win->Height  - rbheight,
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

static ULONG mRefreshRect(struct IClass *cl,Object *obj,struct MP_Panelgroup_RefreshRect *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG xoffset,yoffset; /*offsets in sourcebitmap during 1st pass */
	ULONG backgroundmode = data->backgroundmode;
	//PDB(("refresh %x\n",data->backgroundmode));
	if(data->zipping )
	{
		return( 0 );
	}
	#warning hidden
	if(0)//data->hidden)
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
#warning background picture
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
			FillPixelArray( msg->rp, msg->left, msg->top, msg->width, msg->height,data->backgroundcolor );
		}
	}
	return( 0 );
}

/************************************************************************/
static ULONG mContextMenuBuild(struct IClass *cl,Object *obj,struct MUIP_ContextMenuBuild *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	Object *menu, *quit;
	if( data->cmenu )
	{
		DisposeObject(data->cmenu );
	}
	if((data->cmenu =	(Object*) MenustripObject,
						Child, menu = MenuObject,
							MUIA_Menu_Title, "Project",
					
						Child,  MenuitemObject,MUIA_Menuitem_Title,"Save",MUIA_Menuitem_Shortcut,"W",MUIA_UserData,CM_SAVE_PANEL,End,
						Child,  MenuitemObject,MUIA_Menuitem_Title,"Preferences",MUIA_Menuitem_Shortcut,"P",MUIA_UserData,CM_PREFS,End,
						Child,  MenuitemObject,MUIA_Menuitem_Title,"Locked",
							MUIA_Menuitem_Shortcut,"Q",MUIA_UserData,CM_LOCKED,
							MUIA_Menuitem_Checkit,TRUE,MUIA_Menuitem_Checked,data->locked,End,
						Child,  MenuitemObject,MUIA_Menuitem_Title,"Delete Object",MUIA_UserData,CM_DEL_OBJECT,End,
					//	Child,  MenuitemObject,MUIA_Menuitem_Title,"Delete Panel",MUIA_UserData,CM_DEL_PANEL,End,
						End,
				End))
		{
			if((panel_modus == 3) &&( quit = MenuitemObject,MUIA_Menuitem_Title,"Quit",MUIA_Menuitem_Shortcut,"Q",MUIA_UserData,CM_QUIT,End))
				DoMethod(menu,MUIM_Family_AddTail,quit);
		}
	return (ULONG)data->cmenu;
}

static ULONG mContextMenuChoice(struct IClass *cl,Object *obj,struct MUIP_ContextMenuChoice *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	STRPTR win_name;
	if(msg->item)
	{
		ULONG cm_id = muiUserData(msg->item);
		switch(cm_id)
		{
			case CM_QUIT:
				PanelHasChanged(0,PanelPrefsClose,0,0);
				DoMethod(_app(obj),MUIM_Application_ReturnID,MUIV_Application_ReturnID_Quit);
				break;
			case CM_PREFS:
				if(panel_modus == 3) OpenPrefs("Panels2");
				else OpenPrefs("Panels");
				break;
			case CM_SAVE_PANEL:
				//SaveAll();
				DoMethod(_win(obj),MM_Panelwin_SaveConfig,MV_Panelwin_SaveConfig_Write);
				break;
			case CM_LOCKED:
				data->locked ^= 1;
				break;
			case CM_DEL_OBJECT:
				if(GetAttr(MA_Panelwin_Name,_win(obj),(ULONG*)&win_name))
				{
					DoMethod(obj, MUIM_Group_InitChange);
					#warning dragfusch
					DoMethod(obj, OM_REMMEMBER, data->mobj );
					data->dragobj[0] = 0;
					DoMethod(obj, MUIM_Group_ExitChange );
					SetAttrs(_win(obj), MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );
					DoMethod(_win(obj),MM_Panelwin_AttachToBorder,FALSE,TAG_DONE);
					DoMethod(_win(obj), MM_Panelwin_SaveConfig,0);
					PanelHasChanged(win_name,PanelHasChanged_Remove,0,0);
				}
				break;
		}
	}
	return 0;
}

/************************************************************************/

#define ADDDRAGOBJ(x) ({if (data->dragobj[x]) \
	{ \
		DoMethod( obj, OM_REMMEMBER, data->dragobj[ x ] ); \
		MUI_DisposeObject( data->dragobj[ x ] ); \
	} \
	if ( (data->dragobj[ x ] = NewObject( GetClass("DragButton"), NULL, MA_Paneldrag_Type, x, TAG_DONE ) ) ) \
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

static ULONG mChangeDrag(struct IClass *cl,Object *obj,struct MP_Panelgroup_ChangeDrag *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
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
		//	PDB(("unimplemented dragmode: %ld\n", data->dragmode));
			break;
		#endif
	}

	set( obj, MA_Paneldrag_LockMode, data->locked ); /* broadcast */
	DoMethod( obj, MUIM_Group_ExitChange );
	//if(_win(obj))DoMethod(_win(obj),MM_Panelwin_AttachToBorder,FALSE);
	return( 0 );
}


static BOOL prep_zipping(Object *obj,struct Data *data)
{
	APTR rootobj;
	GetAttr( MUIA_Window_RootObject, _win(obj), (ULONG*) &rootobj );
	/*force all subpanels to close*/
	FORCHILD( obj, MUIA_Group_ChildList )
	{
		DoMethod(child,MM_Panelbutton_Close);
	}
	NEXTCHILD
	data->zipsnap_xs = _width ( rootobj );
	data->zipsnap_ys = _height( rootobj );
	#warning fix for dockmode
	if(data->allowzipping )
	{
		if(( data->dbuf = gfx_dbuf_alloc_reuse( data->dbuf, data->zipsnap_xs, data->zipsnap_ys, DBUF_DISPLAYABLE, _screen( obj )->RastPort.BitMap ) ) )
		{
			struct RastPort *oldrp;
			struct Window *oldwin;
			oldrp        = _rp(obj);
			oldwin       = _window(obj);
			_rp(obj)     = data->dbuf->rp;
			_window(obj) = NULL;
			MUI_Redraw( obj, MADF_DRAWALL );
			//	ClearScreen(_rp(obj));
			_rp(obj)      = oldrp;
			_window(obj)  = oldwin;
			/*
			 * Set all the objects (except paneldrags) to
			 * hidden.
			 */
			data->zipping = TRUE;
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
			return TRUE;
		}
	}
	return FALSE;
}

/************************************************************************/
static ULONG mHandleEvent(struct IClass *cl,Object *obj,struct MUIP_HandleEvent *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	APTR mobj;
	struct List *button_list;
	Object *button;
	APTR object_state;
	static APTR move_pos = 0;
	UWORD *obj_pos = (UWORD*) &move_pos;
	if( data->zipping )
	{ 
		return( 0 );
	}
	if( msg->imsg->Class == IDCMP_MOUSEHOVER)
	{
		if (msg->imsg->Code == HOVERSTART ) data->m_hover = TRUE;
		else if (msg->imsg->Code == HOVERSTOP )
		{
			data->m_hover = FALSE;
		}
	}

	
	if( data->zipped)
	{		
		if(data->dockmode)
		{
			if( msg->imsg->Class == IDCMP_MOUSEBUTTONS)
			{
			//	if(prep_zipping(obj,data))
				{
				data->zipping = 	DoMethod(_app(obj),MM_Application_StartZipping,obj,DIR_BOTTOM_DOCK,FALSE,data->zipsnap_xs,data->zipsnap_ys,0);
				}
					
				//	DoMethod( obj , MM_Panelgroup_ToggleZip, 0);
			}
		
			if(( msg->imsg->Class == IDCMP_MOUSEHOVER)&&(msg->imsg->Code == HOVERSTART ))
			{
				data->zipping = DoMethod(_app(obj),MM_Application_StartZipping,obj,DIR_BOTTOM_DOCK,FALSE,data->zipsnap_xs,data->zipsnap_ys,0);
			}
							
		}
		return( 0 );
	}
/*	if(data->hidden)
	{
		data->hidden = FALSE;
		run_autozip_trigger( obj, data );
		MUI_Redraw( obj, MADF_DRAWALL );
	}
	else*/
	{
		switch( msg->imsg->Class )
		{
			case IDCMP_MOUSEHOVER:
				//	PDB(("hover %x\n\n\n",msg->imsg->Code));
	
				break;			
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
						//if( getprefslong( DSI_PANEL_AUTOSAVE_MOVE ) )
						{
						//	DoMethod( _win(obj), MM_Panelwin_SaveConfig );
						}
						#warning change pointer?
						//pointer_clear( _window(obj) );
						data->moving = FALSE;
					}
				}
				else if( msg->imsg->Code == MIDDLEDOWN )
				{
					GetAttr( MUIA_Window_MouseObject, _win(obj), (ULONG*) &data->active );
					if( data->active )
					{
						UWORD i = 0;
						move_pos = 0;
						data->moving = TRUE;
						FORCHILD( obj, MUIA_Group_ChildList )
						{
							SetAttrs( child, MUIA_InputMode, MUIV_InputMode_None, TAG_DONE );
							if(child == data->active)
							{
								obj_pos[0] = i;
							}
							i++;
						}
						NEXTCHILD
							#warning change pointer?
					//	pointer_set( _window(obj), POINTER_MOVE );
					}
				}
			break;
		
			case IDCMP_MOUSEOBJECT:
				GetAttr( MUIA_Window_MouseObject, _win(obj), (ULONG*) &mobj );
				if( data->moving )
				{
					obj_pos[1] = 0;
					if( (mobj) && ( (ULONG) mobj != (ULONG) data->active ) )
					{
						GetAttr( MUIA_Group_ChildList, obj, (ULONG*) &button_list );
						object_state = button_list->lh_Head;
						if( data->horiz )
						{
							while( ( button = (Object*) NextObject( &object_state ) ) && ( _mright(button) < msg->imsg->MouseX ) )
							{
								obj_pos[1]++;
							}
						} else {
							while( ( button = (Object*) NextObject( &object_state ) ) && ( _mbottom(button) < msg->imsg->MouseY ) )
							{
								obj_pos[1]++;
							}
						}
						DoMethod( obj,MUIM_Group_InitChange );
						DoMethod( obj,MUIM_Group_MoveMember, data->active, obj_pos[1] );
						DoMethod( obj,MUIM_Group_ExitChange );
#warning autosave					
				//		DoMethod(_win(obj),MM_Panelwin_SaveConfig,0);
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


static ULONG mToggleZip(struct IClass *cl,Object *obj, struct MP_Panelgroup_ToggleZip *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	ULONG drag = 0;
	
	if( data->zipping )
	{
		return( 0 );
	}

	set( obj, MA_Paneldrag_Zipping, TRUE ); /* broadcast */
	

	if(data->dragobj[ 0 ] )
	{
		if(data->horiz) drag +=	_width ( data->dragobj[ 0 ] );
		else drag += _height( data->dragobj[ 0 ] );
	}
	if( data->dragobj[ 1 ] )
	{
		if(data->horiz) drag +=	_width ( data->dragobj[ 1 ] );
		else drag += _height( data->dragobj[ 1 ] );
	}
	if(data->zipped)
	{
	//	PDB(("d\n",data->zipdir));
		#warning only changed AFTER zipping	
		
	//	data->zipping = DoMethod(_app(obj),MM_Application_StartZipping,obj,msg->type,FALSE,data->zipsnap_xs,data->zipsnap_ys,drag);
		data->zipping = DoMethod(_app(obj),MM_Application_StartZipping,obj,data->zipdir,FALSE,data->zipsnap_xs,data->zipsnap_ys,drag);
	}
	else if(!DoMethod(obj,MM_Panelgroup_CheckZipLock))
	{
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
			//	PDB(("argh, wrong zipdir\n"));
				break;
		#endif
		}
		if(prep_zipping(obj,data))
		{
			#warning set ziiping status?
		//	data->zipping = TRUE;
			if(data->dockmode)
			{
				#warning non bottom dock?
				data->zipping = DoMethod(_app(obj),MM_Application_StartZipping,obj,DIR_BOTTOM_DOCK,TRUE,_width(obj),6,0);
			}
			else if(data->horiz)	data->zipping = DoMethod(_app(obj),MM_Application_StartZipping,obj,data->zipdir,TRUE,drag,_height(obj),drag);
			else data->zipping = DoMethod(_app(obj),MM_Application_StartZipping,obj,data->zipdir,TRUE,_width(obj),drag,drag);
		}
	}
	return( 0 );
}


static ULONG mZippingFinished(struct IClass *cl,Object *obj, struct MP_Panelgroup_ZippingFinished *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	set( obj, MA_Paneldrag_Zipping, FALSE ); /* broadcast */
	data->zipping = FALSE;
	if( msg->direction ) 
	{
		data->zipped = TRUE;
	} else {
		data->zipped = FALSE;
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
	
	set( obj, MA_Paneldrag_Zipping, FALSE ); /* broadcast */
	}
#warning show hidden dragbar while zipping?
//	if(data->dragobj[ 0 ] ) MUI_Redraw( data->dragobj[ 0 ], MADF_DRAWALL ); /* make sure hidden dragbars are shown while zipped*/
//	if(data->dragobj[ 1 ] ) MUI_Redraw( data->dragobj[ 1 ], MADF_DRAWALL ); /* and hidden again when unzipped*/
	
	return( 0 );
}


/************************************************************************/
static ULONG mCheckZipLock(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	ULONG type, ziplock;
	APTR subwin,subwintbar;
	if(data->newpanel) return TRUE;
	if(data->modus) return TRUE;
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



static ULONG mFindSubPanel(struct IClass *cl,Object *obj, struct MP_Panelgroup_FindSubPanel *msg)
{
	ULONG type, ID;
	APTR subwin,subwintbar;
	
	FORCHILD( obj, MUIA_Group_ChildList )
	{
		get( child, MA_Panel_Type,&type );
		if( type == MV_Panel_Type_SubPanel )
		{
			get( child , MA_SubPanel_ID, &ID );
			get( child , MA_Panelbutton_AttachedObject, (ULONG*) &subwin );
			get( subwin, MA_Panelwin_Group, (ULONG*) &subwintbar );
			if( ID == msg->ID )
			{
				return( (ULONG)subwintbar );
			}
			else
			{
				ULONG subsub = 0;
				subsub = DoMethod(subwintbar,MM_Panelgroup_FindSubPanel,msg->ID);
				if(subsub != 0) return subsub;
			}
		}
	}
	NEXTCHILD
	return( 0 );
}


static ULONG mExitChange(struct IClass *cl,Object *obj, struct MUIP_Group_ExitChange *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
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
	return(	DoSuperMethodA( cl, obj, msg ));
}

/************************************************************************/

static ULONG mAddMember(struct IClass *cl,Object *obj, struct opMember *msg)
{
	if( getv( msg->opam_Object, MA_Panel_Type) != MV_Panel_Type_Drag )
	{
		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );
		DoMethod(msg->opam_Object,MUIM_Notify,MA_PanelZipLock,FALSE,obj,1,MM_Panelgroup_StartZipTimer);
	}
	
	return(	DoSuperMethodA( cl, obj, msg ));
}

static ULONG mRemMember(struct IClass *cl,Object *obj, struct opMember *msg)
{
	ULONG retval = 0;
	retval = DoSuperMethodA( cl, obj, msg );
	if(_win(obj))
	{
		SetAttrs(_win(obj), MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );
		DoMethod(_win(obj),MM_Panelwin_AttachToBorder,FALSE,TAG_DONE);
	}
	return retval;
}

/************************************************************************/

static ULONG mAddHead(struct IClass *cl,Object *obj, struct MUIP_Group_AddHead *msg)
{
	if(  getv( msg->obj, MA_Panel_Type ) != MV_Panel_Type_Drag ) 
	{
		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );
		DoMethod(obj,MUIM_Notify,MA_PanelZipLock,FALSE,obj,1,MM_Panelgroup_StartZipTimer);
	}
	return(	DoSuperMethodA( cl, obj, msg ));
}
/************************************************************************/
static ULONG mAddTail(struct IClass *cl,Object *obj, struct MUIP_Group_AddTail *msg)
{
	if(  getv( msg->obj, MA_Panel_Type ) != MV_Panel_Type_Drag ) 
	{
		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );
		DoMethod(obj,MUIM_Notify,MA_PanelZipLock,FALSE,obj,1,MM_Panelgroup_StartZipTimer);
	}
	return(	DoSuperMethodA( cl, obj, msg ));
}
	
struct PanelNode
{
	struct Node n;
	struct SignalSemaphore semaphore;
	APTR ctx;
};
#if 0
static ULONG m_SaveConfig(struct IClass *cl,Object *obj,struct MP_Panelgroup_SaveConfig *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	struct PanelNode *pn = (struct PanelNode*)msg->panel_lock;
	APTR pctx = pn->ctx;
	if( !data->zipped ) /* XXX: actually we could save a zipped state as well but it might be tricky */
	{
		APTR pl, pi; /* list, item */
		ULONG index = 0;
		ULONG type = 0;

		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );  /*we ain't new anymore get rid of the dodo*/
		RemovePanelItem(msg->panel_lock,NULL, DSI_LISTPOOL_PANEL);
		pl = (APTR) SetPanelAttr(msg->panel_lock, NULL, DSI_LISTPOOL_PANEL, NULL, (ULONG)NULL );
		SetPanelAttr(msg->panel_lock,NULL, DSI_PANELGROUP_BACKMODE, (APTR) &data->backgroundmode, 4);
		SetPanelAttr(msg->panel_lock,NULL, DSI_PANELGROUP_BACKCOLOR, (APTR) &data->backgroundcolor, 4);
		if( data->backgroundmode == MV_Panelgroup_BackMode_Picture )
		{
			setprefsstr_ctx(pctx, DSI_PANELGROUP_BACKDROP, data->backname );
		}
		if( pl )
		{
			FORCHILD( obj, MUIA_Group_ChildList )
			{
				if(!(pi = GetPanelAttr(msg->panel_lock, pl, index | DSF_LISTPOOL, NULL, NULL)))
				{
					pi = (APTR) SetPanelAttr(msg->panel_lock, pl, index | DSF_LISTPOOL, NULL, (ULONG)NULL );
				}

				if( pi )
				{
					SetPanelAttr(msg->panel_lock,pi, DSI_LISTPOOL_PANEL_SUBPANEL,NULL, (ULONG)NULL );
					type = getv( child, MA_Panel_Type );
					if( ( type != MV_Panel_Type_External ) )
					{
						SetPanelAttr(msg->panel_lock,pi, DSI_LISTPOOL_PANEL_TYPE, &type, 4 );
						index += DoMethod( child, MM_Panel_SaveConfig, msg->panel_lock, index );  /* subpanels will add items here*/
					} 
					#warning no save for external
					#if 0
					else {
						STRPTR classname;
						APTR supobj;
						data->ext_pctx = msg->pctx;
						data->ext_pi   = pi;
						data->ext_pl   = pl;

						setprefslong_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_TYPE, MV_Panel_Type_External );
						get( child, MA_Panel_Extern_ClassName, (ULONG*) &classname );
						setprefsstr_lp( msg->pctx, pi, DSI_LISTPOOL_PANEL_EXT_NAME, classname ? classname : (STRPTR)"" );

						/*if( ( get( child, MA_Panelextern_SupportObject, (ULONG*) &supobj ) ) && ( supobj ) )
						{
							DoMethod( supobj, MM_Panelsupport_Saveconfig, msg->pctx, pi );
						}
						else*/
						{
							index += DoMethod( child, MM_Panel_SaveConfig, msg->pctx, pi ); 
						}
						data->ext_pctx = data->ext_pi = data->ext_pl = 0;
					}
				#endif
                   
				}
				/* XXX */
				index++;
			}
			NEXTCHILD
		}
	}
	return (0);
}
#endif

static ULONG mSaveConfig(struct IClass *cl,Object *obj,struct MP_Panelgroup_SaveConfig *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	if( !data->zipped ) /* XXX: actually we could save a zipped state as well but it might be tricky */
	{
		APTR pl, pi; /* list, item */
		ULONG index = 0;
		ULONG type = 0;

		SetAttrs( obj, MA_Panelgroup_New, FALSE, TAG_DONE );  /*we ain't new anymore get rid of the dodo*/
		RemovePPoolItem(msg->panel_lock,NULL, DSI_LISTPOOL_PANEL);
		pl = (APTR) AddPPoolItem(msg->panel_lock, NULL, DSI_LISTPOOL_PANEL, NULL, (ULONG)NULL );
		AddPPoolItem(msg->panel_lock,NULL, DSI_PANELGROUP_BACKMODE, (APTR) &data->backgroundmode, 4);
		AddPPoolItem(msg->panel_lock,NULL, DSI_PANELGROUP_BACKCOLOR, (APTR) &data->backgroundcolor, 4);
		AddPPoolItem(msg->panel_lock,NULL, DSI_PANELGROUP_DOCKMODE , (APTR) &data->dockmode, 4);
		if( data->backgroundmode == MV_Panelgroup_BackMode_Picture )
		{
#warning save pic background
			AddPPoolItem(msg->panel_lock,NULL, DSI_PANELGROUP_BACKDROP, data->backname, strlen(data->backname)+1);
		}
		else
		{
			AddPPoolItem(msg->panel_lock,NULL, DSI_PANELGROUP_BACKDROP, 0, 0);
		}
		if( pl )
		{
			FORCHILD( obj, MUIA_Group_ChildList )
			{
				type = getv( child, MA_Panel_Type );
				if(type > MV_Panel_Type_Drag)
				{
				if(!(pi = (APTR) GetPPoolItem(msg->panel_lock, pl, index | DSF_LISTPOOL, NULL, NULL)))
				{
					pi = (APTR) AddPPoolItem(msg->panel_lock, pl, index | DSF_LISTPOOL, NULL, (ULONG)NULL );
				}
				if( pi )
				{
					ULONG nop = 0;
					STRPTR classname;
				//	APTR supobj;
					AddPPoolItem(msg->panel_lock,pi, DSI_LISTPOOL_PANEL_SUBPANEL,&nop, 4 );
					
				//	PDB(("index %d type %x\n",index,type));
					switch(type)
					{
						
						case MV_Panel_Type_External:
							AddPPoolItem(msg->panel_lock,pi, DSI_LISTPOOL_PANEL_TYPE, &type, 4 );
							get( child, MA_Panel_Extern_ClassName, (ULONG*) &classname );
							AddPPoolItem(msg->panel_lock, pi,DSI_LISTPOOL_PANEL_EXT_NAME,classname , strlen(classname)+1);
							index += DoMethod( child, MM_Panel_SaveConfig, msg->panel_lock, index );
							break;
						case MV_Panel_Type_Drag:
							break;
						default:
							AddPPoolItem(msg->panel_lock,pi, DSI_LISTPOOL_PANEL_TYPE, &type, 4 );
							index += DoMethod( child, MM_Panel_SaveConfig, msg->panel_lock, index );  /* subpanels will add items here*/
							break;
					}
				}
				/* XXX */
				index++;
			}}
			NEXTCHILD
		}
	}
	return (0);
}

static VOID run_autozip_trigger( APTR obj, struct Data *data )
{
	/* XXX: run autozip timer. we should check if mouse is still above panel.. bah.. */

	#warning check for autozip enable
	if( !data->zipped && !data->zipping)// && data->enable_autozip )
	{
		DoMethod(obj,MM_Panelgroup_StartZipTimer);
	}
}

static ULONG mStartZipTimer(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
		
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
		#warning change value for dockmode
		//if(getv(_win(obj),MA_Panelwin_DockMode))
		if(data->dockmode)	data->ihnode.ihn_Millis = 250;
		else data->ihnode.ihn_Millis = 5000;
		data->ihnode.ihn_Object = obj;
		data->ihnode.ihn_Flags  = MUIIHNF_TIMER;
		data->ihnode.ihn_Method = MM_Panelgroup_DelayedZip;
		DoMethod( _app(obj), MUIM_Application_AddInputHandler, &data->ihnode );
	}
	return 0;
}

static ULONG mDelayedZip(struct IClass *cl,Object *obj,Msg msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);

	if( data->ihnode.ihn_Object != NULL )
	{
		DoMethod( _app(obj), MUIM_Application_RemInputHandler, &data->ihnode );
		data->ihnode.ihn_Object = NULL;
	}

	#warning allowzipping
	if(!data->zipped && data->autozip && data->allowzipping  && !DoMethod(obj,MM_Panelgroup_CheckZipLock) && !data->mobj &&!data->m_hover) /* prefs may have changed when delaying... */
	{
		if(data->dragobj[0]) DoMethod( obj, MM_Panelgroup_ToggleZip, MV_Paneldrag_Type_LeftUp );
		else if(data->dragobj[1]) DoMethod( obj, MM_Panelgroup_ToggleZip, MV_Paneldrag_Type_RightBottom );
		else if(data->dockmode)
		{	
			DoMethod( obj, MM_Panelgroup_ToggleZip, 0);
		}
	}
	MUI_Redraw( obj, MADF_DRAWALL );

	return( 0 );
}




static ULONG mDrop(struct IClass *cl,Object *obj,struct MP_Panel_Drop *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);
	Object *cbutton = 0;
	ULONG type = MV_Panel_Type_Drag;
	ULONG pos = 0;
	STRPTR iconpath;
	APTR object_state;
	struct List *button_list;
	Object *button;
	GetAttr( MUIA_Window_MouseObject, _win(obj), (ULONG*) &cbutton );
	GetAttr( MUIA_Group_ChildList   , obj      , (ULONG*) &button_list );
	object_state = button_list->lh_Head;
	if( data->horiz )
	{
		while( ( button = (Object*) NextObject( &object_state ) ) && ( ( _mright( button ) + _mleft( button ) ) / 2 < msg->x ) )
		{
			pos++;
		}
	} else {
		while( ( button = (Object*) NextObject( &object_state ) ) && ( ( _mbottom( button ) + _mtop( button ) ) / 2 < msg->y ) )
		{
			pos++;
		}
	}
	if(cbutton)	GetAttr(MA_Panel_Type,cbutton,&type);
	if ((data->locked)&&(type != MV_Panel_Type_Drag))
	{
		DoMethod( cbutton ,MM_Panelbutton_Launch,msg->path);
	}
	else if((iconpath = name_build_info(msg->path)))
	{
		Object *nbutton = NewObject( GetClass("CommandButton"), NULL,
						 MA_Panel_Imagepath , iconpath,
						 MA_Panel_URI,msg->path,
						TAG_DONE );
		if(type == MV_Panel_Type_Drag)
		{
		//	ULONG drag_type;
			if(data->horiz)
			{
		//		PDB(("%d %d\n",msg->x,_left(obj)));
#warning bad style
				if(msg->x < 13)	pos = 0; 
				else pos = -1;
			}
			else
			{
				if(msg->y < 13)	pos = 0; 
				else pos = -1;
			}
		}
		DoMethod( obj, MUIM_Group_InitChange);
		DoMethod( obj, OM_ADDMEMBER, nbutton );
		DoMethod( obj, MUIM_Group_MoveMember, nbutton, pos );
		DoMethod( obj, MUIM_Group_ExitChange );
		SetAttrs( _win(obj), MUIA_Window_Width, MUIV_Window_Width_MinMax( 0 ), TAG_DONE );
#warning check autosave	
		//if( getprefslong( DSI_PANEL_AUTOSAVE_DROP ) )
		{
			DoMethod( _win(obj), MM_Panelwin_SaveConfig,MV_Panelwin_SaveConfig_Add );
		}
		DoMethod(_win(obj),MM_Panelwin_AttachToBorder,FALSE);
		set( obj, MA_Panelgroup_HasChanged, TRUE );
		name_delete(iconpath);
	}
	
	
	
	return 0;
}

static ULONG mFindID(struct IClass *cl,Object *obj,struct MP_Panelgroup_FindID *msg)
{
	ULONG count = msg->startID;
	ULONG type;
	*msg->obj = 0;
	FORCHILD( obj, MUIA_Group_ChildList )
	{
		get( child, MA_Panel_Type,&type );
		if( type != MV_Panel_Type_Drag) count++;
		if(count == msg->objectID)
		{
			*msg->obj = child;
			return count;
		}
		if( type == MV_Panel_Type_SubPanel )
		{
			APTR subpanel_win,subpanel_tbar;
			if((GetAttr(MA_Panelbutton_AttachedObject,child,(ULONG*)&subpanel_win))&&(subpanel_win))
			{
				if((GetAttr(MA_Panelwin_Group,subpanel_win,(ULONG*)&subpanel_tbar))&&(subpanel_tbar))
				{ 
					ULONG c2;
					c2 = DoMethod(subpanel_tbar,MM_Panelgroup_FindID,msg->objectID,count,msg->obj);
					count = c2; 
					if(*msg->obj) return count;
				}
			}	
		}
	}
	NEXTCHILD
	return count;
}



static ULONG mAppMessage(struct IClass *cl,Object *obj,struct MP_Panelgroup_AppMessage *msg)
{
	struct Data *data = (struct Data*)INST_DATA(cl,obj);	
	struct AppMessage *appmsg = (struct AppMessage *) msg->appmsg;
	APTR mobj = 0;
	
	FORCHILD( obj, MUIA_Group_ChildList )
	{
		if(_isinobject2(child,appmsg->am_MouseX,appmsg->am_MouseY))
		{
			mobj = child;
		}
	}
	NEXTCHILD
	if((data->droptarget)&&(data->droptarget != mobj))
	{
		SetAttrs(data->droptarget,MA_PANEL_DROPTARGET,FALSE,TAG_DONE);
		data->droptarget = 0;
	}
	
	switch(appmsg->am_Class)
	{
		case AM_CLASS_MOUSEMOVE:
			if((mobj) &&(data->droptarget != mobj))
			{
				if(data->droptarget)SetAttrs(data->droptarget,MA_PANEL_DROPTARGET,FALSE,TAG_DONE);
				data->droptarget = mobj;
				SetAttrs(data->droptarget,MA_PANEL_DROPTARGET,TRUE,TAG_DONE);
			}
			break;
		case AM_CLASS_MOUSEEXIT:
			if(data->droptarget)SetAttrs(data->droptarget,MA_PANEL_DROPTARGET,FALSE,TAG_DONE);
		//	PDB(("mouse exit %x\n",data->droptarget));	
			data->droptarget = 0;
			break;
		case AM_CLASS_MOUSEENTER:
			break;
		case 0:  /* good old drop */
			if(mobj)	
			{	
				ULONG type;
				GetAttr(MA_Panel_Type,mobj,&type);	
				if ((data->locked)&&(type != MV_Panel_Type_Drag))
				{
					DoMethod(mobj ,MM_Panelbutton_Launch, appmsg->am_NumArgs,appmsg->am_ArgList);
					return 0;
				}
			}
			{
				struct WBArg *ap;
				int i;
				static char buf[256];
				char *b=buf;
				for (ap=appmsg->am_ArgList,i=0;i<appmsg->am_NumArgs;i++,ap++)
				{
					NameFromLock(ap->wa_Lock,buf,sizeof(buf));
					AddPart(buf,(STRPTR)ap->wa_Name,sizeof(buf));
					DoMethod(obj,MM_Panel_Drop,appmsg->am_MouseX,appmsg->am_MouseY,b);
				}
			}
			data->droptarget = 0;
		
			break;
	}
	return 0;
}



DISPATCHER(DefaultGroupClass)
{
	switch (msg->MethodID)
	{
		case OM_NEW             			: return(mNew				(cl,obj,(struct opSet *)msg));
		case OM_SET        	    			: return(mSet				(cl,obj,(struct opSet *)msg));
		case OM_GET			    			: return(mGet				(cl,obj,(struct opGet *)msg));
		case MUIM_Setup     				: return(mSetup    			(cl,obj,(struct MUIP_Setup *)msg));
		case MUIM_Cleanup    				: return(mCleanup  			(cl,obj,(struct MUIP_Cleanup *)msg));
		case MUIM_AskMinMax		   			: return(mAskMinMax			(cl,obj,(struct MUIP_AskMinMax *)msg));
		case MUIM_Draw     					: return(mDraw     			(cl,obj,(struct MUIP_Draw *)msg));
		case MUIM_HandleEvent 				: return(mHandleEvent		(cl,obj,(struct MUIP_HandleEvent *)msg));
		case MUIM_ContextMenuBuild			: return(mContextMenuBuild	(cl,obj,(struct MUIP_ContextMenuBuild *)msg));
		case MUIM_ContextMenuChoice			: return(mContextMenuChoice	(cl,obj,(struct MUIP_ContextMenuChoice *)msg));
		case MM_Panelgroup_RefreshRect		: return(mRefreshRect		(cl,obj,(struct MP_Panelgroup_RefreshRect *)msg));
		case MM_Panelgroup_ChangeDrag		: return(mChangeDrag		(cl,obj,(struct MP_Panelgroup_ChangeDrag *)msg));
		case MM_Panelgroup_ToggleZip		: return(mToggleZip			(cl,obj,(struct MP_Panelgroup_ToggleZip *)msg));
		case MM_Panelgroup_ZippingFinished	: return(mZippingFinished	(cl,obj,(struct MP_Panelgroup_ZippingFinished *)msg));
		case MM_Panelgroup_CheckZipLock		: return(mCheckZipLock		(cl,obj,(Msg)msg));
		case MM_Panelgroup_FindSubPanel		: return(mFindSubPanel		(cl,obj,(struct MP_Panelgroup_FindSubPanel *)msg));
		case MUIM_Group_ExitChange 			: return(mExitChange 		(cl,obj,(struct MUIP_Group_ExitChange *)msg));
		case OM_ADDMEMBER					: return(mAddMember			(cl,obj,(struct opMember*)msg));
		case OM_REMMEMBER					: return(mRemMember			(cl,obj,(struct opMember*)msg));
		case MUIM_Group_AddHead				: return(mAddHead			(cl,obj,(struct MUIP_Group_AddHead*)msg));
		case MUIM_Group_AddTail				: return(mAddTail			(cl,obj,(struct MUIP_Group_AddTail*)msg));
		case MM_Panelgroup_SaveConfig		: return(mSaveConfig		(cl,obj,(struct MP_Panelgroup_SaveConfig *)msg));
		case MM_Panelgroup_StartZipTimer	: return(mStartZipTimer		(cl,obj,(Msg)msg));
		case MM_Panelgroup_DelayedZip		: return(mDelayedZip		(cl,obj,(Msg)msg));
		case MM_Panel_Drop					: return(mDrop				(cl,obj,(struct MP_Panel_Drop *)msg));
		case MM_Panelgroup_FindID			: return(mFindID			(cl,obj,(struct MP_Panelgroup_FindID *)msg));
		case MM_Panelgroup_AppMessage		: return(mAppMessage		(cl,obj,(struct MP_Panelgroup_AppMessage*)msg));
	}
	return(DoSuperMethodA(cl,obj,msg));
}
DISPATCHER_END



struct MUI_CustomClass *DefaultGroupClass_Class(void)
{
	return MUI_CreateCustomClass(NULL,MUIC_Group,NULL,sizeof(struct Data),DISPATCHER_REF(DefaultGroupClass));
}

